#include "UserConfig.h"

#include "ConfigManager.h"
#include "core/InputManager.h"
#include "external/json_serializers.cpp"
#include "localization/Locale.h"
#include "config/ThemeConfig.h"

namespace Modex
{
	void UserConfig::LoadSettings()
	{
		ASSERT_MSG(!Load(true), "Failed to load user config!");
	
		user.theme 					= ConfigManager::Get<std::string>("Modex Theme", _default.theme);
		user.showMenuKey 			= ConfigManager::Get<uint32_t>("Open Menu Keybind", _default.showMenuKey);
		user.showMenuModifier 		= ConfigManager::Get<uint32_t>("Open Menu Modifier", _default.showMenuModifier);
		user.modListSort 			= ConfigManager::Get<int>("Plugin List Sorting", _default.modListSort);
		user.uiScaleVertical 		= ConfigManager::Get<int>("UI Scale Vertical", _default.uiScaleVertical);
		user.uiScaleHorizontal 		= ConfigManager::Get<int>("UI Scale Horizontal", _default.uiScaleHorizontal);
		user.fullscreen 			= ConfigManager::Get<bool>("Fullscreen", _default.fullscreen);
		user.pauseGame 				= ConfigManager::Get<bool>("Pause Game While Open", _default.pauseGame);
		user.disableInMenu 			= ConfigManager::Get<bool>("Disable Opening In Menu", _default.disableInMenu);
		user.logLevel 				= ConfigManager::Get<std::string>("Log Level", _default.logLevel);
		user.welcomeBanner 			= ConfigManager::Get<bool>("Welcome Banner", _default.welcomeBanner);
		user.smoothScroll 			= ConfigManager::Get<bool>("Smooth Scroll", _default.smoothScroll);
		user.language 				= ConfigManager::Get<std::string>("Language", _default.language);
		user.globalFont 			= ConfigManager::Get<std::string>("Global Font", _default.globalFont);
		user.globalFontSize 		= ConfigManager::Get<float>("Global Font Size", _default.globalFontSize);
		user.screenScaleRatio 		= ConfigManager::Get<ImVec2>("Screen Scale Ratio", _default.screenScaleRatio);

		InputManager::SetCurrentHotkey(user.showMenuModifier, user.showMenuKey);
		Locale::GetSingleton()->SetFilePath(LOCALE_JSON_DIR / (user.language + ".json"));
		ThemeConfig::GetSingleton()->SetFilePath(THEMES_JSON_PATH / (user.theme + ".json"));
	}

	void UserConfig::SaveSettings()
	{
		ConfigManager::Set<std::string>("Modex Theme", user.theme);
		ConfigManager::Set<uint32_t>("Open Menu Keybind", user.showMenuKey);
		ConfigManager::Set<uint32_t>("Open Menu Modifier", user.showMenuModifier);
		ConfigManager::Set<int>("Plugin List Sorting", user.modListSort);
		ConfigManager::Set<int>("UI Scale Vertical", user.uiScaleVertical);
		ConfigManager::Set<int>("UI Scale Horizontal", user.uiScaleHorizontal);
		ConfigManager::Set<bool>("Fullscreen", user.fullscreen);
		ConfigManager::Set<bool>("Pause Game While Open", user.pauseGame);
		ConfigManager::Set<bool>("Disable Opening In Menu", user.disableInMenu);
		ConfigManager::Set<std::string>("Log Level", user.logLevel);
		ConfigManager::Set<bool>("Welcome Banner", user.welcomeBanner);
		ConfigManager::Set<bool>("Smooth Scroll", user.smoothScroll);
		ConfigManager::Set<std::string>("Language", user.language);
		ConfigManager::Set<std::string>("Global Font", user.globalFont);
		ConfigManager::Set<float>("Global Font Size", user.globalFontSize);
		ConfigManager::Set<ImVec2>("Screen Scale Ratio", user.screenScaleRatio);
	
		this->Save();
	}
}

	/*
	static inline void LoadColorsFromJSON()
	{
		const std::filesystem::path a_path = "Data\\Interface\\Modex\\user\\colors.json";
		GetSingleton()->m_colors.clear();

		nlohmann::json j = OpenJSONFile(a_path);

		for (auto& [key, value] : j.items()) {
			if (value.is_array() && value.size() == 4) {
				ImVec4 color = ImVec4(
					value[0].get<float>(),
					value[1].get<float>(),
					value[2].get<float>(),
					value[3].get<float>()
				);
				GetSingleton()->m_colors[key] = color;
			}
		}

		// Manually override ImGui colors that are overlapping
		auto& colors = ImGui::GetStyle().Colors;
		colors[ImGuiCol_Button] = GetSingleton()->m_colors["BUTTON"];
		colors[ImGuiCol_ButtonHovered] = GetSingleton()->m_colors["BUTTON_HOVER"];
		colors[ImGuiCol_ButtonActive] = GetSingleton()->m_colors["BUTTON_ACTIVE"];

		colors[ImGuiCol_Text] = GetSingleton()->m_colors["TEXT"];
		colors[ImGuiCol_TextDisabled] = GetSingleton()->m_colors["TEXT_DISABLED"];
		colors[ImGuiCol_TableRowBg] = GetSingleton()->m_colors["TABLE_ROW_BG"];
		colors[ImGuiCol_TableRowBgAlt] = GetSingleton()->m_colors["TABLE_ROW_BG_ALT"];
		colors[ImGuiCol_TableBorderLight] = GetSingleton()->m_colors["TABLE_ROW_BORDER"];
	}

	static inline ImVec4 GetColor(const std::string& a_key, float a_alphaMult = 1.0f)
	{
		auto& colors = GetSingleton()->m_colors;

		try {
			return colors.at(a_key) * ImVec4(1.0f, 1.0f, 1.0f, a_alphaMult * ImGui::GetStyle().Alpha);
		} catch (const std::out_of_range&) {
			ASSERT_MSG(true, std::format("PersistentData -> Color key '{}' not found!", a_key));
			return ImGui::GetStyleColorVec4(ImGuiCol_FrameBg);
		}
		
	}

	static inline ImU32 GetColorU32(const std::string& a_key, float a_alphaMult = 1.0f)
	{
		ImVec4 color = GetColor(a_key);
		color.w *= a_alphaMult;
		return ImGui::ColorConvertFloat4ToU32(color);
	}
			*/
