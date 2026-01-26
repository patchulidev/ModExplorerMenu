#include "SettingsModule.h"
#include "data/Data.h"
#include "config/UserConfig.h"
#include "config/ThemeConfig.h"
#include "ui/components/UICustom.h"
#include "localization/FontManager.h"
#include "localization/Locale.h"
#include "ui/core/UIManager.h"

namespace Modex
{
	static inline float s_widgetWidth = 150.0f; // Represents fixed with for right-aligned widgets.

	void SettingsModule::BuildBlacklistPlugins()
	{
		m_pluginList.clear();
		m_pluginListVector.clear();

		// BUG: When are these two member variables being set?
		const auto sortType = static_cast<Data::SORT_TYPE>(m_sort);
		const auto pluginType = static_cast<Data::PLUGIN_TYPE>(m_type);

		m_pluginList = Data::GetSingleton()->GetModulePluginListSorted(pluginType, sortType);
		m_pluginListVector = Data::GetSingleton()->GetFilteredListOfPluginNames(pluginType, sortType);
	}

	static inline ImVec4 keyHoverTintColor = ImVec4(0.9f, 0.9f, 0.9f, 0.9f);
	static inline ImVec4 modifierHoverTint = ImVec4(0.9f, 0.9f, 0.9f, 0.9f);

	static inline int s_uiScaleVertical = 100;
	static inline int s_uiScaleHorizontal = 100;
	static inline int s_fontSize = 14;

	void SettingsModule::Draw(float a_offset)
	{
		ImVec2 window_padding = ImGui::GetStyle().WindowPadding;
		const int viewports = static_cast<int>(Viewport::Count);
        const float button_width = (ImGui::GetContentRegionAvail().x / viewports);
        const float button_height = ImGui::GetFrameHeightWithSpacing();

        ImVec2 start_pos = ImGui::GetCursorPos();

        // Tab Button Area
		if (UICustom::BeginTabBar("#Settings::TabBar", button_height, a_offset, start_pos)) {
			ImGui::PushStyleColor(ImGuiCol_Header, ThemeConfig::GetColor("TAB_BUTTON"));
			ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(.0f, .0f));
			if (m_activeViewport == Viewport::UserSettings) ImGui::PushFontBold();
            if (ImGui::Selectable(Translate("TAB_SETTINGS"), m_activeViewport == Viewport::UserSettings, ImGuiSelectableFlags_NoPadWithHalfSpacing, ImVec2(button_width + window_padding.x, button_height))) {
                m_activeViewport = Viewport::UserSettings;
            }
			if (m_activeViewport == Viewport::UserSettings) ImGui::PopFont();

            ImGui::SameLine();

			if (m_activeViewport == Viewport::Blacklist) ImGui::PushFontBold();
            if (ImGui::Selectable(Translate("TAB_BLACKLIST"), m_activeViewport == Viewport::Blacklist, ImGuiSelectableFlags_NoPadWithHalfSpacing, ImVec2(button_width, button_height))) {
                m_activeViewport = Viewport::Blacklist;
            }
			if (m_activeViewport == Viewport::Blacklist) ImGui::PopFont();

			ImGui::PopStyleVar();
			ImGui::PopStyleColor();
        }

        ImGui::EndChild();
        ImGui::SetCursorPos(start_pos);

		// Tab Bar Separator
        ImGui::PushStyleColor(ImGuiCol_Separator, ThemeConfig::GetColor("TAB_BUTTON"));
        ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal, 2.0f);
        ImGui::PopStyleColor();

		start_pos.y += window_padding.y;
        ImGui::SetCursorPos(start_pos);
		
		if (m_activeViewport == Viewport::UserSettings) {
			if (ImGui::BeginChild("Modex::Settings::User", ImVec2(0, 0), false)) {
				DrawGeneralSettings();
			}

			ImGui::EndChild();
		}

		if (m_activeViewport == Viewport::Blacklist) {
			if (ImGui::BeginChild("Modex::Settings::Blacklist", ImVec2(0, 0), false)) {
				if (m_pluginList.empty() || m_pluginListVector.empty()) {
					BuildBlacklistPlugins();
				}

				DrawBlacklistSettings();
			}
			ImGui::EndChild();
		}
	}

	void SettingsModule::Load()
	{
		s_uiScaleVertical 	= UserConfig::Get().uiScaleVertical;
		s_uiScaleHorizontal = UserConfig::Get().uiScaleHorizontal;
		s_fontSize 			= (int)UserConfig::Get().globalFontSize;
	}

	void SettingsModule::DrawGeneralSettings()
	{
		auto& config = UserConfig::Get();

		ImGui::NewLine();

		float center_text_pos = UICustom::GetCenterTextPosX(Translate("SETTINGS_INFO"));
		ImGui::SetCursorPosX(center_text_pos);
		ImGui::Text("%s", Translate("SETTINGS_INFO"));

		center_text_pos = UICustom::GetCenterTextPosX(Translate("SETTINGS_INFO_2"));
		ImGui::SetCursorPosX(center_text_pos);
		ImGui::Text("%s", Translate("SETTINGS_INFO_2"));

		UICustom::Settings_Header(Translate("SETTINGS_HEADER_INPUT"));

		UICustom::Settings_Keybind("SETTINGS_MENU_KEYBIND", "SETTINGS_MENU_KEYBIND_DESC", config.showMenuKey, 211, false);
		UICustom::Settings_Keybind("SETTINGS_MENU_MODIFIER", "SETTINGS_MENU_KEYBIND_DESC", config.showMenuModifier, 0, true);

		ImGui::NewLine();
		UICustom::Settings_Header(Translate("SETTINGS_HEADER_STYLE"));

		if (UICustom::Settings_ThemeDropdown("SETTINGS_THEME", &config.theme))
		{
			UserConfig::GetSingleton()->SaveSettings();
		}

		if (config.fullscreen) ImGui::BeginDisabled();
			UICustom::Settings_SliderInt("SETTINGS_UI_SCALE_VERTICAL", s_uiScaleVertical, 25, 200);

			if (ImGui::IsItemDeactivatedAfterEdit()) {
				config.uiScaleVertical = s_uiScaleVertical;
				UserConfig::GetSingleton()->SaveSettings();
			}

			UICustom::Settings_SliderInt("SETTINGS_UI_SCALE_HORIZONTAL", s_uiScaleHorizontal, 25, 200);

			if (ImGui::IsItemDeactivatedAfterEdit()) {
				config.uiScaleHorizontal = s_uiScaleHorizontal;
				UserConfig::GetSingleton()->SaveSettings();
			}
		if (config.fullscreen) ImGui::EndDisabled();

		if (UICustom::Settings_ToggleButton("SETTINGS_FULLSCREEN", config.fullscreen))
		{
			UserConfig::GetSingleton()->SaveSettings();
		}

		if (UICustom::Settings_ToggleButton("SETTINGS_SMOOTH_SCROLL", config.smoothScroll))
		{
			UserConfig::GetSingleton()->SaveSettings();
		}

		if (UICustom::Settings_ToggleButton("SETTINGS_WELCOME_BANNER", config.welcomeBanner))
		{
			UserConfig::GetSingleton()->SaveSettings();
		}

		ImGui::NewLine();
		UICustom::Settings_Header(Translate("SETTINGS_HEADER_FONT"));

		if (UICustom::Settings_FontDropdown("SETTINGS_FONT", &config.globalFont))
		{
			UserConfig::GetSingleton()->SaveSettings();
		}

		UICustom::Settings_SliderInt("SETTINGS_FONT_SIZE", s_fontSize, 8, 48);

		if (ImGui::IsItemDeactivatedAfterEdit())
		{
			config.globalFontSize = s_fontSize;
			UserConfig::GetSingleton()->SaveSettings();
		}

		if (UICustom::Settings_LanguageDropdown("SETTINGS_LANGUAGE", &config.language)) {
			UserConfig::GetSingleton()->SaveSettings();
		}

		ImGui::NewLine();
		UICustom::Settings_Header(Translate("SETTINGS_HEADER_GENERAL"));

		std::vector<std::string> sorts = Data::GetSortStrings();
		if (UICustom::Settings_Dropdown("SETTINGS_SORT", config.modListSort, sorts)) {
			UserConfig::GetSingleton()->SaveSettings();
		}

		if (UICustom::Settings_ToggleButton("SETTINGS_BASE_PLUGIN", config.basePlugin))
		{
			UserConfig::GetSingleton()->SaveSettings();
		}

		if (UICustom::Settings_ToggleButton("SETTINGS_PAUSE_GAME", config.pauseGame))
		{
			UserConfig::GetSingleton()->SaveSettings();
		}

			std::vector<std::string> levels = { "trace", "debug", "info", "warn", "error", "critical", "off" };
			if (UICustom::Settings_Dropdown("SETTINGS_LOG_LEVEL", config.logLevel, levels, false))
			{
				UserConfig::GetSingleton()->SaveSettings();
			}
		}

		std::vector<std::string> levels = { "trace", "debug", "info", "none" };
		if (UICustom::Settings_Dropdown("SETTINGS_LOG_LEVEL", config.logLevel, levels, false))
		{
			UserConfig::GetSingleton()->SaveSettings();
		}
	}
}
