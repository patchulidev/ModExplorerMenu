#pragma once

#include "ConfigManager.h"
#include "data/BaseObject.h"

namespace Modex
{
	static const std::filesystem::path USERDATA_JSON_PATH =
	std::filesystem::path("data") / "interface" / "modex" / "user" / "userdata.json";

	class UserData
	{
	private:
		struct RecentData {
			std::vector<std::string> items;
			size_t maxSize;
		};

		struct FavoriteData {
			std::unordered_set<std::string> items;
		};

		static inline RecentData m_recent{ {}, 50 };
		static inline FavoriteData m_favorites;

		static inline ConfigManager m_userDataConfig;

	public:
		UserData() = delete;
		UserData(const UserData&) = delete;
		UserData& operator=(const UserData&) = delete;

		static void Save();
		static void Load();

		static void AddRecent(const std::unique_ptr<BaseObject>& a_item);
		static std::vector<std::string> GetRecentAsVector() { return m_recent.items; }
		static RecentData& GetRecent() { return m_recent; }

		static void AddFavorite(const std::unique_ptr<BaseObject>& a_item);
		static FavoriteData& GetFavorites() { return m_favorites; }

		static nlohmann::json& GetData() { return m_userDataConfig.GetData(); }

		template<typename T>
		static void Set(const std::string& a_key, const T& a_value)
		{
			m_userDataConfig.Set<T>(a_key, a_value);
		}

		template<typename T>
		static T Get(const std::string& a_key, const T& a_default = T())
		{
			return m_userDataConfig.Get<T>(a_key, a_default);
		}
	};
}
