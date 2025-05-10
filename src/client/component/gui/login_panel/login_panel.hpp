#ifndef LOGIN_PANEL_HPP
#define LOGIN_PANEL_HPP

#include <string>
#include <imgui.h>
#include "component/gui/utils/imgui_markdown.h"

namespace login_panel
{
    enum class LoginPanelState {
        MAIN_PANEL,
        USERNAME_LOGIN,
        LOADING,
        LOGGED_IN
    };

    extern LoginPanelState current_panel_state;
    ImGui::MarkdownConfig GetMarkdownConfig();
    extern char username_buffer[256];
    extern char password_buffer[256];
    extern bool toggled_login_panel;
    
    void draw_login_panel();

    void perform_discord_login();
    void perform_directus_login(std::string email, std::string password);

    void show_welcome_panel(float window_width, const std::string& welcomeMessage);
    void reset_profile_stats();

    void draw_img_debug(std::string filepath);
}

#endif
