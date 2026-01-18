#include "UICustom.h"

#include "core/Graphic.h"
#include "external/icons/IconsLucide.h"
#include "imgui.h"
#include "imgui_internal.h"
#include "ui/core/UIManager.h"
#include "config/UserConfig.h"
#include "config/ThemeConfig.h"
#include "localization/FontManager.h"
#include "localization/Locale.h"


namespace Modex::UICustom
{
	static inline float s_widgetWidth = 150.0f; // TODO: FUCK YOU

	[[nodiscard]] float GetCenterTextPosX(const char* a_text)
	{
		return ImGui::GetCursorPosX() + (ImGui::GetContentRegionAvail().x / 2.0f) - (ImGui::CalcTextSize(a_text).x / 2.0f);
	}

	[[nodiscard]] float GetCenterTextPosX(const std::string& a_string)
	{
		return GetCenterTextPosX(a_string.c_str());
	}

	void SubCategoryHeader(const char* a_label, ImVec4 a_color)
	{
		ImGui::PushStyleColor(ImGuiCol_Button, a_color);
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, a_color);
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, a_color);
		ImGui::Button(a_label, ImVec2(ImGui::GetContentRegionAvail().x, ImGui::GetFrameHeightWithSpacing()));
		ImGui::PopStyleColor(3);
	}

	void FancyTooltip(const char* a_text)
	{
		const float width = ImGui::GetIO().DisplaySize.x * 0.20f;

		ImGui::SetNextWindowSize(ImVec2(width, 0.0f));
		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 5.0f);
		if (ImGui::BeginTooltip()) {
			const auto& drawList = ImGui::GetWindowDrawList();
			const ImVec2 pos = ImGui::GetCursorScreenPos();
			const ImVec2 size = ImGui::GetWindowSize();
			
			ImGui::TextWrapped("%s", Translate(a_text));

			drawList->AddRectFilled(
				ImVec2(pos.x - ImGui::GetStyle().WindowPadding.x, pos.y + (ImGui::GetFontSize() * 1.5f)),
				ImVec2(pos.x + size.x, pos.y + (ImGui::GetFontSize() * 1.5f) + 1.0f),
				ThemeConfig::GetColorU32("TOOLTIP_SEPARATOR"));

			ImGui::EndTooltip();
		}
		ImGui::PopStyleVar();
	}

	bool IconButton(const char* a_icon, const char* a_tooltip, bool& a_toggle) {
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0, 0, 0, 0));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0, 0, 0, 0));
		ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0, 0, 0, 0));
		ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0, 0, 0, 0));
		ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0, 0, 0, 0));
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));

		float size = ImGui::GetFrameHeightWithSpacing();
		bool pressed = ImGui::Button(a_icon, ImVec2(size, size));

		if (pressed) {
			a_toggle = !a_toggle;
		}

		ImGui::PopStyleColor(6);
		ImGui::PopStyleVar(1);

		if (a_tooltip[0] != '\0') {
			if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal | ImGuiHoveredFlags_NoSharedDelay)) {
				FancyTooltip(a_tooltip);
			}
		}

		return pressed;
	}


	// Exclusively used in the Frame Sidebar window.
	bool SidebarImageButton(const std::string& a_text, const std::string& a_icon, bool a_selected, ImVec2 a_buttonSize, float& a_textMod, bool a_expanded)
	{
		ImGui::PushID(a_text.c_str());
		const ImVec2 pos = ImGui::GetCursorPos();
		const ImVec2 glyph_offset = ImVec2(4.0f, 2.0f);
		const float spacing = ImGui::GetFrameHeight();

		ImGui::SetNextItemAllowOverlap();
		bool pressed = ImGui::Selectable("", a_selected, 0, a_buttonSize);
		bool hovered = ImGui::IsItemHovered();
		bool held = ImGui::IsItemActive();

		ImGui::SetCursorPos(pos);
		ImGui::SetCursorPosY(pos.y + (a_buttonSize.y / 2.0f) - (ImGui::GetFontSize() / 2.0f) - glyph_offset.y);

		if (!a_expanded)
		{
			ImGui::SetCursorPosX(UICustom::GetCenterTextPosX(a_icon) - glyph_offset.x);
		}

		ImGui::PushFont(NULL, ImGui::GetFontSize() + 8.0f);
		ImGui::Text("%s", a_icon.c_str());
		ImGui::PopFont();

		const float anim_step = 0.05f;
		if (hovered || held) {
			if (a_textMod < 1.0f) {
				a_textMod += anim_step;
			} else {
				a_textMod = 1.0f;
			}
		} else {
			if (a_textMod > 0.0f) {
				a_textMod -= anim_step;
			} else {
				a_textMod = 0.0f;
			}
		}

		if (!a_expanded)
		{
			ImGui::PopID();
			ImGui::Spacing();
			return pressed;
		}
		else {
			ImGui::SameLine();
			ImGui::SetCursorPosY(ImGui::GetCursorPosY() + glyph_offset.y);
			ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (spacing * a_textMod));

			if (a_selected) ImGui::PushFontBold();
			ImGui::Text("%s", a_text.c_str()); 
			if (a_selected) ImGui::PopFont();

			ImGui::PopID();
			ImGui::Spacing();
			return pressed;
		}
	};

	bool Settings_ToggleButton(const char* a_localeString, bool& a_toggle)
	{
		auto id = "##Settings::ToggleButton::" + std::string(a_localeString);
		
		ImGui::PushID(id.c_str());
		ImGui::Spacing();


		const ImVec4 button_color = a_toggle == true ? ThemeConfig::GetColor("BUTTON_CONFIRM") : ThemeConfig::GetColor("BUTTON_CANCEL");
		const std::string button_text = a_toggle == true ? Translate("ON") : Translate("OFF"); 

		ImGui::Text("%s", Translate(a_localeString));
		ImGui::SameLine(ImGui::GetContentRegionAvail().x - s_widgetWidth - ImGui::GetStyle().IndentSpacing);
		ImGui::PushStyleColor(ImGuiCol_Button, button_color);
		bool pressed = ImGui::Button(button_text.c_str(), ImVec2(s_widgetWidth, 0.0f));
		ImGui::PopStyleColor();

		if (pressed) {
			a_toggle = !a_toggle;
		}

		ImGui::Spacing();
		ImGui::PopID();
		return pressed;
	}

	bool Settings_SliderInt(const char* a_localeString, int& a_value, int a_min, int a_max)
	{
		auto id = "##Settings::SliderInt::" + std::string(a_localeString);

		ImGui::Spacing();

		ImGui::Text("%s", Translate(a_localeString));
		ImGui::SameLine(ImGui::GetContentRegionAvail().x - s_widgetWidth - ImGui::GetStyle().IndentSpacing);
		ImGui::SetNextItemWidth(s_widgetWidth);
		bool pressed = ImGui::SliderInt(id.c_str(), &a_value, a_min, a_max);

		ImGui::Spacing();
		return pressed;
	}


	bool Settings_ColorPicker(const char* a_text, ImVec4& a_colRef)
	{
		constexpr auto flags = ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel | ImGuiColorEditFlags_AlphaBar;
		auto id = "##Settings::ColorPicker::" + std::string(a_text);
		auto popup = id + "::Popup";
		bool change = false;

		ImGui::Spacing();
		ImGui::Text("%s", Translate(a_text));
		ImGui::SameLine(ImGui::GetContentRegionAvail().x - s_widgetWidth - ImGui::GetStyle().IndentSpacing);

		if (ImGui::ColorButton(id.c_str(), a_colRef, flags, ImVec2(s_widgetWidth, 0))) {
			ImGui::OpenPopup(popup.c_str());
		}

		if (ImGui::BeginPopup(popup.c_str())) {
			if (ImGui::ColorPicker4(id.c_str(), (float*)&a_colRef, flags)) {
				change = true;
			}
			ImGui::EndPopup();
		}
		
		ImGui::Spacing();
		
		return change;
	}

	bool Settings_SliderFloat(const char* a_text, float& a_valRef, float a_min, float a_max)
	{
		auto id = "##Settings::SliderFloat" + std::string(a_text);
		bool changes = false;

		ImGui::Spacing();
		ImGui::Text("%s", Translate(a_text));
		ImGui::SameLine(ImGui::GetContentRegionAvail().x - s_widgetWidth - ImGui::GetStyle().IndentSpacing);
		ImGui::SetNextItemWidth(s_widgetWidth);

		if (ImGui::SliderFloat(id.c_str(), &a_valRef, a_min, a_max)) {
			changes = true;
		}

		ImGui::Spacing();

		return changes;
	}

	bool Settings_Keybind(const char* a_text, uint32_t& a_keybind, uint32_t defaultKey, ImVec4& a_hover)
	{
		auto& config = UserConfig::Get();

		auto id = "##Keybind" + std::string(a_text);
		bool changes = false;

		ImGui::Spacing();
		ImGui::SetCursorPosY(ImGui::GetCursorPosY() + ImGui::GetStyle().ItemSpacing.y / 2 + ImGui::GetFontSize() / 2);
		ImGui::Text("%s", Translate(a_text));

		if (GraphicManager::imgui_library.empty()) {
			const float height = ((config.uiScaleVertical / 100) * 20.0f) + 10.0f;

			ImGui::SameLine(ImGui::GetContentRegionAvail().x - s_widgetWidth - ImGui::GetStyle().IndentSpacing);  // Right Align
			ImGui::SetCursorPosY(ImGui::GetCursorPosY() - (height / 2) + 5.0f);                                            // Center Align

			if (ImGui::Button(ImGui::SkyrimKeymap.at(a_keybind), ImVec2(s_widgetWidth, height + 5.0f))) {
				UIManager::GetSingleton()->ShowHotkey(&a_keybind, defaultKey, [&]() {
					changes = true;
				});
			}
		} else {
			const ImVec2& uv0 = ImVec2(0, 0);
			const ImVec2& uv1 = ImVec2(1, 1);
			const ImVec4& bg_col = ImVec4(0, 0, 0, 0);
			const float alpha = 1.0f;

			GraphicManager::Image img = GraphicManager::imgui_library[ImGui::ImGuiKeymap.at(a_keybind)];

			float scale = config.uiScaleVertical / 100.0f;
			const float imageWidth = ((float)img.width * 0.5f) * scale;
			const float imageHeight = ((float)img.height * 0.5f) * scale;
			const ImVec2& size = ImVec2(imageWidth, imageHeight);

			// ImGui::SameLine(ImGui::GetContentRegionAvail().x - (s_widgetWidth - p_padding) + imageWidth / scale);  // Right Align
			ImGui::SameLine(ImGui::GetContentRegionAvail().x - ImGui::GetStyle().IndentSpacing - imageWidth);
			ImGui::SetCursorPosY(ImGui::GetCursorPosY() - (imageHeight / 2) + 5.0f);  // Center Align

			ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * alpha);
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.f, 0.f, 0.f, 0.f));
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.f, 0.f, 0.f, 0.f));
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.f, 0.f, 0.f, 0.f));
			ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.f, 0.f, 0.f, 0.f));
			ImGui::PushStyleColor(ImGuiCol_BorderShadow, ImVec4(0.f, 0.f, 0.f, 0.f));

			ImTextureID texture = (ImTextureID)(intptr_t)img.texture;
			if (ImGui::ImageButton(id.c_str(), texture, size, uv0, uv1, bg_col, a_hover)) {
				UIManager::GetSingleton()->ShowHotkey(&a_keybind, defaultKey, [&]() {
					changes = true;
				});
			}

			if (ImGui::IsItemHovered()) {
				a_hover = ImVec4(1.f, 1.f, 1.f, 1.f);
			} else {
				a_hover = ImVec4(0.9f, 0.9f, 0.9f, 0.9f);
			}

			ImGui::PopStyleVar();
			ImGui::PopStyleColor(5);
		}

		ImGui::Spacing();

		return changes;
	}

	bool Settings_Dropdown(const char* a_text, int& a_value, const std::vector<std::string>& a_options, bool a_localizeList)
	{
		bool result = false;
		auto id = "##Settings::Dropdown" + std::string(a_text);

		ImGui::Spacing();
		ImGui::Text("%s", Translate(a_text));
		ImGui::SameLine(ImGui::GetContentRegionAvail().x - s_widgetWidth - ImGui::GetStyle().IndentSpacing);
		ImGui::PushItemWidth(s_widgetWidth);
		if (ImGui::BeginCombo(id.c_str(), Translate(a_options[a_value].c_str()))) {
			for (size_t i = 0; i < a_options.size(); ++i) {
				const char* entry = a_localizeList ? Translate(a_options[i].c_str()) : a_options[i].c_str();
				if (ImGui::Selectable(entry)) {
					a_value = static_cast<int>(i);
					result = true;
				}
			}
			ImGui::EndCombo();
		}
		ImGui::Spacing();
		ImGui::PopItemWidth();

		return result;
	}

	bool Settings_FontDropdown(const char* a_text, std::string* a_font)
	{
		auto id = "##Settings::FontDropdown" + std::string(a_text);
		bool result = false;

		ImGui::Spacing();
		ImGui::Text("%s", Translate(a_text));
		ImGui::SameLine(ImGui::GetContentRegionAvail().x - s_widgetWidth - ImGui::GetStyle().IndentSpacing);
		ImGui::PushItemWidth(s_widgetWidth);
		
		const auto fontLibrary = FontManager::GetSingleton()->GetFontLibrary();
		if (ImGui::BeginCombo(id.c_str(), a_font->c_str())) {
			ImGui::PushID("##Settings::FontDropdown::Combo");
			for (const auto& font : fontLibrary) {
				if (ImGui::Selectable(font.name.c_str())) {
					*a_font = font.name;

					FontManager::GetSingleton()->SetFont(font.filepath);
					result = true;
				}
			}
			ImGui::PopID();
			ImGui::EndCombo();
		}
		ImGui::PopItemWidth();

		return result;
	}

	// Draws a header bar inline with a square X button to the right to close the menu
	bool Popup_MenuHeader(const char* label) 
	{
		ImGui::BeginGroup();

		auto DrawList = ImGui::GetWindowDrawList();
		float windowWidth = ImGui::GetWindowWidth();
		const ImVec2 padding = ImGui::GetStyle().WindowPadding;
		ImVec2 p = ImGui::GetCursorScreenPos();
		p.x -= padding.x;
		p.y -= padding.y;
		

		ImVec2 bb = ImVec2(windowWidth, ImGui::GetFrameHeight() + padding.y);
		ImU32 col_a = ImGui::GetColorU32(ImGuiCol_Header, 0.25f);
		ImU32 col_b = ImGui::GetColorU32(ImGuiCol_HeaderActive, 0.25f);
		DrawList->AddRectFilledMultiColor(ImVec2(p.x, p.y), ImVec2(p.x + bb.x, p.y + bb.y), col_a, col_a, col_b, col_b);
		DrawList->AddLine(ImVec2(p.x, p.y + bb.y), ImVec2(p.x + bb.x, p.y + bb.y), ImGui::GetColorU32(ImGuiCol_Border));
		
		ImGui::Text("%s", label);
		ImGui::SameLine(windowWidth - bb.y - padding.x);
		
		ImGui::SetCursorPosY(0);
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.0f, 1.0f, 1.0f, 0.25f));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(1.0f, 1.0f, 1.0f, 0.25f));
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
		
		bool pressed = ImGui::Button(" " ICON_LC_X, ImVec2(bb.y, bb.y));
		ImGui::Spacing();

		ImGui::PopStyleColor(3);
		ImGui::PopStyleVar();

		ImGui::EndGroup();


		return pressed;
	}

	bool Popup_ConfirmDeclineButtons(bool &a_confirm, bool &a_cancel, bool m_navAccept)
	{
		const float button_width = (ImGui::GetContentRegionAvail().x - ImGui::GetStyle().ItemSpacing.x) * 0.5f;

		ImGui::PushStyleColor(ImGuiCol_Button, 
			m_navAccept ? ThemeConfig::GetColor("BUTTON_CONFIRM_HOVER") : ThemeConfig::GetColor("BUTTON_CONFIRM")
		);
		bool confirm = ImGui::Button(Translate("CONFIRM"), ImVec2(button_width, ImGui::GetFrameHeightWithSpacing()));
		ImGui::PopStyleColor();
		
		ImGui::SameLine();

		ImGui::PushStyleColor(ImGuiCol_Button, 
			m_navAccept ? ThemeConfig::GetColor("BUTTON_CANCEL") : ThemeConfig::GetColor("BUTTON_CANCEL_HOVER")
		);
		bool decline = ImGui::Button(Translate("CANCEL"), ImVec2(button_width, ImGui::GetFrameHeightWithSpacing()));
		ImGui::PopStyleColor();

		if (confirm) {
			a_cancel = false;
			return a_confirm = true;
		}

		if (decline) {
			a_confirm = false;
			return a_cancel = true;
		}

		return false;
	}
}
