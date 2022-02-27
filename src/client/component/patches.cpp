#include <std_include.hpp>
#include "loader/component_loader.hpp"

#include "game/game.hpp"
#include "game/dvars.hpp"

#include <utils/hook.hpp>
#include <utils/string.hpp>
#include <component/scheduler.hpp>

#include "version.h"
#include <component/command.hpp>
#include <component/console.hpp>
#include <component/network.hpp>

namespace patches
{
	namespace
	{
		const char* live_get_local_client_name()
		{
			return game::Dvar_FindVar("name")->current.string;
		}

		utils::hook::detour sv_kick_client_num_hook;

		void sv_kick_client_num(const int client_num, const char* reason)
		{
			// Don't kick bot to equalize team balance.
			if (reason == "EXE_PLAYERKICKED_BOT_BALANCE"s)
			{
				return;
			}
			return sv_kick_client_num_hook.invoke<void>(client_num, reason);
		}

		utils::hook::detour gscr_set_save_dvar_hook;
		utils::hook::detour dvar_register_float_hook;

		void gscr_set_save_dvar_stub()
		{
			const auto string = utils::string::to_lower(utils::hook::invoke<const char*>(SELECT_VALUE(0x140375210, 0x140443150), 0));
			if (string == "cg_fov" || string == "cg_fovscale")
			{
				return;
			}

			gscr_set_save_dvar_hook.invoke<void>();
		}

		game::dvar_t* cg_fov = nullptr;
		game::dvar_t* cg_fovScale = nullptr;

		game::dvar_t* dvar_register_float_stub(int hash, const char* dvarName, float value, float min, float max, unsigned int flags)
		{
			static const auto cg_fov_hash = game::generateHashValue("cg_fov");
			static const auto cg_fov_scale_hash = game::generateHashValue("cg_fovscale");

			if (hash == cg_fov_hash)
			{
				return cg_fov;
			}

			if (hash == cg_fov_scale_hash)
			{
				return cg_fovScale;
			}

			return dvar_register_float_hook.invoke<game::dvar_t*>(hash, dvarName, value, min, max, flags);
		}

		std::string get_login_username()
		{
			char username[UNLEN + 1];
			DWORD username_len = UNLEN + 1;
			if (!GetUserNameA(username, &username_len))
			{
				return "Unknown Soldier";
			}

			return std::string{ username, username_len - 1 };
		}

		utils::hook::detour com_register_dvars_hook;

		void com_register_dvars_stub()
		{
			if (game::environment::is_mp())
			{
				// Make name save
				dvars::register_string("name", get_login_username().data(), game::DVAR_FLAG_SAVED, true);

				// Disable data validation error popup
				dvars::register_int("data_validation_allow_drop", 0, 0, 0, game::DVAR_FLAG_NONE, true);

				dvars::register_int("com_maxfps", 0, 10, 1000, game::DVAR_FLAG_SAVED, false);
			}

			return com_register_dvars_hook.invoke<void>();
		}

		int is_item_unlocked()
		{
			return 0; // 0 == yes
		}

		void set_client_dvar_from_server_stub(void* a1, void* a2, const char* dvar, const char* value)
		{
			if (dvar == "cg_fov"s)
			{
				return;
			}

			// CG_SetClientDvarFromServer
			reinterpret_cast<void(*)(void*, void*, const char*, const char*)>(0x140236120)(a1, a2, dvar, value);
		}

		/*void aim_assist_add_to_target_list(void* a1, void* a2)
		{
			if (!dvars::aimassist_enabled->current.enabled)
				return;

			game::AimAssist_AddToTargetList(a1, a2);
		}*/

		void bsp_sys_error_stub(const char* error, const char* arg1)
		{
			if (game::environment::is_dedi())
			{
				game::Sys_Error(error, arg1);
			}
			else
			{
				scheduler::once([]()
				{
					command::execute("reconnect");
				}, scheduler::pipeline::main, 1s);
				game::Com_Error(game::ERR_DROP, error, arg1);
			}
		}

		utils::hook::detour cmd_lui_notify_server_hook;
		void cmd_lui_notify_server_stub(game::mp::gentity_s* ent)
		{
			command::params_sv params{};
			const auto menu_id = atoi(params.get(1));
			const auto client = &game::mp::svs_clients[ent->s.entityNum];

			// 22 => "end_game"
			if (menu_id == 22 && client->header.remoteAddress.type != game::NA_LOOPBACK)
			{
				return;
			}

			cmd_lui_notify_server_hook.invoke<void>(ent);
		}

		void sv_execute_client_message_stub(game::mp::client_t* client, game::msg_t* msg)
		{
			if (client->reliableAcknowledge < 0)
			{
				client->reliableAcknowledge = client->reliableSequence;
				console::info("Negative reliableAcknowledge from %s - cl->reliableSequence is %i, reliableAcknowledge is %i\n",
					client->name, client->reliableSequence, client->reliableAcknowledge);
				network::send(client->header.remoteAddress, "error", "EXE_LOSTRELIABLECOMMANDS", '\n');
				return;
			}

			reinterpret_cast<void(*)(game::mp::client_t*, game::msg_t*)>(0x140481A00)(client, msg);
		}
	}

	class component final : public component_interface
	{
	public:
		void post_unpack() override
		{
			// Register dvars
			com_register_dvars_hook.create(SELECT_VALUE(0x140351B80, 0x1400D9320), &com_register_dvars_stub); // H1(1.4)

			// Unlock fps in main menu
			utils::hook::set<BYTE>(SELECT_VALUE(0x14018D47B, 0x14025B86B), 0xEB); // H1(1.4)

			// Fix mouse lag
			utils::hook::nop(SELECT_VALUE(0x1403E3C05, 0x1404DB1AF), 6);
			scheduler::loop([]()
			{
				SetThreadExecutionState(ES_DISPLAY_REQUIRED);
			}, scheduler::pipeline::main);

			// Prevent game from overriding cg_fov and cg_fovscale values
			gscr_set_save_dvar_hook.create(SELECT_VALUE(0x1402AE020, 0x14036B600), &gscr_set_save_dvar_stub);

			// Make cg_fov and cg_fovscale saved dvars
			cg_fov = dvars::register_float("cg_fov", 65.f, 40.f, 200.f, game::DvarFlags::DVAR_FLAG_SAVED, true);
			cg_fovScale = dvars::register_float("cg_fovScale", 1.f, 0.1f, 2.f, game::DvarFlags::DVAR_FLAG_SAVED, true);

			dvar_register_float_hook.create(game::Dvar_RegisterFloat.get(), dvar_register_float_stub);

			if (game::environment::is_mp())
			{
				patch_mp();
			}
		}

		static void patch_mp()
		{
			// Use name dvar
			utils::hook::jump(0x14050FF90, &live_get_local_client_name); // H1(1.4)

			// Patch SV_KickClientNum
			sv_kick_client_num_hook.create(0x14047ED00, &sv_kick_client_num); // H1(1.4)

			// block changing name in-game
			utils::hook::set<uint8_t>(0x14047FC90, 0xC3); // H1(1.4)

			// patch "Couldn't find the bsp for this map." error to not be fatal in mp
			utils::hook::call(0x1402BA26B, bsp_sys_error_stub); // H1(1.4)

			// client side aim assist dvar
			//dvars::aimassist_enabled = game::Dvar_RegisterBool("aimassist_enabled", true,
			//	game::DvarFlags::DVAR_FLAG_SAVED,
			//	"Enables aim assist for controllers");
			//utils::hook::call(0x140003609, aim_assist_add_to_target_list);

			// unlock all items
			utils::hook::jump(0x140413E60, is_item_unlocked); // LiveStorage_IsItemUnlockedFromTable_LocalClient H1(1.4)
			utils::hook::jump(0x140413860, is_item_unlocked); // LiveStorage_IsItemUnlockedFromTable H1(1.4)
			utils::hook::jump(0x140412B70, is_item_unlocked); // idk ( unlocks loot etc ) H1(1.4)

			// isProfanity
			utils::hook::set(0x1402877D0, 0xC3C033); // MAY BE WRONG H1(1.4)

			// disable emblems
			//dvars::override::register_int("emblems_active", 0, 0, 0, game::DVAR_FLAG_NONE);
			//utils::hook::set<uint8_t>(0x140479590, 0xC3); // don't register commands

			// disable elite_clan
			dvars::override::register_int("elite_clan_active", 0, 0, 0, game::DVAR_FLAG_NONE);
			utils::hook::set<uint8_t>(0x140585680, 0xC3); // don't register commands H1(1.4)

			// disable codPointStore
			dvars::override::register_int("codPointStore_enabled", 0, 0, 0, game::DVAR_FLAG_NONE, true);

			// don't register every replicated dvar as a network dvar
			utils::hook::nop(0x14039E58E, 5); // dvar_foreach H1(1.4)

			// patch "Server is different version" to show the server client version
			utils::hook::inject(0x140480952, VERSION); // H1(1.4)

			 // prevent servers overriding our fov
			 utils::hook::call(0x14023279E, set_client_dvar_from_server_stub);
			 utils::hook::nop(0x1400DAF69, 5);
			 utils::hook::nop(0x140190C16, 5);
			 utils::hook::set<uint8_t>(0x14021D22A, 0xEB);

			// some anti tamper thing that kills performance
			dvars::override::register_int("dvl", 0, 0, 0, game::DVAR_FLAG_READ, true);

			// unlock safeArea_*
			utils::hook::jump(0x1402624F5, 0x140262503); // H1(1.4)
			utils::hook::jump(0x14026251C, 0x140262547); // H1(1.4)
			dvars::override::register_float("safeArea_adjusted_horizontal", 1, 0, 1, game::DVAR_FLAG_SAVED, true);
			dvars::override::register_float("safeArea_adjusted_vertical", 1, 0, 1, game::DVAR_FLAG_SAVED, true);
			dvars::override::register_float("safeArea_horizontal", 1, 0, 1, game::DVAR_FLAG_SAVED, true);
			dvars::override::register_float("safeArea_vertical", 1, 0, 1, game::DVAR_FLAG_SAVED, true);

			// move chat position on the screen above menu splashes
			//dvars::override::Dvar_RegisterVector2("cg_hudChatPosition", 5, 170, 0, 640, game::DVAR_FLAG_SAVED);

			// allow servers to check for new packages more often
			dvars::override::register_int("sv_network_fps", 1000, 20, 1000, game::DVAR_FLAG_SAVED, true);

			// Massively increate timeouts
			dvars::override::register_int("cl_timeout", 90, 90, 1800, game::DVAR_FLAG_NONE, true); // Seems unused
			dvars::override::register_int("sv_timeout", 90, 90, 1800, game::DVAR_FLAG_NONE, true); // 30 - 0 - 1800
			dvars::override::register_int("cl_connectTimeout", 120, 120, 1800, game::DVAR_FLAG_NONE, true); // Seems unused
			dvars::override::register_int("sv_connectTimeout", 120, 120, 1800, game::DVAR_FLAG_NONE, true); // 60 - 0 - 1800

			game::Dvar_RegisterInt(0, "scr_game_spectatetype", 1, 0, 99, game::DVAR_FLAG_REPLICATED);

			// Prevent clients from ending the game as non host by sending 'end_game' lui notification
			cmd_lui_notify_server_hook.create(0x140335A70, cmd_lui_notify_server_stub); // H1(1.4)

			// Prevent clients from sending invalid reliableAcknowledge
			// utils::hook::call(0x1404899C6, sv_execute_client_message_stub); // H1(1.4)
		}
	};
}

REGISTER_COMPONENT(patches::component)
