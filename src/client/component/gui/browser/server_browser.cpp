#include "std_include.hpp"
#include "server_browser.hpp"
#include "imgui_internal.h"

#include "tcp/hmw_tcp_utils.hpp"
#include "component/console.hpp"
#include <component/debug_gui/gui.hpp>
#include "component/discord.hpp"
#include "component/clantags.hpp"
#include "component/clantag_utils.hpp"
#include "component/command.hpp"
#include "component/party.hpp"
#include "component/scheduler.hpp"
#include <component/fastfiles.hpp>
#include <utils/toast.hpp>
#include <utils/io.hpp>
#include <component/gui/utils/ImGuiNotify.hpp>
#include "server_info_panel.hpp"
#include <game/ui_scripting/execution.hpp>

#include "component/gui/themes/serverBrowserTheme.hpp"

// TODO Password entry and password protected server support

namespace {
    std::vector<server_browser::ServerData> serverList; // List of servers
    bool opened = false; // State of the window
	server_browser::Page currentPage = server_browser::Page::INTERNET;
	bool sort_ascending = false;
	server_browser::SortType currentSortType = server_browser::SortType::PLAYERS;

	// Threading
	std::mutex mutex;
	std::mutex server_list_mutex;
	std::mutex interrupt_mutex;

	// These should be refactored
	// Used for when we're refreshing the server / favourites list
	bool getting_server_list = false;
	bool getting_favourites = false;
	bool getting_history = false;

	// Used for stopping the threads abruptly
	bool interrupt_server_list = false;
	bool interrupt_favourites = false;
	bool interrupt_history = false;

	std::string failed_to_join_header = "Failed to join server!";
	std::string failed_to_join_reason = "";

	// Stupid fucking c++ circular reference bullshit demands this to be here
	server_browser::ServerData matchmaking_chosen_server;
}

// Render the server browser window
void server_browser::show_window()
{
	if (!is_open()) {
		return; // If the window is not open, do nothing
	}

	static char searchBuffer[256] = ""; // Buffer to hold search input
	std::vector<ServerData> filteredServerList; // Filtered server list based on search

	static const auto window_flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove; // These are specific window flags we can/wanna use

	// Begin the server browser window
	custom_style::CenterWindowAndScale("Server Browser", &opened, 0.9f, 0.9f, window_flags);

	ImGuiThemes::PushBrowserTheme(); // This sets our LUI styled theme

	// Tab bar for Internet and Favourites
	if (ImGui::BeginTabBar("ServerTabs"))
	{
		// Internet tab
		if (ImGui::BeginTabItem("Internet"))
		{
			if (currentPage != Page::INTERNET) {
				currentPage = Page::INTERNET;
				refresh(Page::INTERNET);
			}

			// Call function to render Internet servers
			render_server_list(searchBuffer, filteredServerList);

			ImGui::EndTabItem();
		}

		// Favourites tab
		if (ImGui::BeginTabItem("Favourites"))
		{
			if (currentPage != Page::FAVORITES) {
				currentPage = Page::FAVORITES;
				refresh(Page::FAVORITES);
			}

			// Call function to render Favourite servers
			render_server_list(searchBuffer, filteredServerList);

			ImGui::EndTabItem();
		}

		if (ImGui::BeginTabItem("History"))
		{
			if (currentPage != Page::HISTORY) {
				currentPage = Page::HISTORY;
				refresh(Page::HISTORY);
			}

			// Call function to render History servers
			render_server_list(searchBuffer, filteredServerList);

			ImGui::EndTabItem();
		}

		ImGui::EndTabBar();
	}
	ImGuiThemes::PopBrowserTheme();
	ImGui::End();
}

// Toggle the visibility of the server browser window
void server_browser::toggle()
{
    opened = !opened;
	// Interrupt favourites / internet if closed
	interrupt_favourites = opened;
	interrupt_server_list = opened;
	interrupt_history = opened;
	if (!opened) {
		server_info_panel::close();
	}
}

// Add a server to the server list
void server_browser::add_server_to_display(const ServerData& data)
{
    serverList.push_back(data);
	//server_browser::sort(serverList, currentSortType, sort_ascending); // putting this here does not see to do anything...
}

// Check if the server browser window is open
bool server_browser::is_open()
{
    return opened;
}

void server_browser::sort(std::vector<ServerData>& list, SortType sortType, bool ascending)
{
	std::lock_guard<std::mutex> lock(server_list_mutex); // Ensure thread-safe sorting
	currentSortType = sortType;

	switch (sortType) {
		case SortType::SERVER_NAME:
			std::sort(list.begin(), list.end(), [ascending](const ServerData& a, const ServerData& b) {
				return ascending ? a.serverName < b.serverName : a.serverName > b.serverName;
				});
			break;

		case SortType::MAP:
			std::sort(list.begin(), list.end(), [ascending](const ServerData& a, const ServerData& b) {
				return ascending ? a.map < b.map : a.map > b.map;
				});
			break;

		case SortType::GAME_MODE:
			std::sort(list.begin(), list.end(), [ascending](const ServerData& a, const ServerData& b) {
				return ascending ? a.gameMode < b.gameMode : a.gameMode > b.gameMode;
				});
			break;

		case SortType::PLAYERS:
			std::sort(list.begin(), list.end(), [ascending](const ServerData& a, const ServerData& b) {
				return ascending ? a.playercount < b.playercount : a.playercount > b.playercount;
				});
			break;

		case SortType::VERIFIED:
			std::sort(list.begin(), list.end(), [ascending](const ServerData& a, const ServerData& b) {
				return ascending ? a.verified < b.verified : a.verified > b.verified;
				});
			break;

		case SortType::PING:
			std::sort(list.begin(), list.end(), [ascending](const ServerData& a, const ServerData& b) {
				return ascending ? a.ping < b.ping : a.ping > b.ping;
				});
			break;

		default:
			console::error("Unknown sort type!");
			break;
	}
}

void server_browser::render_server_list(char* searchBuffer, std::vector<ServerData>& filteredServerList)
{
	// Search field
	ImGui::InputTextWithHint("##Search", "Search by server name...", searchBuffer, sizeof(searchBuffer));

	// Display player count and server count to the right of the search bar
	ImGui::SameLine();
	int totalServers = static_cast<int>(serverList.size());
	int totalPlayers = 0;

	// Calculate total players across all servers
	{
		std::lock_guard<std::mutex> lock(server_list_mutex);
		for (const auto& server : serverList) {
			totalPlayers += server.playercount;
		}
	}

	// Display player and server counts
	ImGui::Text("Servers: %d | Players: %d", totalServers, totalPlayers);

	// Filter the server list based on the search input
	{
		std::lock_guard<std::mutex> lock(server_list_mutex);
		filteredServerList.clear(); // Ensure the filtered list starts empty

		if (strlen(searchBuffer) > 0) {
			std::string searchQueryLower = to_lower(searchBuffer); // Convert the search input to lowercase

			for (const auto& server : serverList) {
				std::string serverNameLower = to_lower(get_clean_server_name(server.serverName)); // Convert server name to lowercase

				if (serverNameLower.find(searchQueryLower) != std::string::npos) {
					filteredServerList.push_back(server); // Add matching servers to the filtered list
				}
			}
		}
		else {
			filteredServerList = serverList; // No search input, show all servers
		}
	}

	ImVec2 childSize = ImVec2(0, 850);
	if (ImGui::BeginChild("ServerListChild", childSize, true, ImGuiWindowFlags_AlwaysUseWindowPadding))
	{
		if (ImGui::BeginTable("ServerList", 6, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Sortable)) {

			float header_height = 40.0f;
			float text_height = ImGui::GetTextLineHeight();
			float header_padding_y = (header_height - text_height) * 0.5f;

			ImGui::TableSetupColumn("Server Name", ImGuiTableColumnFlags_WidthStretch | ImGuiTableColumnFlags_DefaultSort, 5.0f);
			ImGui::TableSetupColumn("Map", ImGuiTableColumnFlags_WidthStretch, 0.8f);
			ImGui::TableSetupColumn("Game Mode", ImGuiTableColumnFlags_WidthStretch, 0.8f);
			ImGui::TableSetupColumn("Players", ImGuiTableColumnFlags_WidthStretch, 0.4f);
			ImGui::TableSetupColumn("Version", ImGuiTableColumnFlags_WidthStretch, 0.5f);
			ImGui::TableSetupColumn("Ping", ImGuiTableColumnFlags_WidthStretch, 0.2f);
			ImGui::TableHeadersRow();

			ImGui::SetCursorPosY(ImGui::GetCursorPosY() + (header_height - ImGui::GetTextLineHeight()));

			if (ImGuiTableSortSpecs* sortSpecs = ImGui::TableGetSortSpecs()) {
				if (sortSpecs->SpecsDirty) {
					for (int n = 0; n < sortSpecs->SpecsCount; n++) {
						const ImGuiTableColumnSortSpecs* sortSpec = &sortSpecs->Specs[n];

						SortType sortType;
						switch (sortSpec->ColumnIndex) {
						case 0: sortType = SortType::SERVER_NAME; break;
						case 1: sortType = SortType::MAP; break;
						case 2: sortType = SortType::GAME_MODE; break;
						case 3: sortType = SortType::PLAYERS; break;
						case 4: sortType = SortType::VERSION; break;
						case 5: sortType = SortType::PING; break;
						default: continue;
						}

						sort_ascending = (sortSpec->SortDirection == ImGuiSortDirection_Ascending);
						sort(serverList, sortType, sort_ascending);
					}
					sortSpecs->SpecsDirty = false;
				}
			}

			if (filteredServerList.empty()) {
				ImGui::TableNextRow();
				ImGui::TableSetColumnIndex(0);
				ImGui::Text("No servers found.");
			}
			else {
				for (int i = 0; i < filteredServerList.size(); ++i) {
					const auto& server = filteredServerList[i];

					ImGui::TableNextRow(0, 40);
					ImGui::TableSetColumnIndex(0);

					float row_height = 40;
					float text_height = ImGui::GetTextLineHeight();
					float vertical_offset = (row_height - text_height) * 0.5f;

					ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(ImGui::GetStyle().FramePadding.x, vertical_offset));
					ImVec2 selectable_size = ImVec2(ImGui::GetColumnWidth(), row_height);
					if (ImGui::Selectable(server.serverName.c_str(), false, ImGuiSelectableFlags_SpanAllColumns, selectable_size)) {
						//
					}
					ImGui::PopStyleVar();

					if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
						add_history(filteredServerList, i);
						join_server(filteredServerList, i);
					}

					if (ImGui::IsItemHovered()) {
						ImGui::SetTooltip("Left Click to join.\nRight Click for more options.");
					}

					if (ImGui::IsItemHovered() && ImGui::IsMouseReleased(ImGuiMouseButton_Right)) {
						ImGui::OpenPopup(("ServerContext" + std::to_string(i)).c_str());
					}

					for (int col = 1; col <= 5; col++) {
						ImGui::TableSetColumnIndex(col);
						ImGui::SetCursorPosY(ImGui::GetCursorPosY() + vertical_offset);
						switch (col) {
						case 1: ImGui::Text("%s", server.map.c_str()); break;
						case 2: ImGui::Text("%s", server.gameMode.c_str()); break;
						case 3: ImGui::Text("%d/%d [%d]", server.playercount, server.maxclients, server.bots); break;
						case 4: ImGui::Text("%s", server.gameVersion.c_str()); break;
						case 5: ImGui::Text("%d", server.ping); break;
						}
					}

					if (ImGui::BeginPopupContextItem(("ServerContext" + std::to_string(i)).c_str())) {
						switch (currentPage) {
						case Page::INTERNET:
							if (ImGui::MenuItem("Show server info")) {
								server_info_panel::open(filteredServerList, i);
							}
							if (ImGui::MenuItem("Add to Favorites")) {
								server_browser::add_favourite(filteredServerList, i);
								console::info("Adding server to favorites: %s", server.serverName.c_str());
							}
							break;
						case Page::FAVORITES:
							if (ImGui::MenuItem("Show server info")) {
								server_info_panel::open(filteredServerList, i);
							}
							if (ImGui::MenuItem("Delete from Favorites")) {
								server_browser::delete_favourite(filteredServerList, i);
								console::info("Deleting server from favorites: %s", server.serverName.c_str());
							}
							break;
						case Page::HISTORY:
							if (ImGui::MenuItem("Show server info")) {
								server_info_panel::open(filteredServerList, i);
							}
							if (ImGui::MenuItem("Add to Favorites")) {
								server_browser::add_favourite(filteredServerList, i);
								console::info("Adding server to favorites: %s", server.serverName.c_str());
							}
							if (ImGui::MenuItem("Delete from History")) {
								server_browser::delete_history(filteredServerList, i);
								console::info("Deleting server from history: %s", server.serverName.c_str());
							}
							break;
						}
						ImGui::EndPopup();
					}

					if (ImGui::IsItemClicked(ImGuiMouseButton_Right)) {
						ImGui::OpenPopup(("ServerContext" + std::to_string(i)).c_str());
					}
				}
			}

			ImGui::EndTable();
		}
	}
	ImGui::EndChild(); // End of constrained child window
	server_info_panel::show_info_panel();
	custom_style::PushCustomButtonStyle();

	if (ImGui::IsKeyPressed(ImGuiKey_F5)) {
		if (is_open()) {
			ImGui::InsertNotification({ ImGuiToastType::Info, 3000, "Refreshing" });
			refresh_current_page();
		}
	}

	if (ImGui::IsKeyPressed(ImGuiKey_Escape)) {
		if (is_open()) {
			toggle();
			ui_scripting::notify("go_back", {});
		}
	}

	// Add buttons for Refresh and Close
	if (ImGui::Button("Refresh", ImVec2(135, 40))) {
		ImGui::InsertNotification({ ImGuiToastType::Info, 3000, "Refreshing" });
		refresh_current_page();
	}
	ImGui::SameLine();
	if (ImGui::Button("Close", ImVec2(135, 40))) {
		toggle();
	}
	custom_style::PopCustomButtonStyle();
}

int server_browser::get_player_count(std::vector<ServerData>& list)
{
	std::lock_guard<std::mutex> _(mutex);
	int count = 0;
	for (ServerData data : list)
	{
		count += data.playercount;
	}
	return count;
}

int server_browser::get_server_count(std::vector<ServerData>& list)
{
	return static_cast<int>(list.size());
}

bool server_browser::get_favourites_file(nlohmann::json& out)
{
	std::string data = utils::io::read_file("players2/favourites.json");

	nlohmann::json obj;
	try
	{
		obj = nlohmann::json::parse(data.data());
	}
	catch (const nlohmann::json::parse_error& ex)
	{
		(void)ex;
		return false;
	}

	if (!obj.is_array())
	{
		console::error("Favourites storage file is invalid!");
		return false;
	}

	out = obj;

	return true;
}

void server_browser::add_favourite(std::vector<ServerData>& list, int server_index)
{
	// Read existing favorites from the file
	nlohmann::json obj;
	std::ifstream favourites_file("players2/favourites.json");

	if (favourites_file.is_open())
	{
		favourites_file >> obj;
		favourites_file.close();
	}

	if (server_index < 0 || server_index >= list.size())
	{
		return;
	}

	ServerData& data = list[server_index];

	// Check if the server is already in favorites
	if (obj.find(data.address) != obj.end())
	{
		utils::toast::show("Error", "Server already marked as favourite.");
		return;
	}

	// Add the new favorite server
	obj.push_back(data.address);

	// Write updated favorites to the file
	utils::io::write_file("players2/favourites.json", obj.dump());
	utils::toast::show("Success", "Server added to favourites.");

	console::debug("added %s to favourites", data.address);
}

void server_browser::delete_favourite(std::vector<ServerData>& list, int server_index)
{
	nlohmann::json obj;
	if (!get_favourites_file(obj))
	{
		return;
	}

	if (server_index < 0 || server_index >= list.size())
	{
		return;
	}

	ServerData& data = list[server_index];

	for (auto it = obj.begin(); it != obj.end(); ++it)
	{
		if (!it->is_string())
		{
			continue;
		}

		if (it->get<std::string>() == data.address) // Check if the element matches the connect address
		{
			console::debug("removed %s from favourites", data.address);
			obj.erase(it);
			break;
		}
	}

	if (!utils::io::write_file("players2/favourites.json", obj.dump()))
	{
		return;
	}

	list.erase(
		std::remove_if(
			list.begin(),
			list.end(),
			[&data](const ServerData& s) {
				return s.address == data.address;
			}
		),
		list.end()
	);

	refresh_current_page();
}

bool server_browser::get_history_file(nlohmann::json& out)
{
	std::string data = utils::io::read_file("players2/history.json");

	nlohmann::json obj;
	try
	{
		obj = nlohmann::json::parse(data.data());
	}
	catch (const nlohmann::json::parse_error& ex)
	{
		(void)ex;
		return false;
	}

	if (!obj.is_array())
	{
		console::error("Favourites storage file is invalid!");
		return false;
	}

	out = obj;

	return true;
}

void server_browser::add_history(std::vector<ServerData>& list, int server_index)
{
	// Read existing favorites from the file
	nlohmann::json obj;
	std::ifstream favourites_file("players2/history.json");

	if (favourites_file.is_open())
	{
		favourites_file >> obj;
		favourites_file.close();
	}

	if (server_index < 0 || server_index >= list.size())
	{
		return;
	}

	ServerData& data = list[server_index];

	// Check if the server is already in favorites
	if (obj.find(data.address) != obj.end())
	{
		//utils::toast::show("Error", "Server already marked as history.");
		return;
	}

	// Add the new favorite server
	obj.push_back(data.address);

	// Write updated favorites to the file
	utils::io::write_file("players2/history.json", obj.dump());
	//utils::toast::show("Success", "Server added to history.");

	console::debug("added %s to history", data.address);
}

void server_browser::delete_history(std::vector<ServerData>& list, int server_index)
{
	nlohmann::json obj;
	if (!get_history_file(obj))
	{
		return;
	}

	if (server_index < 0 || server_index >= list.size())
	{
		return;
	}

	ServerData& data = list[server_index];

	for (auto it = obj.begin(); it != obj.end(); ++it)
	{
		if (!it->is_string())
		{
			continue;
		}

		if (it->get<std::string>() == data.address) // Check if the element matches the connect address
		{
			console::debug("removed %s from history", data.address);
			obj.erase(it);
			break;
		}
	}

	if (!utils::io::write_file("players2/history.json", obj.dump()))
	{
		return;
	}

	list.erase(
		std::remove_if(
			list.begin(),
			list.end(),
			[&data](const ServerData& s) {
				return s.address == data.address;
			}
		),
		list.end()
	);

	refresh_current_page();
}

void server_browser::join_server(std::vector<ServerData>& list, int index)
{
	// Close the server browser if its open
	if (is_open()) {
		toggle();
	}

	interrupt_favourites = true;
	interrupt_server_list = true;

	getting_server_list = false;
	getting_favourites = false;

	scheduler::once([=]()
		{
			console::info("Joining server: %d", index);
			std::lock_guard<std::mutex> _(mutex);

			if (index < list.size())
			{
				console::info("Connecting to server:[%d] {%s} %s\n", index, list[index].address.data(), list[index].serverName.data());
				party::connect(list[index].net_address);

				/* Servers don't respond??
				bool canJoin = check_can_join(list[index].address.data());
				if (canJoin) {
					interrupt_favourites = true;
					interrupt_server_list = true;
					command::execute("connect " + list[index].address);
				}
				else {
					gui::GetNotificationManager().addNotification(failed_to_join_header + "\n" + failed_to_join_reason, NotificationType::Error, 7.0f);
				}*/
			}
		}, scheduler::pipeline::main);
}

bool server_browser::check_can_join(const char* connect_address)
{
	// Ensure connect_address is valid before using it
	if (connect_address == nullptr) {
		console::info("Failed to join server. Invalid connect address.");
		ImGui::InsertNotification({ ImGuiToastType::Error, 3000, "Failed to join server. Invalid connect address." });
		return false;
	}

	std::string game_server_info = std::string(connect_address) + "/join";

	const char* server_address = game_server_info.data();

	std::string clanTag = clantags::get_current_clantag();
	if (clanTag.empty()) {
		clanTag = "\"\""; // empty value http headers get pruned, so we use a pseudo-empty value
	}
	for (auto eliteTag : clantags::tags) {
		auto eliteTagVA = utils::string::va("^%c%c%c%c%s", 1, eliteTag.second.width, eliteTag.second.height, 2, eliteTag.second.short_name.data());
		if (strcmp(clanTag.c_str(), eliteTagVA) == 0) {
			clanTag = eliteTag.first;
			break;
		}
	}
	std::string discordId = discord::get_discord_id();

	std::string game_server_response = hmw_tcp_utils::GET_url(server_address, { { "ClanTag", clanTag },
																				{ "DiscordID", discordId },
																				{ "ClientSecret", "3ad3ae5755ee55b5759dbe16379746dfe33bc3af0a0a5166182c7587b522b429" }
		}, true, 1500L, true, 3);

	if (game_server_response.empty())
	{
		failed_to_join_reason = "Server did not respond.";
		return false;
	}

	nlohmann::json game_server_response_json = nlohmann::json::parse(game_server_response);

	if (game_server_response_json.contains("error_code")) {
		failed_to_join_reason = "Server rejected join request.";
		return false;
	}

	std::string ping = game_server_response_json["ping"];

	// Don't show servers that aren't using the same protocol!
	std::string server_protocol = game_server_response_json["protocol"];
	const auto protocol = std::atoi(server_protocol.c_str());
	if (protocol != PROTOCOL)
	{
		failed_to_join_reason = "Invalid protocol";
		return false;
	}

	// Don't show servers that aren't running!
	std::string server_running = game_server_response_json["sv_running"];
	const auto sv_running = std::atoi(server_running.c_str());
	if (!sv_running)
	{
		failed_to_join_reason = "Server not running.";
		return false;
	}

	// Only handle servers of the same playmode!
	std::string response_playmode = game_server_response_json["playmode"];
	const auto playmode = game::CodPlayMode(std::atoi(response_playmode.c_str()));
	if (game::Com_GetCurrentCoDPlayMode() != playmode)
	{
		failed_to_join_reason = "Invalid playmode.";
		return false;
	}

	// Only show HMW games
	std::string server_gamename = game_server_response_json["gamename"];
	if (server_gamename != "HMW")
	{
		failed_to_join_reason = "Invalid gamename.";
		return false;
	}

	std::string mapname = game_server_response_json["mapname"];
	if (mapname.empty()) {
		failed_to_join_reason = "Invalid map.";
		return false;
	}

	std::string gametype = game_server_response_json["gametype"];
	if (gametype.empty()) {
		failed_to_join_reason = "Invalid gametype.";
		return false;
	}

	std::string maxclients = game_server_response_json["sv_maxclients"];
	std::string clients = game_server_response_json["clients"];
	std::string bots = game_server_response_json["bots"];
	std::string privateclients = game_server_response_json.value("sv_privateClients", "0");

	int max_clients = std::stoi(maxclients);
	int current_clients = std::stoi(clients);
	int private_clients = std::stoi(privateclients);
	int current_bots = std::stoi(bots);
	int player_count = current_clients - current_bots;

	int actual_max_clients = max_clients - private_clients;

	if (player_count == max_clients) {
		failed_to_join_header = "Failed to join server!";
		failed_to_join_reason = "Game is full.";
		return false;
	}

	if (player_count == actual_max_clients) {
		failed_to_join_header = "Reserved Game Full!";
		failed_to_join_reason = "Reserved? Use commandline\n/connect " + std::string(connect_address);
		return false;
	}

	failed_to_join_reason = "";
	return true;
}

std::string server_browser::get_map_name(std::string _map_name)
{
	const auto& map_name = _map_name;
	if (map_name.empty())
	{
		return "Unknown";
	}

	auto map_display_name = game::UI_GetMapDisplayName(map_name.data());
	if (!fastfiles::usermap_exists(map_name) && !fastfiles::exists(map_name, false))
	{
		map_display_name = utils::string::va("^1%s", map_display_name);
	}
	return map_display_name;
}

// Helper function to clean up server names by removing ^[0-9] color codes
std::string server_browser::get_clean_server_name(const std::string& serverName)
{
    // Use regex to find and remove color codes like ^0-9
    static const std::regex colorCodePattern("\\^\\d");
    return std::regex_replace(serverName, colorCodePattern, "");
}

void server_browser::refresh(Page page)
{	
	std::lock_guard<std::mutex> _(mutex);

	switch (page) {
		case Page::INTERNET: 
			{
				interrupt_favourites = getting_favourites;
				interrupt_history = getting_history;

				if (getting_server_list)
				{
					ImGui::InsertNotification({ ImGuiToastType::Info, 3000, "Already getting server list." });
					return;
				}

				serverList.clear();
				interrupt_server_list = false;
				getting_server_list = true;
				scheduler::once([]() {
					populate_server_list();
				}, scheduler::pipeline::network);
				break;
			}
		case Page::FAVORITES:
			{
				interrupt_server_list = getting_server_list;
				interrupt_history = getting_history;

				if (getting_favourites) 
				{
					ImGui::InsertNotification({ ImGuiToastType::Info, 3000, "Already getting favourites." });
					return;
				}

				serverList.clear();
				interrupt_favourites = false;
				getting_favourites = true;
				scheduler::once([]() {
					populate_favorites();
				}, scheduler::pipeline::network);
				break;
			}
		case Page::HISTORY:
			{
				interrupt_server_list = getting_server_list;
				interrupt_favourites = getting_favourites;

				if (getting_history) 
				{
					ImGui::InsertNotification({ ImGuiToastType::Info, 3000, "Already getting history." });
					return;
				}

				serverList.clear();
				interrupt_history = false;
				getting_history = true;
				scheduler::once([]() {
					populate_history();
				}, scheduler::pipeline::network);
			}
	}
}

void server_browser::refresh_current_page()
{
	refresh(currentPage);
}

void server_browser::add_server(const std::string& infoJson, const std::string& connect_address, int server_index)
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

	game::netadr_s address{};
	if (game::NET_StringToAdr(connect_address.c_str(), &address))
	{
		server_browser::ServerData server;
		server.net_address = address;

		server.serverName = game_server_response_json["hostname"];
		std::string map_name = game_server_response_json["mapname"];
		server.map_id = map_name;
		server.map = get_map_name(map_name);
		server.gameVersion = gameversion;
		server.outdated = outdated;

		std::string gametype = game_server_response_json["gametype"];
		std::string gameMode = game::UI_GetGameTypeDisplayName(gametype.c_str());
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

		std::vector<PlayerData> players;

		// Process our new players array here
		if (game_server_response_json.contains("players") && game_server_response_json["players"].is_array())
		{
			for (const auto& player : game_server_response_json["players"])
			{
				PlayerData pData;
				pData.name = player.value("name", "Unknown");
				pData.level = player.value("level", 0);
				pData.ping = player.value("ping", 999); // Default to high ping if missing

				players.push_back(pData);
			}
		}

		server.players = players;

		add_server_to_display(server); // Add to imgui display
	}
}

void server_browser::populate_server_list()
{
	ImGui::InsertNotification({ ImGuiToastType::Info, 3000, "Fetching servers..." });

	std::string master_server_list = hmw_tcp_utils::GET_url(hmw_tcp_utils::MasterServer::get_master_server(), {}, false, 10000L, true, 3);

	auto server_index = std::make_shared<std::atomic<int>>(0);

	console::info("Checking if localhost server is running on default port (27017)");
	std::string port = "27017";
	bool localhost = hmw_tcp_utils::GameServer::is_localhost(port);

	if (localhost) {
		std::string local_res = hmw_tcp_utils::GET_url("localhost:27017/getInfo", {}, true, 1500L, false, 1);
		if (!local_res.empty()) {
			add_server(local_res, "localhost:27017", server_index->fetch_add(1));
		}
	}

	if (master_server_list.empty()) {
		console::info("Failed to get response from master server!");
		getting_server_list = false;
		ImGui::InsertNotification({ ImGuiToastType::Error, 3000, "Failed to get response from master server!" });
		return;
	}

	std::vector<std::thread> threads;

	try {
		nlohmann::json master_server_response_json = nlohmann::json::parse(master_server_list);

		for (const auto& element : master_server_response_json) {
			std::string connect_address = element.get<std::string>();

			{
				std::lock_guard<std::mutex> lock(interrupt_mutex);
				if (interrupt_server_list) {
					break; // If interrupted, break out of the loop
				}
			}

			// Launch threads for fetching server info
			threads.emplace_back([connect_address, server_index]() {
				try {
					fetch_game_server_info(Page::INTERNET, connect_address, server_index);
				}
				catch (std::exception e) {
					ImGui::InsertNotification({ ImGuiToastType::Error, 3000, ("Error fetching server info: %s", std::string(e.what()).data()) });
					console::error("Error fetching server info: %s", std::string(e.what()).data());
				}
				});
		}
	}

	catch (const std::exception& e) {
		getting_server_list = false;
		console::error("Error parsing master server JSON response: %s", std::string(e.what()));
		ImGui::InsertNotification({ ImGuiToastType::Error, 3000, "Failed to parse master server response." });
		return;
	}

	// Join all the threads to ensure they complete
	for (auto& t : threads) {
		if (t.joinable()) {
			t.join();
		}
	}

	{
		std::lock_guard<std::mutex> interrupt_lock(interrupt_mutex);
		interrupt_server_list = false;
		getting_server_list = false;
	}

	// Auto sort after server populate
	server_browser::sort(serverList, currentSortType, sort_ascending); // Sort by players descending
	ImGui::InsertNotification({ ImGuiToastType::Success, 3000, "Finished fetching servers." });
}

void server_browser::populate_favorites()
{
	nlohmann::json obj;
	if (!get_favourites_file(obj)) {
		{
			std::lock_guard<std::mutex> lock(interrupt_mutex);
			interrupt_favourites = false;
			getting_favourites = false;
		}
		ImGui::InsertNotification({ ImGuiToastType::Error, 3000, "No favourites file." });
		console::info("Finished getting favourites!");
		return;
	}

	// @CB: These try catches aren't really needed. But since this is multithreaded, it's better to be safe then sorry
	ImGui::InsertNotification({ ImGuiToastType::Info, 3000, "Getting favourites..." });

	auto server_index = std::make_shared<std::atomic<int>>(0);  // Use shared_ptr for thread-safe atomic
	std::vector<std::thread> threads;

	for (auto& element : obj)for (auto& element : obj) {
		if (!element.is_string()) {
			continue;
		}

		std::string connect_address = element.get<std::string>();
		{
			std::lock_guard<std::mutex> lock(interrupt_mutex);
			if (interrupt_favourites) {
				break;
			}
		}

		// Launch threads for fetching server info
		threads.emplace_back([connect_address, server_index]() {
			try {
				fetch_game_server_info(Page::FAVORITES, connect_address, server_index);
			}
			catch (const std::exception& e) {
				console::error("Error fetching favourite server info: %s", std::string(e.what()));
				ImGui::InsertNotification({ ImGuiToastType::Error, 3000, ("Error fetching favourites server info: %s", std::string(e.what()).data()) });
			}
			});
	}

	// Join all threads to ensure they complete
	for (auto& t : threads) {
		if (t.joinable()) {
			t.join();
		}
	}

	{
		std::lock_guard<std::mutex> interrupt_lock(interrupt_mutex);
		interrupt_favourites = false;
		getting_favourites = false;
	}

	ImGui::InsertNotification({ ImGuiToastType::Success, 3000, "Finished getting favourites!" });

	// Auto sort after parsing favourites
	server_browser::sort(serverList, currentSortType, sort_ascending); // Sort by players descending
}

void server_browser::populate_history()
{
	nlohmann::json obj;
	if (!get_history_file(obj)) {
		{
			std::lock_guard<std::mutex> lock(interrupt_mutex);
			interrupt_history = false;
			getting_history = false;
		}
		ImGui::InsertNotification({ ImGuiToastType::Error, 3000, "No history file." });
		console::info("Finished getting history!");
		return;
	}

	// @CB: These try catches aren't really needed. But since this is multithreaded, it's better to be safe then sorry
	ImGui::InsertNotification({ ImGuiToastType::Info, 3000, "Getting history..." });

	auto server_index = std::make_shared<std::atomic<int>>(0);  // Use shared_ptr for thread-safe atomic
	std::vector<std::thread> threads;

	for (auto& element : obj) {
		if (!element.is_string()) {
			continue;
		}

		std::string connect_address = element.get<std::string>();
		{
			std::lock_guard<std::mutex> lock(interrupt_mutex);
			if (interrupt_history) {
				break;
			}
		}

		// Launch threads for fetching server info
		threads.emplace_back([connect_address, server_index]() {
			try {
				fetch_game_server_info(Page::HISTORY, connect_address, server_index);
			}
			catch (const std::exception& e) {
				console::error("Error fetching historical server info: %s", std::string(e.what()));
				ImGui::InsertNotification({ ImGuiToastType::Error, 3000, ("Error fetching historical server info: %s", std::string(e.what()).data()) });
			}
			});
	}

	// Join all threads to ensure they complete
	for (auto& t : threads) {
		if (t.joinable()) {
			t.join();
		}
	}

	{
		std::lock_guard<std::mutex> interrupt_lock(interrupt_mutex);
		interrupt_history = false;
		getting_history = false;
	}

	ImGui::InsertNotification({ ImGuiToastType::Success, 3000, "Finished getting history!" });

	// Auto sort after parsing favourites
	server_browser::sort(serverList, currentSortType, sort_ascending); // Sort by players descending
}

void server_browser::fetch_game_server_info(Page page, const std::string& connect_address, std::shared_ptr<std::atomic<int>> server_index)
{
	{
		std::lock_guard<std::mutex> lock(interrupt_mutex);
		if (interrupt_server_list || interrupt_favourites || interrupt_history) {
			return;
		}
	}

	try {
		std::string game_server_info = connect_address + "/getInfo";
		std::string game_server_response = hmw_tcp_utils::GET_url(game_server_info.c_str(), {}, true, 1500L, true, 3);

		if (!game_server_response.empty()) {
			{
				std::lock_guard<std::mutex> lock(server_list_mutex);
				if (currentPage == page) {
					add_server(game_server_response, connect_address, server_index->fetch_add(1));
				}
			}
		}
	}
	catch (const std::exception& e) {
		console::error("Failed to fetch server info: %s", std::string(e.what()).data());
		ImGui::InsertNotification({ ImGuiToastType::Error, 3000, ("Failed to fetch server info: %s", std::string(e.what()).data()) });
	}
}

bool server_browser::has_favourite(std::string address)
{
	bool found = false;
	nlohmann::json obj;
	if (!get_favourites_file(obj)) {
		ImGui::InsertNotification({ ImGuiToastType::Error, 3000, "No favourites file." });
		return false;
	}
	for (auto& element : obj) {
		if (!element.is_string()) {
			continue;
		}

		std::string connect_address = element.get<std::string>();
		if (connect_address == address) {
			found = true;
			break;
		}
	}
	
	return found;
}

void server_browser::set_matchmaking_chosen_server(ServerData& data)
{
	matchmaking_chosen_server = data;
}

server_browser::ServerData& server_browser::get_chosen_matchmaking_server()
{
	return matchmaking_chosen_server;
}

std::string server_browser::to_lower(const std::string& str)
{
	std::string lowerStr = str;
	std::transform(lowerStr.begin(), lowerStr.end(), lowerStr.begin(), [](unsigned char c) {
		return std::tolower(c);
	});
	return lowerStr;
}
