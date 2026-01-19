#pragma once

#include "ConfigManager.h"

namespace Modex
{
	static const std::filesystem::path USERDATA_JSON_PATH =
	std::filesystem::path("data") / "interface" / "modex" / "user" / "userdata.json";

	static const std::filesystem::path FAVORITES_JSON_PATH =
	std::filesystem::path("data") / "interface" / "modex" / "user" / "favorites.json";

	static const std::filesystem::path RECENT_JSON_PATH =
	std::filesystem::path("data") / "interface" / "modex" / "user" / "recent.json";

	class UserData
	{
	private:
		ConfigManager m_userDataConfig;
		ConfigManager m_favoriteConfig;
		ConfigManager m_recentConfig;

	public:
		UserData();

		static UserData* GetSingleton()
		{
			static UserData singleton;
			return &singleton;
		}

		static void Save();
		static void LoadAll();

		[[nodiscard]] static ConfigManager& User() { return GetSingleton()->m_userDataConfig; }
		[[nodiscard]] static ConfigManager& Favorite() { return GetSingleton()->m_favoriteConfig; }
		[[nodiscard]] static ConfigManager& Recent() { return GetSingleton()->m_recentConfig; }
	};
}
