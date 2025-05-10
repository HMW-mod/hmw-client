#include <std_include.hpp>
#include "login_panel.hpp"
#include "component/gui/themes/style.hpp"
#include "component/gui/utils/imgui_markdown.h"
#include <d3d11.h>
#include "component/console.hpp"
#include "component/command.hpp"
#include "component/debug_gui/gui.hpp"
#include "component/Matchmaking/hmw_matchmaking.hpp"
#include "utils/io.hpp"
#include "utils/image.hpp"
#include "imgui_internal.h"
#include "tcp/hmw_tcp_utils.hpp"
#include "user_profile.hpp"
#include "game/ui_scripting/execution.hpp"
#include "component/gui/utils/notification.hpp"
#include "component/gui/utils/ImGuiNotify.hpp"

namespace login_panel
{

    char username_buffer[256] = "";
    char password_buffer[256] = "";
    bool toggled_login_panel = false;

    LoginPanelState current_panel_state = LoginPanelState::MAIN_PANEL;

    void LinkCallback(ImGui::MarkdownLinkCallbackData data)
    {
        std::string url(data.link, data.linkLength);

        if (url == "username_login")
        {
            current_panel_state = LoginPanelState::USERNAME_LOGIN;
        }
        else if (url == "register_account")
        {
            ShellExecute(0, "open", "https://backend.horizonmw.org/admin/register", 0, 0, SW_SHOWNORMAL);
        }
    }


    ImGui::MarkdownConfig GetMarkdownConfig()
    {
        ImGui::MarkdownConfig config;
        config.linkCallback = LinkCallback;

        config.headingFormats[0] = { nullptr, true }; // H1
        config.headingFormats[1] = { nullptr, false }; // H2
        config.headingFormats[2] = { nullptr, false }; // H3
        config.userData = nullptr;

        return config;
    }

    void draw_login_panel()
    {

        if (!toggled_login_panel) return;

        custom_style::PushTestStyle();
        custom_style::draw_dimmed_background();

        ImGuiIO& io = ImGui::GetIO();

        ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));

        switch (current_panel_state)
        {
        case LoginPanelState::MAIN_PANEL:
            ImGui::SetNextWindowSize(ImVec2(455, 175));
            break;
        case LoginPanelState::USERNAME_LOGIN:
            ImGui::SetNextWindowSize(ImVec2(455, 295));
            break;
        case LoginPanelState::LOADING:
            ImGui::SetNextWindowSize(ImVec2(180, 180));
            ImGui::SetNextWindowBgAlpha(0.0f);
            break;
        case LoginPanelState::LOGGED_IN:
            ImGui::SetNextWindowSize(ImVec2(600, 425));
            break;
        }

        ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar;

        if (ImGui::Begin("LOGIN DETAILS", &toggled_login_panel, window_flags))
        {
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(15, 15));

            float window_width = ImGui::GetWindowWidth();
            ImGui::MarkdownConfig config = GetMarkdownConfig();

            float input_width = 300.0f;
            std::string buttonText = "";

            switch (current_panel_state)
            {
            case LoginPanelState::MAIN_PANEL:
                buttonText = std::string((const char*)u8"\uf392 ") + "Login with Discord";

                ImGui::SetCursorPosX((window_width - ImGui::CalcTextSize("Login to Matchmaking").x) * 0.5f);
                ImGui::Text("Login to Matchmaking");
                ImGui::Spacing();

                custom_style::PushCustomButtonStyle();

                if (custom_style::CustomButton(buttonText.c_str()))
                {
                    perform_discord_login();
                }

                custom_style::PopCustomButtonStyle();
                custom_style::CustomCheckbox("Remember Me", &toggled_login_panel, custom_style::Alignment::Center);
                custom_style::DrawMarkdownButton(
                    "Login with username and password",
                    nullptr,
                    window_width,
                    []() { current_panel_state = LoginPanelState::USERNAME_LOGIN; },
                    ImVec4(0.5f, 0.5f, 0.5f, 1.0f),
                    ImVec4(1.0f, 0.7f, 0.0f, 1.0f)
                );
                break;


            case LoginPanelState::USERNAME_LOGIN:
                ImGui::SetCursorPosX((window_width - ImGui::CalcTextSize("Login with Email and Password").x) * 0.5f);
                ImGui::Text("Login with Email and Password");
                ImGui::Spacing();

                

                ImGui::SetCursorPosX((window_width - input_width) * 0.5f);
                ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.0f, 0.0f, 0.0f, 0.1f));
                ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.0f, 0.0f, 0.0f, 0.3f));
                ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0.0f, 0.0f, 0.0f, 0.5f));
                ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 5.0f);
                ImGui::PushItemWidth(input_width);
                ImGui::InputTextWithHint("##email", "EMAIL", username_buffer, IM_ARRAYSIZE(username_buffer));
                ImGui::PopItemWidth();
                ImGui::PopStyleColor(3);
                ImGui::PopStyleVar(1);
                ImGui::Spacing();

                ImGui::SetCursorPosX((window_width - input_width) * 0.5f);
                ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.0f, 0.0f, 0.0f, 0.1f));
                ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.0f, 0.0f, 0.0f, 0.3f));
                ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0.0f, 0.0f, 0.0f, 0.5f));
                ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 5.0f);
                ImGui::PushItemWidth(input_width);
                ImGui::InputTextWithHint("##password", "PASSWORD", password_buffer, IM_ARRAYSIZE(password_buffer), ImGuiInputTextFlags_Password);
                ImGui::PopItemWidth();
                ImGui::PopStyleColor(3);
                ImGui::PopStyleVar(1);
                ImGui::Spacing();

                custom_style::DrawMarkdownButton(
                    "Register account",
                    "https://backend.horizonmw.org/admin/register",
                    window_width,
                    nullptr,
                    ImVec4(0.5f, 0.5f, 0.5f, 1.0f),
                    ImVec4(1.0f, 0.7f, 0.0f, 1.0f)
                );

                custom_style::PushCustomButtonStyle();
                if (custom_style::CustomButton("LOGIN"))
                {
                    current_panel_state = LoginPanelState::LOADING;
                    perform_directus_login(std::string(username_buffer), std::string(password_buffer));
                }
                custom_style::PopCustomButtonStyle();

                custom_style::PushCustomButtonStyle();
                if (custom_style::CustomButton("CANCEL"))
                {
                    current_panel_state = LoginPanelState::MAIN_PANEL;
                }
                custom_style::PopCustomButtonStyle();
                break;

            case LoginPanelState::LOADING:
            {
                ImGuiIO& io = ImGui::GetIO();
                ImVec2 center = ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f);
                ImGuiStyle& style = ImGui::GetStyle();
                style.WindowBorderSize = 0.0f;
                custom_style::LoadingIndicatorCircle(
                    "loading_circle",
                    75.0f, // Size
                    ImVec4(0.5f, 0.45f, 0.2f, 1.0f), // Main color
                    ImVec4(0.3f, 0.3f, 0.3f, 0.4f), // sec color
                    8,    // number of circles
                    2.5f  // thickness
                );

                window_flags |= ImGuiWindowFlags_NoBackground;

                if (current_panel_state != LoginPanelState::LOADING)
                {
                    break;
                }
            }
            break;

            case LoginPanelState::LOGGED_IN:
                const std::string welcomeMessage = "Welcome, " + user_profile::get_username() + "!";
                show_welcome_panel(window_width, welcomeMessage);
                break;
            }

            ImGui::PopStyleVar();
        }
        ImGui::End();
        custom_style::PopTestStyle();
    }

    void perform_discord_login()
    {
        current_panel_state = LoginPanelState::LOADING;

        hmw_matchmaking::Auth::AuthState lastState = hmw_matchmaking::Auth::AuthState::FAILED;
        hmw_matchmaking::Auth::Discord::loginWithDiscord([&lastState](hmw_matchmaking::Auth::AuthState response_state, std::string response, std::string bearer_token) {
            if (response.empty()) {
                console::error("Failed to login. No response.");
                reset_profile_stats();
                return;
            }

            nlohmann::json jsonData = nlohmann::json::parse(response);

            if (jsonData.empty()) {
                console::error("Failed to login. Response data is empty.");
                reset_profile_stats();
                return;
            }

            if (jsonData["data"].empty()) {
                console::error("Failed to login. Data is empty.");
                reset_profile_stats();
                return;
            }

            switch (response_state) {
                case hmw_matchmaking::Auth::AuthState::SUCCESS:
                {
                    // Doing it like this sucks... too bad
                    std::string profile_username = jsonData["data"][0]["username"].get<std::string>();
                    int profile_prestige = jsonData["data"][0]["prestige"].get<int>();
                    int profile_rankXp = jsonData["data"][0]["rank_xp"].get<int>();
                    std::string profile_avatar = jsonData["data"][0]["player_account"]["avatar"].get<std::string>();

                    console::info("Successfully logged in with Discord");

                    // Update display vars
                    user_profile::set_username(profile_username);
                    user_profile::set_prestige(profile_prestige);
                    user_profile::set_rank_xp(profile_rankXp);
                    user_profile::set_avatar(profile_avatar);

                    // Download the avatar image.
                    std::string avatar_url = "https://backend.horizonmw.org/assets/" + profile_avatar + "?key=system-medium-cover";
                    utils::io::download_file_with_auth(avatar_url, "account/avatar.jpg", bearer_token);
                    user_profile::login();

                    std::cout << "========== User Info ==========\n"
                        << "Username: " << profile_username << "\n"
                        << "Prestige: " << profile_prestige << "\n"
                        << "Rank XP:  " << profile_rankXp << "\n"
                        << "Avatar: " << profile_avatar << "\n"
                        << "Avatar URL: " << avatar_url << "\n"
                        << "Bearer Token: " << bearer_token << "\n"
                        << "===============================" << std::endl;

                    current_panel_state = LoginPanelState::LOGGED_IN;
                    break;
                }
                case hmw_matchmaking::Auth::AuthState::FAILED:
                    console::info("Failed to login with discord.");
                    ImGui::InsertNotification({ ImGuiToastType::Error, 3000, "Login failed." });
                    reset_profile_stats();
                    break;
                default:
                    ImGui::InsertNotification({ ImGuiToastType::Error, 3000, "Login failed." });
                    console::info("Login with discord return unknown response.");
                    reset_profile_stats();
                    break;
                }

                // The state is different flag lui to update
                if (lastState != response_state) {
                    // Run on main thread
                    scheduler::once([]() {
                        ui_scripting::notify("handleLogin", {}); // Tell lui to update
                        ui_scripting::notify("handleMyProfile", {});
                        command::execute("lui_restart");
                        command::execute("lui_open menu_xboxlive");
                    }, scheduler::main);
                    lastState = response_state;
                }
            });
    }

    void perform_directus_login(std::string email, std::string password)
    {
        current_panel_state = LoginPanelState::LOADING;

        hmw_matchmaking::Auth::AuthState lastState = hmw_matchmaking::Auth::AuthState::FAILED;
        hmw_matchmaking::Auth::Directus::loginWithDirectus(email, password, [&lastState](hmw_matchmaking::Auth::AuthState response_state, std::string response, std::string bearer_token) {
            if (response.empty()) {
                console::error("Failed to login. No response.");
                return;
            }

            nlohmann::json jsonData = nlohmann::json::parse(response);

            if (jsonData.empty()) {
                console::error("Failed to login. Response data is empty.");
                return;
            }

            if (jsonData["data"].empty()) {
                console::error("Failed to login. Data is empty.");
                return;
            }

            switch (response_state) {
                case hmw_matchmaking::Auth::AuthState::SUCCESS:
                {
                    // Doing it like this sucks... too bad
                    std::string profile_username = jsonData["data"][0]["username"].get<std::string>();
                    int profile_prestige = jsonData["data"][0]["prestige"].get<int>();
                    int profile_rankXp = jsonData["data"][0]["rank_xp"].get<int>();
                    std::string profile_avatar = jsonData["data"][0]["player_account"]["avatar"].get<std::string>();

                    console::info("Successfully logged in with Directus");

                    // Update display vars
                    user_profile::set_username(profile_username);
                    user_profile::set_prestige(profile_prestige);
                    user_profile::set_rank_xp(profile_rankXp);
                    user_profile::set_avatar(profile_avatar);

                    // Download the avatar image.
                    std::string avatar_url = "https://backend.horizonmw.org/assets/" + profile_avatar + "?key=system-medium-cover";
                    utils::io::download_file_with_auth(avatar_url, "account/avatar.jpg", bearer_token);
                    user_profile::login();

                    std::cout << "========== User Info ==========\n"
                        << "Username: " << profile_username << "\n"
                        << "Prestige: " << profile_prestige << "\n"
                        << "Rank XP:  " << profile_rankXp << "\n"
                        << "Avatar: " << profile_avatar << "\n"
                        << "Avatar URL: " << avatar_url << "\n"
                        << "Bearer Token: " << bearer_token << "\n"
                        << "===============================" << std::endl;

                    current_panel_state = LoginPanelState::LOGGED_IN;
                    break;
                }
                case hmw_matchmaking::Auth::AuthState::FAILED:
                    console::info("Failed to login with Directus.");
                    ImGui::InsertNotification({ ImGuiToastType::Error, 3000, "Login failed." });
                    reset_profile_stats();
                    break;
                default:
                    ImGui::InsertNotification({ ImGuiToastType::Error, 3000, "Login failed." });
                    console::info("Login with Directus return unknown response.");
                    reset_profile_stats();
                    break;
            }

            // The state is different flag lui to update
            if (lastState != response_state) {
                // Run on main thread
                scheduler::once([]() {
                    ui_scripting::notify("handleLogin", {}); // Tell lui to update
                    ui_scripting::notify("handleMyProfile", {});
                    command::execute("lui_restart");
                    command::execute("lui_open menu_xboxlive");
                    }, scheduler::main);
                lastState = response_state;
            }
            });
    }

    void show_welcome_panel(float window_width, const std::string& welcomeMessage)
    {
        ImGui::Spacing();
        ImGui::SetCursorPosX((window_width - ImGui::CalcTextSize(welcomeMessage.c_str()).x) * 0.5f);
        ImGui::Text(welcomeMessage.c_str());

        ImGui::Separator();
        ImGui::Spacing();

        float column_width = window_width * 0.5f;

        ImGui::Columns(2, nullptr, false);
        ImGui::SetColumnWidth(0, column_width);
        ImGui::SetColumnWidth(1, column_width);

        static ID3D11ShaderResourceView* avatar_texture = nullptr;
        static int avatar_width = 0;
        static int avatar_height = 0;
        static bool texture_loaded = false;

        if (!texture_loaded && !user_profile::get_avatar().empty())
        {
            std::string avatar_path = "account/avatar.jpg";
            if (utils::io::file_exists(avatar_path))
            {
                bool res = gui::create_texture_view_from_filepath(avatar_path, avatar_texture, avatar_width, avatar_height);
                if (res)
                {
                    texture_loaded = true;
                }
            }
        }

        if (avatar_texture)
        {
            float image_size = 200.0f;
            ImGui::SetCursorPosX((column_width - image_size) * 0.5f);
            ImGui::Image((ImTextureID)(intptr_t)avatar_texture, ImVec2(image_size, image_size));
        }
        else
        {
            ImGui::Text("Profile picture not loaded");
        }

        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 5.0f);

        ImGui::SetCursorPosX((column_width - 200.0f) * 0.5f);
        if (custom_style::CustomButton("Logout", 200.0f, 40.0f))
        {
            reset_profile_stats();
            utils::io::remove_directory("account");
            scheduler::once([]() {
                ui_scripting::notify("handleLogin", {}); // Tell lui to update
                ui_scripting::notify("handleMyProfile", {});
                command::execute("lui_restart");
                command::execute("lui_open menu_xboxlive");
            }, scheduler::main);
            current_panel_state = LoginPanelState::MAIN_PANEL;
        }

        ImGui::NextColumn();

        std::string prestigeText = "Prestige: " + std::to_string(user_profile::get_prestige());
        ImGui::Text(prestigeText.c_str());

        std::string rankXpText = "Rank XP: " + std::to_string(user_profile::get_rank_xp());
        ImGui::Text(rankXpText.c_str());

        ImGui::Text("Other stats can go here...");

        ImGui::Columns(1);

        ImGui::Spacing();
        ImGui::Separator();
    }
    
    void reset_profile_stats() {
        user_profile::reset();
        // Also hide loading screen here
        current_panel_state = LoginPanelState::MAIN_PANEL;
    }
}

    // this is ONLY an example of how to implement images in imgui using the gui image helpers
    // FOR OPTIMIZATION REASONS YOU SHOULD NOT BE GENERATING THE RESOURCE VIEW ON EVERY RENDER LOOP, AS I DO HERE.
    // THE BELOW CODE IS PURELY PROOF OF CONCEPT
    void draw_img_debug(std::string filepath)
    {
        if (!utils::io::file_exists(filepath) || !login_panel::toggled_login_panel)
        {
            return;
        }
        // get texture view from gui helper
        int img_width = 0;
        int img_height = 0;
        ID3D11ShaderResourceView* shader_resource_view = nullptr;

        bool res = gui::create_texture_view_from_filepath(filepath, shader_resource_view, img_width, img_height);
        if (!res) {
            return;
        }

        ImGuiIO& io = ImGui::GetIO();
        ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar;

        ImGui::Begin("DirectX11 Texture Test");
        ImGui::Text("pointer = %p", filepath);
        ImGui::Text("size = %d x %d", img_width, img_height);
        ImGui::Image((ImTextureID)(intptr_t)shader_resource_view, ImVec2(img_width, img_height));
        ImDrawList* draw_list = ImGui::GetBackgroundDrawList();
        draw_list->AddImage(
            (ImTextureID)(intptr_t)shader_resource_view,
            ImVec2(0, 0), io.DisplaySize,
            ImVec2(0.0f, 0.0f),
            ImVec2(1.0f, 1.0f)
        );
    ImGui::End();

    // README (LISA) - This is how you convert a character buffer in memory into ID3D11ShaderResourceView for use with IMGui
    // NOTE: generating the resource view ideally should only be done ONCE rather than during ever render call in imgui
    // so all the imgui resources should be preloaded or something when the imgui ui inits for the first time or something
    // otherwise you'll see a massive performance dip as the resource is recreated every frame
    // 
    // int lisa_img_width = 0;
    // int lisa_img_height = 0;
    // ID3D11ShaderResourceView* lisa_img = nullptr;
    // std::string imageData(reinterpret_cast<const char*>(test_image_from_memory), sizeof test_image_from_memory);
    // bool res2 = gui::create_texture_view_from_memory(imageData, lisa_img, lisa_img_width, lisa_img_height);
    // if (!res2) {
    //     return;
    // }
    // ImGui::Begin("DirectX11 Memory Texture Test");
    // ImGui::Text("pointer = %p", filepath);
    // ImGui::Text("size = %d x %d", lisa_img_width, lisa_img_height);
    // ImGui::Image((ImTextureID)(intptr_t)lisa_img, ImVec2(lisa_img_width, lisa_img_height));
    // ImGui::End();
};
