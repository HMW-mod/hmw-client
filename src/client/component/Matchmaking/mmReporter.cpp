// mmReporter.cpp

#include "std_include.hpp"
#include "mmReporter.hpp"
#include <curl/curl.h>
#include "component/console.hpp"
#include "game/scripting/execution.hpp"
#include "tcp/hmw_tcp_utils.hpp"
#include "json.hpp"
#include <iostream>
#include "game/structs.hpp"

struct CurlGlobalInit {
    CurlGlobalInit() { curl_global_init(CURL_GLOBAL_DEFAULT); }
    ~CurlGlobalInit() { curl_global_cleanup(); }
};
static CurlGlobalInit curlInit;

const game::dvar_t* sv_lanOnly;

inline bool sv_is_lanOnly()
{
    return (sv_lanOnly && sv_lanOnly->current.enabled);
}

namespace mmReporter
{
        // generic helper function to make sure we fetch
        // fields safely and return a default value when
        // something shits the bed
        template<typename T>
        T safeField(const scripting::entity& ent, const char* fieldName, T defaultValue = T()) {
            try {
                return ent.get(fieldName).as<T>();
            }
            catch (...) {
                return defaultValue;
            }
        }

        inline std::string playerStatsJson() {
            using namespace scripting;
            using json = nlohmann::json;

            int maxClients = *game::svs_numclients;
            if (maxClients <= 0) {
                return "";
            }

            array allEnts = call("getentarray").as<array>();
            if (allEnts.size() == 0)
                return "";

            json stats;
            stats["players"] = json::object();

            for (size_t i = 0; i < allEnts.size(); ++i) {
                entity ent = allEnts[i].as<entity>();
                auto ref = ent.get_entity_reference();

                if (ref.classnum != 0) {
                    continue;
                }

                int clientNum = ref.entnum;
                if (clientNum < 0 || clientNum >= maxClients) {
                    continue;
                }

                auto* gEnt = &game::g_entities[clientNum];
                if (!gEnt || !gEnt->client) {
                    continue;
                }

                const char* nameC = gEnt->client->name;
                // we dont care about bots
                if (nameC[0] == '\0' || game::SV_BotIsBot(clientNum)) {
                    continue;
                }

                std::string name = nameC;

                json pStats;
                // we can use internal functions for those
                pStats["guid"] = game::SV_GetGuid(clientNum);
                pStats["ping"] = game::SV_GetClientPing(clientNum);
                pStats["score"] = game::G_GetClientScore(clientNum);

                // we get those from in engine scripting api (check h1_token for more fields)
                // should probably get replaced with score_t
                // or scoreInfo at some point?
                pStats["kills"] = safeField<int>(ent, "kills", 0);
                pStats["assists"] = safeField<int>(ent, "assists", 0);
                pStats["deaths"] = safeField<int>(ent, "deaths", 0);
                pStats["headshots"] = safeField<int>(ent, "headshots", 0);
                pStats["killstreakcount"] = safeField<int>(ent, "killstreakcount", 0);

                stats["players"][name] = std::move(pStats);
            }

            if (stats["players"].empty()) {
                return "";
            }

            return stats.dump(4);
        }

    void reportPlayerStats()
    {
        if (sv_is_lanOnly())
            return;

        auto payload = playerStatsJson();
        if (payload.empty()) {
            console::info("No players? Skip");
            return;
        }

        console::info(("Player stats JSON:\n" + payload).c_str());

        CURL* curl = curl_easy_init();
        if (!curl) {
            console::error("curl_easy_init() failed");
            return;
        }

            // change this url to the proper endpoint
        curl_easy_setopt(curl, CURLOPT_URL, "https://backend.horizonmw.org/bleh");
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PUT");
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payload.c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, payload.size());
        struct curl_slist* headers = nullptr;
        headers = curl_slist_append(headers, "Content-Type: application/json");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

        CURLcode res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            std::cerr << "Failed to send player stats: " << curl_easy_strerror(res) << "\n";
        }
        else {
            long code = 0;
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);
            if (code == 204) {
                console::info("[OK] Player stats sent");
            }
            else if (code == 400) {
                console::error("[ERROR] Bad request");
            }
            else if (code == 524) {
                console::error("[ERROR] Timeout on server side");
            }
        }

        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
    }
}
