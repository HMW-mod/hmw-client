#include "std_include.hpp"
#include "serverInfoPopup.hpp"
#include "imgui.h"
#include "imgui_internal.h"
#include "../../fonts/IconsFontAwesome6.hpp"
#include <d3d11.h>
#include <iostream>
#include "../../themes/style.hpp"
#include "../../utils/gui_helper.hpp"


void renderInfoPopup(ServerData& currentInfoServer, bool& showInfoWindow, float& infoWindowOpenAnimationStartTime,
    float& infoWindowCloseAnimationStartTime, bool& startClosing, bool& isClosing)
{
    if (showInfoWindow) {
        ImGui::OpenPopup("InfoModal");
    }
    ImGuiWindow* pBrowserWindow = ImGui::FindWindowByName("Server Browser");
    ImVec2 browserPos = pBrowserWindow ? pBrowserWindow->Pos : ImVec2(0, 0);
    ImVec2 browserSize = pBrowserWindow ? pBrowserWindow->Size : ImGui::GetIO().DisplaySize;

    float infoWindowWidth = browserSize.x / 3.0f;
    float infoWindowHeight = browserSize.y;
    float finalX = browserPos.x + browserSize.x - infoWindowWidth;
    float finalY = browserPos.y;

    float currentTime = ImGui::GetTime();
    const float openDuration = 0.5f;
    const float closeDuration = 0.3f;
    float elapsed = 0.0f, t = 0.0f, duration = 0.0f;

    if (startClosing) {
        isClosing = true;
        if (infoWindowCloseAnimationStartTime < 0.0f)
            infoWindowCloseAnimationStartTime = currentTime;
        elapsed = currentTime - infoWindowCloseAnimationStartTime;
        duration = closeDuration;
        t = elapsed / duration;
        if (t > 1.0f)
            t = 1.0f;
    }
    else {
        if (infoWindowOpenAnimationStartTime < 0.0f)
            infoWindowOpenAnimationStartTime = currentTime;
        elapsed = currentTime - infoWindowOpenAnimationStartTime;
        duration = openDuration;
        t = elapsed / duration;
        if (t > 1.0f)
            t = 1.0f;
    }

    float factor = (!isClosing) ? (1.0f - (1.0f - t) * (1.0f - t)) : ((1.0f - t) * (1.0f - t));
    float currentWidth = infoWindowWidth * factor;

    float posX = finalX + infoWindowWidth - currentWidth;
    float posY = finalY;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowMinSize, ImVec2(0, 0));
    ImGui::SetNextWindowSizeConstraints(ImVec2(0, infoWindowHeight), ImVec2(infoWindowWidth, infoWindowHeight));
    ImGui::SetNextWindowPos(ImVec2(posX, posY));
    ImGui::SetNextWindowSize(ImVec2(currentWidth, infoWindowHeight));

    ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
    ImGui::PushStyleColor(ImGuiCol_ModalWindowDimBg, ImVec4(0, 0, 0, 0.7f));
    if (ImGui::BeginPopupModal("InfoModal", nullptr, window_flags)) {
        custom_style::PushBrowserInfoTheme();
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));

        if (currentWidth > 50.0f) {
            if (!isClosing) {
                if (ImGui::Button(ICON_FA_ARROW_LEFT_LONG)) {
                    startClosing = true;
                    infoWindowCloseAnimationStartTime = currentTime;
                }
            }

            ImGui::Separator();
            ImGui::Dummy(ImVec2(20, 0));

            ImGui::TextWrapped("%s", currentInfoServer.info.c_str());

            ImGui::Dummy(ImVec2(20, 0));
            ImGui::Separator();

            ImGui::Dummy(ImVec2(20, 0));

            ImGui::BeginTabBar("FILLER");
            if (ImGui::BeginTabItem("Server Info"))
            {
				gui::helper::gfx::draw_custom_material_image("loadscreen_mp_plaza2_vote", infoWindowWidth - 15);
                //ImGui::Image((ImTextureID)g_backgroundSRV, ImVec2(infoWindowWidth - 15, 300));
                ImGui::Spacing();

                ImGui::TextWrapped("%s", currentInfoServer.motd.c_str());
                ImGui::Dummy(ImVec2(0, 55));

                ImGui::SetCursorPosX(25);
                ImGui::Text("Map: %s", currentInfoServer.map.c_str());
                ImGui::SameLine(325);
                ImGui::Text("Game Mode: %s", currentInfoServer.gameMode.c_str());
                ImGui::Dummy(ImVec2(0, 25));
                ImGui::SetCursorPosX(25);
                ImGui::Text("Game Type: %s", currentInfoServer.gameType.c_str());
                ImGui::SameLine(325);
                ImGui::Text("Players: %d/%d", currentInfoServer.playercount, currentInfoServer.maxclients);
                ImGui::Dummy(ImVec2(0, 25));
                if (!currentInfoServer.anticheat.empty())
                {
                    ImGui::SetCursorPosX(25);
                    ImGui::Text("Anticheat Protected: True");
                }
                else
                {
                    ImGui::SetCursorPosX(25);
                    ImGui::Text("Anticheat Protected: False");
                }
                ImGui::SameLine(325);
                ImGui::Text("Version: %s", currentInfoServer.gameVersion.c_str());

                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Player List"))
            {
                ImGui::Text("Dummy Text 2");
                ImGui::EndTabItem();
            }

            float availHeight = ImGui::GetContentRegionAvail().y;
            ImGui::Dummy(ImVec2(0, availHeight - 50));

            float windowWidth = ImGui::GetWindowContentRegionMax().x;
            float buttonWidth = 150;
            ImGui::SetCursorPosX((windowWidth - buttonWidth) * 0.5f);
            if (ImGui::Button("JOIN", ImVec2(buttonWidth, 35)))
            {
                // join functionality
            }

            ImGui::EndTabBar();
        }

        ImGui::PopStyleColor();
        custom_style::PopBrowserInfoTheme();
        ImGui::EndPopup();
    }
    ImGui::PopStyleColor();
    ImGui::PopStyleVar();

    if (isClosing && (t >= 1.0f || currentWidth < 10.0f)) {
        ImGui::CloseCurrentPopup();
        showInfoWindow = false;
        isClosing = false;
        startClosing = false;
        infoWindowOpenAnimationStartTime = -1.0f;
        infoWindowCloseAnimationStartTime = -1.0f;
    }
}
