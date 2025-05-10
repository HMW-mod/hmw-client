#include "std_include.hpp"
#include "style.hpp"
#include "imgui.h"
#include "component/gui/utils/imgui_markdown.h"
#include "component/gui/login_panel/login_panel.hpp"
#include "imgui_internal.h"
#include "../fonts/fa-brands-400.hpp"
#include "../fonts/fa-solid-900.hpp"
#include "../fonts/h1_bank_pre.hpp"

ImFont* standardFont = nullptr;
ImFont* iconFontSolid = nullptr;
ImFont* iconFontBrands = nullptr;

static const ImWchar icon_ranges_solid[] = { 0xf000, 0xf3ff, 0 };
static const ImWchar icon_ranges_brands[] = { 0xf000, 0xf3ff, 0 };

namespace custom_style
{
	// Default window size is 90% of the whole display size!
	void CenterWindowAndScale(const char* window_name, bool* opened, float width_percent, float height_percent, ImGuiWindowFlags flags)
	{
		ImGuiIO& io = ImGui::GetIO();
		ImVec2 window_size = ImVec2(io.DisplaySize.x * width_percent, io.DisplaySize.y * height_percent);
		ImGui::SetNextWindowSize(window_size, ImGuiCond_Always);
		ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));

		if (!ImGui::Begin(window_name, opened, flags | ImGuiWindowFlags_NoCollapse))
		{
			ImGui::End();
			return;
		}
	}

	void LoadingIndicatorCircle(const char* label, const float indicator_radius,
		const ImVec4& main_color, const ImVec4& backdrop_color,
		const int circle_count, const float speed)
	{
		ImGuiWindow* window = ImGui::GetCurrentWindow();
		if (window->SkipItems) {
			return;
		}

		ImGuiContext& g = *GImGui;
		const ImGuiID id = window->GetID(label);

		const ImVec2 pos = window->DC.CursorPos;
		const float circle_radius = indicator_radius / 15.0f;
		const float updated_indicator_radius = indicator_radius - 4.0f * circle_radius;
		const ImRect bb(pos, ImVec2(pos.x + indicator_radius * 2.0f, pos.y + indicator_radius * 2.0f));
		ImGui::ItemSize(bb);
		if (!ImGui::ItemAdd(bb, id)) {
			return;
		}
		const float t = g.Time;
		const auto degree_offset = 2.0f * IM_PI / circle_count;
		for (int i = 0; i < circle_count; ++i) {
			const auto x = updated_indicator_radius * std::sin(degree_offset * i);
			const auto y = updated_indicator_radius * std::cos(degree_offset * i);
			const auto growth = std::max(0.0f, std::sin(t * speed - i * degree_offset));
			ImVec4 color;
			color.x = main_color.x * growth + backdrop_color.x * (1.0f - growth);
			color.y = main_color.y * growth + backdrop_color.y * (1.0f - growth);
			color.z = main_color.z * growth + backdrop_color.z * (1.0f - growth);
			color.w = 1.0f;
			window->DrawList->AddCircleFilled(ImVec2(pos.x + indicator_radius + x,
				pos.y + indicator_radius - y),
				circle_radius + growth * circle_radius, ImGui::GetColorU32(color));
		}
	}

	void load_fonts()
	{
		ImFontAtlas* atlas = ImGui::GetIO().Fonts;

		ImFontConfig config;
		config.MergeMode = false;

		if (h1_bank_pre)
			standardFont = atlas->AddFontFromMemoryCompressedBase85TTF(h1_bank_pre, 16.0f, &config);

		config.MergeMode = true;

		if (fa_solid_900)
			atlas->AddFontFromMemoryCompressedBase85TTF(fa_solid_900, 16.0f, &config, icon_ranges_solid);

		if (fa_brands_400)
			atlas->AddFontFromMemoryCompressedBase85TTF(fa_brands_400, 16.0f, &config, icon_ranges_brands);
	}


	void draw_dimmed_background()
	{
		ImGuiIO& io = ImGui::GetIO();

		// we can maybe blurr background in the future?
		ImDrawList* draw_list = ImGui::GetBackgroundDrawList();
		draw_list->AddRectFilled(
			ImVec2(0, 0),
			io.DisplaySize,
			IM_COL32(5, 5, 5, 200)
		);
	}

	bool ToggleSwitch(const char* label, bool* v, Alignment alignment)
	{
		ImGui::PushID(label);
		ImGui::BeginGroup();

		const float height = 20.0f;
		const float width = 40.0f;
		const float radius = height * 0.5f;

		ImVec2 labelSize = ImGui::CalcTextSize(label);

		float availableWidth = ImGui::GetContentRegionAvail().x;
		float toggleWidth = width + ImGui::GetStyle().ItemSpacing.x + labelSize.x;

		float offsetX = 0.0f;
		if (alignment == Alignment::Center)
		{
			offsetX = (availableWidth - toggleWidth) / 2.0f;
		}
		else if (alignment == Alignment::Right)
		{
			offsetX = availableWidth - toggleWidth;
		}

		if (offsetX > 0.0f)
		{
			ImGui::SetCursorPosX(ImGui::GetCursorPosX() + offsetX);
		}

		ImVec2 pos = ImGui::GetCursorScreenPos();
		ImDrawList* draw_list = ImGui::GetWindowDrawList();

		ImGui::InvisibleButton(label, ImVec2(width, height));
		bool clicked = ImGui::IsItemClicked();

		if (clicked)
		{
			*v = !*v;
		}

		ImU32 col_bg = *v ? IM_COL32(0, 200, 0, 255) : IM_COL32(200, 0, 0, 255);
		draw_list->AddRectFilled(pos, ImVec2(pos.x + width, pos.y + height), col_bg, radius);

		ImU32 col_knob = IM_COL32(255, 255, 255, 255);
		float knob_x = *v ? (pos.x + width - radius) : (pos.x + radius);
		draw_list->AddCircleFilled(ImVec2(knob_x, pos.y + radius), radius - 2.0f, col_knob);

		ImGui::SameLine();
		ImGui::TextUnformatted(label);

		ImGui::EndGroup();
		ImGui::PopID();

		return clicked;
	}

	bool CustomCheckbox(const char* label, bool* v, Alignment alignment)
	{
		ImGui::PushID(label);
		ImGui::BeginGroup();

		const float size = ImGui::GetFontSize();
		ImVec2 labelSize = ImGui::CalcTextSize(label);

		float availableWidth = ImGui::GetContentRegionAvail().x;
		float buttonWidth = size + ImGui::GetStyle().ItemSpacing.x + labelSize.x;

		float offsetX = 0.0f;
		if (alignment == Alignment::Center)
		{
			offsetX = (availableWidth - buttonWidth) / 2.0f;
		}
		else if (alignment == Alignment::Right)
		{
			offsetX = availableWidth - buttonWidth;
		}

		if (offsetX > 0.0f)
		{
			ImGui::SetCursorPosX(ImGui::GetCursorPosX() + offsetX);
		}

		ImVec2 pos = ImGui::GetCursorScreenPos();
		ImDrawList* draw_list = ImGui::GetWindowDrawList();

		ImGui::InvisibleButton(label, ImVec2(size, size));
		bool clicked = ImGui::IsItemClicked();

		if (clicked)
		{
			*v = !*v;
		}

		ImU32 col_bg = *v ? IM_COL32(0, 200, 0, 255) : IM_COL32(50, 50, 50, 255);
		draw_list->AddRectFilled(pos, ImVec2(pos.x + size, pos.y + size), col_bg, 4.0f);

		if (*v)
		{
			ImU32 col_check = IM_COL32(255, 255, 255, 255);
			float pad = size * 0.25f;
			draw_list->AddLine(
				ImVec2(pos.x + pad, pos.y + size * 0.5f),
				ImVec2(pos.x + size * 0.5f, pos.y + size - pad),
				col_check, 2.0f);
			draw_list->AddLine(
				ImVec2(pos.x + size * 0.5f, pos.y + size - pad),
				ImVec2(pos.x + size - pad, pos.y + pad),
				col_check, 2.0f);
		}

		ImGui::SameLine();
		ImGui::TextUnformatted(label);

		ImGui::EndGroup();
		ImGui::PopID();

		return clicked;
	}

	void PushLuiStyle()
	{
		// Push Style Variables
		ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 6.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 6.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_PopupRounding, 6.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarRounding, 6.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_GrabRounding, 6.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_TabRounding, 6.0f);

		// Push Style Colors
		ImGui::PushStyleColor(ImGuiCol_TitleBg, ImVec4(0.15f, 0.10f, 0.05f, 1.00f)); // Default background for the title bar
		ImGui::PushStyleColor(ImGuiCol_TitleBgActive, ImVec4(0.30f, 0.25f, 0.15f, 1.00f)); // Active background for the title bar
		ImGui::PushStyleColor(ImGuiCol_TitleBgCollapsed, ImVec4(0.10f, 0.08f, 0.05f, 1.00f)); // Collapsed background for the title bar
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.90f, 0.85f, 0.60f, 1.00f)); // Golden text
		ImGui::PushStyleColor(ImGuiCol_TextDisabled, ImVec4(0.50f, 0.45f, 0.30f, 1.00f)); // Dimmed golden text
		ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.10f, 0.10f, 0.05f, 1.00f)); // Very dark brown/yellow background
		ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.12f, 0.12f, 0.07f, 1.00f)); // Slightly lighter background
		ImGui::PushStyleColor(ImGuiCol_PopupBg, ImVec4(0.15f, 0.15f, 0.10f, 0.95f)); // Dark popup background
		ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.50f, 0.40f, 0.20f, 0.70f)); // Subtle golden border color
		ImGui::PushStyleColor(ImGuiCol_BorderShadow, ImVec4(0.00f, 0.00f, 0.00f, 0.00f)); // No shadow
		ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.20f, 0.18f, 0.10f, 1.00f)); // Dark yellow for frames
		ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.30f, 0.25f, 0.15f, 1.00f)); // Lighter yellow when hovered
		ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0.50f, 0.40f, 0.20f, 1.00f)); // Gold for active frames
		ImGui::PushStyleColor(ImGuiCol_TitleBg, ImVec4(0.12f, 0.10f, 0.05f, 1.00f)); // Dark brown for the title background
		ImGui::PushStyleColor(ImGuiCol_TitleBgActive, ImVec4(0.30f, 0.25f, 0.10f, 1.00f)); // Golden title background when active
		ImGui::PushStyleColor(ImGuiCol_TitleBgCollapsed, ImVec4(0.10f, 0.08f, 0.05f, 1.00f)); // Subtle title when collapsed
		ImGui::PushStyleColor(ImGuiCol_MenuBarBg, ImVec4(0.15f, 0.12f, 0.05f, 1.00f)); // Dark golden menu bar
		ImGui::PushStyleColor(ImGuiCol_ScrollbarBg, ImVec4(0.10f, 0.10f, 0.05f, 1.00f)); // Background for scrollbar
		ImGui::PushStyleColor(ImGuiCol_ScrollbarGrab, ImVec4(0.40f, 0.35f, 0.20f, 1.00f)); // Golden accents for scrollbar
		ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabHovered, ImVec4(0.50f, 0.45f, 0.25f, 1.00f)); // Brighter scrollbar when hovered
		ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabActive, ImVec4(0.60f, 0.50f, 0.30f, 1.00f)); // Active gold for scrollbar
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.25f, 0.20f, 0.10f, 1.00f)); // Dark golden buttons
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.40f, 0.35f, 0.20f, 1.00f)); // Bright buttons when hovered
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.50f, 0.45f, 0.25f, 1.00f)); // Golden buttons when active
		ImGui::PushStyleColor(ImGuiCol_CheckMark, ImVec4(0.80f, 0.70f, 0.40f, 1.00f)); // Bright gold for check marks
		ImGui::PushStyleColor(ImGuiCol_SliderGrab, ImVec4(0.70f, 0.60f, 0.30f, 1.00f)); // Yellow-golden slider
		ImGui::PushStyleColor(ImGuiCol_SliderGrabActive, ImVec4(0.80f, 0.70f, 0.40f, 1.00f)); // Brighter slider when active
		ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.20f, 0.18f, 0.10f, 1.00f)); // Subtle header color
		ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.30f, 0.25f, 0.15f, 1.00f)); // Brighter yellow when hovered
		ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0.40f, 0.35f, 0.20f, 1.00f)); // Strong gold when active
		ImGui::PushStyleColor(ImGuiCol_Tab, ImVec4(0.15f, 0.12f, 0.05f, 1.00f)); // Dark yellow tabs
		ImGui::PushStyleColor(ImGuiCol_TabHovered, ImVec4(0.30f, 0.25f, 0.15f, 1.00f)); // Bright tabs when hovered
		ImGui::PushStyleColor(ImGuiCol_TabActive, ImVec4(0.40f, 0.35f, 0.20f, 1.00f)); // Golden tabs when active
		ImGui::PushStyleColor(ImGuiCol_TabUnfocused, ImVec4(0.10f, 0.08f, 0.05f, 1.00f)); // Subtle dark tabs
		ImGui::PushStyleColor(ImGuiCol_TabUnfocusedActive, ImVec4(0.20f, 0.18f, 0.10f, 1.00f)); // Bright active tabs
		ImGui::PushStyleColor(ImGuiCol_PlotLines, ImVec4(0.70f, 0.60f, 0.30f, 1.00f)); // Lines in graphs
		ImGui::PushStyleColor(ImGuiCol_PlotLinesHovered, ImVec4(0.80f, 0.70f, 0.40f, 1.00f)); // Line color when hovered
		ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, ImVec4(0.30f, 0.25f, 0.15f, 1.00f)); // Highlight color for selected text
		ImGui::PushStyleColor(ImGuiCol_ModalWindowDimBg, ImVec4(0.00f, 0.00f, 0.00f, 0.80f)); // Transparent black background
		ImGui::PushStyleColor(ImGuiCol_TableRowBg, ImVec4(0.15f, 0.12f, 0.05f, 1.00f)); // Dark yellow table rows (even rows)
		ImGui::PushStyleColor(ImGuiCol_TableRowBgAlt, ImVec4(0.10f, 0.08f, 0.05f, 1.00f)); // Subtle dark table rows (uneven rows)
	}

	void PopLuiStyle()
	{
		// MAKE SURE U POP THE SAME AMOUNT OF PUSHES
		ImGui::PopStyleVar(7);
		ImGui::PopStyleColor(41);
	}

	void PushTestStyle()
	{
		// Push Style Variables
		ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 6.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 6.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_PopupRounding, 6.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarRounding, 6.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_GrabRounding, 6.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_TabRounding, 6.0f);

		// Push Style Colors
		ImGui::PushStyleColor(ImGuiCol_TitleBg, ImVec4(0.15f, 0.10f, 0.05f, 1.00f)); // Default background for the title bar
		ImGui::PushStyleColor(ImGuiCol_TitleBgActive, ImVec4(0.30f, 0.25f, 0.15f, 1.00f)); // Active background for the title bar
		ImGui::PushStyleColor(ImGuiCol_TitleBgCollapsed, ImVec4(0.10f, 0.08f, 0.05f, 1.00f)); // Collapsed background for the title bar
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.00f)); // Golden text
		ImGui::PushStyleColor(ImGuiCol_TextDisabled, ImVec4(0.50f, 0.45f, 0.30f, 1.00f)); // Dimmed golden text
		ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.0f, 0.0f, 0.0f, 0.45f)); // Very dark brown/yellow background
		ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.12f, 0.12f, 0.07f, 1.00f)); // Slightly lighter background
		ImGui::PushStyleColor(ImGuiCol_PopupBg, ImVec4(0.15f, 0.15f, 0.10f, 0.95f)); // Dark popup background
		ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.8549, 0.8039, 0.3451, 0.20));// Subtle golden border color
		ImGui::PushStyleColor(ImGuiCol_BorderShadow, ImVec4(0.00f, 0.00f, 0.00f, 0.00f)); // No shadow
		ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.20f, 0.18f, 0.10f, 1.00f)); // Dark yellow for frames
		ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.30f, 0.25f, 0.15f, 1.00f)); // Lighter yellow when hovered
		ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0.50f, 0.40f, 0.20f, 1.00f)); // Gold for active frames
		ImGui::PushStyleColor(ImGuiCol_TitleBg, ImVec4(0.12f, 0.10f, 0.05f, 1.00f)); // Dark brown for the title background
		ImGui::PushStyleColor(ImGuiCol_TitleBgActive, ImVec4(0.30f, 0.25f, 0.10f, 1.00f)); // Golden title background when active
		ImGui::PushStyleColor(ImGuiCol_TitleBgCollapsed, ImVec4(0.10f, 0.08f, 0.05f, 1.00f)); // Subtle title when collapsed
		ImGui::PushStyleColor(ImGuiCol_MenuBarBg, ImVec4(0.15f, 0.12f, 0.05f, 1.00f)); // Dark golden menu bar
		ImGui::PushStyleColor(ImGuiCol_ScrollbarBg, ImVec4(0.10f, 0.10f, 0.05f, 1.00f)); // Background for scrollbar
		ImGui::PushStyleColor(ImGuiCol_ScrollbarGrab, ImVec4(0.40f, 0.35f, 0.20f, 1.00f)); // Golden accents for scrollbar
		ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabHovered, ImVec4(0.50f, 0.45f, 0.25f, 1.00f)); // Brighter scrollbar when hovered
		ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabActive, ImVec4(0.60f, 0.50f, 0.30f, 1.00f)); // Active gold for scrollbar
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.25f, 0.20f, 0.10f, 1.00f)); // Dark golden buttons
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.0f, 1.0f, 1.0f, 0.70f)); // Bright buttons when hovered
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.50f, 0.45f, 0.25f, 1.00f)); // Golden buttons when active
		ImGui::PushStyleColor(ImGuiCol_CheckMark, ImVec4(0.80f, 0.70f, 0.40f, 1.00f)); // Bright gold for check marks
		ImGui::PushStyleColor(ImGuiCol_SliderGrab, ImVec4(0.70f, 0.60f, 0.30f, 1.00f)); // Yellow-golden slider
		ImGui::PushStyleColor(ImGuiCol_SliderGrabActive, ImVec4(0.80f, 0.70f, 0.40f, 1.00f)); // Brighter slider when active
		ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.20f, 0.18f, 0.10f, 1.00f)); // Subtle header color
		ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.30f, 0.25f, 0.15f, 1.00f)); // Brighter yellow when hovered
		ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0.40f, 0.35f, 0.20f, 1.00f)); // Strong gold when active
		ImGui::PushStyleColor(ImGuiCol_Tab, ImVec4(0.15f, 0.12f, 0.05f, 1.00f)); // Dark yellow tabs
		ImGui::PushStyleColor(ImGuiCol_TabHovered, ImVec4(0.30f, 0.25f, 0.15f, 1.00f)); // Bright tabs when hovered
		ImGui::PushStyleColor(ImGuiCol_TabActive, ImVec4(0.40f, 0.35f, 0.20f, 1.00f)); // Golden tabs when active
		ImGui::PushStyleColor(ImGuiCol_TabUnfocused, ImVec4(0.10f, 0.08f, 0.05f, 1.00f)); // Subtle dark tabs
		ImGui::PushStyleColor(ImGuiCol_TabUnfocusedActive, ImVec4(0.20f, 0.18f, 0.10f, 1.00f)); // Bright active tabs
		ImGui::PushStyleColor(ImGuiCol_PlotLines, ImVec4(0.70f, 0.60f, 0.30f, 1.00f)); // Lines in graphs
		ImGui::PushStyleColor(ImGuiCol_PlotLinesHovered, ImVec4(0.80f, 0.70f, 0.40f, 1.00f)); // Line color when hovered
		ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, ImVec4(0.30f, 0.25f, 0.15f, 1.00f)); // Highlight color for selected text
		ImGui::PushStyleColor(ImGuiCol_ModalWindowDimBg, ImVec4(0.00f, 0.00f, 0.00f, 0.80f)); // Transparent black background
		ImGui::PushStyleColor(ImGuiCol_TableRowBg, ImVec4(0.15f, 0.12f, 0.05f, 1.00f)); // Dark yellow table rows (even rows)
		ImGui::PushStyleColor(ImGuiCol_TableRowBgAlt, ImVec4(0.10f, 0.08f, 0.05f, 1.00f)); // Subtle dark table rows (uneven rows)
	}

	void PopTestStyle()
	{
		// MAKE SURE U POP THE SAME AMOUNT OF PUSHES
		ImGui::PopStyleVar(7);
		ImGui::PopStyleColor(41);
	}

	void PushCustomButtonStyle()
	{
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.1f, 0.1f, 0.1f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.7f, 0.6f, 0.0f, 0.50f));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.8f, 0.8f, 0.2f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(1.0f, 1.0f, 1.0f, 0.4f));
		ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.5f);
		ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 5.0f);
	}

	void PopCustomButtonStyle()
	{
		ImGui::PopStyleVar(2);
		ImGui::PopStyleColor(4);
	}

	bool custom_style::CustomButton(const char* label, float buttonWidth, float buttonHeight)
	{
		if (buttonWidth == 0.0f)
		{
			buttonWidth = ImGui::GetContentRegionAvail().x;
		}
		return ImGui::Button(label, ImVec2(buttonWidth, buttonHeight));
	}

	void PushLoginPanelTheme()
	{
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.4f, 0.35f, 0.1f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.5f, 0.45f, 0.2f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.6f, 0.5f, 0.2f, 1.0f));
	}

	void PopLoginPanelTheme()
	{
		ImGui::PopStyleColor(3);
	}

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

	void custom_style::DrawMarkdownButton(
		const char* markdownText,
		const char* url,
		float window_width,
		std::function<void()> onClick,
		ImVec4 normalColor,
		ImVec4 hoverColor
	)
	{
		float markdownWidth = ImGui::CalcTextSize(markdownText).x;
		ImGui::SetCursorPosX((window_width - markdownWidth) * 0.5f);

		ImVec2 cursorPos = ImGui::GetCursorScreenPos();
		ImVec2 textEnd = ImVec2(cursorPos.x + markdownWidth, cursorPos.y + ImGui::GetTextLineHeight());
		bool isHovered = ImGui::IsMouseHoveringRect(cursorPos, textEnd);

		if (isHovered)
		{
			ImGui::PushStyleColor(ImGuiCol_Text, hoverColor);
			ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
		}
		else
		{
			ImGui::PushStyleColor(ImGuiCol_Text, normalColor);
		}

		ImGui::Markdown(markdownText, strlen(markdownText), login_panel::GetMarkdownConfig());
		ImGui::PopStyleColor();

		if (isHovered && ImGui::IsMouseClicked(0))
		{
			if (onClick)
			{
				onClick();
			}
			if (url)
			{
				ShellExecute(0, "open", url, 0, 0, SW_SHOWNORMAL);
			}
		}
	}
}