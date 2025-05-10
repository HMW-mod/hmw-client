#include <std_include.hpp>
#ifdef DEBUG
#include "loader/component_loader.hpp"
#include "component/scheduler.hpp"
#include "game/game.hpp"
#include "game/dvars.hpp"
#include "gui.hpp"
#include <utils/string.hpp>
#include <utils/hook.hpp>
#include <json.hpp>
#include "component/gui/utils/gui_helper.hpp"
#include "component/gui/utils/ImGuiNotify.hpp"

#pragma region macros

#define ADD_FLOAT(__string__, __dvar__) \
		ADD_FLOAT_STEP(__string__, __dvar__, 0.01f);

#define ADD_FLOAT_STEP(__string__, __dvar__, __step__ ) \
		ImGui::DragFloat(__string__, &__dvar__->current.value, __step__, __dvar__->domain.value.min, __dvar__->domain.value.max);

#define ADD_COLOUR(__string__, __dvar__) \
		ImGui::ColorEdit3(__string__, __dvar__->current.vector);

#define ADD_INT(__string__, __dvar__) \
		ImGui::DragInt(__string__, &__dvar__->current.integer, 1.0f, __dvar__->domain.integer.min, __dvar__->domain.integer.max);

#define ADD_BOOL(__string__, __dvar__) \
		if (ImGui::Checkbox(__string__, &__dvar__->current.enabled)) { \
			const char* state = __dvar__->current.enabled ? "Enabled" : "Disabled"; \
			ImGui::InsertNotification({ ImGuiToastType::Debug, 3000, utils::string::va("%s %s", state, __string__) }); \
		}

#pragma endregion

namespace gui::dvar_editor
{
	namespace
	{
		static game::dvar_t* sv_cheats = nullptr;
		static game::dvar_t* cg_drawmaterial = nullptr;

		void init_dvars()
		{
			sv_cheats = game::Dvar_FindVar("sv_cheats");
			cg_drawmaterial = game::Dvar_FindVar("cg_drawmaterial");
		}
	}

	void render_window()
	{
		init_dvars();
		static auto* enabled = &gui::enabled_menus["dvar_editor"];
		ImGui::SetNextWindowSize(ImVec2(300, 400), ImGuiCond_FirstUseEver);
		ImGui::Begin("Dvar Editor", enabled);

		ADD_BOOL("SV Cheats", sv_cheats);
		ADD_BOOL("Draw Material", cg_drawmaterial);

		ImGui::End();
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

			scheduler::once([]()
				{
					init_dvars();
				});
			gui::register_menu("dvar_editor", "Dvar Editor", render_window);
		}
	};
}

REGISTER_COMPONENT(gui::dvar_editor::component)
#endif
