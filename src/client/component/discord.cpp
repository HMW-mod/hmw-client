#include <std_include.hpp>
#include "loader/component_loader.hpp"

#include "console.hpp"
#include "command.hpp"
#include "discord.hpp"
#include "fastfiles.hpp"
#include "materials.hpp"
#include "network.hpp"
#include "party.hpp"
#include "scheduler.hpp"

#include "game/game.hpp"
#include "game/ui_scripting/execution.hpp"

#include <utils/string.hpp>
#include <utils/cryptography.hpp>
#include <utils/http.hpp>

#include <discord_rpc.h>

#define DEFAULT_AVATAR "discord_default_avatar"
#define AVATAR "discord_avatar_%s"

#define DEFAULT_AVATAR_URL "https://cdn.discordapp.com/embed/avatars/0.png"
#define AVATAR_URL "https://cdn.discordapp.com/avatars/%s/%s.png?size=128"

namespace discord
{
	namespace
	{
		DiscordRichPresence discord_presence;

		void update_discord()
		{
			if (!game::CL_IsCgameInitialized() || game::VirtualLobby_Loaded())
			{
				discord_presence.details = SELECT_VALUE("Singleplayer", "Multiplayer");
				discord_presence.state = "Main Menu";

				discord_presence.partySize = 0;
				discord_presence.partyMax = 0;
				discord_presence.startTimestamp = 0;
				discord_presence.largeImageKey = SELECT_VALUE("menu_singleplayer", "menu_multiplayer");
				discord_presence.largeImageText = "";
				discord_presence.smallImageKey = "";

				discord_presence.matchSecret = "";
				discord_presence.joinSecret = "";
				discord_presence.partyId = "";

				const auto in_firing_range = game::Dvar_FindVar("virtualLobbyInFiringRange");
				if (in_firing_range && in_firing_range->current.enabled == 1)
				{
					discord_presence.state = "Firing Range";
					discord_presence.largeImageKey = "mp_vlobby_room";
				}
			}
			else
			{
				static char details[0x80] = {0};
				const auto mapname = game::Dvar_FindVar("mapname")->current.string;
				const auto presence_key = utils::string::va("PRESENCE_%s%s", SELECT_VALUE("SP_", ""), mapname);
				const char* clean_mapname = mapname;

				if (game::DB_XAssetExists(game::ASSET_TYPE_LOCALIZE, presence_key) && !game::DB_IsXAssetDefault(game::ASSET_TYPE_LOCALIZE, presence_key))
				{
					clean_mapname = game::UI_SafeTranslateString(presence_key);
				}

				if (game::environment::is_mp())
				{
					static char clean_gametype[0x80] = {0};
					const auto gametype = game::UI_GetGameTypeDisplayName(game::Dvar_FindVar("g_gametype")->current.string);
					utils::string::strip(gametype, clean_gametype, sizeof(clean_gametype));
					strcpy_s(details, 0x80, utils::string::va("%s on %s", clean_gametype, clean_mapname));

					static char clean_hostname[0x80] = {0};
					utils::string::strip(game::Dvar_FindVar("sv_hostname")->current.string, clean_hostname, sizeof(clean_hostname));

					auto max_clients = party::server_client_count();

					if (game::SV_Loaded())
					{
						strcpy_s(clean_hostname, "Private Match");
						max_clients = game::Dvar_FindVar("sv_maxclients")->current.integer;
						discord_presence.partyPrivacy = DISCORD_PARTY_PRIVATE;
					}
					else
					{
						const auto server_net_info = party::get_state_host();
						const auto server_ip_port = utils::string::va("%i.%i.%i.%i:%i",
							static_cast<int>(server_net_info.ip[0]), 
							static_cast<int>(server_net_info.ip[1]),
							static_cast<int>(server_net_info.ip[2]),
							static_cast<int>(server_net_info.ip[3]),
							static_cast<int>(ntohs(server_net_info.port))
						);

						static char join_secret[0x80] = {0};
						strcpy_s(join_secret, 0x80, server_ip_port);

						static char party_id[0x80] = {0};
						const auto server_ip_port_hash = utils::cryptography::sha1::compute(server_ip_port, true).substr(0, 8);
						strcpy_s(party_id, 0x80, server_ip_port_hash.data());

						discord_presence.partyId = party_id;
						discord_presence.joinSecret = join_secret;
						discord_presence.partyPrivacy = DISCORD_PARTY_PUBLIC;
					}

					const auto client_state = *game::mp::client_state;
					if (client_state != nullptr)
					{
						discord_presence.partySize = client_state->num_players;
					}
					else
					{
						discord_presence.partySize = 0;
					}

					discord_presence.partyMax = max_clients;
					discord_presence.state = clean_hostname;

					auto discord_map_image_asset = mapname;
					if (!fastfiles::is_stock_map(mapname))
					{
						discord_map_image_asset = "menu_multiplayer"; // TODO: maybe add usermap images
					}

					auto discord_server_info = party::get_discord_server_image();
					if (!discord_server_info.empty())
					{
						// prioritize server image as large image instead
						discord_presence.smallImageKey = discord_map_image_asset;
						discord_presence.largeImageKey = discord_server_info.data();
						discord_server_info = party::get_discord_server_text();
						if (!discord_server_info.empty())
						{
							discord_presence.largeImageText = discord_server_info.data();
						}
					}
					else
					{
						discord_presence.smallImageKey = "";
						discord_presence.largeImageKey = discord_map_image_asset;
						discord_presence.largeImageText = "";
					}
				}
				else if (game::environment::is_sp())
				{
					discord_presence.state = "";
					discord_presence.largeImageKey = mapname;
					discord_presence.smallImageKey = "";
					strcpy_s(details, 0x80, clean_mapname);
				}

				discord_presence.details = details;

				if (!discord_presence.startTimestamp)
				{
					discord_presence.startTimestamp = std::chrono::duration_cast<std::chrono::seconds>(
						std::chrono::system_clock::now().time_since_epoch()).count();
				}
			}

			Discord_UpdatePresence(&discord_presence);
		}

		void download_user_avatar(const std::string& id, const std::string& avatar)
		{
			const auto data = utils::http::get_data(
				utils::string::va(AVATAR_URL, id.data(), avatar.data()));
			if (!data.has_value())
			{
				return;
			}

			const auto& value = data.value();
			if (value.code != CURLE_OK)
			{
				return;
			}

			materials::add(utils::string::va(AVATAR, id.data()), value.buffer);
		}

		bool has_default_avatar = false;
		void download_default_avatar()
		{
			const auto data = utils::http::get_data(DEFAULT_AVATAR_URL);
			if (!data.has_value())
			{
				return;
			}

			const auto& value = data.value();
			if (value.code != CURLE_OK)
			{
				return;
			}

			has_default_avatar = true;
			materials::add(DEFAULT_AVATAR, value.buffer);
		}
	}

	std::string get_avatar_material(const std::string& id)
	{
		const auto avatar_name = utils::string::va(AVATAR, id.data());
		if (materials::exists(avatar_name))
		{
			return avatar_name;
		}

		if (has_default_avatar)
		{
			return DEFAULT_AVATAR;
		}

		return "black";
	}

	void respond(const std::string& id, int reply)
	{
		scheduler::once([=]()
		{
			Discord_Respond(id.data(), reply);
		}, scheduler::pipeline::async);
	}

	class component final : public component_interface
	{
	public:
		void post_load() override
		{
			if (game::environment::is_dedi())
			{
				return;
			}

			DiscordEventHandlers handlers;
			ZeroMemory(&handlers, sizeof(handlers));
			handlers.ready = ready;
			handlers.errored = errored;
			handlers.disconnected = errored;
			handlers.spectateGame = nullptr;

			if (game::environment::is_mp())
			{
				handlers.joinGame = join_game;
				handlers.joinRequest = join_request;
			}
			else
			{
				handlers.joinGame = nullptr;
				handlers.joinRequest = nullptr;
			}

			Discord_Initialize("947125042930667530", &handlers, 1, nullptr);

			scheduler::once(download_default_avatar, scheduler::pipeline::async);

			scheduler::once([]
			{
				scheduler::once(update_discord, scheduler::pipeline::async);
				scheduler::loop(update_discord, scheduler::pipeline::async, 5s);
				scheduler::loop(Discord_RunCallbacks, scheduler::pipeline::async, 1s);
			}, scheduler::pipeline::main);

			initialized_ = true;
		}

		void pre_destroy() override
		{
			if (!initialized_ || game::environment::is_dedi())
			{
				return;
			}

			Discord_Shutdown();
		}

	private:
		bool initialized_ = false;

		static void ready(const DiscordUser* request)
		{
			ZeroMemory(&discord_presence, sizeof(discord_presence));
			discord_presence.instance = 1;
			console::info("Discord: Ready on %s (%s)\n", request->username, request->userId);
			Discord_UpdatePresence(&discord_presence);
		}

		static void errored(const int error_code, const char* message)
		{
			console::error("Discord: Error (%i): %s\n", error_code, message);
		}

		static void join_game(const char* join_secret)
		{
			scheduler::once([=]
			{
				game::netadr_s target{};
				if (game::NET_StringToAdr(join_secret, &target))
				{
					console::info("Discord: Connecting to server: %s\n", join_secret);
					party::connect(target);
				}
			}, scheduler::pipeline::main);
		}

		static void join_request(const DiscordUser* request)
		{
			console::info("Discord: Join request from %s (%s)\n", request->username, request->userId);

			if (game::Com_InFrontend() || !ui_scripting::lui_running())
			{
				Discord_Respond(request->userId, DISCORD_REPLY_IGNORE);
				return;
			}

			std::string user_id = request->userId;
			std::string avatar = request->avatar;
			std::string discriminator = request->discriminator;
			std::string username = request->username;

			scheduler::once([=]
			{
				const ui_scripting::table request_table{};
				request_table.set("avatar", avatar);
				request_table.set("discriminator", discriminator);
				request_table.set("userid", user_id);
				request_table.set("username", username);

				ui_scripting::notify("discord_join_request",
				{
					{"request", request_table}
				});
			}, scheduler::pipeline::lui);

			if (!materials::exists(utils::string::va(AVATAR, user_id.data())))
			{
				download_user_avatar(user_id, avatar);
			}
		}
	};
}

REGISTER_COMPONENT(discord::component)
