#include "std_include.hpp"
#include "hmw_matchmaking.hpp"
#include "tcp/hmw_tcp_utils.hpp"
#include "tomcrypt_private.h"
#include "component/scheduler.hpp"
#include "component/console.hpp"
#include "component/command.hpp"

#include "component/dvars.hpp"
#include "game/game.hpp"
#include "game/dvars.hpp"

#include <utils/io.hpp>
#include <string>
#include <random>
#include <algorithm>
#include "component/gui/login_panel/user_profile.hpp"
#include "component/gui/utils/notification.hpp"
#include "component/debug_gui/gui.hpp"
#include "game/ui_scripting/execution.hpp"
#include "component/gui/utils/ImGuiNotify.hpp"
#include "component/gui/browser/server_browser.hpp"
#include "component/party.hpp"

namespace {
    hmw_matchmaking::Auth::AuthState currentRequestState = hmw_matchmaking::Auth::AuthState::UNKNOWN;
    std::string currentRequestResponse = "";
    std::string DIRECTUS_PROVIDER = "";
    std::string DISCORD_PROVIDER = "discord";

    int auto_login_attempts = 0;
    int auto_login_max_attempts = 5;
    bool auto_login_successful = false;


    std::mutex mutex;
    std::mutex server_list_mutex;
    std::mutex interrupt_mutex;

    bool getting_server_list = false;
    bool interrupt_server_list = false;

    std::vector<server_browser::ServerData> servers;
    
    std::string playlist_type = "Standard"; // Core by default (Standard, Hardcore, Classic, Weekend Warfare)
    std::string selected_mode = "";
    // Data from server
    std::string map_id = "";
    std::string map_name = "";
    std::string game_mode = "";
    int party_max = 1;
    int party_size = 1;
    bool in_matchmaking_match = false;

    // Lui stuff
    bool showing_party_list = false;
    std::string search_status = "Connecting to master server...";

    std::string removeNonPrintable(const std::string& str) {
        std::string result;
        for (char c : str) {
            if (std::isprint(static_cast<unsigned char>(c))) {
                result += c;
            }
        }
        return result;
    }

    struct CandidateServer {
        std::string address;
        int ping;
        server_browser::ServerData info;
    };

    std::vector<std::string> lobby_players;

    // Matchmaking logic
    CandidateServer selectBestServer(int party_size, int partyAvgPing, const std::string& gameMode)
    {
        CandidateServer bestCandidate;
        int bestScore = -std::numeric_limits<int>::max();
        const int FULLNESS_WEIGHT = 2; // bonus per existing player
        const int BASELINE = 100;

        for (const auto& server : servers) {
            console::info("Checking server: %s -> %s", server.address.data(), server.gameMode.data());
            std::string clean_server = removeNonPrintable(server.gameMode);
            std::string clean_gamemode = removeNonPrintable(gameMode);
            bool match = (clean_server == clean_gamemode ? true : false);
            // If a specific gametype is requested, skip servers that don't match.
            if (gameMode != "any" && !match)
                continue;

            // Ensure the party can fit in the server.
            if (server.playercount + party_size > server.maxclients)
                continue;

            int diff = server.ping - partyAvgPing;
            int score = BASELINE - diff + (FULLNESS_WEIGHT * server.playercount);
            console::info("[Matchmaking] Server: %s -> scored: %d", server.address.data(), score);

            if (score > bestScore) {
                bestScore = score;
                bestCandidate.address = server.address;
                bestCandidate.ping = server.ping;
                bestCandidate.info = server;
            }
        }
        return bestCandidate;
    }

    void join_match(const game::netadr_s& target)
    {
        search_status = "Match found! Joining game...";
        scheduler::once([=]()
            {
                std::lock_guard<std::mutex> _(mutex);
                party::connect(target);
            }, scheduler::pipeline::main, 1s);
    }
}

void hmw_matchmaking::MatchMaker::add_server(const std::string& infoJson, const std::string& connect_address, int server_index)
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
        server.map = server_browser::get_map_name(map_name);
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

        std::vector<server_browser::PlayerData> players;

        // Process our new players array here
        if (game_server_response_json.contains("players") && game_server_response_json["players"].is_array())
        {
            for (const auto& player : game_server_response_json["players"])
            {
                server_browser::PlayerData pData;
                pData.name = player.value("name", "Unknown");
                pData.level = player.value("level", 0);
                pData.ping = player.value("ping", 999); // Default to high ping if missing

                players.push_back(pData);
            }
        }

        server.players = players;
        servers.push_back(server);
    }
}

void hmw_matchmaking::MatchMaker::fetch_game_server_info(const std::string& connect_address, std::shared_ptr<std::atomic<int>> server_index)
{
    {
        std::lock_guard<std::mutex> lock(interrupt_mutex);
        if (interrupt_server_list) {
            return;
        }
    }

    try {
        std::string game_server_info = connect_address + "/getInfo";
        std::string game_server_response = hmw_tcp_utils::GET_url(game_server_info.c_str(), {}, true, 1500L, true, 3);

        if (!game_server_response.empty()) {
            {
                if (interrupt_server_list || selected_mode.empty()) {
                    return;
                }

                std::lock_guard<std::mutex> lock(server_list_mutex);
                add_server(game_server_response, connect_address, server_index->fetch_add(1));
            }
        }
    }
    catch (const std::exception& e) {
        console::error("Failed to fetch server info: %s", std::string(e.what()).data());
        ImGui::InsertNotification({ ImGuiToastType::Error, 3000, ("Failed to fetch server info: %s", std::string(e.what()).data()) });
    }
}

void hmw_matchmaking::MatchMaker::find_match(std::string gameMode)
{
    if (getting_server_list) {
        console::info("Already getting matchmaking servers...");
        return;
    }

    // Ensure this is false before starting
    interrupt_server_list = false;
    servers.clear();
    lobby_players.clear();
    
    search_status = "Fetching matchmaking servers...";
    console::info("Getting matchmaking servers");

    std::string mode_filter = removeNonPrintable(gameMode);
    size_t pos = 0;
    while ((pos = mode_filter.find(" ", pos)) != std::string::npos) {
        mode_filter.replace(pos, 1, "%20");
        pos += 3;
    }
    std::string endpoint = std::string(hmw_tcp_utils::MasterServer::get_matchmaking_master_server()) + "?gamemode=" + mode_filter;
    console::info("Endpoint: %s", endpoint.data());
    std::string master_server_list = hmw_tcp_utils::GET_url(endpoint.c_str(), {}, false, 10000L, true, 3);

    auto server_index = std::make_shared<std::atomic<int>>(0);

    if (master_server_list.empty()) {
        console::info("Failed to get response from matchmaking master server!");
        search_status = "Failed to get response from matchmaking master server!";
        getting_server_list = false;
        retry();
        return;
    }

    std::vector<std::thread> threads;

    try {
        nlohmann::json master_server_response_json = nlohmann::json::parse(master_server_list);
        int server_count = static_cast<int>(master_server_response_json.size());
        search_status = "Found " + std::to_string(server_count) + " servers!";

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
                    fetch_game_server_info(connect_address, server_index);
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
        search_status = "Invalid master server response!";
        console::error("Error parsing matchmaking master server JSON response: %s", std::string(e.what()));
        retry();
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

    search_status = "Finished getting matchmaking servers!";
    console::info("Got matchmaking servers");

    scheduler::once([=]()
        {
            if (servers.size() > 0) {
                search_status = "Finding best canidate server...";
                CandidateServer best = selectBestServer(1, 20, gameMode);

                server_browser::set_matchmaking_chosen_server(best.info);
                if (!best.address.empty()) {
                    for (server_browser::PlayerData player : best.info.players) {
                        lobby_players.push_back(player.name);
                    }
                    map_id = best.info.map_id;
                    map_name = best.info.map;
                    game_mode = best.info.gameMode;
                    ui_scripting::notify("matchmaking_server_found", { {"players", lobby_players} });

                    stop_matchmaking(); // We have the sure so ensure the system stops
                    in_matchmaking_match = true;
                    join_match(best.info.net_address);
                }
                else {
                    map_id = "";
                    map_name = "";
                    game_mode = "";
                    search_status = "No suitable server found!";
                    console::error("No suitable server found!");
                    retry();
                }
            }
            else {
                map_id = "";
                map_name = "";
                game_mode = "";
                search_status = "Failed to find any matchmaking servers.";
                console::error("Failed to find any matchmaking servers.");
                retry();
            }
        }, scheduler::pipeline::main);
}

void hmw_matchmaking::MatchMaker::retry() {
    return;

    scheduler::once([=]()
        {
            if (selected_mode.empty()) {
                return;
            }

            find_match(selected_mode);
        }, scheduler::pipeline::network, 10s);
}

void hmw_matchmaking::MatchMaker::stop_matchmaking()
{
    interrupt_server_list = true;
    getting_server_list = false;
    selected_mode = "";
    console::info("Stopped matchmaking!");
}

double hmw_matchmaking::MatchMaker::calculatePartyAverage(const std::vector<int>& partyPings)
{
    double sum = 0.0;
    for (int ping : partyPings) {
        sum += ping;
    }
    return (partyPings.empty() ? 0 : sum / partyPings.size());
}

void hmw_matchmaking::MatchMaker::set_playlist_type(std::string type)
{
    playlist_type = type;
    console::info("Set playlist type to: %s", type.data());
}

void hmw_matchmaking::MatchMaker::set_selected_mode(std::string mode)
{
    selected_mode = mode;
    console::info("Selected gamemode: %s", selected_mode.data());
}

std::string hmw_matchmaking::MatchMaker::get_playlist()
{
    return removeNonPrintable(playlist_type);
}

std::string hmw_matchmaking::MatchMaker::get_selected_mode()
{
    return removeNonPrintable(selected_mode);
}

void hmw_matchmaking::MatchMaker::set_party_screen(bool state)
{
    showing_party_list = state;
}

bool hmw_matchmaking::MatchMaker::showing_party_screen()
{
    return showing_party_list;
}

int hmw_matchmaking::MatchMaker::get_lobby_players_size()
{
    return static_cast<int>(lobby_players.size());
}

std::string hmw_matchmaking::MatchMaker::get_lobby_player(int index)
{
    if (index < 0 || index >= get_lobby_players_size()) {
        return "Unknown";
    }

    return lobby_players[index];
}

std::string hmw_matchmaking::MatchMaker::get_search_status()
{
    return search_status;
}

std::string hmw_matchmaking::MatchMaker::get_map_id()
{
    return removeNonPrintable(map_id);
}

std::string hmw_matchmaking::MatchMaker::get_map_name()
{
    return removeNonPrintable(map_name);
}

std::string hmw_matchmaking::MatchMaker::get_gamemode()
{
    return removeNonPrintable(game_mode);
}

void hmw_matchmaking::MatchMaker::set_party_max(int max)
{
    party_max = max;
}

int hmw_matchmaking::MatchMaker::get_party_max()
{
    return party_max;
}

void hmw_matchmaking::MatchMaker::set_party_limit(int amt)
{
    party_size = amt;
}

int hmw_matchmaking::MatchMaker::get_party_size()
{
    return party_size;
}

void hmw_matchmaking::MatchMaker::set_matchmaking_match_state(bool state)
{
    in_matchmaking_match = state;
}

bool hmw_matchmaking::MatchMaker::is_in_matchmaking_match()
{
    return in_matchmaking_match;
}

void hmw_matchmaking::Auth::attempt_autologin()
{
    // Already logged in
    if (user_profile::is_logged_in()) {
        return;
    }

    // No account directory do not try to auto login
    if (!utils::io::directory_exists("account")) {
        return;
    }
    // No data file do not try to auto login
    if (!utils::io::file_exists("account/data.json")) {
        return;
    }

    // Auto login is opt-ed out.
    if (!hmw_matchmaking::Auth::AUTOLOGIN) {
        return;
    }

    // Auto login successful
    if (auto_login_successful) {
        return;
    }

    // Max auto login attempts
    if (auto_login_attempts >= auto_login_max_attempts) {
        return;
    }

    console::info("Attempting to auto login");

    create_or_load_account_file();

    hmw_matchmaking::Auth::AuthState auth = hmw_matchmaking::Auth::processAuthRequest("https://backend.horizonmw.org/items/player_data?fields[]=*&fields[]=player_account.*&filter[player_account][id][_eq]=$CURRENT_USER");
    nlohmann::json jsonData = nlohmann::json::parse(currentRequestResponse);

    switch (auth) {
        case hmw_matchmaking::Auth::AuthState::SUCCESS:
        {
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
            utils::io::download_file_with_auth(avatar_url, "account/avatar.jpg", BEARER_TOKEN);

            user_profile::login();
            std::cout << "========== User Info ==========\n"
                << "Username: " << profile_username << "\n"
                << "Prestige: " << profile_prestige << "\n"
                << "Rank XP:  " << profile_rankXp << "\n"
                << "Avatar: " << profile_avatar << "\n"
                << "Avatar URL: " << avatar_url << "\n"
                << "Bearer Token: " << BEARER_TOKEN << "\n"
                << "===============================" << std::endl;

            ImGui::InsertNotification({ ImGuiToastType::Success, 3000, ("Logged in as " + profile_username + "!").c_str() });
            console::info("Auto login successful");
            auto_login_successful = true;
            break;
        }
        case hmw_matchmaking::Auth::AuthState::FAILED:
            console::info("Auto login failed.");
            ImGui::InsertNotification({ ImGuiToastType::Error, 3000, "Auto login failed!" });
            auto_login_attempts++;
            break;
        default:
            ImGui::InsertNotification({ ImGuiToastType::Error, 3000, "Auto login failed! Unknown." });
            console::info("Auto login failed. Unknown.");
            auto_login_attempts++;
            break;
    }
    currentRequestResponse = ""; // We're done with the response now
    // This doesn't need the check as its not polling
    if (auto_login_successful) {
        scheduler::once([]() {
            ui_scripting::notify("handleLogin", {}); // Tell lui to update
            ui_scripting::notify("handleMyProfile", {});
            command::execute("lui_restart");
            command::execute("lui_open menu_xboxlive");
            }, scheduler::main);
    }
}

// AUTH

void hmw_matchmaking::Auth::update_loggedin_state(bool state) {
    scheduler::once([state]()
        {
            dvars::override::register_bool("mm_loggedin", state, game::DVAR_FLAG_NONE);
            if (state) {
                // We do NOT want to keep the username and password dvars now that we used them
                dvars::override::register_string("mm_username", "", game::DVAR_FLAG_NONE);
                dvars::override::register_string("mm_password", "", game::DVAR_FLAG_NONE);
            }
        }, scheduler::pipeline::main);
}

bool hmw_matchmaking::Auth::add_auth_client()
{   
    if (ACCESS_TOKEN.empty()) {
        std::cerr << "Failed to add auth client. No token." << std::endl;
        return false;
    }

    std::string secret = generate_random_string();

    std::string endpoint = HOST + "/endpoints/authorize-game-client?secret=" + secret;
    // Timeout after 3 seconds, do not retry
    auto[response, statusCode] = hmw_tcp_utils::GET_EASY_url(endpoint.c_str(), ACCESS_TOKEN, 3000L);
    if (response.empty()) {
        std::cout << "Failed to add auth client. No response." << std::endl;
        return false;
    }

    // Handle the response
    if (statusCode == 204) {
        std::cout << "Added Authorized client." << std::endl;
        return true;
    }
    else if (statusCode == 400) {
        std::cerr << "Failed to add auth client. Bad request." << std::endl;
        return false;
    }
    else {
        std::cout << "Failed to add auth client. Unknown Response (" << statusCode << "): " << response << std::endl;
        return false;
    }
    // Default
    return false;
}

bool hmw_matchmaking::Auth::authorize_game_client()
{
    if (ACCESS_TOKEN.empty()) {
        std::cerr << "Failed to add auth client. No token." << std::endl;
        return false;
    }

    std::string secret = generate_random_string();

    std::string endpoint = HOST + "/endpoints/authorize-game-client?secret=" + secret;
    // Timeout after 3 seconds, do not retry
    auto [response, statusCode] = hmw_tcp_utils::GET_EASY_url(endpoint.c_str(), ACCESS_TOKEN, 3000L);
    if (response.empty()) {
        std::cout << "Failed to authorize client. No response." << std::endl;
        return false;
    }

    // Handle the response
    if (statusCode == 204) {
        std::cout << "Authorized client." << std::endl;
        return true;
    }
    else if (statusCode == 400) {
        std::cerr << "Failed to authorize client. Bad request." << std::endl;
        return false;
    }
    else {
        std::cout << "Failed to authorize client. Unknown Response (" << statusCode << "): " << response << std::endl;
        return false;
    }
    // Default
    return false;
}

std::string hmw_matchmaking::Auth::generate_random_string(size_t length)
{
    const std::string characters =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz"
        "0123456789";

    // Use a random device to seed the generator
    std::random_device rd;
    std::mt19937 generator(rd());
    std::uniform_int_distribution<size_t> distribution(0, characters.size() - 1);

    // Generate the random string
    std::string randomString;
    std::generate_n(std::back_inserter(randomString), length, [&]() {
        return characters[distribution(generator)];
        });

    return randomString;
}

std::string hmw_matchmaking::Auth::sha256_hash(const std::string& input) {
    // Initialize variables
    hash_state md;
    unsigned char hash[32]; // SHA-256 produces a 32-byte hash

    // Initialize the hash state
    sha256_init(&md);

    // Process the input data
    sha256_process(&md, reinterpret_cast<const unsigned char*>(input.c_str()), (long) input.size());

    // Finalize the hash computation
    sha256_done(&md, hash);

    // Convert the hash to a hexadecimal string
    std::ostringstream oss;
    for (int i = 0; i < 32; ++i) {
        oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(hash[i]);
    }
    return oss.str();
}

void hmw_matchmaking::Auth::create_or_load_account_file()
{
    nlohmann::json account_obj;
    if (!get_account_file(account_obj)) {
        BEARER_TOKEN = generate_random_string();
        AUTOLOGIN = true;
        PROVIDER = DISCORD_PROVIDER;
        save_account_file();
    }
    else {
        BEARER_TOKEN = account_obj["bearer_token"].get<std::string>();
        AUTOLOGIN = account_obj["autologin"].get<bool>();
        PROVIDER = account_obj["provider"].get<std::string>();
    }
}

bool hmw_matchmaking::Auth::get_account_file(nlohmann::json& out)
{
    std::string data = utils::io::read_file("account/data.json");

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

    out = obj;

    return true;
}

void hmw_matchmaking::Auth::save_account_file()
{
    nlohmann::json obj;

    if (!utils::io::directory_exists("account")) {
        utils::io::create_directory("account");
    }

    std::ifstream data_file("account/data.json");

    if (data_file.is_open())
    {
        data_file >> obj;
        data_file.close();
    }

    obj["bearer_token"] = hmw_matchmaking::Auth::BEARER_TOKEN;
    obj["autologin"] = hmw_matchmaking::Auth::AUTOLOGIN;
    obj["provider"] = hmw_matchmaking::Auth::PROVIDER;

    utils::io::write_file("account/data.json", obj.dump());
}

// Discord auth DONE

void hmw_matchmaking::Auth::Discord::loginWithDiscord(std::function<void(AuthState, std::string, std::string)> callback)
{
    // Already logged in
    if (user_profile::is_logged_in()) {
        if (callback) {
            callback(currentRequestState, currentRequestResponse, BEARER_TOKEN);
        }
        return;
    }

    if (BEARER_TOKEN == "") {
        create_or_load_account_file();
    }

    if (PROVIDER != DISCORD_PROVIDER) {
        std::string error = "Attempted to login via discord with incorrect provider: " + PROVIDER;
        console::info(error.c_str());
        return;
    }

    std::string redirect = "https://backend.horizonmw.org/auth/login/discord?redirect=https%3A%2F%2Fbackend.horizonmw.org%2Fadmin%2Flogin%3Fredirect%3D%252Fauthorize-game-client%253Fsecret%253D" + BEARER_TOKEN  + "%26continue%3D";
    ShellExecute(0, "open", redirect.data(), 0, 0, SW_SHOWNORMAL);

    auto attemptCount = std::make_shared<int>(0);
    auto polling_task = std::make_shared<uint64_t>(0);
    // Poll the profile
    *polling_task = scheduler::repeat(
        [polling_task, callback, attemptCount]() mutable
        {
            auto request = processAuthRequest("https://backend.horizonmw.org/items/player_data?fields[]=*&fields[]=player_account.*&filter[player_account][id][_eq]=$CURRENT_USER");
            bool successful = request == hmw_matchmaking::Auth::AuthState::SUCCESS;

            // It was successful, treat as a success and stop the repeat early
            if (successful) {
                currentRequestState = request;

                if (callback) {
                    callback(currentRequestState, currentRequestResponse, BEARER_TOKEN);
                }

                completeDiscordAuth();
                // Stop polling
                scheduler::stop(*polling_task, scheduler::async);
            }
            else {
                (*attemptCount)++;

                // Reached the final run, process as a failure
                if (*attemptCount >= 59) {
                    currentRequestState = request;

                    if (callback) {
                        callback(currentRequestState, currentRequestResponse, BEARER_TOKEN);
                    }

                    completeDiscordAuth();
                }
            }
        },
        scheduler::async, 1s, 60); // Repeats once a second 60 times. So basically once a second for a minute
}

void hmw_matchmaking::Auth::Discord::completeDiscordAuth()
{
    // Reset the state back
    currentRequestState = hmw_matchmaking::Auth::AuthState::UNKNOWN;
    currentRequestResponse = "";
}

// Executed every second for 60 seconds
hmw_matchmaking::Auth::AuthState hmw_matchmaking::Auth::processAuthRequest(std::string url)
{
    // Make a request to the url with your auth token. Time out after 1 second
    std::pair<std::string, long> response = hmw_tcp_utils::GET_EASY_url(url.data(), BEARER_TOKEN, 1000L);
    std::string res = response.first;
    long res_code = response.second;

    currentRequestResponse = res;

    try {
        nlohmann::json j = nlohmann::json::parse(res);

        if (j.contains("errors") && j["errors"].is_array() && !j["errors"].empty())
        {
            // Errors were found fail
            console::error("Failed to process auth. An error occured.");
            return hmw_matchmaking::Auth::AuthState::FAILED;
        }
    }
    catch (const std::exception& e)
    {
        // Something went wrong fail
        console::error(("JSON parse error: " + std::string(e.what())).c_str());
        return hmw_matchmaking::Auth::AuthState::FAILED;
    }

    // Response is empty. Fail
    if (res.empty()) {
        console::error("Failed to process auth request. Empty.");
        return hmw_matchmaking::Auth::AuthState::FAILED;
    }

    // No errors found and response is not empty. Success!
    return hmw_matchmaking::Auth::AuthState::SUCCESS;
}

hmw_matchmaking::Auth::AuthState hmw_matchmaking::Auth::getCurrentRequestState()
{
    return currentRequestState;
}

std::string hmw_matchmaking::Auth::getCurrentRequestResponse() {
    return currentRequestResponse;
}

void hmw_matchmaking::Auth::Directus::loginWithDirectus(std::string email, std::string password, std::function<void(AuthState, std::string, std::string)> callback)
{
    if (user_profile::is_logged_in()) {
        if (callback) {
            callback(currentRequestState, currentRequestResponse, BEARER_TOKEN);
        }
        return;
    }

    if (BEARER_TOKEN == "") {
        create_or_load_account_file();
    }

    if (PROVIDER != DIRECTUS_PROVIDER) {
        std::string error = "Attempted to login via directus with incorrect provider: " + PROVIDER;
        console::info(error.c_str());
        return;
    }

    std::string endpoint = hmw_matchmaking::HOST + "/auth/login";

    nlohmann::json body;
    body["email"] = email;
    body["password"] = password;

    std::cerr << "Trying to log in. body: " << body.dump() << std::endl;

    hmw_matchmaking::Auth::AuthState res = hmw_matchmaking::Auth::AuthState::FAILED;

    std::string response = hmw_tcp_utils::POST_url(endpoint.c_str(), body.dump(), 10000L); // Timeout after 10 second
    if (response.empty()) {
        std::cerr << "No response from login." << std::endl;
        return;
    }

    try {
        // Parse the response as JSON
        nlohmann::json jsonResponse = nlohmann::json::parse(response);

        // Check for errors
        if (jsonResponse.contains("errors")) {
            for (const auto& error : jsonResponse["errors"]) {
                std::cerr << "Error: " << error["message"].get<std::string>() << std::endl;
                if (error.contains("extensions") && error["extensions"].contains("reason")) {
                    std::cerr << "Reason: " << error["extensions"]["reason"].get<std::string>() << std::endl;
                }
            }
            return;
        }

        expires = jsonResponse["data"]["expires"].get<long>();
        ACCESS_TOKEN = jsonResponse["data"]["access_token"].get<std::string>();
        REFRESH_TOKEN = jsonResponse["data"]["refresh_token"].get<std::string>();

        /// ^^^ wtf am I suppose to do with this x_x

        // Normal junk from Discord side
        // Now check directus is actually logged in
        res = hmw_matchmaking::Auth::AuthState::SUCCESS;
        currentRequestResponse = response;
        currentRequestState = res;

        if (callback) {
            callback(currentRequestState, currentRequestResponse, BEARER_TOKEN);
        }

        completeDirectusAuth();
    }
    catch (const nlohmann::json::exception& e) {
        std::cerr << "JSON Parsing Error: " << e.what() << std::endl;
    }
}

void hmw_matchmaking::Auth::Directus::completeDirectusAuth()
{
    currentRequestState = hmw_matchmaking::Auth::AuthState::UNKNOWN;
    currentRequestResponse = "";
}
