#ifndef SERVER_BROWSER_DATA_H
#define SERVER_BROWSER_DATA_H

#include <string>
#include <vector>

struct ServerData {
    std::string serverName;
    std::string map;
    std::string gameMode;
    int playercount;
    int maxclients;
    int bots;
    std::string gameVersion;
    int ping;
    std::string anticheat;
    std::string info;
    std::string motd;
    std::string gameType;
};

std::vector<ServerData> GetFakeServerData();

#endif // SERVER_BROWSER_DATA_H
