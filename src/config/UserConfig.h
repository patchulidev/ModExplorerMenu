#pragma once

#include "ConfigManager.h"

namespace Modex
{
	inline const std::filesystem::path USERCONFIG_JSON_PATH = 
	std::filesystem::path("data") / "interface" / "modex" / "user" / "settings.json";

	class UserConfig : private ConfigManager
	{
	public:
		static inline UserConfig* GetSingleton()
		{
			static UserConfig singleton;
			return std::addressof(singleton);
		}

		UserConfig() {
			SetFilePath(USERCONFIG_JSON_PATH);
		}

		void LoadSettings();
		void LoadFontSettings();
		void SaveSettings();

		void CreateDefaultMaster();
		static void FormatMasterIni(CSimpleIniA& a_ini);

		struct UserSettings
		{
			uint32_t showMenuKey 		= 211;
			uint32_t showMenuModifier 	= 0;

			int modListSort 		= 0;
			int uiScaleVertical 		= 100;
			int uiScaleHorizontal 		= 100;
			int globalFontSize 		= 16;
			int logLevel 			= 1;

			bool fullscreen 		= false;
			bool pauseGame 			= false;
			bool disableInMenu 		= false;
			bool disableAlt			= false;
			bool welcomeBanner 		= true;
			bool smoothScroll 		= true;

			std::string language 		= "English";
			std::string theme 		= "default";
			std::string globalFont 		= "Ubuntu-Regular";

			ImVec2 screenScaleRatio		= ImVec2(1.0f, 1.0f);
		};

		UserSettings _default;
		UserSettings user;

		[[nodiscard]] static UserSettings& Get() { return GetSingleton()->user; };
		[[nodiscard]] static UserSettings& GetDefault() { return GetSingleton()->_default; };

	private:
	};
}
