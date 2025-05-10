#pragma once
#include <string>
#include <vector>
#include "server_browser_data.hpp"

enum class Page { INTERNET, FAVORITES, HISTORY };
enum class SortType { SERVER_NAME, MAP, GAME_MODE, PLAYERS, VERSION, PING, ANTICHEAT, INFO };

class serverBrowser {
public:
    serverBrowser();
    void toggle();
    void draw();
private:
    void renderServerList();
    void sortServerList(SortType sortType, bool ascending);
    std::string toLower(const std::string& str);
    std::string getSelectedMapsPreview() const;
    std::string getSelectedGameModesPreview() const;

    bool opened;
    Page currentPage;
    SortType currentSortType;
    bool sortAscending;
    bool showInfoWindow;
    float infoWindowAnimationStartTime;
    float infoWindowOpenAnimationStartTime;
    float infoWindowCloseAnimationStartTime;

    bool filterShowAnticheat;
    bool filterShowFullServers;
    bool filterShowEmptyServers;
    int filterMinPing;
    int filterMaxPing;
    char searchBuffer[256];
    char mapFilterInput[256];
    char gameModeFilterInput[256];
    std::vector<ServerData> serverList;
    std::vector<ServerData> filteredServerList;
    std::vector<std::string> filterSelectedMaps;
    std::vector<std::string> filterSelectedGameModes;
    ServerData currentInfoServer;

    bool isClosing;
    bool startClosing;
};
