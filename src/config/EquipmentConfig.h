#pragma once

#include "data/BaseObject.h"
#include "config/ConfigManager.h"

namespace Modex
{
	static const std::filesystem::path EQUIPMENT_JSON_PATH = 
	std::filesystem::path("data") / "interface" / "modex" / "user" / "equipment";

	class EquipmentConfig
	{
	private:
		std::unordered_map<std::string, KitData> m_cache; // filepath, data

	public:
		static inline EquipmentConfig* GetSingleton()
		{
			static EquipmentConfig singleton;
			return std::addressof(singleton);
		}

		static bool Load();
		static bool ValidateKeyName(const std::string& a_keyName);

		static std::optional<Kit> LoadKit(const KitData& a_metadata);
		static std::optional<Kit> LoadKit(const std::filesystem::path& a_fullPath);
		static std::optional<Kit> CopyKit(const Kit& a_kit);

		static void DeleteKit(const Kit& a_kit);
		static bool SaveKit(const Kit& a_kit);

		static std::optional<Kit> RenameKit(Kit& a_kit, std::string a_new_name);
		static std::optional<Kit> CreateKit(const std::filesystem::path& a_relativePath);
		
		static std::vector<BaseObject> 	GetItems(const Kit& a_kit);
		static std::vector<BaseObject> 	GetItems(const KitData& a_metadata);
		static std::optional<Kit> 		KitLookup(const std::string& a_name);
		static std::shared_ptr<KitItem> CreateKitItem(const BaseObject& a_object);

		static std::vector<std::string> GetEquipmentListSortedKeys();
		static std::vector<std::string> GetEquipmentListSortedTails();
		static std::unordered_map<std::string, KitData>& GetEquipmentList();

		static KitData At(const std::string& a_key) {
			auto it = GetSingleton()->m_cache.find(a_key);

			if (it != GetSingleton()->m_cache.end()) {
				return it->second;
			} else {
				return KitData();
			}
		}
	};
}
