#include "std_include.hpp"
#include "serverBrowser.hpp"
#include "server_browser_data.hpp"
#include "imgui.h"
#include <algorithm>
#include <cctype>
#include <cstring>
#include <cstdio>
#include <iostream>
#include <set>
#include <string>

#include "imgui_internal.h"
#include <d3d11.h>
#include "component/gui/fonts/IconsFontAwesome6.hpp"
#include "serverInfoPopup.hpp"
#include "component/gui/utils/ImGuiRangeSlider.h"
#include "component/gui/themes/style.hpp"
#include "../server_browser.hpp"

extern ID3D11ShaderResourceView* g_backgroundSRV;

static int naturalCompare(const std::string& a, const std::string& b) {
    size_t i = 0, j = 0;
    while (i < a.size() && j < b.size()) {
        if (std::isdigit(a[i]) && std::isdigit(b[j])) {
            long numA = 0, numB = 0;
            size_t start_i = i, start_j = j;
            while (i < a.size() && std::isdigit(a[i])) {
                numA = numA * 10 + (a[i] - '0');
                i++;
            }
            while (j < b.size() && std::isdigit(b[j])) {
                numB = numB * 10 + (b[j] - '0');
                j++;
            }
            if (numA != numB) {
                return (numA < numB) ? -1 : 1;
            }
            size_t lenA = i - start_i, lenB = j - start_j;
            if (lenA != lenB) {
                return (lenA < lenB) ? -1 : 1;
            }
            continue;
        }
        else {
            char ca = std::tolower(a[i]);
            char cb = std::tolower(b[j]);
            if (ca != cb) {
                return (ca < cb) ? -1 : 1;
            }
            i++;
            j++;
        }
    }
    if (i < a.size()) return 1;
    if (j < b.size()) return -1;
    return 0;
}

serverBrowser::serverBrowser()
    : opened(true), currentPage(Page::INTERNET), currentSortType(SortType::SERVER_NAME), sortAscending(true),
    showInfoWindow(false), infoWindowAnimationStartTime(0.0f),
    filterShowAnticheat(false), filterShowFullServers(true), filterShowEmptyServers(true),
    filterMinPing(0), filterMaxPing(999),
    infoWindowOpenAnimationStartTime(-1.0f), infoWindowCloseAnimationStartTime(-1.0f),
    isClosing(false), startClosing(false)
{
    memset(searchBuffer, 0, sizeof(searchBuffer));
    memset(mapFilterInput, 0, sizeof(mapFilterInput));
    memset(gameModeFilterInput, 0, sizeof(gameModeFilterInput));
    serverList = GetFakeServerData();
    filteredServerList = serverList;
}

void serverBrowser::toggle() {
    opened = !opened;
}

void serverBrowser::draw() {
    if (!opened)
        return;

    custom_style::PushBrowserTheme();

    ImGui::SetNextWindowSize(ImVec2(1700, 900));

    if (ImGui::Begin("Server Browser", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoMove)) {
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10, 10));
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 10);

        ImGuiStyle& style = ImGui::GetStyle();
        auto FramePadding = style.FramePadding;
        style.FramePadding = ImVec2(25, 10);

        if (ImGui::BeginTabBar("ServerTabs")) {
            if (ImGui::BeginTabItem("Internet")) {
                style.FramePadding = FramePadding;
                currentPage = Page::INTERNET;
                renderServerList();
                ImGui::EndTabItem();
            }

            style.FramePadding = ImVec2(25, 10);
            if (ImGui::BeginTabItem("Favourites")) {
                style.FramePadding = FramePadding;
                currentPage = Page::FAVORITES;
                renderServerList();
                ImGui::EndTabItem();
            }

            style.FramePadding = ImVec2(25, 10);
            if (ImGui::BeginTabItem("History")) {
                style.FramePadding = FramePadding;
                currentPage = Page::HISTORY;
                renderServerList();
                ImGui::EndTabItem();
            }
            style.FramePadding = FramePadding;
            ImGui::EndTabBar();
        }
        ImGui::PopStyleVar();

        custom_style::PopBrowserTheme();
        ImGui::End();
    }

    if (showInfoWindow) {
        renderInfoPopup(currentInfoServer, showInfoWindow, infoWindowOpenAnimationStartTime, infoWindowCloseAnimationStartTime, startClosing, isClosing);
    }
}

void serverBrowser::renderServerList() {
    ImGui::SetNextItemWidth(950);
    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.15f, 0.15f, 0.15f, 1.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8, 11));
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 9);
    ImGui::InputTextWithHint("##Search", "Search by Servername...", searchBuffer, sizeof(searchBuffer));
    ImGui::SameLine();

    if (ImGui::Button("Refresh", ImVec2(135, 40))) {
        serverList = GetFakeServerData();
    }

    ImGui::SameLine();
    if (ImGui::Button(ICON_FA_FILTER, ImVec2(135, 40))) {
        ImVec2 btnPos = ImGui::GetItemRectMin();
        ImVec2 btnSize = ImGui::GetItemRectSize();
        float popupWidth = 300.0f;
        ImVec2 popupPos(btnPos.x + (btnSize.x - popupWidth) * 0.5f, btnPos.y + btnSize.y);
        ImGui::SetNextWindowPos(popupPos);
        ImGui::SetNextWindowSize(ImVec2(popupWidth, 0));
        ImGui::OpenPopup("FilterPopup");
    }

    custom_style::PopBrowserTheme();
    custom_style::PushBrowserInfoTheme();

    if (ImGui::BeginPopup("FilterPopup", ImGuiWindowFlags_NoMove))
    {
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4, 4));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 6));

        ImGui::PushItemWidth(100);
        ImGui::Checkbox("Show Anticheat Protected Servers", &filterShowAnticheat);
        ImGui::Checkbox("Show Full Servers", &filterShowFullServers);
        ImGui::Checkbox("Show Empty Servers", &filterShowEmptyServers);
        ImGui::SetNextItemWidth(200);
        ImGui::PopItemWidth();

        ImGui::Dummy(ImVec2(0, 10));
        ImGui::Separator();
        ImGui::Dummy(ImVec2(0, 10));

        ImGui::Text("Min and Max Ping");
        ImGui::RangeSliderInt("PING SLIDER", 0, 999, filterMinPing, filterMaxPing);

        ImGui::Dummy(ImVec2(0, 10));
        ImGui::Separator();
        ImGui::Dummy(ImVec2(0, 10));

        ImGui::Text("Filter by Map:");
        if (ImGui::BeginCombo("##MapFilter", getSelectedMapsPreview().c_str()))
        {
            ImGui::InputText("##MapFilterInput", mapFilterInput, IM_ARRAYSIZE(mapFilterInput));
            std::set<std::string> mapsSet;
            for (const auto& server : serverList) {
                mapsSet.insert(server.map);
            }
            for (const auto& map : mapsSet)
            {
                if (strlen(mapFilterInput) == 0 || std::string(map).find(mapFilterInput) != std::string::npos) {
                    bool selected = std::find(filterSelectedMaps.begin(), filterSelectedMaps.end(), map) != filterSelectedMaps.end();
                    if (ImGui::Selectable(map.c_str(), selected)) {
                        if (selected)
                            filterSelectedMaps.erase(std::remove(filterSelectedMaps.begin(), filterSelectedMaps.end(), map), filterSelectedMaps.end());
                        else
                            filterSelectedMaps.push_back(map);
                    }
                }
            }
            ImGui::EndCombo();
        }

        ImGui::Text("Filter by Game Mode:");
        if (ImGui::BeginCombo("##GameModeFilter", getSelectedGameModesPreview().c_str()))
        {
            ImGui::InputText("##GameModeFilterInput", gameModeFilterInput, IM_ARRAYSIZE(gameModeFilterInput));
            std::set<std::string> gameModesSet;
            for (const auto& server : serverList) {
                gameModesSet.insert(server.gameMode);
            }
            for (const auto& mode : gameModesSet)
            {
                if (strlen(gameModeFilterInput) == 0 || std::string(mode).find(gameModeFilterInput) != std::string::npos) {
                    bool selected = std::find(filterSelectedGameModes.begin(), filterSelectedGameModes.end(), mode) != filterSelectedGameModes.end();
                    if (ImGui::Selectable(mode.c_str(), selected)) {
                        if (selected)
                            filterSelectedGameModes.erase(std::remove(filterSelectedGameModes.begin(), filterSelectedGameModes.end(), mode), filterSelectedGameModes.end());
                        else
                            filterSelectedGameModes.push_back(mode);
                    }
                }
            }
            ImGui::EndCombo();
        }

        ImGui::Dummy(ImVec2(0, 10));
        if (ImGui::Button("Reset", ImVec2(255, 25)))
        {
            filterShowAnticheat = false;
            filterShowFullServers = true;
            filterShowEmptyServers = true;
            filterMinPing = 0;
            filterMaxPing = 999;
            filterSelectedMaps.clear();
            filterSelectedGameModes.clear();
            memset(mapFilterInput, 0, sizeof(mapFilterInput));
            memset(gameModeFilterInput, 0, sizeof(gameModeFilterInput));
        }

        ImGui::PopStyleVar(2);
        ImGui::EndPopup();
    }
    custom_style::PopBrowserInfoTheme();
    custom_style::PushBrowserTheme();

    ImGui::PopStyleVar();
    ImGui::PopStyleColor();
    ImGui::SameLine();

    int totalServers = static_cast<int>(serverList.size());
    int totalPlayers = 0;
    for (const auto& server : serverList) {
        totalPlayers += server.playercount;
    }

    filteredServerList.clear();
    if (strlen(searchBuffer) > 0) {
        std::string searchQuery = toLower(searchBuffer);
        for (const auto& server : serverList) {
            if (toLower(server.serverName).find(searchQuery) != std::string::npos) {
                filteredServerList.push_back(server);
            }
        }
    }
    else {
        filteredServerList = serverList;
    }

    {
        std::vector<ServerData> tempList;
        for (const auto& server : filteredServerList) {
            bool show = true;
            if (filterShowAnticheat && server.anticheat.empty()) {
                show = false;
            }
            if (!filterShowFullServers && (server.playercount >= server.maxclients)) {
                show = false;
            }
            if (!filterShowEmptyServers && (server.playercount == 0)) {
                show = false;
            }
            if (server.ping < filterMinPing || server.ping > filterMaxPing) {
                show = false;
            }
            if (!filterSelectedMaps.empty()) {
                if (std::find(filterSelectedMaps.begin(), filterSelectedMaps.end(), server.map) == filterSelectedMaps.end())
                    show = false;
            }
            if (!filterSelectedGameModes.empty()) {
                if (std::find(filterSelectedGameModes.begin(), filterSelectedGameModes.end(), server.gameMode) == filterSelectedGameModes.end())
                    show = false;
            }
            if (show) {
                tempList.push_back(server);
            }
        }
        filteredServerList = tempList;
    }

    float paddingRight = 20.0f;
    std::string statusText = "Servers: " + std::to_string(totalServers) + " | Players: " + std::to_string(totalPlayers);
    float textWidth = ImGui::CalcTextSize(statusText.c_str()).x;
    float availableWidth = ImGui::GetWindowContentRegionMax().x;
    float xPos = availableWidth - textWidth - paddingRight;
    ImGui::SetCursorPosX(xPos);
    ImGui::SetCursorPosY(4);
    ImGui::Text("%s", statusText.c_str());

    custom_style::PushBrowserTheme();

    ImVec2 availSize = ImGui::GetContentRegionAvail();
    availSize.y -= 50;
    if (ImGui::BeginChild("ServerListChild", availSize, true, ImGuiWindowFlags_AlwaysUseWindowPadding))
    {
        ImGuiTableFlags flags = ImGuiTableFlags_BordersH
            | ImGuiTableFlags_RowBg
            | ImGuiTableFlags_Sortable
            | ImGuiTableColumnFlags_NoResize
            | ImGuiTableFlags_SizingStretchProp
            | ImGuiTableFlags_BordersH;

        if (ImGui::BeginTable("ServerList", 8, flags))
        {
            ImVec4 headerColor = ImVec4(0.15f, 0.15f, 0.15f, 1.0f);
            ImGui::TableSetupColumn("AC", ImGuiTableColumnFlags_None, 0.8f);
            ImGui::TableSetupColumn("Server Name", ImGuiTableColumnFlags_DefaultSort, 15.0f);
            ImGui::TableSetupColumn("INFO", ImGuiTableColumnFlags_DefaultSort, 1.0f);
            ImGui::TableSetupColumn("Map", ImGuiTableColumnFlags_None, 3.0f);
            ImGui::TableSetupColumn("Game Mode", ImGuiTableColumnFlags_None, 3.0f);
            ImGui::TableSetupColumn("Players", ImGuiTableColumnFlags_None, 2.0f);
            ImGui::TableSetupColumn("Version", ImGuiTableColumnFlags_None, 2.0f);
            ImGui::TableSetupColumn("Ping", ImGuiTableColumnFlags_None, 1.0f);

            float headerHeight = 25.0f;
            ImGui::TableNextRow(ImGuiTableRowFlags_Headers, headerHeight);
            for (int col = 0; col < 8; col++) {
                ImGui::TableSetColumnIndex(col);
                ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, ImGui::ColorConvertFloat4ToU32(headerColor));

                float textHeight = ImGui::GetTextLineHeight();
                float yOffset = (headerHeight - textHeight) * 0.5f;
                ImVec2 cursorPos = ImGui::GetCursorPos();
                ImGui::SetCursorPosY(cursorPos.y + yOffset);
                switch (col) {
                case 0:
                    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 10);
                    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 6);
                    ImGui::TableHeader("AC");
                    break;
                case 1:
                    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 6);
                    ImGui::TableHeader("Server Name");
                    break;
                case 2:
                    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 6);
                    ImGui::TableHeader("INFO");
                    break;
                case 3:
                    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 6);
                    ImGui::TableHeader("Map");
                    break;
                case 4:
                    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 6);
                    ImGui::TableHeader("Game Mode");
                    break;
                case 5:
                    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 6);
                    ImGui::TableHeader("Players");
                    break;
                case 6:
                    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 6);
                    ImGui::TableHeader("Version");
                    break;
                case 7:
                    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 6);
                    ImGui::TableHeader("Ping");
                    break;
                }
            }

            if (ImGuiTableSortSpecs* sortSpecs = ImGui::TableGetSortSpecs()) {
                if (sortSpecs->SpecsDirty && sortSpecs->SpecsCount > 0) {
                    const ImGuiTableColumnSortSpecs* sortSpec = &sortSpecs->Specs[0];
                    switch (sortSpec->ColumnIndex) {
                    case 0: currentSortType = SortType::ANTICHEAT; break;
                    case 1: currentSortType = SortType::SERVER_NAME; break;
                    case 2: currentSortType = SortType::INFO; break;
                    case 3: currentSortType = SortType::MAP;         break;
                    case 4: currentSortType = SortType::GAME_MODE;     break;
                    case 5: currentSortType = SortType::PLAYERS;       break;
                    case 6: currentSortType = SortType::VERSION;       break;
                    case 7: currentSortType = SortType::PING;          break;
                    default: break;
                    }
                    sortAscending = (sortSpec->SortDirection == ImGuiSortDirection_Ascending);
                    sortSpecs->SpecsDirty = false;
                }
            }

            sortServerList(currentSortType, sortAscending);

            float rowHeight = 50.0f;
            if (filteredServerList.empty()) {
                ImGui::TableNextRow(ImGuiTableRowFlags_None, rowHeight);
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("^1NO SERVERS FOUND!");
            }
            else {
                for (int rowIndex = 0; rowIndex < static_cast<int>(filteredServerList.size()); rowIndex++) {
                    const auto& server = filteredServerList[rowIndex];

                    ImGui::TableNextRow(ImGuiTableRowFlags_None, rowHeight);
                    ImGui::PushID(rowIndex);

                    ImGui::TableSetColumnIndex(0);
                    ImGui::PushStyleVar(ImGuiStyleVar_SelectableTextAlign, ImVec2(0, 0.5f));
                    ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.2f, 0.2f, 0.2f, 0.5f));
                    bool selected = ImGui::Selectable("##RowClickable", false,
                        ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowDoubleClick,
                        ImVec2(0, rowHeight));

                    if (ImGui::IsItemHovered()) {
                        ImGui::SetItemAllowOverlap();
                    }
                    ImGui::PopStyleColor();
                    ImGui::PopStyleVar();

                    if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0)) {
                        std::cout << "Server " << server.serverName << " joined! \n" << std::endl;
                    }

                    {
                        float textHeight = ImGui::GetTextLineHeight();
                        float offset = (rowHeight - textHeight) * 0.5f;
                        ImVec2 pos = ImGui::GetCursorScreenPos();
                        ImGui::SetCursorScreenPos(ImVec2(pos.x + 8, pos.y - rowHeight));
                        ImGui::TableSetColumnIndex(0);
                        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + offset);
                        ImGui::TextUnformatted(server.anticheat.c_str());
                    }

                    ImGui::TableSetColumnIndex(1);
                    {
                        float textHeight = ImGui::GetTextLineHeight();
                        float offset = (rowHeight - textHeight) * 0.5f;
                        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + offset);
                        ImGui::TextUnformatted(server.serverName.c_str());
                    }

                    {
                        // Info-Button: Beim Klick wird der modale Popup "InfoModal" geöffnet.
                        ImGui::TableSetColumnIndex(2);
                        if (!server.info.empty()) {
                            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
                            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.3f, 0.3f, 0.5f));
                            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.5f, 0.5f, 0.5f, 0.5f));

                            float cellWidth = ImGui::GetContentRegionAvail().x;
                            ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX() + (cellWidth - rowHeight) * 0.5f,
                                ImGui::GetCursorPosY()));
                            ImGui::SetItemAllowOverlap();
                            bool info_button = ImGui::Button(ICON_FA_INFO, ImVec2(rowHeight, rowHeight));
                            if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(0)) {
                                std::cout << "Click info button\n";
                                if (!showInfoWindow) {
                                    showInfoWindow = true;
                                    currentInfoServer = server;
                                    infoWindowOpenAnimationStartTime = -1.0f;
                                    infoWindowCloseAnimationStartTime = -1.0f;
                                    startClosing = false;
                                    isClosing = false;
                                    ImGui::OpenPopup("InfoModal");
                                }
                            }
                            ImGui::PopStyleColor(3);
                        }
                    }

                    ImGui::TableSetColumnIndex(3);
                    {
                        float textHeight = ImGui::GetTextLineHeight();
                        float offset = (rowHeight - textHeight) * 0.5f;
                        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + offset);
                        ImGui::TextUnformatted(server.map.c_str());
                    }

                    ImGui::TableSetColumnIndex(4);
                    {
                        float textHeight = ImGui::GetTextLineHeight();
                        float offset = (rowHeight - textHeight) * 0.5f;
                        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + offset);
                        ImGui::TextUnformatted(server.gameMode.c_str());
                    }

                    ImGui::TableSetColumnIndex(5);
                    {
                        float textHeight = ImGui::GetTextLineHeight();
                        float offset = (rowHeight - textHeight) * 0.5f;
                        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + offset);
                        ImGui::Text("%d/%d [%d]", server.playercount, server.maxclients, server.bots);
                    }

                    ImGui::TableSetColumnIndex(6);
                    {
                        float textHeight = ImGui::GetTextLineHeight();
                        float offset = (rowHeight - textHeight) * 0.5f;
                        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + offset);
                        ImGui::TextUnformatted(server.gameVersion.c_str());
                    }

                    ImGui::TableSetColumnIndex(7);
                    {
                        float textHeight = ImGui::GetTextLineHeight();
                        float offset = (rowHeight - textHeight) * 0.5f;
                        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + offset);
                        ImVec4 pingColor;
                        if (server.ping < 70)
                            pingColor = ImVec4(0.5f, 1.0f, 0.5f, 1.0f);
                        else if (server.ping < 110)
                            pingColor = ImVec4(0.0f, 0.8f, 0.0f, 1.0f);
                        else if (server.ping < 150)
                            pingColor = ImVec4(1.0f, 0.65f, 0.0f, 1.0f);
                        else
                            pingColor = ImVec4(1.0f, 0.0f, 0.0f, 1.0f);
                        ImGui::PushStyleColor(ImGuiCol_Text, pingColor);
                        ImGui::Text("%d", server.ping);
                        ImGui::PopStyleColor();
                    }
                    ImGui::PopID();
                }
            }
            ImGui::EndTable();
        }
        ImGui::EndChild();
    }
    custom_style::PopBrowserTheme();
}

std::string serverBrowser::toLower(const std::string& str) {
    std::string lowerStr = str;
    std::transform(lowerStr.begin(), lowerStr.end(), lowerStr.begin(), [](unsigned char c) {
        return std::tolower(c);
        });
    return lowerStr;
}

void serverBrowser::sortServerList(SortType sortType, bool ascending) {
    auto comparator = [sortType, ascending](const ServerData& a, const ServerData& b) {
        switch (sortType) {
        case SortType::SERVER_NAME:
            return ascending ? naturalCompare(a.serverName, b.serverName) < 0
                : naturalCompare(a.serverName, b.serverName) > 0;
        case SortType::MAP:
            return ascending ? a.map < b.map : a.map > b.map;
        case SortType::GAME_MODE:
            return ascending ? a.gameMode < b.gameMode : a.gameMode > b.gameMode;
        case SortType::PLAYERS:
            return ascending ? a.playercount < b.playercount : a.playercount > b.playercount;
        case SortType::VERSION:
            return ascending ? a.gameVersion < b.gameVersion : a.gameVersion > b.gameVersion;
        case SortType::PING:
            return ascending ? a.ping < b.ping : a.ping > b.ping;
        case SortType::ANTICHEAT:
            return ascending ? a.anticheat < b.anticheat : a.anticheat > b.anticheat;
        case SortType::INFO: {
            bool aHas = !a.info.empty();
            bool bHas = !b.info.empty();
            return ascending ? ((int)aHas < (int)bHas) : ((int)aHas > (int)bHas);
        }
        default:
            return true;
        }
        };
    std::sort(filteredServerList.begin(), filteredServerList.end(), comparator);
}

std::string serverBrowser::getSelectedMapsPreview() const {
    if (filterSelectedMaps.empty())
        return "All Maps";
    std::string preview;
    for (const auto& map : filterSelectedMaps) {
        if (!preview.empty())
            preview += ", ";
        preview += map;
    }
    return preview;
}

std::string serverBrowser::getSelectedGameModesPreview() const {
    if (filterSelectedGameModes.empty())
        return "All Modes";
    std::string preview;
    for (const auto& mode : filterSelectedGameModes) {
        if (!preview.empty())
            preview += ", ";
        preview += mode;
    }
    return preview;
}
