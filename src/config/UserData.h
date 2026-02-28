#pragma once

#include "ConfigManager.h"
#include "data/BaseObject.h"

// TODO: chore: move new juvenile event system into its own implementation.

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
		Favorited,
		Unfavorited,
		SetDefaultOutfit,
		SetSleepOutfit,
		EquipOutfit,
		Total,
	};

	class UserData
	{
	private:
		struct RecentData {
			std::vector<SerializedObject> items;
			size_t maxSize;
		};

		struct FavoriteData {
			std::vector<SerializedObject> items;
		};

		static inline RecentData m_recent{ {}, 50 };
		static inline FavoriteData m_favorites{ {} };

		static inline ConfigManager m_userDataConfig;

	public:
		UserData() = delete;
		UserData(const UserData&) = delete;
		UserData& operator=(const UserData&) = delete;

		// baseclass
		static void Save();
		static void Load();

		// event system
		static void SendEvent(ModexActionType a_actionType, const std::unique_ptr<BaseObject>& a_item);
		static void SendEvent(ModexActionType a_actionType, const std::string& a_text, Ownership a_owner);
		static void SendEvent(ModexActionType a_actionType, RE::FormID a_refid, Ownership a_owner);

		// recent
		static std::vector<SerializedObject> GetRecentAsVector() { return m_recent.items; }
		static RecentData& GetRecent() { return m_recent; }

		// favorites
		static std::vector<SerializedObject> GetFavoritesAsVector() { return m_favorites.items; }
		static FavoriteData& GetFavorites() { return m_favorites; }
		static bool IsFavorited(const std::string& a_editorid);
		static bool IsFavorited(RE::FormID a_refid);

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

