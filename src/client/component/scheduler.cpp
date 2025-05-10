#include <std_include.hpp>
#include "loader/component_loader.hpp"

#include "scheduler.hpp"
#include "game/game.hpp"

#include <utils/hook.hpp>
#include <utils/concurrency.hpp>
#include <utils/string.hpp>
#include <utils/thread.hpp>

namespace scheduler
{
	namespace
	{
		std::atomic<uint64_t> task_id_counter = 0;

		uint64_t generate_task_id()
		{
			uint64_t counter = task_id_counter.fetch_add(1, std::memory_order_relaxed) + 1;
			uint64_t timestamp = static_cast<uint64_t>(std::chrono::steady_clock::now().time_since_epoch().count());

			// Combine the timestamp and counter to generate a unique ID
			return (timestamp << 32) | (counter & 0xFFFFFFFF);
		}

		struct task
		{
			std::function<bool()> handler{};
			std::chrono::milliseconds interval{};
			std::chrono::high_resolution_clock::time_point last_call{};
			std::shared_ptr<bool> stop_flag{ std::make_shared<bool>(false) };
			uint64_t id{}; // Unique ID for the task
		};

		using task_list = std::vector<task>;

		class task_pipeline
		{
		public:
			void add(task&& task)
			{
				new_callbacks_.access([&task](task_list& tasks)
				{
					tasks.emplace_back(std::move(task));
				});
			}

			void execute()
			{
				callbacks_.access([&](task_list& tasks)
				{
					this->merge_callbacks();

					for (auto i = tasks.begin(); i != tasks.end();)
					{
						const auto now = std::chrono::high_resolution_clock::now();
						const auto diff = now - i->last_call;

						if (diff < i->interval)
						{
							++i;
							continue;
						}

						i->last_call = now;

						const auto res = i->handler();
						if (res == cond_end)
						{
							i = tasks.erase(i);
						}
						else
						{
							++i;
						}
					}
				});
			}

			void stop_task(uint64_t task_id)
			{
				callbacks_.access([&](task_list& tasks) {
					for (auto& task : tasks)
					{
						if (task.id == task_id)
						{
							*task.stop_flag = true;
							break;
						}
					}
					});
			}

		private:
			utils::concurrency::container<task_list> new_callbacks_;
			utils::concurrency::container<task_list, std::recursive_mutex> callbacks_;

			void merge_callbacks()
			{
				callbacks_.access([&](task_list& tasks)
				{
					new_callbacks_.access([&](task_list& new_tasks)
					{
							tasks.insert(tasks.end(), std::move_iterator<task_list::iterator>(new_tasks.begin()),
								std::move_iterator<task_list::iterator>(new_tasks.end()));
						new_tasks = {};
					});
				});
			}
		};

		const int REPEAT_FINISHED = -1;
		volatile bool kill = false;
		std::thread thread_async;
		std::thread network_thread;
		task_pipeline pipelines[pipeline::count];
		utils::hook::detour r_end_frame_hook;
		utils::hook::detour g_run_frame_hook;
		utils::hook::detour main_frame_hook;
		utils::hook::detour hks_frame_hook;

		void execute(const pipeline type)
		{
			assert(type >= 0 && type < pipeline::count);
			pipelines[type].execute();
		}

		void r_end_frame_stub()
		{
			execute(pipeline::renderer);
		}

		void server_frame_stub()
		{
			g_run_frame_hook.invoke<void>();
			execute(pipeline::server);
		}

		void* main_frame_stub()
		{
			const auto _0 = gsl::finally([]()
			{
				execute(pipeline::main);
			});

			return main_frame_hook.invoke<void*>();
		}

		void hks_frame_stub()
		{
			hks_frame_hook.invoke<void>();

			if (*game::hks::lua_state)
			{
				execute(pipeline::lui);
			}
		}
	}

	void schedule(const std::function<bool()>& callback, const pipeline type,
	              const std::chrono::milliseconds delay, uint64_t* out_task_id)
	{
		assert(type >= 0 && type < pipeline::count);

		task task;
		task.handler = [callback, stop_flag = task.stop_flag]() mutable -> bool {
			return *stop_flag ? cond_end : callback();
			};
		task.interval = delay;
		task.last_call = std::chrono::high_resolution_clock::now();
		task.id = generate_task_id();

		if (out_task_id)
		{
			*out_task_id = task.id;
		}

		pipelines[type].add(std::move(task));
	}

	void stop(const uint64_t task_id, const pipeline type)
	{
		assert(type >= 0 && type < pipeline::count);

		pipelines[type].stop_task(task_id);
	}

	uint64_t loop(const std::function<void()>& callback, const pipeline type,
	          const std::chrono::milliseconds delay)
	{
		uint64_t task_id = 0;
		schedule([callback]()
		{
			callback();
			return cond_continue;
		}, type, delay, &task_id);
		return task_id;
	}

	// Captain Barbossa: I had to make this repeat function. It's like loop but runs X times every Y delay. So delay of 1s and a repeatFor of 60 is to run every 1 second for 60 seconds
	uint64_t repeat(const std::function<void()>& callback, pipeline type, std::chrono::milliseconds delay, int repeatFor)
	{
		if (repeatFor <= 0)
		{
			return REPEAT_FINISHED;
		}

		auto counter = std::make_shared<int>(repeatFor);
		uint64_t task_id = 0;
		schedule([callback, counter]() mutable -> bool
			{
				callback();
				(*counter)--;

				return (*counter > 0) ? cond_continue : cond_end;
			}, type, delay, &task_id);
		return task_id;
	}

	void once(const std::function<void()>& callback, const pipeline type,
	          const std::chrono::milliseconds delay)
	{
		schedule([callback]()
		{
			callback();
			return cond_end;
		}, type, delay);
	}

	void on_game_initialized(const std::function<void()>& callback, const pipeline type,
	                         const std::chrono::milliseconds delay)
	{
		schedule([=]()
		{
			const auto dw_init = game::Live_SyncOnlineDataFlags(0) == 0;
			if (dw_init && game::Sys_IsDatabaseReady2())
			{
				once(callback, type, delay);
				return cond_end;
			}

			return cond_continue;
		}, pipeline::main);
	}

	class component final : public component_interface
	{
	public:
		void post_start() override
		{
			thread_async = utils::thread::create_named_thread("Async Scheduler", []()
			{
				while (!kill)
				{
					execute(pipeline::async);
					std::this_thread::sleep_for(10ms);
				}
			});

			network_thread = std::thread([]()
			{
				while (!kill)
				{
					execute(pipeline::network);
					std::this_thread::sleep_for(std::chrono::milliseconds(50));
				}
			});
		}

		void post_unpack() override
		{
			utils::hook::jump(0x6A6300_b, utils::hook::assemble([](utils::hook::assembler& a)
			{
				a.pushad64();
				a.call_aligned(r_end_frame_stub);
				a.popad64();

				a.sub(rsp, 0x28);
				a.call(0x6A5C20_b);
				a.mov(rax, 0xEAB4308_b);
				a.mov(rax, qword_ptr(rax));
				a.jmp(0x6A6310_b);
			}), true);

			g_run_frame_hook.create(0x417940_b, scheduler::server_frame_stub);
			main_frame_hook.create(0x3438B0_b, scheduler::main_frame_stub);
			hks_frame_hook.create(0x2792E0_b, scheduler::hks_frame_stub);
		}

		void pre_destroy() override
		{
			kill = true;
			if (thread_async.joinable())
			{
				thread_async.join();
			}

			if (network_thread.joinable())
			{
				network_thread.join();
			}
		}
	};
}

REGISTER_COMPONENT(scheduler::component)