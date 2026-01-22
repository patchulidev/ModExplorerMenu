#include "SettingsModule.h"
#include "data/Data.h"
#include "config/UserConfig.h"
#include "config/ThemeConfig.h"
#include "imgui_internal.h"
#include "ui/components/UICustom.h"
#include "localization/FontManager.h"
#include "localization/Locale.h"
#include "ui/core/UIManager.h"

namespace Modex
{
	static inline float s_widgetWidth = 150.0f; // Represents fixed with for right-aligned widgets.

	void InitializeBlacklistPlugins(std::vector<const RE::TESFile*>& a_pluginList, std::vector<std::string>& a_pluginListVector)
	{
		a_pluginList.clear();
		a_pluginListVector.clear();

		const auto& config = UserConfig::Get();
		const auto sortType = static_cast<Data::SORT_TYPE>(config.modListSort);
		const auto pluginType = Data::PLUGIN_TYPE::ALL;
		a_pluginList = Data::GetSingleton()->GetModulePluginListSorted(pluginType, sortType);
		a_pluginListVector = Data::GetSingleton()->GetFilteredListOfPluginNames(pluginType, (Data::SORT_TYPE)config.modListSort);
		a_pluginListVector.insert(a_pluginListVector.begin(), Translate("Show All Plugins"));
	}

	static inline ImVec4 keyHoverTintColor = ImVec4(0.9f, 0.9f, 0.9f, 0.9f);
	static inline ImVec4 modifierHoverTint = ImVec4(0.9f, 0.9f, 0.9f, 0.9f);

	static inline int s_uiScaleVertical = 100;
	static inline int s_uiScaleHorizontal = 100;
	static inline int s_fontSize = 14;

	void SettingsModule::Draw(float a_offset)
	{
		ImVec2 window_padding = ImGui::GetStyle().WindowPadding;
        const float button_width = (ImGui::GetContentRegionAvail().x / static_cast<int>(Viewport::Count)) - window_padding.x / 2.0f;
        const float button_height = ImGui::GetFontSize() * 1.5f;
        const float tab_bar_height = button_height + (ImGui::GetStyle().WindowPadding.y * 2.0f);

		// Top Left
		ImGui::SameLine();
        ImGui::SetCursorPosX(window_padding.x + a_offset);
        ImGui::SetCursorPosY(window_padding.y);
        ImVec2 start_pos = ImGui::GetCursorPos();

        // Tab Button Area
        if (ImGui::BeginChild("##Settings::TabBar", ImVec2(0.0f, button_height), 0, ImGuiWindowFlags_NoFocusOnAppearing)) {
            ImGui::PushStyleVar(ImGuiStyleVar_SelectableTextAlign, ImVec2(0.5f, 0.5f));
            if (ImGui::Selectable("User Settings", m_activeViewport == Viewport::UserSettings, 0, ImVec2(button_width, button_height))) {
                m_activeViewport = Viewport::UserSettings;
            }

            ImGui::SameLine();

            if (ImGui::Selectable("Plugin Blacklist", m_activeViewport == Viewport::Blacklist, 0, ImVec2(button_width, button_height))) {
                m_activeViewport = Viewport::Blacklist;
            }

            ImGui::PopStyleVar();
        }

        ImGui::EndChild();
        ImGui::SameLine();
        ImGui::SetCursorPos(start_pos);

		// Tab Bar Separator
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + button_height);
        ImGui::PushStyleColor(ImGuiCol_Separator, ImGui::GetStyleColorVec4(ImGuiCol_Button));
        ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal);
        ImGui::PopStyleColor();
        ImGui::SetCursorPos(start_pos);
		
		ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX(), ImGui::GetCursorPosY() + tab_bar_height));
		if (m_activeViewport == Viewport::UserSettings) {
			if (ImGui::BeginChild("Modex::Settings::User", ImVec2(0, 0), true)) {
				DrawGeneralSettings();
			}

			ImGui::EndChild();
		}

		if (m_activeViewport == Viewport::Blacklist) {
			if (ImGui::BeginChild("Modex::Settings::Blacklist", ImVec2(0, 0), true)) {
				if (m_pluginList.empty() || m_pluginListVector.empty()) {
					InitializeBlacklistPlugins(m_pluginList, m_pluginListVector);
					m_totalPlugins = static_cast<int>(m_pluginList.size());
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
		m_selectedMod = Translate("Show All Plugins"); // TEST: Why are we localizing Show All?
		m_updateHidden = true;
		m_totalHidden = 0;
		m_totalBlacklisted = 0; // deprecated?
		m_primaryFilter = RE::FormType::None;
	}

	void SettingsModule::DrawGeneralSettings()
	{
		auto& config = UserConfig::Get();

		UICustom::SubCategoryHeader(Translate("SETTING_GENERAL"));
		{
			if (UICustom::Settings_Keybind("SETTING_MENU_KEYBIND", config.showMenuKey, 211, keyHoverTintColor))
			{
				UserConfig::GetSingleton()->SaveSettings();
			}
		}

		ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal);
		{
			if (UICustom::Settings_Keybind("SETTING_MENU_MODIFIER", config.showMenuModifier, 0, modifierHoverTint))
			{
				UserConfig::GetSingleton()->SaveSettings();
			}
		}

		ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal);
		{
			ImGui::Spacing();
			ImGui::Text("%s", Translate("SETTINGS_THEME"));
			
			ImGui::SameLine(ImGui::GetContentRegionAvail().x - s_widgetWidth - ImGui::GetStyle().IndentSpacing);

			ImGui::PushItemWidth(s_widgetWidth);
			std::vector<ModexTheme> themes = ThemeConfig::GetAvailableThemes();
			if (ImGui::BeginCombo("##ThemeSelection", config.theme.c_str())) {
				for (size_t i = 0; i < themes.size(); ++i) {
					if (ImGui::Selectable(Translate(themes[i].m_name.c_str()))) {
						config.theme = themes[i].m_name;
						ThemeConfig::GetSingleton()->LoadTheme(themes[i]);
						UserConfig::GetSingleton()->SaveSettings();
					}
				}
				ImGui::EndCombo();
			}
			ImGui::Spacing();
			ImGui::PopItemWidth();
		}
		
		// Plugin List Sort Dropdown
		ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal);
		{
			std::vector<std::string> sorts = { "SETTING_SORT_ALPHA", "SETTING_SORT_LOAD_ASC", "SETTING_SORT_LOAD_DESC" };
			if (UICustom::Settings_Dropdown("SETTING_SORT", config.modListSort, sorts)) {
				UserConfig::GetSingleton()->SaveSettings();
			}
		}

		// Vertical & Horizontal UI Scale Sliders
		ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal);
		{
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
		}

		ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal);
		{
			if (UICustom::Settings_ToggleButton("SETTING_FULLSCREEN", config.fullscreen))
			{
				UserConfig::GetSingleton()->SaveSettings();
			}
		}

		// NOTE: Unimplemented old feature, still needed?
		ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal);
		{
			if (UICustom::Settings_ToggleButton("SETTINGS_PAUSE_GAME", config.pauseGame))
			{
				UserConfig::GetSingleton()->SaveSettings();
			}
		}

		// NOTE: Unimplemented old feature, still needed?
		// Disable Menu opening over Skyrim Menu's (deprecated)
		ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal);
		{
			if (UICustom::Settings_ToggleButton("SETTINGS_DISABLE_IN_MENU", config.disableInMenu))
			{
				UserConfig::GetSingleton()->SaveSettings();
			}
		}

		// Smooth Scroll Toggle
		ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal);
		{
			if (UICustom::Settings_ToggleButton("SETTINGS_SMOOTH_SCROLL", config.smoothScroll))
			{
				UserConfig::GetSingleton()->SaveSettings();
			}
		}

		// Log Level Dropdown
		ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal);
		{
			std::vector<std::string> levels = { "trace", "debug", "info", "none" };

			if (UICustom::Settings_Dropdown("SETTINGS_LOG_LEVEL", config.logLevel, levels, false))
			{
				UserConfig::GetSingleton()->SaveSettings();
			}
		}

		// Welcome Banner Toggle
		ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal);
		{
			if (UICustom::Settings_LanguageDropdown("SETTINGS_LANGUAGE", &config.language)) {
				UserConfig::GetSingleton()->SaveSettings();
			}
		}

		UICustom::SubCategoryHeader(Translate("SETTING_FONT_AND_LANGUAGE"));
		
		// Font Selection Dropdown
		ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal);
		{
			if (UICustom::Settings_ToggleButton("SETTINGS_BASE_PLUGIN", config.basePlugin))
			{
				UserConfig::GetSingleton()->SaveSettings();
			}
		}

		// Font Size Slider
		ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal);
		{
			UICustom::Settings_SliderInt("SETTING_FONT_SIZE", s_fontSize, 8, 48);

			if (ImGui::IsItemDeactivatedAfterEdit())
			{
				config.globalFontSize = s_fontSize;
				UserConfig::GetSingleton()->SaveSettings();
			}
		}

		ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal);
	}
}
