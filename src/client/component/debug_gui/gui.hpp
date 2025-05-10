#pragma once

#include "loader/component_loader.hpp"

#include "game/game.hpp"
#include "game/dvars.hpp"

#include "component/scheduler.hpp"
#include "component/console.hpp"
#include "component/gui/themes/style.hpp"
#include "component/gui/login_panel/login_panel.hpp"

#include <backends/imgui_impl_win32.h>
#include <backends/imgui_impl_dx11.h>

#include <utils/string.hpp>
#include <utils/hook.hpp>
#include <utils/concurrency.hpp>
#include "component/gui/utils/notification.hpp"

namespace gui
{
	struct notification_t
	{
		std::string title;
		std::string text;
		std::chrono::milliseconds duration{};
		std::chrono::high_resolution_clock::time_point creation_time{};
	};

	NotificationManager& GetNotificationManager();

	extern std::unordered_map<std::string, bool> enabled_menus;

	extern float hud_color_r;
	extern float hud_color_g;
	extern float hud_color_b;

	bool gui_key_event(const int local_client_num, const int key, const int down);
	bool gui_char_event(const int local_client_num, const int key);
	bool gui_mouse_event(const int local_client_num, int x, int y);

	void on_frame(const std::function<void()>& callback, bool always = false);
	bool is_menu_open(const std::string& name);
	void notification(const std::string& title, const std::string& text, const std::chrono::milliseconds duration = 3s);
	void copy_to_clipboard(const std::string& text);

	void register_menu(const std::string& name, const std::string& title,
		const std::function<void()>& callback, bool always = false);

	void register_callback(const std::function<void()>& callback, bool always = false);

	bool InputU8(const char* label, unsigned char* v, int step = 1, int step_fast = 100, ImGuiInputTextFlags flags = 0);
	bool InputUInt6(const char* label, unsigned int v[6], ImGuiInputTextFlags flags = 0);

	bool create_texture_view_from_filepath(const std::string& filepath, ID3D11ShaderResourceView*& out_tex_view, int& out_width, int& out_height);
	bool create_texture_view_from_memory(const std::string& data_buffer, ID3D11ShaderResourceView*& out_tex_view, int& out_width, int& out_height);
}
