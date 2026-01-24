#pragma once

#include "ConfigManager.h"

namespace Modex
{
	inline const std::filesystem::path USERCONFIG_JSON_PATH = 
	std::filesystem::path("data") / "interface" / "modex" / "user" / "settings.json";

	class UserConfig : private ConfigManager
	{
	private:
		void CreateDefaultMaster();
		static void FormatMasterIni(CSimpleIniA& a_ini);

	public:
		static inline UserConfig* GetSingleton()
		{
			static UserConfig singleton;
			return std::addressof(singleton);
		}

		UserConfig();

		void LoadSettings();
		void LoadFontSettings();
		void SaveSettings();

		struct UserSettings
		{
			uint32_t showMenuKey 		= 211;
			uint32_t showMenuModifier 	= 0;
			uint32_t modListSort 		= 0;
			uint32_t logLevel 			= 1;

			int uiScaleVertical 	= 100;
			int uiScaleHorizontal 	= 100;
			int globalFontSize 		= 16;

			bool fullscreen 		= false;
			bool pauseGame 			= false;
			bool disableSplash		= false;
			bool disableInMenu 		= false;
			bool disableAlt			= false;
			bool welcomeBanner 		= true;
			bool smoothScroll 		= true;
			bool basePlugin			= true;

			std::string language 		= "English";
			std::string theme 			= "default";
			std::string globalFont 		= "Ubuntu-Regular";

			ImVec2 screenScaleRatio		= ImVec2(1.0f, 1.0f);
		};

		UserSettings _default;
		UserSettings user;

		[[nodiscard]] static int32_t GetCompileIndex() { return GetSingleton()->user.basePlugin ? 0 : -1; };
		[[nodiscard]] static UserSettings& Get() { return GetSingleton()->user; };
		[[nodiscard]] static UserSettings& GetDefault() { return GetSingleton()->_default; };
		[[nodiscard]] static std::array<uint32_t, 2> GetShowMenuKeys();
		[[nodiscard]] static std::string GetShowMenuKeysAsText();
	};
}
