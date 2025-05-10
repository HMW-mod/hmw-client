#ifndef STYLE_HPP
#define STYLE_HPP

#include "imgui.h"
#include "component/gui/utils/imgui_markdown.h"
#include <functional>

extern ImFont* standardFont;
extern ImFont* iconFontSolid;
extern ImFont* iconFontBrands;

namespace custom_style
{
	void CenterWindowAndScale(const char* window_name, bool* opened, float width_percent = 0.9f, float height_percent = 0.9f, ImGuiWindowFlags flags = 0);

	void LoadingIndicatorCircle(const char* label, const float indicator_radius,
						const ImVec4& main_color, const ImVec4& backdrop_color,
						const int circle_count, const float speed);
	void load_fonts();
	void draw_dimmed_background();
	void PushLuiStyle();
	void PopLuiStyle();
	void PushTestStyle();
	void PopTestStyle();
	void PushLoginPanelTheme();
	void PopLoginPanelTheme();

	void PushBrowserTheme();
	void PopBrowserTheme();

	enum class Alignment
	{
		Left,
		Center,
		Right
	};
	bool CustomCheckbox(const char* label, bool* v, Alignment alignment);
	bool ToggleSwitch(const char* label, bool* v, Alignment alignment);
	void PushCustomButtonStyle();
	void PopCustomButtonStyle();
	bool CustomButton(const char* label, float buttonWidth = 0.0f, float buttonHeight = 40.0f);

	void PushBrowserInfoTheme();
	void PopBrowserInfoTheme();

	void DrawMarkdownButton(
		const char* markdownText,
		const char* url = nullptr,
		float window_width = 0.0f,
		std::function<void()> onClick = nullptr,
		ImVec4 normalColor = ImVec4(1.0f, 1.0f, 1.0f, 1.0f),
		ImVec4 hoverColor = ImVec4(1.0f, 1.0f, 0.0f, 1.0f)
	);
}

#endif