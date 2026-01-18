#pragma once

namespace Modex::UICustom
{
	// These methods can be used anywhere.
	bool IconButton(const char* a_icon, const char* a_tooltip, bool& condition);
	bool ToggleButton(const char* a_localeString, bool& a_value);
	void FancyTooltip(const char* a_localeString);
	bool SidebarImageButton(const std::string& a_title, const std::string& a_icon, bool a_selected, ImVec2 a_buttonSize, float& a_textMod, bool a_expanded);
	void SubCategoryHeader(const char* label, ImVec4 color = ImVec4(0.22f, 0.22f, 0.22f, 0.9f));

	// These methods include fixed width styling seen in settings panel.
	bool Settings_SliderFloat(const char* a_text, float& a_valRef, float a_min, float a_max);
	bool Settings_Keybind(const char* a_text, uint32_t& a_keybind, uint32_t defaultKey, ImVec4& a_hover);
	bool Settings_Dropdown(const char* a_text, int& a_value, const std::vector<std::string>& a_options, bool a_localizeList = true);
	bool Settings_FontDropdown(const char* a_text, std::string* a_font);
	bool Settings_ToggleButton(const char* a_localeString, bool& a_value);
	bool Settings_SliderInt(const char* a_localeString, int& a_value, int a_min, int a_max);

	bool Popup_MenuHeader(const char* a_text);
	bool Popup_ConfirmDeclineButtons(bool& a_confirm, bool& a_cancel, bool m_navAccept = true);

	// Helpers, probably belong in ImGui namespace.
	[[nodiscard]] float GetCenterTextPosX(const char* a_text);
	[[nodiscard]] float GetCenterTextPosX(const std::string& a_string);

}
