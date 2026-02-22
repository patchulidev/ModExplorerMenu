#pragma once

#include "ConfigManager.h"
#include "data/BaseObject.h"

namespace Modex
{
	static const std::filesystem::path USERDATA_JSON_PATH =
	std::filesystem::path("data") / "interface" / "modex" / "user" / "userdata.json";

	enum class ModexActionType : uint32_t
	{
		AddItem = 0,
		RemoveItem,
		EquipItem,
		PlaceAtMe,
		ReadBook,
		ResetInventory,
		ClearInventory,
		GotoReference,
		BringReference,
		KillActor,
		ReviveActor,
		EnableReference,
		DisableReference,
		SaveKit,
		CreateKit,
		DeleteKit,
		RenameKit,
		CopyKit,
		CenterOnCell,
		Favorited,
		Total,
	};

	class UserData
	{
	private:
		struct RecentData {
			std::vector<std::string> items;
			size_t maxSize;
		};

		struct FavoriteData {
			std::vector<std::string> items;
		};

		static inline RecentData m_recent{ {}, 50 };
		static inline FavoriteData m_favorites{ {} };

		static inline ConfigManager m_userDataConfig;

	public:
		UserData() = delete;
		UserData(const UserData&) = delete;
		UserData& operator=(const UserData&) = delete;

		static void Save();
		static void Load();

		static void SendEvent(ModexActionType a_actionType, const std::unique_ptr<BaseObject>& a_item);
		static void SendEvent(ModexActionType a_actionType, const std::string& a_editorid = "");

		// static void AddRecent(const std::unique_ptr<BaseObject>& a_item);
		static std::vector<std::string> GetRecentAsVector() { return m_recent.items; }
		static RecentData& GetRecent() { return m_recent; }

		static void AddFavorite(const std::unique_ptr<BaseObject>& a_item);
		static std::vector<std::string> GetFavoritesAsVector() { return m_favorites.items; }
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
