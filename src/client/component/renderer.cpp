#include <std_include.hpp>
#include "loader/component_loader.hpp"

#include "dvars.hpp"

#include "game/game.hpp"
#include "game/dvars.hpp"

#include <utils/hook.hpp>

namespace renderer
{
	namespace
	{
		utils::hook::detour r_init_draw_method_hook;
		utils::hook::detour r_update_front_end_dvar_options_hook;

		utils::hook::detour db_load_xassets_hook;

		game::dvar_t* r_red_dot_brightness_scale;
		game::dvar_t* r_use_custom_red_dot_brightness;
		float tonemap_highlight_range = 16.f;

		std::unordered_map<std::string, float> tonemap_highlight_range_overrides =
		{
			// all these are 16 by default (except mp_bog & mp_cargoship which have it at 0) which makes red dots hard to see
			{"mp_convoy", 22.f},
			{"mp_backlot", 20.f},
			{"mp_bog", 12.f},
			{"mp_bloc", 30.f},
			{"mp_countdown", 20.f},
			{"mp_crash", 20.f},
			{"mp_creek", 20.f},
			{"mp_crossfire", 26.f},
			{"mp_citystreets", 30.f},
			{"mp_farm", 20.f},
			{"mp_overgrown", 22.f},
			{"mp_pipeline", 26.f},
			{"mp_shipment", 20.f},
			{"mp_showdown", 24.f},
			{"mp_strike", 24.f},
			{"mp_vacant", 20.f},
			{"mp_cargoship", 14.f},
			{"mp_crash_snow", 24.f},
			{"mp_bog_summer", 24.f},
		};

		int get_fullbright_technique()
		{
			switch (dvars::r_fullbright->current.integer)
			{
			case 4:
				return 3;
			case 3:
				return 13;
			case 2:
				return 25;
			default:
				return game::TECHNIQUE_UNLIT;
			}
		}

		void gfxdrawmethod()
		{
			game::gfxDrawMethod->drawScene = game::GFX_DRAW_SCENE_STANDARD;
			game::gfxDrawMethod->baseTechType = dvars::r_fullbright->current.enabled ? get_fullbright_technique() : game::TECHNIQUE_LIT;
			game::gfxDrawMethod->emissiveTechType = dvars::r_fullbright->current.enabled ? get_fullbright_technique() : game::TECHNIQUE_EMISSIVE;
			game::gfxDrawMethod->forceTechType = dvars::r_fullbright->current.enabled ? get_fullbright_technique() : 242;
		}

		void r_init_draw_method_stub()
		{
			gfxdrawmethod();
		}

		bool r_update_front_end_dvar_options_stub()
		{
			if (dvars::r_fullbright->modified)
			{
				game::Dvar_ClearModified(dvars::r_fullbright);
				game::R_SyncRenderThread();

				gfxdrawmethod();
			}

			return r_update_front_end_dvar_options_hook.invoke<bool>();
		}

		void set_tonemap_highlight_range()
		{
			auto* mapname = game::Dvar_FindVar("mapname");
			if (mapname != nullptr && tonemap_highlight_range_overrides.find(mapname->current.string)
				!= tonemap_highlight_range_overrides.end())
			{
				tonemap_highlight_range = tonemap_highlight_range_overrides[mapname->current.string];
			}
			else
			{
				tonemap_highlight_range = 16.f;
			}
		}

		void db_load_xassets_stub(void* a1, void* a2, void* a3)
		{
			set_tonemap_highlight_range();
			db_load_xassets_hook.invoke<void>(a1, a2, a3);
		}

		int get_red_dot_brightness()
		{
			static auto* r_tonemap_highlight_range = game::Dvar_FindVar("r_tonemapHighlightRange");
			auto value = r_tonemap_highlight_range->current.value;
			if (r_use_custom_red_dot_brightness->current.enabled)
			{
				value = tonemap_highlight_range * r_red_dot_brightness_scale->current.value;
			}

			return *reinterpret_cast<int*>(&value);
		}

		void* get_tonemap_highlight_range_stub()
		{
			return utils::hook::assemble([](utils::hook::assembler& a)
			{
				a.push(r9);
				a.push(rax);
				a.pushad64();
				a.call_aligned(get_red_dot_brightness);
				a.mov(qword_ptr(rsp, 0x80), rax);
				a.popad64();
				a.pop(rax);
				a.pop(r9);

				a.mov(dword_ptr(r9, 0x1E84), eax);

				a.jmp(0x1C4136_b);
			});
		}
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

			dvars::r_fullbright = dvars::register_int("r_fullbright", 0, 0, 4, game::DVAR_FLAG_CHEAT, "Toggles rendering without lighting");

			r_init_draw_method_hook.create(SELECT_VALUE(0x5467E0_b, 0x669580_b), &r_init_draw_method_stub);
			r_update_front_end_dvar_options_hook.create(SELECT_VALUE(0x583560_b, 0x6A78C0_b), &r_update_front_end_dvar_options_stub);

			if (game::environment::is_mp())
			{
				// Adjust red dot brightness
				utils::hook::jump(0x1C4125_b, get_tonemap_highlight_range_stub(), true);
				db_load_xassets_hook.create(0x397500_b, db_load_xassets_stub);

				r_red_dot_brightness_scale = dvars::register_float("r_redDotBrightnessScale", 
					1.f, 0.1f, 5.f, game::DVAR_FLAG_SAVED, "Adjust red-dot reticle brightness");

				r_use_custom_red_dot_brightness = dvars::register_bool("r_useCustomRedDotBrightness",
					true, game::DVAR_FLAG_SAVED, "Use custom red-dot brightness values");
			}
		}
	};
}

REGISTER_COMPONENT(renderer::component)
