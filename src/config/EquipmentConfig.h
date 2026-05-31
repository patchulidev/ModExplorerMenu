#pragma once

#include "data/BaseObject.h"
#include "config/ConfigManager.h"

namespace Modex
{
	static const std::filesystem::path EQUIPMENT_JSON_PATH = 
	std::filesystem::path("data") / "interface" / "modex" / "user" / "kits";

	class EquipmentConfig
	{
	private:
		std::unordered_map<std::string, Kit> m_cache; // key, kit
		std::set<std::string> m_knownTags;            // runtime tag registry, rebuilt on Load/Save/Delete

		static std::optional<Kit> LoadKit(const std::filesystem::path& a_fullPath);
		static void RebuildKnownTags();

	public:
		static inline EquipmentConfig* GetSingleton()
		{
			static EquipmentConfig singleton;
			return std::addressof(singleton);
		}

		static bool Load();
		static bool ValidateKeyName(const std::string& a_keyName);

		// Creation / mutation functions return an empty Kit on failure.
		// Use `if (result)` or `if (!result.empty())` to check success —
		// Kit::operator bool() and Kit::empty() are already defined.
		static Kit CopyKit(const Kit& a_kit);

		static void DeleteKit(const Kit& a_kit);
		static bool SaveKit(const Kit& a_kit);

		static Kit RenameKit(Kit& a_kit, std::string a_new_name);
		static Kit CreateKit(const std::string& a_name);
		static Kit CreateKitFromReference(const std::string& a_name, RE::TESObjectREFR* a_reference, bool a_wornOnly = false);

		static std::vector<BaseObject> 	GetItems(const Kit& a_kit);
		static Kit* 					KitLookup(const std::string& a_key);
		static std::shared_ptr<KitItem>  CreateKitItem(const BaseObject& a_object);
		static std::shared_ptr<KitSpell> CreateKitSpell(const BaseObject& a_object);
		static bool                     CreateKitFromOutfit(const std::string& a_name, RE::BGSOutfit* a_outfit, uint16_t a_level = 0);

		static std::vector<std::string> GetEquipmentListSortedKeys();
		static std::vector<std::string> GetEquipmentListSortedTails();
		static std::unordered_map<std::string, Kit>& GetEquipmentList();

		// Runtime tag registry derived from all loaded kits' CSV collection strings.
		static std::vector<std::string> GetKnownTags();
		static int DeleteTagFromAllKits(const std::string& a_tag);

		static Kit* At(const std::string& a_key) {
			auto it = GetSingleton()->m_cache.find(a_key);
			if (it != GetSingleton()->m_cache.end()) {
				return &it->second;
			}
			return nullptr;
		}
	};
}
