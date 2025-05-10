#include <std_include.hpp>
#include "gui.hpp"
#include "utils/image.hpp"
#include "utils/io.hpp"

#include "imgui.h"
#include "component/command.hpp"
#include "component/gui/utils/notification.hpp"
#include "component/gui/browser/server_browser.hpp"
#include "component/gui/fonts/fa-solid-900.h"
#include "component/gui/fonts/IconsFontAwesome6.h"
#include "component/gui/utils/ImGuiNotify.hpp"
#include "component/gui/utils/gui_helper.hpp"
#include "component/experimental.hpp"
#include "component/stats.hpp"
#include "component/Matchmaking/mmReporter.hpp"
#include "component/gui/browser/rework/serverBrowser.hpp"

ID3D11ShaderResourceView* Image = nullptr;

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace gui
{
	NotificationManager& GetNotificationManager() {
		static NotificationManager manager;
		return manager;
	}
	static bool show_notification_window = false;
	static bool show_styler_window = false;
	static bool show_hudColor_window = false;
	static bool show_new_browser_window = false;
	static serverBrowser myServerBrowser;

	float hud_color_r = 0.86f;
	float hud_color_g = 0.81f;
	float hud_color_b = 0.34f;
	float hudColorValues[3] = { 0.86f, 0.81f, 0.34f }; // default yellow

	std::unordered_map<std::string, bool> enabled_menus;

	char username_buffer[256] = "";
	char password_buffer[256] = "";

	namespace
	{
		struct frame_callback
		{
			std::function<void()> callback;
			bool always;
		};

		struct event
		{
			HWND hWnd;
			UINT msg;
			WPARAM wParam;
			LPARAM lParam;
		};

		struct menu_t
		{
			std::string name;
			std::string title;
			std::function<void()> render;
		};

		utils::concurrency::container<std::vector<frame_callback>> on_frame_callbacks;
		utils::concurrency::container<std::deque<notification_t>> notifications;
		utils::concurrency::container<std::vector<event>> event_queue;
		std::vector<menu_t> menus;

		ID3D11Device* device;
		ID3D11DeviceContext* device_context;
		bool initialized = false;
		bool toggled = false;
		NotificationManager notificationManager;

		void initialize_gui_context()
		{
			if (!device || !device_context)
			{
				console::error("[ImGui] Cannot initialize - device or context is null");
				return;
			}

			ImGui::CreateContext();
			ImGui::StyleColorsDark();

			if (!ImGui_ImplWin32_Init(*game::hWnd)) {
				console::error("[ImGui] Failed to initialize Win32 implementation");
				return;
			}

			if (!ImGui_ImplDX11_Init(device, device_context)) {
				console::error("[ImGui] Failed to initialize DirectX 11 implementation");
				return;
			}

			custom_style::load_fonts();
			ImGuiIO& io = ImGui::GetIO();
			io.Fonts->AddFontDefault();

			float baseFontSize = 16.0f;
			float iconFontSize = baseFontSize * 2.0f / 3.0f; // FontAwesome fonts need to have their sizes reduced by 2.0f/3.0f in order to align correctly

			static constexpr ImWchar iconsRanges[] = { ICON_MIN_FA, ICON_MAX_FA, 0 };
			ImFontConfig iconsConfig;
			iconsConfig.MergeMode = true;
			iconsConfig.PixelSnapH = true;
			iconsConfig.GlyphMinAdvanceX = iconFontSize;
			io.Fonts->AddFontFromMemoryCompressedTTF(fa_solid_900_compressed_data, fa_solid_900_compressed_size, iconFontSize, &iconsConfig, iconsRanges);

			initialized = true;
			console::info("[ImGui] Initialized successfully");
		}

		void run_event_queue()
		{
			event_queue.access([](std::vector<event>& queue)
				{
					for (const auto& event : queue)
					{
						ImGui_ImplWin32_WndProcHandler(event.hWnd, event.msg, event.wParam, event.lParam);
					}

					queue.clear();
				});
		}

		std::vector<int> imgui_colors =
		{
			ImGuiCol_FrameBg,
			ImGuiCol_FrameBgHovered,
			ImGuiCol_FrameBgActive,
			ImGuiCol_TitleBgActive,
			ImGuiCol_ScrollbarGrabActive,
			ImGuiCol_CheckMark,
			ImGuiCol_SliderGrab,
			ImGuiCol_SliderGrabActive,
			ImGuiCol_Button,
			ImGuiCol_ButtonHovered,
			ImGuiCol_ButtonActive,
			ImGuiCol_Header,
			ImGuiCol_HeaderHovered,
			ImGuiCol_HeaderActive,
			ImGuiCol_SeparatorHovered,
			ImGuiCol_SeparatorActive,
			ImGuiCol_ResizeGrip,
			ImGuiCol_ResizeGripHovered,
			ImGuiCol_ResizeGripActive,
			ImGuiCol_TextSelectedBg,
			ImGuiCol_NavHighlight,
		};

		void debug_cursor()
		{
			ImGuiIO& io = ImGui::GetIO();

			POINT cursor_pos;
			GetCursorPos(&cursor_pos);
			ScreenToClient(*game::hWnd, &cursor_pos);

			io.MousePos = ImVec2(static_cast<float>(cursor_pos.x), static_cast<float>(cursor_pos.y));

			console::info("[Cursor Debug] Mouse Pos: %.2f, %.2f", io.MousePos.x, io.MousePos.y);
			console::info("[Cursor Debug] MouseDrawCursor: %d", io.MouseDrawCursor);
		}

		void debug_render_state()
		{
			if (device == nullptr || device_context == nullptr)
			{
				console::error("[Render Debug] Device or context is null!");
			}
			else
			{
				console::info("[Render Debug] Device: %p, Context: %p", device, device_context);
			}
		}

		void debug_screen_size()
		{
			ImGuiIO& io = ImGui::GetIO();

			// Update display size from your application's window
			RECT rect;
			if (GetClientRect(*game::hWnd, &rect))
			{
				io.DisplaySize = ImVec2(
					static_cast<float>(rect.right - rect.left),
					static_cast<float>(rect.bottom - rect.top)
				);
			}
			else
			{
				io.DisplaySize = ImVec2(1920.0f, 1080.0f); // Set a fixed display size for fallback
			}
			console::info("[Screen Debug] Display Size: %.2f x %.2f", io.DisplaySize.x, io.DisplaySize.y);
		}

		void update_colors()
		{
			auto& style = ImGui::GetStyle();
			const auto colors = style.Colors;

			const auto now = std::chrono::system_clock::now();
			const auto days = std::chrono::floor<std::chrono::days>(now);
			std::chrono::year_month_day y_m_d{ days };

			if (y_m_d.month() != std::chrono::month(6))
			{
				return;
			}

			for (const auto& id : imgui_colors)
			{
				const auto color = colors[id];

				ImVec4 hsv_color =
				{
					static_cast<float>((game::Sys_Milliseconds() / 100) % 256) / 255.f,
					1.f, 1.f, 1.f,
				};

				ImVec4 rgba_color{};
				ImGui::ColorConvertHSVtoRGB(hsv_color.x, hsv_color.y, hsv_color.z, rgba_color.x, rgba_color.y, rgba_color.z);

				rgba_color.w = color.w;
				colors[id] = rgba_color;
			}
		}

		void new_gui_frame()
		{
			// Get the current render target
			ID3D11RenderTargetView* rtv = nullptr;
			ID3D11DepthStencilView* dsv = nullptr;
			device_context->OMGetRenderTargets(1, &rtv, &dsv);

			if (!rtv) {
				console::error("[ImGui] No render target view available");
				if (dsv) dsv->Release();
				return;
			}

			ImGuiIO& io = ImGui::GetIO();
			RECT rect;
			GetClientRect(*game::hWnd, &rect);
			io.DisplaySize = ImVec2(
				static_cast<float>(rect.right - rect.left),
				static_cast<float>(rect.bottom - rect.top)
			);

			ImGui::GetIO().MouseDrawCursor = toggled || login_panel::toggled_login_panel || server_browser::is_open();

			if (toggled || login_panel::toggled_login_panel || server_browser::is_open())
			{
				*reinterpret_cast<int*>(0xC9DC405_b) = 0;
				*game::keyCatchers |= 0x10;
			}
			else
			{
				*reinterpret_cast<int*>(0xC9DC405_b) = 1;
				*game::keyCatchers &= ~0x10;
			}

			update_colors();

			ImGui_ImplDX11_NewFrame();
			ImGui_ImplWin32_NewFrame();
			run_event_queue();

			ImGui::NewFrame();

			// Release resources
			rtv->Release();
			if (dsv) dsv->Release();
		}

		void end_gui_frame()
		{
			// Get current render target again for rendering
			ID3D11RenderTargetView* rtv = nullptr;
			ID3D11DepthStencilView* dsv = nullptr;
			device_context->OMGetRenderTargets(1, &rtv, &dsv);

			if (!rtv) {
				console::error("[ImGui] No render target view available for rendering");
				if (dsv) dsv->Release();
				return;
			}

			ImGui::EndFrame();
			ImGui::Render();

			// Render ImGui
			ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

			// Release resources
			rtv->Release();
			if (dsv) dsv->Release();
		}

		void toggle_menu(const std::string& name)
		{
			enabled_menus[name] = !enabled_menus[name];
		}

		std::string truncate(const std::string& text, const size_t length, const std::string& end)
		{
			return text.size() <= length
				? text
				: text.substr(0, length - end.size()) + end;
		}

		void show_notifications()
		{
			GetNotificationManager().render();

			static const auto window_flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoSavedSettings |
				ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav |
				ImGuiWindowFlags_NoMove;

			notifications.access([](std::deque<notification_t>& notifications_)
				{
					auto index = 0;
					for (auto i = notifications_.begin(); i != notifications_.end();)
					{
						const auto now = std::chrono::high_resolution_clock::now();
						if (now - i->creation_time >= i->duration)
						{
							i = notifications_.erase(i);
							continue;
						}

						const auto title = truncate(i->title, 34, "...");
						const auto text = truncate(i->text, 34, "...");

						ImGui::SetNextWindowSizeConstraints(ImVec2(250, 50), ImVec2(250, 50));
						ImGui::SetNextWindowBgAlpha(0.6f);
						ImGui::Begin(utils::string::va("Notification #%i", index), nullptr, window_flags);

						ImGui::SetWindowPos(ImVec2(10, 30.f + static_cast<float>(index) * 60.f));
						ImGui::SetWindowSize(ImVec2(250, 0));
						ImGui::Text(title.data());
						ImGui::Text(text.data());

						ImGui::End();

						++i;
						++index;
					}
				});
		}

		void menu_checkbox(const std::string& name, const std::string& menu)
		{
			ImGui::Checkbox(name.data(), &enabled_menus[menu]);
		}

		void run_frame_callbacks()
		{
			on_frame_callbacks.access([](std::vector<frame_callback>& callbacks)
				{
					for (const auto& callback : callbacks)
					{
						if (callback.always || toggled)
						{
							callback.callback();
						}
					}
				});
		}

		bool show_material_debug = false;
#ifdef DEBUG
		bool getMaterialFromView()
		{
			if (!show_material_debug)
				return false;

			ImGui::SetNextWindowSizeConstraints(ImVec2(200, 200), ImVec2(800, 800));
			ImGui::Begin("Material Debug", &show_material_debug);

			static double lastUpdateTime = 0.0;
			const double updateInterval = 0.1; // 0.1 seconds = 100 ms
			double currentTime = ImGui::GetTime();

			static std::string cached_material_name;
			static game::Material* cached_material = nullptr;
			static std::string material_name;

			if ((currentTime - lastUpdateTime) > updateInterval)
			{
				material_name = experimental::stuff::get_target_material_name();
				lastUpdateTime = currentTime;
			}

			bool name_changed = (material_name != cached_material_name);

			if (material_name.empty())
			{
				ImGui::Text("No material detected.");
				cached_material_name.clear();
				cached_material = nullptr;
			}
			else
			{
				ImGui::Text("Material: %s", material_name.c_str());

				if (name_changed || !cached_material)
				{
					cached_material_name = material_name;
					cached_material = helper::gfx::get_custom_material(material_name);
				}

				if (cached_material)
				{
					static constexpr unsigned char texture_types[] = {
						0x0, 0x1, 0x2, 0x3, 0x4, 0x5,
						0x6, 0x7, 0x8, 0x9, 0xA,
						0xB, 0xC, 0xD, 0xE
					};

					for (const auto& type : texture_types)
					{
						helper::gfx::draw_material_texture_by_type_debug(
							cached_material,
							type,
							128.0f,
							true
						);
					}
				}
				else
				{
					ImGui::Text("Failed to load material: %s", material_name.c_str());
				}
			}

			ImGui::End();
			return true;
		}
#endif

		void draw_main_menu_bar()
		{
			if (ImGui::BeginMainMenuBar())
			{

				if (ImGui::BeginMenu("Windows"))
				{
					std::unordered_map<std::string, bool> added_menus;

					for (const auto& menu : menus)
					{
						if (added_menus.find(menu.title) == added_menus.end())
						{
							menu_checkbox(menu.title, menu.name);
							added_menus[menu.title] = true;
						}
					}

					ImGui::EndMenu();
				}

				// Just for testing
				if (ImGui::BeginMenu("Testing"))
				{
					if (ImGui::MenuItem("NOTIFICATIONS"))
					{
						show_notification_window = true;
					}

					ImGui::EndMenu();
				}

				if (ImGui::BeginMenu("HUD Color Changer"))
				{
					if (ImGui::MenuItem("HUD Color Changer"))
					{
						show_hudColor_window = true;
					}
					ImGui::EndMenu();
				}
				ImGui::EndMainMenuBar();
			}

			if (show_notification_window)
			{
				ImGui::Begin("Notification Options", &show_notification_window);

				ImGui::Text("Choose your Notification:");
				ImGui::Separator();

				gui::helper::gfx::draw_custom_material_image("loadscreen_mp_plaza2_vote", 100.f);

				ImGui::Spacing();

				if (ImGui::Button("Show Success Notification"))
				{
					ImGui::InsertNotification({ ImGuiToastType::Success, 3000, "Hi, I'm a Success notification." });
				}

				if (ImGui::Button("Show Error Notification"))
				{
					ImGui::InsertNotification({ ImGuiToastType::Error, 3000, "Hi, I'm a Error notification."});
				}

				if (ImGui::Button("Show Info Notification"))
				{
					ImGui::InsertNotification({ ImGuiToastType::Info, 3000, "Hi, I'm a Info notification." });
				}

				if (ImGui::Button("Show Debug Notification"))
				{
					ImGui::InsertNotification({ ImGuiToastType::Debug, 3000, "Hi, I'm a Debug notification." });
				}
				ImGui::Spacing();

				if (helper::gfx::draw_custom_material_image_button("loadscreen_mp_plaza2_vote", 100.0f))
				{
					std::cout << "Do smth" << std::endl;
				}

				// "rankedMatchData" Public game stats
				// "customClasses" Public game classes
				// 0 What class 0-9
				// "weaponSetups" Iunno
				// 0 0-1 Weapon Primary/Secondary
				// "weapon" This determines if its a weapon i think?
				// "h2_masada" What exacly u wanna set eg. h2_masada for ACR

				std::string setPlayerClassCommand = R"(setRankedStat "rankedMatchData" "customClasses" 0 "weaponSetups" 0 "weapon" "h2_masada")";

				ImGui::Spacing();
				if (ImGui::Button("TEST COMMAND"))
				{
					command::execute("disconnect");
				}

				ImGui::Spacing();
				if (ImGui::Button("SetPlayerClass"))
				{
					command::execute(setPlayerClassCommand);
					std::cout << "Command send!" << std::endl;
				}

				if (ImGui::Button("Styler"))
				{
					show_styler_window = true;
				}
				ImGui::Spacing();
				ImGui::Spacing();
#ifdef DEBUG
				ImGui::Checkbox("Show Material Debug", &show_material_debug);

				if (show_material_debug)
				{
					getMaterialFromView();
				}

				ImGui::Checkbox("Show New Browser", &show_new_browser_window);
				if (show_new_browser_window)
				{
					myServerBrowser.draw();
				}

				ImGui::End();
#endif
			}

			// Hud Color Changer
			if (show_hudColor_window)
			{
				ImGui::SetNextWindowSize(ImVec2(310, 100));
				ImGui::Begin("HUD Color Changer", &show_hudColor_window);
				ImGui::Separator();

				ImGui::SetCursorPos(ImVec2(90, 35));
				ImGui::Text("RGB");
				if (ImGui::SliderFloat3("", hudColorValues, 0.0f, 1.0f))
				{
					hud_color_r = hudColorValues[0];
					hud_color_g = hudColorValues[1];
					hud_color_b = hudColorValues[2];
				}

				ImGui::SameLine();
				ImVec4 previewColor = ImVec4(hud_color_r, hud_color_g, hud_color_b, 1.0f);
				ImGui::ColorButton("##hud_color_preview", previewColor, ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoPicker, ImVec2(20, 20));
				ImGui::SameLine();
				if (ImGui::Button("Apply"))
				{
					command::execute("hud_color_r " + std::to_string(hud_color_r));
					command::execute("hud_color_g " + std::to_string(hud_color_g));
					command::execute("hud_color_b " + std::to_string(hud_color_b));
					command::execute("lui_restart");
				}
				ImGui::End();
			}

			if (show_styler_window)
			{
				ImGui::Begin("Styler", &show_styler_window);
				ImGui::ShowStyleEditor();
				ImGui::End();
			}
		}

		void draw_login_panel() {
			login_panel::draw_login_panel();
		}

		void draw_img_debug(std::string filepath)
		{
			login_panel::draw_img_debug(filepath);
		}

		void gui_on_frame()
		{
			if (!game::Sys_IsDatabaseReady2()) {
				return;
			}

			if (!initialized) {
				console::info("[ImGui] Initializing\n");
				initialize_gui_context();
			}
			else {
				new_gui_frame();
				run_frame_callbacks();

				if (toggled) {
					static bool main_menu_drawn = false;

					if (!main_menu_drawn) {
						draw_main_menu_bar();
						main_menu_drawn = true;
					}
				}

				ImGui::RenderNotifications();
				notificationManager.render();
				draw_login_panel();
				server_browser::show_window();
				show_notifications();

#ifdef DEBUG
				if (initialized && ImGui::GetCurrentContext() != nullptr)
				{
					if (show_material_debug)
					{
						getMaterialFromView();
					}
				}
#endif

				end_gui_frame();
			}
		}


		void shutdown_gui()
		{
			if (initialized)
			{
				ImGui_ImplDX11_Shutdown();
				ImGui_ImplWin32_Shutdown();
				ImGui::DestroyContext();
				console::info("[ImGui] Shutdown");
			}

			initialized = false;
		}

		HRESULT d3d11_create_device_stub(IDXGIAdapter* pAdapter, D3D_DRIVER_TYPE DriverType, HMODULE Software,
			UINT Flags, const D3D_FEATURE_LEVEL* pFeatureLevels, UINT FeatureLevels, UINT SDKVersion,
			ID3D11Device** ppDevice, D3D_FEATURE_LEVEL* pFeatureLevel, ID3D11DeviceContext** ppImmediateContext)
		{
			shutdown_gui();

			const auto result = D3D11CreateDevice(pAdapter, DriverType, Software, Flags, pFeatureLevels,
				FeatureLevels, SDKVersion, ppDevice, pFeatureLevel, ppImmediateContext);

			if (SUCCEEDED(result) && ppDevice != nullptr && ppImmediateContext != nullptr)
			{
				device = *ppDevice;
				device_context = *ppImmediateContext;
				console::info("[ImGui] D3D11 device created successfully");
			}

			return result;
		}

		utils::hook::detour wnd_proc_hook;
		LRESULT wnd_proc_stub(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
		{
			if (wParam != VK_ESCAPE && (toggled || login_panel::toggled_login_panel || server_browser::is_open()))
			{
				event_queue.access([hWnd, msg, wParam, lParam](std::vector<event>& queue)
					{
						queue.emplace_back(hWnd, msg, wParam, lParam);
					});
			}

			return wnd_proc_hook.invoke<LRESULT>(hWnd, msg, wParam, lParam);
		}
	}

	bool gui_key_event(const int local_client_num, const int key, const int down)
	{
#ifdef DEBUG  // We only want the Debug menu to work in Debug
		if (key == game::K_F11 && down)
		{
			toggled = !toggled;
			return false;
		}

		if (key == game::K_INS && down)
		{
			login_panel::toggled_login_panel = !login_panel::toggled_login_panel;
			return false;
		}
#endif
		if (key == game::K_ESCAPE && down && (toggled || login_panel::toggled_login_panel || server_browser::is_open()))
		{
			toggled = false;
			login_panel::toggled_login_panel = false;
			if (server_browser::is_open()) {
				server_browser::toggle();
			}
			return false;
		}

		return !(toggled || login_panel::toggled_login_panel || server_browser::is_open());
	}

	bool gui_char_event(const int local_client_num, const int key)
	{
		return !toggled;
	}

	bool gui_mouse_event(const int local_client_num, int x, int y)
	{
		return !toggled;
	}

	void on_frame(const std::function<void()>& callback, bool always)
	{
		on_frame_callbacks.access([always, callback](std::vector<frame_callback>& callbacks)
			{
				callbacks.emplace_back(callback, always);
			});
	}

	bool is_menu_open(const std::string& name)
	{
		return enabled_menus[name];
	}

	void gui::notification(const std::string& title, const std::string& text, const std::chrono::milliseconds duration) {
		notification_t notification{};
		notification.title = title;
		notification.text = text;
		notification.duration = duration;
		notification.creation_time = std::chrono::high_resolution_clock::now();

		gui::GetNotificationManager().addNotification(title, NotificationType::Info, duration.count() / 1000.0f);
	}

	void copy_to_clipboard(const std::string& text)
	{
		utils::string::set_clipboard_data(text);
		gui::notification("Text copied to clipboard", utils::string::va("\"%s\"", text.data()));
	}

	void register_menu(const std::string& name, const std::string& title, const std::function<void()>& callback, bool always)
	{
		auto it = std::find_if(menus.begin(), menus.end(), [&](const menu_t& menu) {
			return menu.name == name || menu.title == title;
			});

		if (it != menus.end())
		{
			return;
		}

		menus.emplace_back(menu_t{ name, title, callback });
		enabled_menus[name] = false;

		on_frame([=]
			{
				if (enabled_menus.at(name))
				{
					callback();
				}
			}, always);
	}

	void register_callback(const std::function<void()>& callback, bool always)
	{
		on_frame([=]
			{
				callback();
			}, always);
	}

	bool InputU8(const char* label, unsigned char* v, int step, int step_fast, ImGuiInputTextFlags flags)
	{
		// Hexadecimal input provided as a convenience but the flag name is awkward. Typically you'd use InputText() to parse your own data, if you want to handle prefixes.
		const char* format = (flags & ImGuiInputTextFlags_CharsHexadecimal) ? "%08X" : "%d";
		return ImGui::InputScalar(label, ImGuiDataType_U8, (void*)v, (void*)(step > 0 ? &step : NULL), (void*)(step_fast > 0 ? &step_fast : NULL), format, flags);
	}

	bool InputUInt6(const char* label, unsigned int v[6], ImGuiInputTextFlags flags)
	{
		return ImGui::InputScalarN(label, ImGuiDataType_U32, v, 6, NULL, NULL, "%d", flags);
	}

	void register_window_menu(const std::string& name, const std::string& title,
		const std::function<void()>& callback, bool always = false)
	{
		register_menu(name, title, [title, callback]()
			{
				if (ImGui::Begin(title.c_str()))
				{
					callback();
				}
				ImGui::End();
			}, always);
	}

	bool create_texture_view_from_memory(const std::string& data_buffer, ID3D11ShaderResourceView*& out_tex_view, int& out_width, int& out_height)
	{
		if (device == NULL)
		{
			console::error("Tried to create DX11 text view while gui dx11 device pointer was null.");
			return false;
		}

		auto img = utils::image(data_buffer);
		const char* raw_buffer = reinterpret_cast<const char*>(img.get_buffer());
		int img_width = img.get_width();
		int img_height = img.get_height();

		// Create texture
		D3D11_TEXTURE2D_DESC desc = {};
		ZeroMemory(&desc, sizeof(desc));
		desc.Width = img_width;
		desc.Height = img_height;
		desc.MipLevels = 1;
		desc.ArraySize = 1;
		desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		desc.SampleDesc.Count = 1;
		desc.Usage = D3D11_USAGE_DEFAULT;
		desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		desc.CPUAccessFlags = 0;

		ID3D11Texture2D* pTexture = nullptr;
		D3D11_SUBRESOURCE_DATA subResource;
		subResource.pSysMem = raw_buffer;
		subResource.SysMemPitch = desc.Width * 4;
		subResource.SysMemSlicePitch = 0;
		HRESULT res = device->CreateTexture2D(&desc, &subResource, &pTexture);
		if (res != S_OK || pTexture == NULL) {
			console::error("Unable to process 2D GUI Texture");
			return false;
		}

		// Create texture view
		D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
		ZeroMemory(&srvDesc, sizeof(srvDesc));
		srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MipLevels = desc.MipLevels;
		srvDesc.Texture2D.MostDetailedMip = 0;
		auto hr = device->CreateShaderResourceView(pTexture, &srvDesc, &out_tex_view);
		if (FAILED(hr))
		{
			return false;
		}

		pTexture->Release();

		out_width = img_width;
		out_height = img_height;

		return true;
	}

	bool create_texture_view_from_filepath(const std::string& filepath, ID3D11ShaderResourceView*& out_tex_view, int& out_width, int& out_height)
	{
		const std::string file_data = utils::io::read_file(filepath);

		bool res = create_texture_view_from_memory(file_data, out_tex_view, out_width, out_height);
		if (res == false)
		{
			console::error(utils::string::va("Unable to generate texture from file with path: %s", filepath));
			return false;
		}
	}

	class component final : public component_interface
	{
	public:
		void* load_import(const std::string& library, const std::string& function) override
		{
			if (function == "D3D11CreateDevice" && !game::environment::is_dedi())
			{
				return d3d11_create_device_stub;
			}

			return nullptr;
		}

		void post_unpack() override
		{
			if (game::environment::is_dedi())
			{
				return;
			}

			utils::hook::nop(0x6CB16D_b, 9);
			utils::hook::call(0x6CB170_b, gui_on_frame);
			wnd_proc_hook.create(0x5BFF60_b, wnd_proc_stub);

			on_frame([]
				{
					show_notifications();
					draw_main_menu_bar();
				});
		}

		void pre_destroy() override
		{
			if (game::environment::is_dedi())
			{
				return;
			}

			wnd_proc_hook.clear();

			on_frame_callbacks.access([](std::vector<frame_callback>& callbacks) {
				callbacks.clear();
				});
			event_queue.access([](std::vector<event>& queue) {
				queue.clear();
				});

			shutdown_gui();
		}
	};
}

REGISTER_COMPONENT(gui::component)