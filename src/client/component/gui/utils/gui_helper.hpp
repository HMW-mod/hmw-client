#pragma once
#include "game/structs.hpp"
#include "imgui.h"
#include "component/fastfiles.hpp"
#include "game/database.hpp"

namespace gui::helper
{
    namespace gfx {
        game::Material* get_custom_material(const std::string& material_name);
        void draw_custom_material_image(const std::string& material_name, float size = 200.f);
        void draw_material_texture_by_type(const std::string& material_name, unsigned char texture_type, float size, bool show_name);
        void draw_material_texture_by_type_debug(game::Material* material, unsigned char texture_type, float size, bool show_name);
        void draw_material_texture_by_name(const std::string& material_name, const std::string& texture_name, float size = 200.f);
        bool draw_custom_material_image_button(const std::string& material_name, float size);
        std::string get_texture_name_by_type(unsigned char type);
    }
}