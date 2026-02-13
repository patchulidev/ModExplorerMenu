#include "UserConfig.h"
#include "ConfigManager.h"
#include "external/json_serializers.cpp"
#include "localization/Locale.h"
#include "config/ThemeConfig.h"
#include "config/Keycodes.h"
#include "spdlog/spdlog.h"

namespace Modex
{
	std::array<uint32_t, 2> UserConfig::GetShowMenuKeys()
	{
		auto& config = UserConfig::Get();
		return { config.showMenuKey, config.showMenuModifier };
	}

	std::string UserConfig::GetShowMenuKeysAsText()
	{
		auto& config = UserConfig::Get();

		// BUG: Can we gaurantee this lookup?
		const std::string keyName = ImGui::SkyrimKeymap.at(config.showMenuKey);
		const std::string modifierKeyName = ImGui::SkyrimKeymap.at(config.showMenuModifier);

		if (!modifierKeyName.empty()) {
			return std::format("{} + {}", modifierKeyName, keyName);
		}
		
		if (!keyName.empty()) {
			return keyName;
		}

		return "Unknown?";
	}
	void UserConfig::LoadSettings()
	{
		ASSERT_MSG(!Load(true), "Failed to load user config!");
	
		user.showMenuKey 	= ConfigManager::Get<uint32_t>("Open Menu Keybind", _default.showMenuKey);
		user.showMenuModifier 	= ConfigManager::Get<uint32_t>("Open Menu Modifier", _default.showMenuModifier);
		user.modListSort 	= ConfigManager::Get<uint32_t>("Plugin List Sorting", _default.modListSort);
		user.logLevel 		= ConfigManager::Get<uint32_t>("Log Level", _default.logLevel);

		user.uiScaleVertical 	= ConfigManager::Get<int>("UI Scale Vertical", _default.uiScaleVertical);
		user.uiScaleHorizontal 	= ConfigManager::Get<int>("UI Scale Horizontal", _default.uiScaleHorizontal);
		user.globalFontSize 	= ConfigManager::Get<int>("Global Font Size", _default.globalFontSize);

		user.fullscreen 	= ConfigManager::Get<bool>("Fullscreen", _default.fullscreen);
		user.pauseGame 		= ConfigManager::Get<bool>("Pause Game While Open", _default.pauseGame);
		user.disableInMenu 	= ConfigManager::Get<bool>("Disable Opening In Menu", _default.disableInMenu);
		user.welcomeBanner 	= ConfigManager::Get<bool>("Welcome Banner", _default.welcomeBanner);
		user.smoothScroll 	= ConfigManager::Get<bool>("Smooth Scroll", _default.smoothScroll);
		user.disableAlt		= ConfigManager::Get<bool>("Disable Alt Key Shortcut", _default.disableAlt);
		user.basePlugin 	= ConfigManager::Get<bool>("Use Base Plugin", _default.basePlugin);
		user.showMissing	= ConfigManager::Get<bool>("Show Missing Plugins", _default.showMissing);

		user.language 		= ConfigManager::Get<std::string>("Language", _default.language);
		user.theme 		= ConfigManager::Get<std::string>("Modex Theme", _default.theme);
		user.globalFont 	= ConfigManager::Get<std::string>("Global Font", _default.globalFont);

		user.developerMode  = ConfigManager::Get<bool>("Developer Mode", _default.developerMode);
		user.screenScaleRatio 	= ConfigManager::Get<ImVec2>("Screen Scale Ratio", _default.screenScaleRatio);

		Locale::GetSingleton()->SetFilePath(LOCALE_JSON_DIR / (user.language + ".json"));
		ThemeConfig::GetSingleton()->SetFilePath(THEMES_JSON_PATH / (user.theme + ".json"));
		spdlog::set_level(static_cast<spdlog::level::level_enum>(user.logLevel));
		spdlog::flush_on(static_cast<spdlog::level::level_enum>(user.logLevel));
	}

	void UserConfig::SaveSettings()
	{
		ConfigManager::Set<uint32_t>("Open Menu Keybind", user.showMenuKey);
		ConfigManager::Set<uint32_t>("Open Menu Modifier", user.showMenuModifier);
		ConfigManager::Set<uint32_t>("Plugin List Sorting", user.modListSort);
		ConfigManager::Set<uint32_t>("Log Level", user.logLevel);

		ConfigManager::Set<int>("UI Scale Vertical", user.uiScaleVertical);
		ConfigManager::Set<int>("UI Scale Horizontal", user.uiScaleHorizontal);
		ConfigManager::Set<int>("Global Font Size", user.globalFontSize);

		ConfigManager::Set<bool>("Fullscreen", user.fullscreen);
		ConfigManager::Set<bool>("Pause Game While Open", user.pauseGame);
		ConfigManager::Set<bool>("Disable Opening In Menu", user.disableInMenu);
		ConfigManager::Set<bool>("Welcome Banner", user.welcomeBanner);
		ConfigManager::Set<bool>("Smooth Scroll", user.smoothScroll);
		ConfigManager::Set<bool>("Disable Alt Key Shortcut", user.disableAlt);
		ConfigManager::Set<bool>("Use Base Plugin", user.basePlugin);
		ConfigManager::Set<bool>("Show Missing Plugins", user.showMissing);

		ConfigManager::Set<std::string>("Language", user.language);
		ConfigManager::Set<std::string>("Modex Theme", user.theme);
		ConfigManager::Set<std::string>("Global Font", user.globalFont);

		ConfigManager::Set<bool>("Developer Mode", user.developerMode);
		ConfigManager::Set<ImVec2>("Screen Scale Ratio", user.screenScaleRatio);
	
		this->Save();
	}

	UserConfig::UserConfig() {
		SetFilePath(USERCONFIG_JSON_PATH);
	}
}
