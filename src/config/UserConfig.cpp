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

		user.language 		= ConfigManager::Get<std::string>("Language", _default.language);
		user.theme 		= ConfigManager::Get<std::string>("Modex Theme", _default.theme);
		user.globalFont 	= ConfigManager::Get<std::string>("Global Font", _default.globalFont);

		user.screenScaleRatio 	= ConfigManager::Get<ImVec2>("Screen Scale Ratio", _default.screenScaleRatio);

		InputManager::SetCurrentHotkey(user.showMenuModifier, user.showMenuKey);
		Locale::GetSingleton()->SetFilePath(LOCALE_JSON_DIR / (user.language + ".json"));
		ThemeConfig::GetSingleton()->SetFilePath(THEMES_JSON_PATH / (user.theme + ".json"));
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

		ConfigManager::Set<std::string>("Language", user.language);
		ConfigManager::Set<std::string>("Modex Theme", user.theme);
		ConfigManager::Set<std::string>("Global Font", user.globalFont);

		ConfigManager::Set<ImVec2>("Screen Scale Ratio", user.screenScaleRatio);
	
		this->Save();
	}

	UserConfig::UserConfig() {
		SetFilePath(USERCONFIG_JSON_PATH);
	}
}
