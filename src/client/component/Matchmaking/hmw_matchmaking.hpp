#pragma once
	namespace hmw_matchmaking {
		static std::string HOST = "https://backend.horizonmw.org";

		namespace MatchMaker {
			void add_server(const std::string& infoJson, const std::string& connect_address, int server_index);
			void fetch_game_server_info(const std::string& connect_address, std::shared_ptr<std::atomic<int>> server_index);
			void find_match(std::string gameType = "any");
			void retry();
			void stop_matchmaking();

			double calculatePartyAverage(const std::vector<int>& partyPings);
			void set_playlist_type(std::string type);
			void set_selected_mode(std::string mode);

			std::string get_playlist();
			std::string get_selected_mode();

			void set_party_screen(bool state);
			bool showing_party_screen();

			int get_lobby_players_size();
			std::string get_lobby_player(int index);
			std::string get_search_status();
			// Server data functions
			std::string get_map_id();
			std::string get_map_name();
			std::string get_gamemode();

			void set_party_max(int max);
			int get_party_max();

			void set_party_limit(int amt);
			int get_party_size();

			void set_matchmaking_match_state(bool state);
			bool is_in_matchmaking_match();
		}

		namespace Auth {
			static std::string BEARER_TOKEN = "";
			static bool AUTOLOGIN = true;
			static std::string PROVIDER = "discord"; // Discord by default

			// OLD
			static std::string ACCESS_TOKEN = "";
			static std::string REFRESH_TOKEN = "";
			static long expires;

			enum AuthState {
				UNKNOWN,
				FAILED,
				SUCCESS
			};

			AuthState processAuthRequest(std::string url);
			AuthState getCurrentRequestState();
			std::string getCurrentRequestResponse();

			namespace Discord {
				void loginWithDiscord(std::function<void(AuthState, std::string, std::string)> callback);
				void completeDiscordAuth();
			}

			namespace Directus {
				void loginWithDirectus(std::string email, std::string password, std::function<void(AuthState, std::string, std::string)> callback);
				void completeDirectusAuth();
			}

			void attempt_autologin();
			// Auth
			// Deprecated
			bool add_auth_client();				
			bool authorize_game_client();
			std::string generate_random_string(size_t length = 32);					// Generate a random string of a defined length

			// Misc
			void update_loggedin_state(bool state);
			std::string sha256_hash(const std::string& input);

			void create_or_load_account_file();
			bool get_account_file(nlohmann::json& out);
			void save_account_file();
		}
};

