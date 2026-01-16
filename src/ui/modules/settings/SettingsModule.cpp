// #include "include/U/UserSettings.h"
// #include "include/S/Settings.h"
// #include "include/P/Persistent.h"
// #include "include/D/Data.h"
// #include "include/U/Util.h"
// #include "include/U/UICustom.h"
// #include "include/U/UIManager.h"

#include "SettingsModule.h"

#include "data/Data.h"
#include "config/UserConfig.h"
#include "config/ThemeConfig.h"
#include "imgui_internal.h"
#include "ui/components/UICustom.h"
#include "localization/Language.h"
#include "localization/Locale.h"
#include "ui/core/UIManager.h"

namespace Modex
{
	static inline float p_fixedWidth = 150.0f; // TODO: FUCK YOU

	void InitializeBlacklistPlugins(std::vector<const RE::TESFile*>& a_pluginList, std::vector<std::string>& a_pluginListVector)
	{
		a_pluginList.clear();
		a_pluginListVector.clear();

		const auto& config = UserConfig::Get();
		a_pluginList = Data::GetSingleton()->GetModulePluginListSorted(Data::PLUGIN_TYPE::ALL, (Data::SORT_TYPE)config.modListSort);
		a_pluginListVector = Data::GetSingleton()->GetFilteredListOfPluginNames(Data::PLUGIN_TYPE::ALL, (Data::SORT_TYPE)config.modListSort, RE::FormType::None);
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
		m_selectedMod = Translate("Show All Plugins"); // TODO: Why?
		m_updateHidden = true;
		m_totalHidden = 0;
		m_totalBlacklisted = 0; // deprecated?
		m_primaryFilter = RE::FormType::None;
	}

	void SettingsModule::DrawGeneralSettings()
	{
		auto& config = UserConfig::Get();

		ImGui::SubCategoryHeader(Translate("SETTING_GENERAL"));

		if (UICustom::AddKeybind("SETTING_MENU_KEYBIND", config.showMenuKey, 211, keyHoverTintColor)) {
			UserConfig::GetSingleton()->SaveSettings();
		}

		ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal);

		if (UICustom::AddKeybind("SETTING_MENU_MODIFIER", config.showMenuModifier, 0, modifierHoverTint)) {
			UserConfig::GetSingleton()->SaveSettings();
		}

		ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal);

		std::vector<ModexTheme> themes = ThemeConfig::GetAvailableThemes();
		ImGui::Spacing();
		ImGui::Text(Translate("SETTINGS_THEME"));
		ImGui::SameLine(ImGui::GetContentRegionMax().x - p_fixedWidth - ImGui::GetStyle().IndentSpacing);
		ImGui::PushItemWidth(p_fixedWidth);
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

		ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal);

		std::vector<std::string> sorts = { "SETTING_SORT_ALPHA", "SETTING_SORT_LOAD_ASC", "SETTING_SORT_LOAD_DESC" };
		if (UICustom::AddSelectionDropdown("SETTING_SORT", config.modListSort, sorts)) {
			UserConfig::GetSingleton()->SaveSettings();
		}

		ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal);

		// UI Scale Setting
		ImGui::Spacing();
		ImGui::Text(Translate("SETTINGS_UI_SCALE_VERTICAL"));
		ImGui::SameLine(ImGui::GetContentRegionMax().x - p_fixedWidth - ImGui::GetStyle().IndentSpacing);
		ImGui::PushItemWidth(p_fixedWidth);
		ImGui::SliderInt("##UIVerticalScaleSelection", &s_uiScaleVertical, 50, 150, "%d%%");

		if (ImGui::IsItemDeactivatedAfterEdit()) {
			config.uiScaleVertical = s_uiScaleVertical;
			UserConfig::GetSingleton()->SaveSettings();
		}

		ImGui::Spacing();
		ImGui::PopItemWidth();

		ImGui::Spacing();
		ImGui::Text(Translate("SETTINGS_UI_SCALE_HORIZONTAL"));
		ImGui::SameLine(ImGui::GetContentRegionMax().x - p_fixedWidth - ImGui::GetStyle().IndentSpacing);
		ImGui::PushItemWidth(p_fixedWidth);
		ImGui::SliderInt("##UIHorizontalScaleSelection", &s_uiScaleHorizontal, 50, 150, "%d%%");

		if (ImGui::IsItemDeactivatedAfterEdit()) {
			config.uiScaleHorizontal = s_uiScaleHorizontal;

			UserConfig::GetSingleton()->SaveSettings();
		}
		ImGui::Spacing();
		ImGui::PopItemWidth();
		// End UI Scale Setting

		ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal);

		if (UICustom::AddToggleButton("SETTING_FULLSCREEN", config.fullscreen)) {
			UserConfig::GetSingleton()->SaveSettings();
		}

		ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal);

		if (UICustom::AddToggleButton("SETTINGS_PAUSE_GAME", config.pauseGame)) {
			UserConfig::GetSingleton()->SaveSettings();
		}

		ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal);

		if (UICustom::AddToggleButton("SETTINGS_DISABLE_IN_MENU", config.disableInMenu)) {
			UserConfig::GetSingleton()->SaveSettings();
		}

		ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal);

		if (UICustom::AddToggleButton("SETTINGS_SMOOTH_SCROLL", config.smoothScroll)) {
			UserConfig::GetSingleton()->SaveSettings();
		}

		ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal);

		std::vector<std::string> levels = { "trace", "debug", "info", "warn", "error", "critical" };

		ImGui::Spacing();
		ImGui::Text(Translate("SETTING_LOG_LEVEL"));
		ImGui::SameLine(ImGui::GetContentRegionMax().x - p_fixedWidth - ImGui::GetStyle().IndentSpacing);
		ImGui::PushItemWidth(p_fixedWidth);
		if (ImGui::BeginCombo("##LogLevelSelection", Translate(config.logLevel.c_str()))) {
			for (size_t i = 0; i < levels.size(); ++i) {
				if (ImGui::Selectable(Translate(levels[i].c_str()))) {
					config.logLevel = levels[i];
					UserConfig::GetSingleton()->SaveSettings();
				}
			}
			ImGui::EndCombo();
		}
		ImGui::Spacing();
		ImGui::PopItemWidth();

		ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal);

		if (UICustom::AddToggleButton("SETTING_WELCOME_BANNER", config.welcomeBanner)) {
			UserConfig::GetSingleton()->SaveSettings();
		}

		ImGui::SubCategoryHeader(Translate("SETTING_FONT_AND_LANGUAGE"));

		// Language Dropdown
		ImGui::Spacing();
		ImGui::Text(Translate("Language"));
		ImGui::SameLine(ImGui::GetContentRegionMax().x - p_fixedWidth - ImGui::GetStyle().IndentSpacing);
		ImGui::PushItemWidth(p_fixedWidth);

		auto languages = Language::GetLanguages();

		if (ImGui::BeginCombo("##LanguageSelection", config.language.c_str())) {
			for (auto& language : languages) {
				if (ImGui::Selectable(language.c_str())) {
					config.language = language;

					Locale::GetSingleton()->SetFilePath(LOCALE_JSON_DIR / (config.language + ".json"));
					Locale::GetSingleton()->Load(false);
					UserConfig::GetSingleton()->SaveSettings();
				}
			}
			ImGui::EndCombo();
		}
		ImGui::Spacing();
		ImGui::PopItemWidth();

		ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal);

		// Glyph Dropdown
		ImGui::Spacing();
		ImGui::Text(Translate("SETTING_GLYPH_RANGE"));
		ImGui::SameLine(ImGui::GetContentRegionMax().x - p_fixedWidth - ImGui::GetStyle().IndentSpacing);
		ImGui::PushItemWidth(p_fixedWidth);

		auto glyphs = Language::GetListOfGlyphNames();
		// TODO: REFACTOR: GetGlyphName to Int instead of Glyph Range type.
		auto currentGlyph = Language::GetGlyphName(config.glyphRange);
		// auto currentGlyph = Language::GetGlyphName(Language::GlyphRanges::English); // TEMPORARY FIX

		if (ImGui::BeginCombo("##GlyphSelection", currentGlyph.data())) {
			for (auto& glyph : glyphs) {
				if (ImGui::Selectable(glyph.data())) {
					config.glyphRange = Language::GetGlyphRange(glyph);

					UIManager::GetSingleton()->RefreshFont();
					UserConfig::GetSingleton()->SaveSettings();
				}
			}
			ImGui::EndCombo();
		}
		ImGui::Spacing();
		ImGui::PopItemWidth();

		ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal);

		if (UICustom::AddFontDropdown("SETTING_FONT", &config.globalFont)) {
			UserConfig::GetSingleton()->SaveSettings();
		}

		ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal);

		// Font Size Setting
		ImGui::Spacing();
		ImGui::Text(Translate("SETTING_FONT_SIZE"));
		ImGui::SameLine(ImGui::GetContentRegionMax().x - p_fixedWidth - ImGui::GetStyle().IndentSpacing);
		ImGui::PushItemWidth(p_fixedWidth);

		ImGui::SliderInt("##FontSizeSelection", &s_fontSize, 8, 28, "%d");

		if (ImGui::IsItemDeactivatedAfterEdit()) {
			config.globalFontSize = static_cast<float>(s_fontSize);

			UserConfig::GetSingleton()->SaveSettings();
			UIManager::GetSingleton()->RefreshFont();
		}
		ImGui::Spacing();
		ImGui::PopItemWidth();
	}
}
