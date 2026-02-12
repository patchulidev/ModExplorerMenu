#pragma once

namespace Modex::UICustom
{
	// These methods can be used anywhere.
	ImU32 GetFormTypeColor(const RE::FormType& a_type);
	bool IconButton(const char* a_icon, const char* a_tooltip, bool& condition);
	bool ToggleButton(const char* a_id, bool& a_value, float a_width);
	bool SidebarImageButton(const std::string& a_title, const std::string& a_icon, bool a_selected, ImVec2 a_buttonSize, float& a_textMod, bool a_expanded);
	void SubCategoryHeader(const char* label, ImVec4 color = ImVec4(0.22f, 0.22f, 0.22f, 0.9f));
	bool ActionButton(const char* a_translate, const ImVec2& a_size, const bool a_condition);

	void FancyTooltip(const char* a_localeString);
	bool FancyInputText(const char* a_id, const char* a_hint, const char* a_tooltip, char* a_buffer, float a_width, ImGuiInputTextFlags a_flags = 0);
	bool FancyDropdown(const char* a_id, const char* a_tooltip, uint8_t& a_currentItem, const std::vector<std::string>& a_items, float a_width);
	bool FancyDropdown(const char* a_id, const char* a_tooltip, int& a_currentItem, const std::vector<std::string>& a_items, float a_width);

	void InputAmountHandler(bool a_condition, std::function<void(uint32_t)> a_onAmountEntered);
	bool BeginTabBar(const char* a_id, float a_height, float a_offset, ImVec2& a_start);
	void EndTabBar();

	// These methods include fixed width styling seen in settings panel.
	bool Settings_SliderFloat(const char* a_text, float& a_valRef, float a_min, float a_max);
	void Settings_Keybind(const char* a_title, const char* a_desc, uint32_t& a_keybind, uint32_t defaultKey, bool a_modifierOnly);
	bool Settings_Dropdown(const char* a_text, uint32_t& a_value, const std::vector<std::string>& a_options, bool a_localizeList = true);
	bool Settings_FontDropdown(const char* a_text, std::string* a_font);
	bool Settings_LanguageDropdown(const char* a_text, std::string* a_language);
	bool Settings_ToggleButton(const char* a_localeString, bool& a_value);
	bool Settings_SliderInt(const char* a_localeString, int& a_value, int a_min, int a_max);
	bool Settings_ThemeDropdown(const char* a_text, std::string* a_theme);
	void Settings_Header(const char* a_localeString);

	// These methods are exclusively used for popup window widgets.
	bool Popup_MenuHeader(const char* a_text);
	bool Popup_ConfirmDeclineButtons(bool& a_confirm, bool& a_cancel, bool m_navAccept = true);

	// Helpers, probably belong in ImGui namespace.
	[[nodiscard]] float GetCenterTextPosX(const char* a_text);
	[[nodiscard]] float GetCenterTextPosX(const std::string& a_string);
}

namespace ImGui
{
	static void HelpMarker(const std::string& a_localeString);
}
