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

		void update_discord_frontend()
		{
			discord_presence.details = SELECT_VALUE("Singleplayer", "Multiplayer");
			discord_presence.startTimestamp = 0;

			const auto in_firing_range = game::Dvar_FindVar("virtualLobbyInFiringRange");
			if (in_firing_range && in_firing_range->current.enabled == 1)
			{
				discord_presence.state = "Firing Range";
				discord_presence.largeImageKey = "mp_vlobby_room";
			}
			else
			{
				discord_presence.state = "Main Menu";
				discord_presence.largeImageKey = SELECT_VALUE("menu_singleplayer", "menu_multiplayer");
			}

			Discord_UpdatePresence(&discord_presence);
		}

		void update_discord_ingame()
		{
			static const auto mapname_dvar = game::Dvar_FindVar("mapname");
			auto mapname = mapname_dvar->current.string;

			discord_presence.largeImageKey = mapname;

			const auto presence_key = utils::string::va("PRESENCE_%s%s", SELECT_VALUE("SP_", ""), mapname);
			if (game::DB_XAssetExists(game::ASSET_TYPE_LOCALIZE, presence_key) && 
				!game::DB_IsXAssetDefault(game::ASSET_TYPE_LOCALIZE, presence_key))
			{
				mapname = game::UI_SafeTranslateString(presence_key);
			}

			if (game::environment::is_mp())
			{
				// setup Discord details (Free-for-all on Shipment)
				static char gametype[0x80] = {0};
				static const auto gametype_dvar = game::Dvar_FindVar("g_gametype");
				const auto gametype_display_name = game::UI_GetGameTypeDisplayName(gametype_dvar->current.string);
				utils::string::strip(gametype_display_name, gametype, sizeof(gametype));

				discord_presence.details = utils::string::va("%s on %s", gametype, mapname);

				// setup Discord party (1 of 18)
				const auto client_state = *game::mp::client_state;
				if (client_state != nullptr)
				{
					discord_presence.partySize = client_state->num_players;
				}

				if (game::SV_Loaded())
				{
					discord_presence.state = "Private Match";
					static const auto maxclients_dvar = game::Dvar_FindVar("sv_maxclients");
					discord_presence.partyMax = maxclients_dvar->current.integer;
					discord_presence.partyPrivacy = DISCORD_PARTY_PRIVATE;
				}
				else
				{
					static char hostname[0x80] = {0};
					static const auto hostname_dvar = game::Dvar_FindVar("sv_hostname");
					utils::string::strip(hostname_dvar->current.string, hostname, sizeof(hostname));
					discord_presence.state = hostname;

					const auto server_connection_state = party::get_server_connection_state();

					const auto server_ip_port = utils::string::va("%i.%i.%i.%i:%i",
						static_cast<int>(server_connection_state.host.ip[0]),
						static_cast<int>(server_connection_state.host.ip[1]),
						static_cast<int>(server_connection_state.host.ip[2]),
						static_cast<int>(server_connection_state.host.ip[3]),
						static_cast<int>(ntohs(server_connection_state.host.port))
					);

					static char party_id[0x80] = {0};
					const auto server_ip_port_hash = utils::cryptography::sha1::compute(server_ip_port, true).substr(0, 8);
					strcpy_s(party_id, 0x80, server_ip_port_hash.data());
					discord_presence.partyId = party_id;
					discord_presence.partyMax = server_connection_state.max_clients;
					discord_presence.partyPrivacy = DISCORD_PARTY_PUBLIC;

					static char join_secret[0x80] = { 0 };
					strcpy_s(join_secret, 0x80, server_ip_port);
					discord_presence.joinSecret = join_secret;
				}

				auto server_discord_information = party::get_server_discord_information();
				if (!server_discord_information.image.empty())
				{
					discord_presence.smallImageKey = server_discord_information.image.data();
					discord_presence.smallImageText = server_discord_information.image_text.data();
				}
			}
			else if (game::environment::is_sp())
			{
				discord_presence.details = mapname;
			}

			if (discord_presence.startTimestamp == 0)
			{
				discord_presence.startTimestamp = std::chrono::duration_cast<std::chrono::seconds>(
					std::chrono::system_clock::now().time_since_epoch()).count();
			}

			Discord_UpdatePresence(&discord_presence);
		}

		void update_discord()
		{
			Discord_RunCallbacks();

			// reset presence data
			const auto saved_time = discord_presence.startTimestamp;
			discord_presence = {};
			discord_presence.startTimestamp = saved_time;

			if (!game::CL_IsCgameInitialized() || game::VirtualLobby_Loaded())
			{
				update_discord_frontend();
			}
			else
			{
				update_discord_ingame();
			}
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

		void ready(const DiscordUser* request)
		{
			DiscordRichPresence presence{};
			presence.instance = 1;
			presence.state = "";
			console::info("Discord: Ready on %s (%s)\n", request->username, request->userId);
			Discord_UpdatePresence(&presence);
		}

		void errored(const int error_code, const char* message)
		{
			console::error("Discord: %s (%i)\n", message, error_code);
		}

		void join_game(const char* join_secret)
		{
			console::debug("Discord: join_game called with secret '%s'\n", join_secret);

			scheduler::once([=]
			{
				game::netadr_s target{};
				if (game::NET_StringToAdr(join_secret, &target))
				{
					console::info("Discord: Connecting to server '%s'\n", join_secret);
					party::connect(target);
				}
			}, scheduler::pipeline::main);
		}

		void join_request(const DiscordUser* request)
		{
			console::debug("Discord: Join request from %s (%s)\n", request->username, request->userId);

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
		void post_unpack() override
		{
			if (game::environment::is_dedi())
			{
				return;
			}

			DiscordEventHandlers handlers{};
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
			scheduler::loop(update_discord, scheduler::pipeline::async, 5s);

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
	};
}

REGISTER_COMPONENT(discord::component)
