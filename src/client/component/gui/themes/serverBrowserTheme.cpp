#include "std_include.hpp"
#include "serverBrowserTheme.hpp"

namespace ImGuiThemes
{
    void PushBrowserTheme()
    {
        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 1.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8, 8));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowMinSize, ImVec2(32, 32));
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4, 3));
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8, 4));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemInnerSpacing, ImVec2(4, 4));
        ImGui::PushStyleVar(ImGuiStyleVar_IndentSpacing, 21.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarSize, 14.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarRounding, 9.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_GrabMinSize, 10.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_GrabRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_TabRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarRounding, 0.0f);


        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.90f, 0.90f, 0.90f, 1.00f));
        ImGui::PushStyleColor(ImGuiCol_TextDisabled, ImVec4(0.50f, 0.50f, 0.50f, 1.00f));
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.10f, 0.10f, 0.10f, 0.75f));
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.00f, 0.00f, 0.00f, 0.00f));
        ImGui::PushStyleColor(ImGuiCol_PopupBg, ImVec4(0.10f, 0.10f, 0.10f, 0.90f));
        ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_BorderShadow, ImVec4(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.20f, 0.20f, 0.20f, 0.30f));
        ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.25f, 0.25f, 0.25f, 0.30f));
        ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0.30f, 0.30f, 0.30f, 0.90f));
        ImGui::PushStyleColor(ImGuiCol_TitleBg, ImVec4(0.16f, 0.16f, 0.16f, 0.90f));
        ImGui::PushStyleColor(ImGuiCol_TitleBgActive, ImVec4(0.16f, 0.16f, 0.16f, 1.00f));
        ImGui::PushStyleColor(ImGuiCol_TitleBgCollapsed, ImVec4(0.05f, 0.05f, 0.05f, 0.30f));
        ImGui::PushStyleColor(ImGuiCol_MenuBarBg, ImVec4(0.14f, 0.14f, 0.14f, 0.90f));
        ImGui::PushStyleColor(ImGuiCol_ScrollbarBg, ImVec4(0.10f, 0.10f, 0.10f, 0.90f));
        ImGui::PushStyleColor(ImGuiCol_ScrollbarGrab, ImVec4(0.30f, 0.30f, 0.30f, 0.90f));
        ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabHovered, ImVec4(0.40f, 0.40f, 0.40f, 0.90f));
        ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabActive, ImVec4(0.50f, 0.50f, 0.50f, 0.90f));
        ImGui::PushStyleColor(ImGuiCol_CheckMark, ImVec4(1.00f, 1.00f, 1.00f, 1.00f));
        ImGui::PushStyleColor(ImGuiCol_SliderGrab, ImVec4(0.30f, 0.30f, 0.30f, 0.90f));
        ImGui::PushStyleColor(ImGuiCol_SliderGrabActive, ImVec4(0.40f, 0.40f, 0.40f, 0.90f));
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.15f, 0.15f, 0.15f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.25f, 0.25f, 0.25f, 1.00f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.35f, 0.35f, 0.35f, 1.00f));
        ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.25f, 0.25f, 0.25f, 0.30f));
        ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.35f, 0.35f, 0.35f, 0.30f));
        ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0.45f, 0.45f, 0.45f, 0.90f));
        ImGui::PushStyleColor(ImGuiCol_Separator, ImVec4(0.50f, 0.50f, 0.50f, 0.60f));
        ImGui::PushStyleColor(ImGuiCol_SeparatorHovered, ImVec4(0.60f, 0.60f, 0.60f, 0.90f));
        ImGui::PushStyleColor(ImGuiCol_SeparatorActive, ImVec4(0.70f, 0.70f, 0.70f, 1.00f));
        ImGui::PushStyleColor(ImGuiCol_ResizeGrip, ImVec4(0.20f, 0.20f, 0.20f, 0.30f));
        ImGui::PushStyleColor(ImGuiCol_ResizeGripHovered, ImVec4(0.30f, 0.30f, 0.30f, 0.90f));
        ImGui::PushStyleColor(ImGuiCol_ResizeGripActive, ImVec4(0.40f, 0.40f, 0.40f, 0.90f));
        ImGui::PushStyleColor(ImGuiCol_Tab, ImVec4(0.15f, 0.15f, 0.15f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_TabHovered, ImVec4(0.30f, 0.30f, 0.30f, 0.80f));
        ImGui::PushStyleColor(ImGuiCol_TabActive, ImVec4(0.35f, 0.35f, 0.35f, 0.80f));
        ImGui::PushStyleColor(ImGuiCol_TabUnfocused, ImVec4(0.10f, 0.10f, 0.10f, 0.80f));
        ImGui::PushStyleColor(ImGuiCol_TabUnfocusedActive, ImVec4(0.20f, 0.20f, 0.20f, 0.80f));
        ImGui::PushStyleColor(ImGuiCol_DockingPreview, ImVec4(0.20f, 0.20f, 0.20f, 0.30f));
        ImGui::PushStyleColor(ImGuiCol_DockingEmptyBg, ImVec4(0.00f, 0.00f, 0.00f, 1.00f));
        ImGui::PushStyleColor(ImGuiCol_PlotLines, ImVec4(0.61f, 0.61f, 0.61f, 1.00f));
        ImGui::PushStyleColor(ImGuiCol_PlotLinesHovered, ImVec4(1.00f, 0.43f, 0.35f, 1.00f));
        ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(0.90f, 0.70f, 0.00f, 1.00f));
        ImGui::PushStyleColor(ImGuiCol_PlotHistogramHovered, ImVec4(1.00f, 0.60f, 0.00f, 1.00f));
        ImGui::PushStyleColor(ImGuiCol_TableHeaderBg, ImVec4(0.16f, 0.16f, 0.16f, 0.90f));
        ImGui::PushStyleColor(ImGuiCol_TableBorderLight, ImVec4(0.27f, 0.27f, 0.27f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_TableBorderStrong, ImVec4(0.00f, 0.00f, 0.00f, 0.00f));
        ImGui::PushStyleColor(ImGuiCol_TableRowBg, ImVec4(0.00f, 0.00f, 0.00f, 0.00f));
        ImGui::PushStyleColor(ImGuiCol_TableRowBgAlt, ImVec4(0.00f, 0.00f, 0.00f, 0.00f));
        ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, ImVec4(0.26f, 0.59f, 0.98f, 0.35f));
        ImGui::PushStyleColor(ImGuiCol_DragDropTarget, ImVec4(1.00f, 1.00f, 0.00f, 0.90f));
        ImGui::PushStyleColor(ImGuiCol_NavHighlight, ImVec4(0.26f, 0.59f, 0.98f, 0.80f));
        ImGui::PushStyleColor(ImGuiCol_NavWindowingHighlight, ImVec4(1.00f, 1.00f, 1.00f, 0.30f));
        ImGui::PushStyleColor(ImGuiCol_NavWindowingDimBg, ImVec4(0.80f, 0.80f, 0.80f, 0.20f));
        ImGui::PushStyleColor(ImGuiCol_ModalWindowDimBg, ImVec4(0.20f, 0.20f, 0.20f, 0.30f));
    }

    void PopBrowserTheme()
    {
        ImGui::PopStyleColor(55);
        ImGui::PopStyleVar(17);
    }




    void PushBrowserInfoTheme()
    {
        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 1.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8, 8));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowMinSize, ImVec2(32, 32));
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4, 3));
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8, 4));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemInnerSpacing, ImVec2(4, 4));
        ImGui::PushStyleVar(ImGuiStyleVar_IndentSpacing, 21.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarSize, 14.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarRounding, 9.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_GrabMinSize, 10.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_GrabRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_TabRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarRounding, 0.0f);


        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.90f, 0.90f, 0.90f, 1.00f));
        ImGui::PushStyleColor(ImGuiCol_TextDisabled, ImVec4(0.50f, 0.50f, 0.50f, 1.00f));
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.10f, 0.10f, 0.10f, 1.00f));
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.00f, 0.00f, 0.00f, 1.00f));
        ImGui::PushStyleColor(ImGuiCol_PopupBg, ImVec4(0.10f, 0.10f, 0.10f, 1.00f));
        ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_BorderShadow, ImVec4(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.20f, 0.20f, 0.20f, 1.00f));
        ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.25f, 0.25f, 0.25f, 1.00f));
        ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0.30f, 0.30f, 0.30f, 1.00f));
        ImGui::PushStyleColor(ImGuiCol_TitleBg, ImVec4(0.16f, 0.16f, 0.16f, 1.00f));
        ImGui::PushStyleColor(ImGuiCol_TitleBgActive, ImVec4(0.16f, 0.16f, 0.16f, 1.00f));
        ImGui::PushStyleColor(ImGuiCol_TitleBgCollapsed, ImVec4(0.05f, 0.05f, 0.05f, 1.00f));
        ImGui::PushStyleColor(ImGuiCol_MenuBarBg, ImVec4(0.14f, 0.14f, 0.14f, 1.00f));
        ImGui::PushStyleColor(ImGuiCol_ScrollbarBg, ImVec4(0.10f, 0.10f, 0.10f, 0.90f));
        ImGui::PushStyleColor(ImGuiCol_ScrollbarGrab, ImVec4(0.30f, 0.30f, 0.30f, 0.90f));
        ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabHovered, ImVec4(0.40f, 0.40f, 0.40f, 0.90f));
        ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabActive, ImVec4(0.50f, 0.50f, 0.50f, 0.90f));
        ImGui::PushStyleColor(ImGuiCol_CheckMark, ImVec4(1.00f, 1.00f, 1.00f, 1.00f));
        ImGui::PushStyleColor(ImGuiCol_SliderGrab, ImVec4(0.30f, 0.30f, 0.30f, 0.90f));
        ImGui::PushStyleColor(ImGuiCol_SliderGrabActive, ImVec4(0.40f, 0.40f, 0.40f, 0.90f));
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.25f, 0.25f, 0.25f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.35f, 0.35f, 0.35f, 1.00f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.45f, 0.45f, 0.45f, 1.00f));
        ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.25f, 0.25f, 0.25f, 0.30f));
        ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.35f, 0.35f, 0.35f, 0.30f));
        ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0.45f, 0.45f, 0.45f, 0.90f));
        ImGui::PushStyleColor(ImGuiCol_Separator, ImVec4(0.50f, 0.50f, 0.50f, 0.60f));
        ImGui::PushStyleColor(ImGuiCol_SeparatorHovered, ImVec4(0.60f, 0.60f, 0.60f, 0.90f));
        ImGui::PushStyleColor(ImGuiCol_SeparatorActive, ImVec4(0.70f, 0.70f, 0.70f, 1.00f));
        ImGui::PushStyleColor(ImGuiCol_ResizeGrip, ImVec4(0.20f, 0.20f, 0.20f, 0.30f));
        ImGui::PushStyleColor(ImGuiCol_ResizeGripHovered, ImVec4(0.30f, 0.30f, 0.30f, 0.90f));
        ImGui::PushStyleColor(ImGuiCol_ResizeGripActive, ImVec4(0.40f, 0.40f, 0.40f, 0.90f));
        ImGui::PushStyleColor(ImGuiCol_Tab, ImVec4(0.15f, 0.15f, 0.15f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_TabHovered, ImVec4(0.30f, 0.30f, 0.30f, 0.80f));
        ImGui::PushStyleColor(ImGuiCol_TabActive, ImVec4(0.35f, 0.35f, 0.35f, 0.80f));
        ImGui::PushStyleColor(ImGuiCol_TabUnfocused, ImVec4(0.10f, 0.10f, 0.10f, 0.80f));
        ImGui::PushStyleColor(ImGuiCol_TabUnfocusedActive, ImVec4(0.20f, 0.20f, 0.20f, 0.80f));
        ImGui::PushStyleColor(ImGuiCol_DockingPreview, ImVec4(0.20f, 0.20f, 0.20f, 0.30f));
        ImGui::PushStyleColor(ImGuiCol_DockingEmptyBg, ImVec4(0.00f, 0.00f, 0.00f, 1.00f));
        ImGui::PushStyleColor(ImGuiCol_PlotLines, ImVec4(0.61f, 0.61f, 0.61f, 1.00f));
        ImGui::PushStyleColor(ImGuiCol_PlotLinesHovered, ImVec4(1.00f, 0.43f, 0.35f, 1.00f));
        ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(0.90f, 0.70f, 0.00f, 1.00f));
        ImGui::PushStyleColor(ImGuiCol_PlotHistogramHovered, ImVec4(1.00f, 0.60f, 0.00f, 1.00f));
        ImGui::PushStyleColor(ImGuiCol_TableHeaderBg, ImVec4(0.16f, 0.16f, 0.16f, 0.90f));
        ImGui::PushStyleColor(ImGuiCol_TableBorderLight, ImVec4(0.27f, 0.27f, 0.27f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_TableBorderStrong, ImVec4(0.00f, 0.00f, 0.00f, 0.00f));
        ImGui::PushStyleColor(ImGuiCol_TableRowBg, ImVec4(0.00f, 0.00f, 0.00f, 0.00f));
        ImGui::PushStyleColor(ImGuiCol_TableRowBgAlt, ImVec4(0.00f, 0.00f, 0.00f, 0.00f));
        ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, ImVec4(0.26f, 0.59f, 0.98f, 0.35f));
        ImGui::PushStyleColor(ImGuiCol_DragDropTarget, ImVec4(1.00f, 1.00f, 0.00f, 0.90f));
        ImGui::PushStyleColor(ImGuiCol_NavHighlight, ImVec4(0.26f, 0.59f, 0.98f, 0.80f));
        ImGui::PushStyleColor(ImGuiCol_NavWindowingHighlight, ImVec4(1.00f, 1.00f, 1.00f, 0.30f));
        ImGui::PushStyleColor(ImGuiCol_NavWindowingDimBg, ImVec4(0.80f, 0.80f, 0.80f, 0.20f));
        ImGui::PushStyleColor(ImGuiCol_ModalWindowDimBg, ImVec4(0.20f, 0.20f, 0.20f, 0.30f));
    }

    void PopBrowserInfoTheme()
    {
        ImGui::PopStyleColor(55);
        ImGui::PopStyleVar(17);
    }
}
