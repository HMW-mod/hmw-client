#include "std_include.hpp"
#include "server_browser_data.hpp"
#include <vector>
#include <string>
#include <cstdlib>
#include <ctime>
#include "component/gui/fonts/RpgAwesome.h"

std::vector<ServerData> GetFakeServerData() {
    std::vector<ServerData> servers;
    servers.reserve(500);
    std::srand(std::time(nullptr));
    for (int i = 1; i <= 500; i++) {
        std::string serverName = "^*rLorem ipsum dolor sit amet, consetetur sadipscing elitr, sed diam nonumy eirmod tempor " + std::to_string(i);
        std::string mapName = "Lorem ipsum dolor " + std::to_string((i % 5) + 1);
        std::string gameMode;
        if (i % 3 == 0)
            gameMode = "Deathmatch";
        else if (i % 4 == 1)
            gameMode = "Team Deathmatch";
        else
            gameMode = "Capture The Flag";
        int playercount = (i % 20) + 1;
        int maxclients = 20;
        int bots = (i % 3);
        std::string gameVersion = (std::rand() % 2 == 0) ? "1.3.2" : "1.2.9";
        int ping = 20 + ((i * 3) % 400);
        std::string anticheat = (std::rand() % 2) == 0 ? ICON_RPG_TWO_DRAGONS : "";
        std::string info = "";
        if (std::rand() % 2 == 0) {
            info = "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Sed do eiusmod tempor incididunt ut labore et dolore magna aliqua.";
        }
        std::string motd = "Welcome to " + serverName;
        if (std::rand() % 2 == 0) {
            motd = "This is the message of the day for server " + std::to_string(i);
        }
        std::string gameType = (std::rand() % 2 == 0) ? "Core" : "Hardcore";
        servers.push_back({ serverName, mapName, gameMode, playercount, maxclients, bots, gameVersion, ping, anticheat, info, motd, gameType });
    }
    return servers;
}
