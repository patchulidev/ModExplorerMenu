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
	static inline float s_widgetWidth = 150.0f; // Fixed Settings right-align widget width.

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

	bool FancyInputText(const char* a_id, const char *a_hint, const char* a_tooltip, char* a_buffer, float a_width, ImGuiInputTextFlags a_flags)
	{
		auto bufferSize = IM_ARRAYSIZE(a_buffer);
		bool changed = false;
		auto pos = ImGui::GetCursorScreenPos();

		ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 12.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8.0f, 8.0f));

		ImGui::SetNextItemWidth(a_width);
		if (ImGui::InputTextWithHint(a_id, a_hint, a_buffer, bufferSize, a_flags)) {
			changed = true;
		}

		if (Locale::GetSingleton()->HasTooltip(a_tooltip)) {
			if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal | ImGuiHoveredFlags_NoSharedDelay)) {
				UICustom::FancyTooltip(a_tooltip);
			}
		}

		ImGui::SameLine();

		ImGui::PushFont(NULL, 18.0f);
		auto DrawList = ImGui::GetWindowDrawList();
		pos.x += a_width - ImGui::GetFrameHeightWithSpacing() + ImGui::GetStyle().FramePadding.x;
		pos.y += (ImGui::GetItemRectSize().y / 2.0f) - (ImGui::GetFontSize() / 2.0f);

		DrawList->AddText(pos, ThemeConfig::GetColorU32("TEXT"), ICON_LC_SEARCH);
		ImGui::PopFont();
		
		ImGui::PopStyleVar(2);
		return changed;
	}

	bool FancyDropdown(const char* a_id, const char* a_tooltip, uint32_t& a_currentItem, const std::vector<std::string>& a_items, float a_width)
	{
		auto tempIndex = static_cast<int>(a_currentItem);
		return FancyDropdown(a_id, a_tooltip, tempIndex, a_items, a_width) && (a_currentItem = static_cast<uint32_t>(tempIndex), true);
	}

	bool FancyDropdown(const char* a_id, const char* a_tooltip, int& a_currentItem, const std::vector<std::string>& a_items, float a_width)
	{
		bool changed = false;

		ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 12.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8.0f, 8.0f));
		auto pos = ImGui::GetCursorScreenPos();

		if (a_width == 0.0f) {
			a_width = ImGui::GetContentRegionAvail().x;
		}

		ImGui::SetNextItemWidth(a_width);
		if (ImGui::BeginCombo(a_id, a_items[a_currentItem].c_str(), ImGuiComboFlags_NoArrowButton)) {
			for (size_t i = 0; i < a_items.size(); i++) {
				bool isSelected = (a_currentItem == static_cast<int>(i));
				if (ImGui::Selectable(a_items[i].c_str(), isSelected)) {
					a_currentItem = static_cast<int>(i);
					changed = true;
				}

				if (isSelected) {
					ImGui::SetItemDefaultFocus();
				}
			}
			ImGui::EndCombo();
		}

		if (Locale::GetSingleton()->HasTooltip(a_tooltip)) {
			if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal | ImGuiHoveredFlags_NoSharedDelay)) {
				UICustom::FancyTooltip(a_tooltip);
			}
		}

		ImGui::SameLine();

		ImGui::PushFont(NULL, 18.0f);
		auto DrawList = ImGui::GetWindowDrawList();
		pos.x += a_width - ImGui::GetFrameHeightWithSpacing() + ImGui::GetStyle().FramePadding.x;
		pos.y += (ImGui::GetItemRectSize().y / 2.0f) - (ImGui::GetFontSize() / 2.0f);

		DrawList->AddText(pos, ThemeConfig::GetColorU32("TEXT"), ICON_LC_SQUARE_CHEVRON_DOWN);
		ImGui::PopFont();

		ImGui::PopStyleVar(2);
		return changed;
	}


	// OPTIMIZE: Can we also include hovering logic? Check references.
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

	bool BeginTabBar(const char* a_id, float a_height, float a_offset, ImVec2& a_start)
	{
		const ImVec2 window_padding = ImGui::GetStyle().WindowPadding;
		const float tab_height = a_height + window_padding.y;

		ImGui::SameLine();
		ImGui::SetCursorPosX(window_padding.x + a_offset);
		ImGui::SetCursorPosY(window_padding.y);

		a_start.y += ImGui::GetFrameHeightWithSpacing() + window_padding.y;
		
		return ImGui::BeginChild(a_id, ImVec2(0.0f, tab_height), false, false);
	}

	bool IconButton(const char* a_icon, const char* a_tooltip, bool& a_toggle) 
	{
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
		ImGui::Indent();

		auto id = "##Settings::ToggleButton::" + std::string(a_localeString);
		
		ImGui::PushID(id.c_str());
		const ImVec4 button_color = a_toggle == true ? ThemeConfig::GetColor("BUTTON_CONFIRM") : ThemeConfig::GetColor("BUTTON_CANCEL");
		const std::string button_text = a_toggle == true ? Translate("ON") : Translate("OFF"); 

		ImGui::AlignTextToFramePadding();
		ImGui::Text("%s", Translate(a_localeString));
		ImGui::HelpMarker(a_localeString);

		ImGui::SameLine(ImGui::GetContentRegionAvail().x - s_widgetWidth);
		ImGui::PushStyleColor(ImGuiCol_Button, button_color);
		bool pressed = ImGui::Button(button_text.c_str(), ImVec2(s_widgetWidth, 0.0f));
		ImGui::PopStyleColor();

		if (pressed) {
			a_toggle = !a_toggle;
		}

		ImGui::PopID();
		ImGui::Unindent();
		return pressed;
	}

	bool Settings_SliderInt(const char* a_localeString, int& a_value, int a_min, int a_max)
	{
		ImGui::Indent();

		auto id = "##Settings::SliderInt::" + std::string(a_localeString);

		ImGui::AlignTextToFramePadding();
		ImGui::Text("%s", Translate(a_localeString));
		ImGui::HelpMarker(a_localeString);

		ImGui::SameLine(ImGui::GetContentRegionAvail().x - s_widgetWidth);
		ImGui::SetNextItemWidth(s_widgetWidth);
		bool pressed = ImGui::SliderInt(id.c_str(), &a_value, a_min, a_max);

		ImGui::Unindent();
		return pressed;
	}

	bool Settings_SliderFloat(const char* a_localeString, float& a_valRef, float a_min, float a_max)
	{
		ImGui::Indent();

		auto id = "##Settings::SliderFloat" + std::string(a_localeString);

		bool changes = false;
		ImGui::AlignTextToFramePadding();
		ImGui::Text("%s", Translate(a_localeString));
		ImGui::HelpMarker(a_localeString);

		ImGui::SameLine(ImGui::GetContentRegionAvail().x - s_widgetWidth);
		ImGui::SetNextItemWidth(s_widgetWidth);
		if (ImGui::SliderFloat(id.c_str(), &a_valRef, a_min, a_max, "%.3f", ImGuiSliderFlags_ClampOnInput)) {
			changes = true;
		}
		ImGui::Unindent();

		return changes;
	}

	void Settings_Keybind(const char* a_title, const char* a_desc, uint32_t& a_keybind, uint32_t defaultKey, bool a_modifierOnly)
	{
		ImGui::Indent();

		auto id = "##Keybind" + std::string(a_title);

		ImGui::SetCursorPosY(ImGui::GetCursorPosY() + (ImGui::GetFrameHeightWithSpacing() / 2.0f)); 
		ImGui::AlignTextToFramePadding();
		ImGui::Text("%s", Translate(a_title));

		ImGui::HelpMarker(a_title);

		if (GraphicManager::imgui_library.empty()) {
			const float height = ImGui::GetFrameHeight() * 2.0f;
			const std::string label = a_keybind == 0 ? Translate("NONE") : ImGui::SkyrimKeymap.at(a_keybind);

			ImGui::SameLine(ImGui::GetContentRegionAvail().x - s_widgetWidth);  // Right Align
			ImGui::SetCursorPosY(ImGui::GetCursorPosY() - (ImGui::GetFrameHeight() / 2.0f));

			if (ImGui::Button(label.c_str(), ImVec2(s_widgetWidth, height))) {
				UIManager::GetSingleton()->ShowHotkey(Translate(a_title), Translate(a_desc), &a_keybind, defaultKey, a_modifierOnly, [&]() {
					UserConfig::GetSingleton()->SaveSettings();
				});
			}
		} else {
			GraphicManager::Image img;

			const auto keyIt = ImGui::SkyrimKeymap.find(a_keybind);
			if (a_keybind == 0 || keyIt == ImGui::SkyrimKeymap.end()) {
				img = GraphicManager::imgui_library.at("UnknownKey");
			} else {
				const std::string& keyName = keyIt->second;
				img = GraphicManager::imgui_library.at(keyName);
			}

			// Scale the image to FrameHeight, while keeping aspect ratio maintained.
			const float imageRatio = static_cast<float>(img.width) / static_cast<float>(img.height);
			const float imageWidth = (ImGui::GetFrameHeight() * 2.0f) * imageRatio;
			const float imageHeight = ImGui::GetFrameHeight() * 2.0f;
			const ImVec2 size = ImVec2(imageWidth, imageHeight);

			ImGui::SameLine(ImGui::GetContentRegionAvail().x - imageWidth);

			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.f, 0.f, 0.f, 0.f));
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.f, 1.f, 1.f, 0.05f));
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.f, 0.f, 0.f, 0.f));
			ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.f, 0.f, 0.f, 0.f));
			ImGui::PushStyleColor(ImGuiCol_BorderShadow, ImVec4(0.f, 0.f, 0.f, 0.f));

			ImTextureID texture = (ImTextureID)(intptr_t)img.texture;
			ImGui::SetCursorPosY(ImGui::GetCursorPosY() - (ImGui::GetFrameHeight() / 2.0f));
			if (ImGui::ImageButton(id.c_str(), texture, size)) {
				UIManager::GetSingleton()->ShowHotkey(Translate(a_title), Translate(a_desc), &a_keybind, defaultKey, a_modifierOnly, []() {
						UserConfig::GetSingleton()->SaveSettings();
				});
			}

			if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNone)) {
				if (ImGui::IsKeyPressed(ImGuiKey_T, false)) {
					a_keybind = defaultKey;
					UserConfig::GetSingleton()->SaveSettings();
				}
			}

			ImGui::PopStyleColor(5);
		}
		ImGui::Unindent();
	}

	bool Settings_Dropdown(const char* a_localeString, uint32_t& a_value, const std::vector<std::string>& a_options, bool a_localizeList)
	{
		ImGui::Indent();

		auto id = "##Settings::Dropdown" + std::string(a_localeString);

		bool result = false;
		ImGui::AlignTextToFramePadding();
		ImGui::Text("%s", Translate(a_localeString));
		ImGui::HelpMarker(a_localeString);

		ImGui::SameLine(ImGui::GetContentRegionAvail().x - s_widgetWidth);
		ImGui::SetNextItemWidth(s_widgetWidth);
		if (ImGui::BeginCombo(id.c_str(), Translate(a_options[a_value].c_str()))) {
			for (size_t i = 0; i < a_options.size(); ++i) {
				const char* entry = a_localizeList ? Translate(a_options[i].c_str()) : a_options[i].c_str();
				if (ImGui::Selectable(entry)) {
					a_value = static_cast<uint32_t>(i); // WARN: size_t to uint32_t conversion
					result = true;
				}
			}
			ImGui::EndCombo();
		}
		ImGui::Unindent();

		return result;
	}

	bool Settings_FontDropdown(const char* a_localeString, std::string* a_font)
	{
		ImGui::Indent();

		auto id = "##Settings::FontDropdown" + std::string(a_localeString);

		bool result = false;
		ImGui::AlignTextToFramePadding();
		ImGui::Text("%s", Translate(a_localeString));
		ImGui::HelpMarker(a_localeString);

		ImGui::SameLine(ImGui::GetContentRegionAvail().x - s_widgetWidth);
		
		ImGui::SetNextItemWidth(s_widgetWidth);
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
		ImGui::Unindent();

		return result;
	}

	bool Settings_LanguageDropdown(const char* a_localeString, std::string* a_config)
	{
		ImGui::Indent();

		auto id = "##Settings::LanguageDropdown" + std::string(a_localeString);

		bool result = false;
		ImGui::AlignTextToFramePadding();
		ImGui::Text("%s", Translate(a_localeString));
		ImGui::HelpMarker(a_localeString);

		ImGui::SameLine(ImGui::GetContentRegionAvail().x - s_widgetWidth);
		ImGui::SetNextItemWidth(s_widgetWidth);
		const auto languages = Locale::GetSingleton()->GetLanguages();
		if (ImGui::BeginCombo(id.c_str(), a_config->c_str())) {
			ImGui::PushID("##Settings::Language::Combo");
			for (const auto& language : languages) {
				if (ImGui::Selectable(language.c_str())) {
					const auto filepath = Locale::GetSingleton()->GetFilepath(language);
					if (!filepath.empty()) {
						Locale::GetSingleton()->SetFilePath(filepath);
						bool success = Locale::GetSingleton()->Load(false);
						
						if (success) {
							*a_config = language;
							result = true;
						}
					}
				}
			}
			ImGui::PopID();
			ImGui::EndCombo();
		}
		ImGui::Unindent();

		return result;
	}
	
	bool Settings_ThemeDropdown(const char* a_localeString, std::string* a_config)
	{
		ImGui::Indent();

		auto id = "##Settings::ThemeDropdown" + std::string(a_localeString);

		bool result = false;
		ImGui::AlignTextToFramePadding();
		ImGui::Text("%s", Translate(a_localeString));
		
		ImGui::SameLine(ImGui::GetContentRegionAvail().x - s_widgetWidth);
		ImGui::SetNextItemWidth(s_widgetWidth);

		std::vector<ModexTheme> themes = ThemeConfig::GetAvailableThemes();
		if (ImGui::BeginCombo("##ThemeSelection", a_config->c_str())) {
			for (size_t i = 0; i < themes.size(); ++i) {
				if (ImGui::Selectable(Translate(themes[i].m_name.c_str()))) {
					result = ThemeConfig::GetSingleton()->LoadTheme(themes[i]);

					if (result) {
						*a_config = themes[i].m_name;
					}
				}
			}
			ImGui::EndCombo();
		}
		ImGui::Unindent();

		return result;
	}

	void Settings_Header(const char* a_localeString)
	{
		ImGui::PushFontBold();
		ImGui::PushStyleColor(ImGuiCol_Text, ThemeConfig::GetColor("HEADER"));
		ImGui::SeparatorText(Translate(a_localeString));
		ImGui::PopStyleColor();
		ImGui::PopFont();
		ImGui::NewLine();
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

namespace ImGui
{
	static void HelpMarker(const std::string& a_localeString)
	{
		const char* tooltip = Modex::Locale::GetSingleton()->GetTooltip(a_localeString.c_str());

		if (tooltip && tooltip[0] != '\0') {
			ImGui::SameLine();
			ImGui::TextDisabled(ICON_LC_MESSAGE_CIRCLE_QUESTION);

			if (ImGui::IsItemHovered(ImGuiHoveredFlags_NoSharedDelay | ImGuiHoveredFlags_DelayNone)) {
				Modex::UICustom::FancyTooltip(tooltip);
			}
		}
	}
}
