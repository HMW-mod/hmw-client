#pragma once
#include <string>
#include <vector>
#include "game/game.hpp"
#include <json.hpp>
#include <algorithm>
#include <cctype>

namespace server_browser {
    
    struct PlayerData {
        int level;                      // The level of the player
        std::string name;               // The name of the player
        int ping;                       // The players current ping at the time of the request
    };

    struct ServerData {
        game::netadr_s net_address;     // Net address
        std::string address;            // Address used for connecting
        std::string serverName;         // Name of the server
        std::string map;                // Map being played Example: (Terminal)
        std::string map_id;             // Map ID Example: (mp_terminal)
        std::string gameType;           // Game type (dom, tdm, ctf)
        std::string gameMode;           // Game mode (Domination, Team Deathmatch, Capture the Flag)
        std::string gameVersion;        // Game version
        game::CodPlayMode play_mode;    // Play mode   
        int ping;                       // Ping
        int clients;                    // Number of players
        int maxclients;                 // Total server capacity
        int bots;                       // Number of bots
        int playercount;                // Number of real players
        bool verified;                  // Whether the server is verified
        bool outdated;                  // Is the server outdated
        bool is_private;                // Is a private server
        std::string mod_name;           // fs_game
        std::string motd;               // Message of the day
        std::vector<PlayerData> players;// Players in the game
    };

    enum SortType {
        SERVER_NAME,
        MAP,
        GAME_MODE,
        GAME_VERSION,
        PING,
        PLAYERS,
        VERSION,
        VERIFIED
    };

    enum Page {
        INTERNET,
        FAVORITES,
        HISTORY
    };

    // Function declarations
    void show_window();
    void toggle();
    bool is_open();

    std::string get_clean_server_name(const std::string& serverName);
    void refresh(Page page);
    void refresh_current_page();
    void sort(std::vector<ServerData>& list, SortType sortType, bool ascending);
    void render_server_list(char* searchBuffer, std::vector<ServerData>& filteredServerList);

    int get_player_count(std::vector<ServerData>& list);
    int get_server_count(std::vector<ServerData>& list);

    bool get_favourites_file(nlohmann::json& out);
    void add_favourite(std::vector<ServerData>& list, int server_index);
    void delete_favourite(std::vector<ServerData>& list, int server_index);

    bool get_history_file(nlohmann::json& out);
    void add_history(std::vector<ServerData>& list, int server_index);
    void delete_history(std::vector<ServerData>& list, int server_index);
    
    void join_server(std::vector<ServerData>& list, int index);
    bool check_can_join(const char* connect_address);
    std::string get_map_name(std::string map_name);

    void add_server_to_display(const ServerData& data);
    void add_server(const std::string& infoJson, const std::string& connect_address, int server_index);
    void populate_server_list();
    void populate_favorites();
    void populate_history();
    void fetch_game_server_info(Page page, const std::string& connect_address, std::shared_ptr<std::atomic<int>> server_index);
    bool has_favourite(std::string address);

    // Matchmaking
    void set_matchmaking_chosen_server(ServerData& data);
    ServerData& get_chosen_matchmaking_server();

    // Helper functions
    std::string to_lower(const std::string& str);
}
