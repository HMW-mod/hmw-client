#include "std_include.hpp"
#include "server_info_panel.hpp"
#include "imgui_internal.h"
#include "component/fastfiles.hpp"
#include "component/console.hpp"
#include "component/scheduler.hpp"
#include <tcp/hmw_tcp_utils.hpp>
#include <component/gui/utils/ImGuiNotify.hpp>

namespace {
    bool opened = false;
    std::vector<server_browser::ServerData> list;
    int server_index;
    game::Material* map_image;
    int refresh_counter = 5;
    const int REFRESH_MAX = 5;
    uint64_t refresh_task = -1;
    bool is_favourited = false;
}

void server_info_panel::show_info_panel()
{
    if (!opened) {
        return;
    }

    if (server_index < 0 || server_index >= list.size())
    {
        return;
    }

    ImGui::SetNextWindowSize(ImVec2(600, 340), ImGuiCond_Always);

    // Begin window with non-resizable flag
    ImGui::SetNextWindowFocus();
    ImGui::Begin("Server Info Panel", NULL, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoNavFocus);
    server_browser::ServerData& server_data = list[server_index];

    // Begin child region for the player list with a fixed width (e.g., 250 pixels) and full height (0 means use remaining height)
    ImGui::BeginChild("PlayerListChild", ImVec2(250, 300), true, ImGuiWindowFlags_NoScrollbar);
    if (ImGui::BeginTable("PlayerList", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg))
    {
        ImGui::TableSetupColumn("Player name");
        ImGui::TableSetupColumn("Ping");
        ImGui::TableHeadersRow();

        for (const auto& player : server_data.players) {
            ImGui::TableNextRow();

            ImGui::TableSetColumnIndex(0);
            ImGui::Text("%s", player.name.c_str());

            ImGui::TableSetColumnIndex(1);
            ImGui::Text("%d", player.ping);
        }

        ImGui::EndTable();
    }

    if (ImGui::Button("Close"))
    {
        close();
    }
    ImGui::EndChild();

    // Place the next child on the same line to render it to the right of the player list
    ImGui::SameLine();

    // Begin child region for the map and server details which will take up the remaining space
    ImGui::BeginChild("ServerDetailsChild", ImVec2(0, 300), true, ImGuiWindowFlags_NoScrollbar);
    ImGui::Text("%s", server_data.serverName.c_str());
    draw_map_image(map_image);

    ImGui::Text("Playing: %s on %s", server_data.gameMode.c_str(), server_data.map.c_str());
    ImGui::Text("\"%s\"", server_data.motd.c_str());

    // Favourite buttons
    if (!is_favourited) {
        if (ImGui::Button("Add to favourites"))
        {
            server_browser::add_favourite(list, server_index);
        }
    }
    else {
        if (ImGui::Button("Remove from favourites"))
        {
            server_browser::delete_favourite(list, server_index);
        }
    }

    // Other buttons on the same line
    ImGui::SameLine();
    if (ImGui::Button("Join"))
    {
        server_browser::join_server(list, server_index);
    }
    ImGui::EndChild();

    ImGui::End();
}


void server_info_panel::refresh(std::string address)
{
    return; // Disabled for now as it causes crashes

    ImGui::InsertNotification({ ImGuiToastType::Info, 2, "Refreshing" });
    server_browser::ServerData& server_data = list[server_index];

    // Refresh the server info with new data (update the server listing too)
    scheduler::once([&address, &server_data] {
        std::string game_server_info = address + "/getInfo";
        // Request the data. Time out getting the refresh data just before requesting a new refresh 4.5 seconds. Refresh every 5
        std::string game_server_response = hmw_tcp_utils::GET_url(game_server_info.c_str(), {}, true, 4500L, false, 1);
        if (!game_server_response.empty()) {
            update_server(game_server_response, address, server_data);
        }
        else {
            ImGui::InsertNotification({ ImGuiToastType::Error, 2, "Failed to refresh server info" });
        }
    }, scheduler::pipeline::network);
}

void server_info_panel::open(std::vector<server_browser::ServerData> _list, int _server_index)
{
    list = _list;
    server_index = _server_index;

    server_browser::ServerData& server_data = list[server_index];
    map_image = get_map_material();
    is_favourited = server_browser::has_favourite(server_data.address);
    opened = true;
    // Start refresh sequence
    refresh_task = scheduler::loop([server_data]()
        {
            refresh(server_data.address);
            refresh_counter--;
            if (refresh_counter <= 0) {
                refresh_counter = REFRESH_MAX;
            }
        }, scheduler::pipeline::async, 1s);
}

void server_info_panel::close()
{
    opened = false;
    // It is assumed this is running
    if (refresh_task != -1) {
        scheduler::stop(refresh_task, scheduler::pipeline::async);
        refresh_task = -1;
    }
}

game::Material* server_info_panel::get_map_material()
{
    server_browser::ServerData& server_data = list[server_index];
    std::string loadscreen_name = "loadscreen_" + server_data.map_id;
    game::Material* material = get_material(loadscreen_name);
    if (material == nullptr) {
        material = get_material("$default2d");
    }
    return material;
}

game::Material* server_info_panel::get_material(std::string _asset_name)
{
    game::Material* material = nullptr;

    database::XAssetType assetType = game::ASSET_TYPE_MATERIAL;
    fastfiles::enum_asset_entries(assetType, [&](const game::XAssetEntry* entry)
    {
            const auto asset = entry->asset;
            auto asset_name = std::string(game::DB_GetXAssetName(&asset));
            bool matches = asset_name == _asset_name;
            if (matches) {
                const auto header = reinterpret_cast<game::Material*>(game::DB_FindXAssetHeader(assetType, _asset_name.data(), 0).data);
                material = header;
            }
    }, true);
    return material;
}

void server_info_panel::draw_map_image(game::Material* asset)
{
    // If its null do nothing
    if (asset == nullptr) {
        ImGui::Text("Failed to load map image.");
        return;
    }

    // This will render every texture if there is more then 1, if there is 1, i == 0
    for (auto i = 0; i < asset->textureCount; i++)
    {
        if (asset->textureTable && asset->textureTable->u.image && asset->textureTable->u.image->texture.shaderView)
        {
            const auto width = asset->textureTable[i].u.image->width;
            const auto height = asset->textureTable[i].u.image->height;
            const auto ratio = static_cast<float>(width) / static_cast<float>(height);
            constexpr auto size = 200.f;

            ImGui::Image(reinterpret_cast<ImTextureID>(asset->textureTable[i].u.image->texture.shaderView), ImVec2(size * ratio, size));
        }
    }
}

void server_info_panel::update_server(const std::string& infoJson, const std::string& connect_address, server_browser::ServerData& server)
{
    nlohmann::json game_server_response_json = nlohmann::json::parse(infoJson);
    std::string ping = game_server_response_json["ping"];

    // Don't show servers that aren't using the same protocol!
    std::string server_protocol = game_server_response_json["protocol"];
    const auto protocol = std::atoi(server_protocol.c_str());
    if (protocol != PROTOCOL)
    {
        return;
    }

    // Don't show servers that aren't dedicated!
    std::string server_dedicated = game_server_response_json["dedicated"];
    const auto dedicated = std::atoi(server_dedicated.c_str());
    if (!dedicated)
    {
        return;
    }

    // Don't show servers that aren't running!
    std::string server_running = game_server_response_json["sv_running"];
    const auto sv_running = std::atoi(server_running.c_str());
    if (!sv_running)
    {
        return;
    }

    // Only handle servers of the same playmode!
    std::string response_playmode = game_server_response_json["playmode"];
    const auto playmode = game::CodPlayMode(std::atoi(response_playmode.c_str()));
    if (game::Com_GetCurrentCoDPlayMode() != playmode)
    {
        return;
    }

    // Only show HMW games
    std::string server_gamename = game_server_response_json["gamename"];
    if (server_gamename != "HMW")
    {
        return;
    }

    std::string gameversion = game_server_response_json.value("gameversion", "Unknown");
    bool outdated = gameversion != hmw_tcp_utils::get_version();

    server.serverName = game_server_response_json["hostname"];
    std::string map_name = game_server_response_json["mapname"];
    server.map_id = map_name;
    server.map = server_browser::get_map_name(map_name);
    server.gameVersion = gameversion;
    server.outdated = outdated;

    std::string gametype = game_server_response_json["gametype"];
    std::string gameMode = game::UI_GetGameTypeDisplayName(gametype.c_str());
    server.gameType = gametype;
    server.gameMode = gameMode;
    server.mod_name = game_server_response_json["fs_game"];

    server.play_mode = playmode;
    server.verified = false;

    std::string clients = game_server_response_json["clients"];
    server.clients = atoi(clients.c_str());

    std::string max_clients = game_server_response_json["sv_maxclients"];
    server.maxclients = atoi(max_clients.c_str());

    std::string bots = game_server_response_json["bots"];
    server.bots = atoi(bots.c_str());

    int player_count = server.clients - server.bots;
    server.playercount = player_count;

    int latency = std::atoi(ping.c_str());
    server.ping = latency >= 999 ? 999 : latency; // Cap latency display to 999

    std::string isPrivate = game_server_response_json["isPrivate"];
    server.is_private = atoi(isPrivate.c_str()) == 1;

    server.motd = game_server_response_json["sv_motd"];

    server.address = connect_address;

    // Process our new players array here
    if (game_server_response_json.contains("players") && game_server_response_json["players"].is_array())
    {
        server.players.clear(); // Clear old list
        for (const auto& player : game_server_response_json["players"])
        {
            server_browser::PlayerData pData;
            pData.name = player.value("name", "Unknown");
            pData.level = player.value("level", 0);
            pData.ping = player.value("ping", 999); // Default to high ping if missing

            server.players.push_back(pData);
        }
    }
}
