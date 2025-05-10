#include "std_include.hpp"
#include "gui_helper.hpp"
#include "game/structs.hpp"
#include "imgui.h"
#include "component/fastfiles.hpp"
#include "game/database.hpp"

namespace gui::helper
{
    namespace gfx
    {

        static game::Material* find_or_cache_material(const std::string& material_name)
        {
            static std::unordered_map<std::string, game::Material*> material_cache;
            if (auto it = material_cache.find(material_name); it != material_cache.end())
            {
                return it->second;
            }

            game::Material* found_material = nullptr;

            const auto assetType = game::ASSET_TYPE_MATERIAL;

            fastfiles::enum_asset_entries(
                assetType,
                [&](const game::XAssetEntry* entry)
                {
                    auto asset_name = std::string(game::DB_GetXAssetName(&entry->asset));
                    if (asset_name == material_name)
                    {
                        auto header = reinterpret_cast<game::Material*>(
                            game::DB_FindXAssetHeader(assetType, asset_name.data(), 0).data
                            );
                        found_material = header;
                    }
                },
                true
            );

            if (found_material)
            {
                material_cache[material_name] = found_material;
            }
            else
            {
                material_cache[material_name] = nullptr;
            }

            return found_material;
        }

        game::Material* get_custom_material(const std::string& material_name)
        {
            return find_or_cache_material(material_name);
        }

        std::string get_texture_name_by_type(unsigned char type)
        {
            static const std::unordered_map<unsigned char, std::string> texture_types = {
                {0x0, "2D"},
                {0x1, "FUNCTION"},
                {0x2, "COLOR_MAP"},
                {0x3, "DETAIL_MAP"},
                {0x4, "UNUSED_2"},
                {0x5, "NORMAL_MAP"},
                {0x6, "UNUSED_3"},
                {0x7, "UNUSED_4"},
                {0x8, "SPECULAR_MAP"},
                {0x9, "UNUSED_5"},
                {0xA, "OCEANFLOW_DISPLACEMENT_MAP"},
                {0xB, "WATER_MAP"},
                {0xC, "OCEAN_DISPLACEMENT_MAP"},
                {0xD, "DISPLACEMENT_MAP"},
                {0xE, "PARALLAX_MAP"}
            };

            auto it = texture_types.find(type);
            return (it != texture_types.end()) ? it->second : "UNKNOWN";
        }

        // [LALISA] - This is just another example on how
		// to get a material texture by name
        // and use it for an ImGui::ImageButton
        bool draw_custom_material_image_button(const std::string& material_name, float size)
        {
            game::Material* material = get_custom_material(material_name);
            if (!material)
            {
                ImGui::Text("Failed to load material: %s", material_name.c_str());
                return false;
            }

            for (int i = 0; i < material->textureCount; i++)
            {
                if (material->textureTable && material->textureTable[i].u.image && material->textureTable[i].u.image->texture.shaderView)
                {
                    const int width = material->textureTable[i].u.image->width;
                    const int height = material->textureTable[i].u.image->height;
                    const float ratio = static_cast<float>(width) / static_cast<float>(height);

                    std::string button_id = "##" + material_name;

                    return ImGui::ImageButton(
                        button_id.c_str(),
                        (ImTextureID)(uintptr_t)(material->textureTable[i].u.image->texture.shaderView),
                        ImVec2(size * ratio, size),
                        ImVec2(0, 0),
                        ImVec2(1, 1),
                        ImVec4(0, 0, 0, 0),
                        ImVec4(1, 1, 1, 1)
                    );
                }
            }

            ImGui::Text("Material found but has no valid texture.");
            return false;
        }

        void draw_custom_material_image(const std::string& material_name, float size)
        {
            game::Material* material = get_custom_material(material_name);
            if (!material)
            {
                ImGui::Text("Failed to load material: %s", material_name.c_str());
                return;
            }

            for (auto i = 0; i < material->textureCount; i++)
            {
                if (material->textureTable && material->textureTable[i].u.image && material->textureTable[i].u.image->texture.shaderView)
                {
                    const auto width = material->textureTable[i].u.image->width;
                    const auto height = material->textureTable[i].u.image->height;
                    const auto ratio = static_cast<float>(width) / static_cast<float>(height);

                    ImGui::Image(reinterpret_cast<ImTextureID>(material->textureTable[i].u.image->texture.shaderView),
                        ImVec2(size * ratio, size));
                    return;
                }
            }

            ImGui::Text("Material found but has no valid texture.");
        }

        void draw_material_texture_by_type(const std::string& material_name, unsigned char texture_type, float size, bool show_name)
        {
            game::Material* material = get_custom_material(material_name);
            if (!material)
            {
                ImGui::Text("Failed to load material: %s", material_name.c_str());
                return;
            }

            // Iterate through all textures in the material to find the specified texture type
            for (auto i = 0; i < material->textureCount; i++)
            {
                if (material->textureTable && material->textureTable[i].u.image && material->textureTable[i].u.image->texture.shaderView)
                {
                    // Check if the texture matches the requested type
                    if (material->textureTable[i].semantic == texture_type)
                    {
                        const auto width = material->textureTable[i].u.image->width;
                        const auto height = material->textureTable[i].u.image->height;
                        const auto ratio = static_cast<float>(width) / static_cast<float>(height);

                        // get the texture name
                        const char* texture_name = material->textureTable[i].u.image->name;

                        // Show The texture name
                        if (show_name && texture_name)
                        {
                            ImGui::Text("Texture: %s", texture_name);
                        }

                        ImGui::Image(reinterpret_cast<ImTextureID>(material->textureTable[i].u.image->texture.shaderView),
                            ImVec2(size * ratio, size));
                        return; // Exit the function after the first match
                    }
                }
            }

            ImGui::Text("No texture of type %s found in material: %s",
                get_texture_name_by_type(texture_type).c_str(),
                material_name.c_str());
        }

        void draw_material_texture_by_type_debug(game::Material* material, unsigned char texture_type, float size, bool show_name)
        {
            if (!material)
            {
                ImGui::Text("Material pointer is null!");
                return;
            }

            for (auto i = 0; i < material->textureCount; i++)
            {
                if (material->textureTable &&
                    material->textureTable[i].u.image &&
                    material->textureTable[i].u.image->texture.shaderView)
                {
                    if (material->textureTable[i].semantic == texture_type)
                    {
                        const auto width = material->textureTable[i].u.image->width;
                        const auto height = material->textureTable[i].u.image->height;
                        const auto ratio = static_cast<float>(width) / static_cast<float>(height);

                        const char* texture_name = material->textureTable[i].u.image->name;
                        if (show_name && texture_name)
                        {
                            ImGui::Text("Texture: %s", texture_name);
                        }

                        ImGui::Image(
                            reinterpret_cast<ImTextureID>(material->textureTable[i].u.image->texture.shaderView),
                            ImVec2(size * ratio, size)
                        );

                        return;
                    }
                }
            }
        }

        void draw_material_texture_by_name(const std::string& material_name, const std::string& texture_name, float size)
        {
            game::Material* material = get_custom_material(material_name);
            if (!material)
            {
                ImGui::Text("Failed to load material: %s", material_name.c_str());
                return;
            }

            // Iterate through all textures in the material to find the specified texture by name
            for (auto i = 0; i < material->textureCount; i++)
            {
                if (material->textureTable && material->textureTable[i].u.image && material->textureTable[i].u.image->texture.shaderView)
                {
                    // Check if the texture matches the requested name
                    if (texture_name == material->textureTable[i].u.image->name)
                    {
                        const auto width = material->textureTable[i].u.image->width;
                        const auto height = material->textureTable[i].u.image->height;
                        const auto ratio = static_cast<float>(width) / static_cast<float>(height);

                        ImGui::Image(reinterpret_cast<ImTextureID>(material->textureTable[i].u.image->texture.shaderView),
                            ImVec2(size * ratio, size));
                        return; // Exit the function after the first match
                    }
                }
            }

            ImGui::Text("No texture with name \"%s\" found in material: %s",
                texture_name.c_str(),
                material_name.c_str());
        }
    }
}
