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

	static inline ImVec4 keyHoverTintColor = ImVec4(0.9f, 0.9f, 0.9f, 0.9f);
	static inline ImVec4 modifierHoverTint = ImVec4(0.9f, 0.9f, 0.9f, 0.9f);

	static inline int s_uiScaleVertical = 100;
	static inline int s_uiScaleHorizontal = 100;
	static inline int s_fontSize = 14;

	void SettingsModule::Draw()
	{
		DrawTabMenu();
	}

	void DrawSettingsLayout(std::vector<std::unique_ptr<UITable>>& a_tables)
	{
		(void)a_tables;

		if (ImGui::BeginChild("##Modex::UserSettings::Layout", ImVec2(0, 0), false)) {

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

			if (ImGui::Button(Translate("SETTINGS_RELOAD_THEME"))) {
				ThemeConfig::GetSingleton()->Load(false);
				Locale::GetSingleton()->Load(false);
			}

			if (UICustom::Settings_ToggleButton("SETTINGS_PAUSE_GAME", config.pauseGame))
			{
				UserConfig::GetSingleton()->SaveSettings();
			}

			// NOTE: Unimplemented old feature, still needed?
			// Disable Menu opening over Skyrim Menu's (deprecated)
			if (UICustom::Settings_ToggleButton("SETTINGS_DISABLE_IN_MENU", config.disableInMenu))
			{
				UserConfig::GetSingleton()->SaveSettings();
			}

			std::vector<std::string> levels = { "trace", "debug", "info", "warn", "error", "critical", "off" };
			if (UICustom::Settings_Dropdown("SETTINGS_LOG_LEVEL", config.logLevel, levels, false))
			{
				UserConfig::GetSingleton()->SaveSettings();
			}

		}

		ImGui::EndChild();
	}

	void SettingsModule::DrawBlacklistLayout(std::vector<std::unique_ptr<UITable>>& a_tables)
	{
		(void)a_tables;

		if (ImGui::BeginChild("##Modex::Blacklist::Settings", ImVec2(0, 0), false)) {
			DrawBlacklistSettings();
		}

		ImGui::EndChild();
	}

	SettingsModule::~SettingsModule()
	{
		// Destructor
	}

	SettingsModule::SettingsModule()
		: m_modSearchBuffer{ 0 }
		, m_pluginList()
		, m_pluginListVector()
		, m_sort(0)
		, m_type(0)
	{
		// static
		s_uiScaleVertical 	= UserConfig::Get().uiScaleVertical;
		s_uiScaleHorizontal = UserConfig::Get().uiScaleHorizontal;
		s_fontSize 			= (int)UserConfig::Get().globalFontSize;

		// layouts
		m_layouts.push_back({ Translate("TAB_SETTINGS"), true, DrawSettingsLayout});

		m_layouts.push_back({ Translate("TAB_BLACKLIST"), false, 
			[this](std::vector<std::unique_ptr<UITable>>& a_tables) {
				DrawBlacklistLayout(a_tables);
		}});

		BuildBlacklistPlugins();
	}
}
