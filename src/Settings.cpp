#include "include/S/Settings.h"
#include "SimpleIni.h"
#include "include/I/INI.h"
#include "include/I/InputManager.h"
#include "include/M/Menu.h"
#include "include/U/Util.h"
#include <format>

using namespace IniHelper;

namespace Modex
{

	void Settings::GetIni(const std::filesystem::path& a_path, const std::function<void(CSimpleIniA&)> a_func)
	{
		CSimpleIniA ini;

		if (a_path.empty()) {
			logger::critical("[Settings] Invalid Path provided to GetIni");
		}

		// This is okay since we're interfacing with an external API
		const std::wstring wide_path = a_path.wstring();

		ini.SetUnicode();

		if (const auto rc = ini.LoadFile(wide_path.c_str())) {
			if (rc < SI_OK) {
				(void)ini.SaveFile(wide_path.c_str());  // Could not locate, let's procreate.
			}
		}

		a_func(ini);

		(void)ini.SaveFile(wide_path.c_str());
	}

	template <class T>
	T GET_VALUE(const char* section, const char* key, T a_default, CSimpleIniA& a_ini)
	{
		std::string value;

		if (a_ini.GetValue(section, key) == nullptr) {
			a_ini.SetValue(section, key, Settings::ToString(a_default, false).c_str());
			return a_default;
		} else {
			value = a_ini.GetValue(section, key);
		}

		if (value.empty()) {
			logger::warn("[Settings] Failed to parse value from .ini file! Ensure you're using the correct format!");
			return a_default;
		}

		// A+ plus de-serialization.
		if constexpr (std::is_same_v<T, ImVec4>) {
			auto color = Settings::GetColor<T>(value);
			return (color.second ? color.first : a_default);
		} else if constexpr (std::is_same_v<T, ImVec2>) {
			auto vec = Settings::GetVec2(value);
			return (vec.second ? vec.first : a_default);
		} else if constexpr (std::is_same_v<T, std::string>) {
			return Settings::GetString(value);
		} else if constexpr (std::is_same_v<T, bool>) {
			return Settings::GetBool(value);
		} else if constexpr (std::is_same_v<T, int>) {
			return Settings::GetInt(value);
		} else if constexpr (std::is_same_v<T, uint32_t>) {
			return Settings::GetUInt(value);
		} else if constexpr (std::is_same_v<T, float>) {
			return Settings::GetFloat(value);
		} else if constexpr (std::is_same_v<T, GraphicManager::Image>) {
			return GraphicManager::GetImage(value);
		} else if constexpr (std::is_same_v<T, Language::GlyphRanges>) {
			return Language::GetGlyphRange(value);
		} else {
			logger::error("[Settings] Unhandled type passed to GET_VALUE in Menu.cpp!");
			return a_default;
		}
	}

	void Settings::FormatMasterIni(CSimpleIniA& a_ini)
	{
		a_ini.SetValue(rSections[Main], NULL, NULL, GetHeader(iHeader::MasterPresetHeader));
		a_ini.SetValue(rSections[Modules], NULL, NULL, GetHeader(iHeader::PresetMainModules));
	}

	// Master ini has no defaults, they're established here. Theme defaults are derived from def class.
	void Settings::CreateDefaultMaster()
	{
		GetIni(L"Data/Interface/Modex/Modex.ini", [](CSimpleIniA& a_ini) {
			FormatMasterIni(a_ini);

			auto& _default = Settings::GetSingleton()->def.config;

			// General
			a_ini.SetValue(rSections[Main], "ShowMenuKey", std::to_string(_default.showMenuKey).c_str(), GetComment(iComment::ConfigShowMenuKey));
			a_ini.SetValue(rSections[Main], "ShowMenuModifier", std::to_string(_default.showMenuModifier).c_str(), GetComment(iComment::ConfigShowMenuModifier));
			a_ini.SetValue(rSections[Main], "ModListSort", std::to_string(_default.modListSort).c_str(), GetComment(iComment::ConfigModListSort));
			a_ini.SetValue(rSections[Main], "UI Scale", std::to_string(_default.uiScaleVertical).c_str(), GetComment(iComment::ConfigUIScale));
			a_ini.SetValue(rSections[Main], "UI Scale Horizontal", std::to_string(_default.uiScaleHorizontal).c_str(), GetComment(iComment::ConfigUIScaleHorizontal));
			a_ini.SetValue(rSections[Main], "Fullscreen", ToString(_default.fullscreen).c_str(), GetComment(iComment::ConfigFullscreen));
			a_ini.SetValue(rSections[Main], "PauseGame", ToString(_default.pauseGame).c_str(), GetComment(iComment::ConfigPauseGame));
			a_ini.SetValue(rSections[Main], "DisableInMenu", ToString(_default.disableInMenu).c_str(), GetComment(iComment::ConfigDisableInMenu));

			// Font & Localization
			a_ini.SetValue(rSections[Main], "Language", ToString(_default.language, false).c_str(), GetComment(iComment::ConfigLanguage));
			a_ini.SetValue(rSections[Main], "GlyphRange", Language::GetGlyphName(_default.glyphRange).c_str(), GetComment(iComment::ConfigGlyphRange));
			a_ini.SetValue(rSections[Main], "GlobalFont", _default.globalFont.c_str(), GetComment(iComment::ConfigGlobalFont));
			a_ini.SetValue(rSections[Main], "GlobalFontSize", std::to_string(_default.globalFontSize).c_str(), GetComment(iComment::ConfigGlobalFontSize));

			// Modules
			a_ini.SetValue(rSections[Modules], "DefaultShow", std::to_string(_default.defaultShow).c_str(), GetComment(iComment::ConfigDefaultShow));
			a_ini.SetValue(rSections[Modules], "ShowHomeMenu", ToString(_default.showHomeMenu).c_str(), GetComment(iComment::ConfigShowHomeMenu));
			a_ini.SetValue(rSections[Modules], "ShowAddItemMenu", ToString(_default.showAddItemMenu).c_str(), GetComment(iComment::ConfigShowAddItemMenu));
			a_ini.SetValue(rSections[Modules], "ShowObjectMenu", ToString(_default.showObjectMenu).c_str(), GetComment(iComment::ConfigShowObjectMenu));
			a_ini.SetValue(rSections[Modules], "ShowNPCMenu", ToString(_default.showNPCMenu).c_str(), GetComment(iComment::ConfigShowNPCMenu));
			a_ini.SetValue(rSections[Modules], "ShowTeleportMenu", ToString(_default.showTeleportMenu).c_str(), GetComment(iComment::ConfigShowTeleportMenu));
		});
	}

	// Execute ini value assignment where necessary.
	void Settings::LoadSettings(const std::filesystem::path& a_path)
	{
		// If master ini doesn't exist this will default it to "Default"
		if (!std::filesystem::exists(a_path)) {
			logger::warn("[Settings] Master ini not found! Creating default master ini...");

			CreateDefaultMaster();
		}

		// LogExpert won't show proper unicode support!
		const std::u8string path_string = a_path.u8string();

		if (path_string.c_str()) {
			logger::info("[Settings] Loading settings from: {}", std::string(path_string.begin(), path_string.end()));
		} else {
			logger::error("[Settings] Critical Error: Failed to convert path to wide string! Please report this immediately");
		}

		// Master
		GetIni(a_path, [](CSimpleIniA& a_ini) {
			Settings::GetSingleton()->LoadMasterIni(a_ini);
		});

		// Language
		Translate::GetSingleton()->LoadLanguage(user.config.language);

		logger::info("[Settings] Settings loaded successfully.");
	}

	// Load font separately to allow GraphicManager to read language config first.
	// Also need the settings to be loaded first to load proper fonts.
	void Settings::LoadUserFontSetting()
	{
		GetIni(L"Data/Interface/Modex/Modex.ini", [](CSimpleIniA& a_ini) {
			Settings::GetSingleton()->user.config.globalFont = GET_VALUE<std::string>(rSections[Main], "GlobalFont", Settings::GetSingleton()->def.config.globalFont, a_ini);
		});

		logger::info("[Settings] Loaded user font settings: {}", user.config.globalFont);
	}

	void Settings::SaveSettings()
	{
		GetIni(L"Data/Interface/Modex/Modex.ini", [](CSimpleIniA& a_ini) {
			FormatMasterIni(a_ini);

			// General
			a_ini.SetValue(rSections[Main], "ShowMenuKey", std::to_string(Settings::GetSingleton()->user.config.showMenuKey).c_str());
			a_ini.SetValue(rSections[Main], "ShowMenuModifier", std::to_string(Settings::GetSingleton()->user.config.showMenuModifier).c_str());
			a_ini.SetValue(rSections[Main], "ModListSort", std::to_string(Settings::GetSingleton()->user.config.modListSort).c_str());
			a_ini.SetValue(rSections[Main], "UI Scale", std::to_string(Settings::GetSingleton()->user.config.uiScaleVertical).c_str());
			a_ini.SetValue(rSections[Main], "UI Scale Horizontal", std::to_string(Settings::GetSingleton()->user.config.uiScaleHorizontal).c_str());
			a_ini.SetValue(rSections[Main], "Fullscreen", ToString(Settings::GetSingleton()->user.config.fullscreen).c_str());
			a_ini.SetValue(rSections[Main], "PauseGame", ToString(Settings::GetSingleton()->user.config.pauseGame).c_str());
			a_ini.SetValue(rSections[Main], "DisableInMenu", ToString(Settings::GetSingleton()->user.config.disableInMenu).c_str());

			// Font & Localization
			a_ini.SetValue(rSections[Main], "Language", ToString(Settings::GetSingleton()->user.config.language).c_str());
			a_ini.SetValue(rSections[Main], "GlyphRange", Language::GetGlyphName(Settings::GetSingleton()->user.config.glyphRange).c_str());
			a_ini.SetValue(rSections[Main], "GlobalFont", Settings::GetSingleton()->user.config.globalFont.c_str());
			a_ini.SetValue(rSections[Main], "GlobalFontSize", std::to_string(Settings::GetSingleton()->user.config.globalFontSize).c_str());

			// Modules
			a_ini.SetValue(rSections[Modules], "DefaultShow", std::to_string(Settings::GetSingleton()->user.config.defaultShow).c_str());
			a_ini.SetValue(rSections[Modules], "ShowHomeMenu", ToString(Settings::GetSingleton()->user.config.showHomeMenu).c_str());
			a_ini.SetValue(rSections[Modules], "ShowAddItemMenu", ToString(Settings::GetSingleton()->user.config.showAddItemMenu).c_str());
			a_ini.SetValue(rSections[Modules], "ShowObjectMenu", ToString(Settings::GetSingleton()->user.config.showObjectMenu).c_str());
			a_ini.SetValue(rSections[Modules], "ShowNPCMenu", ToString(Settings::GetSingleton()->user.config.showNPCMenu).c_str());
			a_ini.SetValue(rSections[Modules], "ShowTeleportMenu", ToString(Settings::GetSingleton()->user.config.showTeleportMenu).c_str());
		});

		// This function body is called when a change is made, so send updates out:
		InputManager::GetSingleton()->UpdateSettings();
	}

	// This is executed within the scope of Modex.ini
	// Loads the preset specified in master ini. Then loads theme from there.
	// Executed within the scope of the Modex.ini file.
	void Settings::LoadMasterIni(CSimpleIniA& a_ini)
	{
		auto& _default = def.config;

		user.config.showMenuKey = GET_VALUE<uint32_t>(rSections[Main], "ShowMenuKey", _default.showMenuKey, a_ini);
		user.config.showMenuModifier = GET_VALUE<uint32_t>(rSections[Main], "ShowMenuModifier", _default.showMenuModifier, a_ini);
		user.config.modListSort = GET_VALUE<int>(rSections[Main], "ModListSort", _default.modListSort, a_ini);
		user.config.uiScaleVertical = GET_VALUE<int>(rSections[Main], "UI Scale", _default.uiScaleVertical, a_ini);
		user.config.uiScaleHorizontal = GET_VALUE<int>(rSections[Main], "UI Scale Horizontal", _default.uiScaleHorizontal, a_ini);
		user.config.fullscreen = GET_VALUE<bool>(rSections[Main], "Fullscreen", _default.fullscreen, a_ini);
		user.config.pauseGame = GET_VALUE<bool>(rSections[Main], "PauseGame", _default.pauseGame, a_ini);
		user.config.disableInMenu = GET_VALUE<bool>(rSections[Main], "DisableInMenu", _default.disableInMenu, a_ini);

		user.config.language = GET_VALUE<std::string>(rSections[Main], "Language", _default.language, a_ini);
		user.config.glyphRange = GET_VALUE<Language::GlyphRanges>(rSections[Main], "GlyphRange", _default.glyphRange, a_ini);
		user.config.globalFont = GET_VALUE<std::string>(rSections[Main], "GlobalFont", _default.globalFont, a_ini);
		user.config.globalFontSize = GET_VALUE<float>(rSections[Main], "GlobalFontSize", _default.globalFontSize, a_ini);

		user.config.defaultShow = GET_VALUE<int>(rSections[Modules], "DefaultShow", _default.defaultShow, a_ini);
		user.config.showHomeMenu = GET_VALUE<bool>(rSections[Modules], "ShowHomeMenu", _default.showHomeMenu, a_ini);
		user.config.showAddItemMenu = GET_VALUE<bool>(rSections[Modules], "ShowAddItemMenu", _default.showAddItemMenu, a_ini);
		user.config.showObjectMenu = GET_VALUE<bool>(rSections[Modules], "ShowObjectMenu", _default.showObjectMenu, a_ini);
		user.config.showNPCMenu = GET_VALUE<bool>(rSections[Modules], "ShowNPCMenu", _default.showNPCMenu, a_ini);
		user.config.showTeleportMenu = GET_VALUE<bool>(rSections[Modules], "ShowTeleportMenu", _default.showTeleportMenu, a_ini);
	}
}