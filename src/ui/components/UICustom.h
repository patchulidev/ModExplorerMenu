#pragma once

namespace Modex::UICustom
{
	bool IconButton(const char* a_icon, const char* a_tooltip, bool& condition);
	void FancyTooltip(const char* a_text);
	bool SidebarImageButton(const char* a_text, bool a_selected, ImTextureID a_texture, ImVec2 a_imageSize, ImVec2 a_buttonSize, float& a_textMod);
	void SubCategoryHeader(const char* label, ImVec4 color = ImVec4(0.22f, 0.22f, 0.22f, 0.9f));

	bool Settings_SliderFloat(const char* a_text, float& a_valRef, float a_min, float a_max);
	bool Settings_Keybind(const char* a_text, uint32_t& a_keybind, uint32_t defaultKey, ImVec4& a_hover);
	bool Settings_Dropdown(const char* a_text, int& a_value, const std::vector<std::string>& a_options);
	bool Settings_FontDropdown(const char* a_text, std::string* a_font);

	bool Popup_MenuHeader(const char* a_text);
	bool Popup_ConfirmDeclineButtons(bool& a_confirm, bool& a_cancel, bool m_navAccept = true);

	[[nodiscard]] float GetCenterTextPosX(const char* a_text);
	[[nodiscard]] float GetCenterTextPosX(const std::string& a_string);

}
