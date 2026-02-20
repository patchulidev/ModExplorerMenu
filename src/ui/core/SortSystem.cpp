#include "SortSystem.h"

namespace Modex
{
	bool SortSystem::Load(bool a_create)
	{
		ASSERT_MSG(a_create && m_file_path.empty(), "SortSystem::Load() called before setting file path!");

		if (!ConfigManager::Load(a_create)) 
			return m_initialized;

		if (m_data.contains("SortProperty")) {
			const auto& key_array = m_data["SortProperty"];

			for (const auto& key_entry : key_array) {
				std::string key_name = key_entry.get<std::string>();
				auto filter = FilterProperty::FromString(key_name);

				if (filter.has_value()) {
					AddAvailableFilter(std::move(filter.value()));
				}
			}

			return m_initialized = true;
		}

		ASSERT_MSG(a_create, "SortSystem::Load() - No SortProperty found in JSON. File: {}", m_file_path.string());
		return m_initialized = !a_create;
	}

	nlohmann::json SortSystem::SerializeState() const
	{
		nlohmann::json j;

		j["PrimarySortProperty"] = m_primarySortFilter.ToString();
		j["SecondarySortProperty"] = m_secondarySortFilter.ToString();
		j["Ascending"] = m_ascending;
		j["UsePrimary"] = m_usePrimary;

		return j;
	}

	void SortSystem::DeserializeState(const nlohmann::json& a_state)
	{
		if (a_state.contains("PrimarySortProperty") && a_state["PrimarySortProperty"].is_string()) {
			std::string prop_str = a_state["PrimarySortProperty"].get<std::string>();
			auto filter = FilterProperty::FromString(prop_str);

			if (filter.has_value()) {
				m_primarySortFilter = filter.value();
			} else {
				m_primarySortFilter = FilterProperty(PropertyType::kNone);
			}
		}

		if (a_state.contains("SecondarySortProperty") && a_state["SecondarySortProperty"].is_string()) {
			std::string prop_str = a_state["SecondarySortProperty"].get<std::string>();
			auto filter = FilterProperty::FromString(prop_str);

			if (filter.has_value()) {
				m_secondarySortFilter = filter.value();
			} else {
				m_secondarySortFilter = FilterProperty(PropertyType::kNone);
			}
		}

		if (a_state.contains("Ascending") && a_state["Ascending"].is_boolean()) {
			m_ascending = a_state["Ascending"].get<bool>();
		}

		if (a_state.contains("UsePrimary") && a_state["UsePrimary"].is_boolean()) {
			m_usePrimary = a_state["UsePrimary"].get<bool>();
		}
	}

	// Need special definitions for properties of int, float, or none string types.
	bool SortSystem::SortFn(const std::unique_ptr<BaseObject>& a_lhs, const std::unique_ptr<BaseObject>& a_rhs) const
    {
		const auto property = m_usePrimary ? m_primarySortFilter.GetPropertyType() : m_secondarySortFilter.GetPropertyType();
		const auto lhs_value = a_lhs->GetPropertyByValue(property);
		const auto rhs_value = a_rhs->GetPropertyByValue(property);

		// Check if properties are empty/invalid - these should always go to the bottom
		const bool lhs_empty = lhs_value.empty() || lhs_value == "0";
		const bool rhs_empty = rhs_value.empty() || rhs_value == "0";

		// Items without the property always go to the bottom
		if (lhs_empty && !rhs_empty) return false;  // lhs goes after rhs
		if (!lhs_empty && rhs_empty) return true;   // lhs goes before rhs
		if (lhs_empty && rhs_empty) return false;   // both empty, maintain order

		int delta = 0;

		// NOTE: Better static analysis with switch-case for all property types.

		switch (property) {
			case PropertyType::kNone:
			case PropertyType::kArmor:
			case PropertyType::kBook:
			case PropertyType::kKeyMaster:
			case PropertyType::kIngredient:
			case PropertyType::kAlchemy:
			case PropertyType::kAmmo:
			case PropertyType::kScroll:
			case PropertyType::kMisc:
			case PropertyType::kWeapon:
			case PropertyType::kNPC:
			case PropertyType::kContainer:
			case PropertyType::kStatic:
			case PropertyType::kLight:
			case PropertyType::kDoor:
			case PropertyType::kFurniture:
			case PropertyType::kActivator:
			case PropertyType::kTree:
			case PropertyType::kFlora:
			case PropertyType::kPlugin:
			case PropertyType::kFormID:
			case PropertyType::kName:
			case PropertyType::kEditorID:
			case PropertyType::kFormType:
			case PropertyType::kWeaponType:
			case PropertyType::kWeaponSkill:
			case PropertyType::kArmorSlot:
			case PropertyType::kArmorType:
			case PropertyType::kReferenceID:
			case PropertyType::kRace:
			case PropertyType::kGender:
			case PropertyType::kClass:
			case PropertyType::kDefaultOutfit:
			case PropertyType::kSleepOutfit:
			case PropertyType::kKeywordList:
			case PropertyType::kFactionList:
			case PropertyType::kSpellList:
			case PropertyType::kSpellType:
			case PropertyType::kSpellCastType:
			case PropertyType::kSpellDelivery:
			case PropertyType::kTomeSpell:
			case PropertyType::kTomeSkill:
			case PropertyType::kImGuiSeparator:
			case PropertyType::kCell:
			case PropertyType::kLand:
			case PropertyType::kTotal: {
				delta = rhs_value.compare(lhs_value);
				break; // All direct string comparions.
			}
			case PropertyType::kIsWeapon: {
				const auto lhs_is_weapon = (lhs_value == "true") ? 1 : 0;
				const auto rhs_is_weapon = (rhs_value == "true") ? 1 : 0;
				delta = lhs_is_weapon - rhs_is_weapon;
				break;
			} 
			case PropertyType::kIsArmor: {
				const auto lhs_is_armor = (lhs_value == "true") ? 1 : 0;
				const auto rhs_is_armor = (rhs_value == "true") ? 1 : 0;
				delta = lhs_is_armor - rhs_is_armor;
				break;
			}
			case PropertyType::kKitItemCount: {
				const auto lhs_count = std::stoi(lhs_value);
				const auto rhs_count = std::stoi(rhs_value);
				delta = lhs_count - rhs_count;
				break;
			}
			case PropertyType::kWeaponDamage: {
				const auto lhs_damage = std::stoi(lhs_value);
				const auto rhs_damage = std::stoi(rhs_value);
				delta = lhs_damage - rhs_damage;
				break;
			}
			case PropertyType::kWeaponSpeed: {
				const auto lhs_speed = std::stof(lhs_value);
				const auto rhs_speed = std::stof(rhs_value);
				delta = (lhs_speed > rhs_speed) ? 1 : (lhs_speed < rhs_speed) ? -1 : 0;
				break;
			}
			case PropertyType::kWeaponCriticalDamage: {
				const auto lhs_crit = std::stof(lhs_value);
				const auto rhs_crit = std::stof(rhs_value);
				delta = (lhs_crit > rhs_crit) ? 1 : (lhs_crit < rhs_crit) ? -1 : 0;
				break;
			}
			case PropertyType::kWeaponDamagePerSecond: {
				const auto lhs_dps = std::stof(lhs_value);
				const auto rhs_dps = std::stof(rhs_value);
				delta = (lhs_dps > rhs_dps) ? 1 : (lhs_dps < rhs_dps) ? -1 : 0;
				break;
			}
			case PropertyType::kWeaponRange: {
				const auto lhs_range = std::stof(lhs_value);
				const auto rhs_range = std::stof(rhs_value);
				delta = (lhs_range > rhs_range) ? 1 : (lhs_range < rhs_range) ? -1 : 0;
				break;
			}
			case PropertyType::kWeaponStagger: {
				const auto lhs_stagger = std::stof(lhs_value);
				const auto rhs_stagger = std::stof(rhs_value);
				delta = (lhs_stagger > rhs_stagger) ? 1 : (lhs_stagger < rhs_stagger) ? -1 : 0;
				break;
			}
			case PropertyType::kArmorRating: {
				const auto lhs_armor = std::stoi(lhs_value);
				const auto rhs_armor = std::stoi(rhs_value);
				delta = lhs_armor - rhs_armor;
				break;
			}
			case PropertyType::kPlayable: {
				const auto lhs_playable = (lhs_value == "true") ? 1 : 0;
				const auto rhs_playable = (rhs_value == "true") ? 1 : 0;
				delta = lhs_playable - rhs_playable;
				break;
			}
			case PropertyType::kEnchanted: {
				const auto lhs_enchanted = (lhs_value == "true") ? 1 : 0;
				const auto rhs_enchanted = (rhs_value == "true") ? 1 : 0;
				delta = lhs_enchanted - rhs_enchanted;
				break;
			}
			case PropertyType::kGoldValue: {
				const auto lhs_gold = std::stoi(lhs_value);
				const auto rhs_gold = std::stoi(rhs_value);
				delta = lhs_gold - rhs_gold;
				break;
			}
			case PropertyType::kCarryWeight: {
				const auto lhs_weight = std::stoi(lhs_value);
				const auto rhs_weight = std::stoi(rhs_value);
				delta = lhs_weight - rhs_weight;
				break;
			}
			case PropertyType::kUnique: {
				const auto lhs_unique = (lhs_value == "true") ? 1 : 0;
				const auto rhs_unique = (rhs_value == "true") ? 1 : 0;
				delta = lhs_unique - rhs_unique;
				break;
			}
			case PropertyType::kEssential: {
				const auto lhs_essential = (lhs_value == "true") ? 1 : 0;
				const auto rhs_essential = (rhs_value == "true") ? 1 : 0;
				delta = lhs_essential - rhs_essential;
				break;
			}
			case PropertyType::kDisabled: {
				const auto lhs_disabled = (lhs_value == "true") ? 1 : 0;
				const auto rhs_disabled = (rhs_value == "true") ? 1 : 0;
				delta = lhs_disabled - rhs_disabled;
				break;
			}
			case PropertyType::kHealth: {
				const auto lhs_health = std::stoi(lhs_value);
				const auto rhs_health = std::stoi(rhs_value);
				delta = lhs_health - rhs_health;
				break;
			}
			case PropertyType::kMagicka: {
				const auto lhs_magicka = std::stoi(lhs_value);
				const auto rhs_magicka = std::stoi(rhs_value);
				delta = lhs_magicka - rhs_magicka;
				break;
			}
			case PropertyType::kStamina: {
				const auto lhs_stamina = std::stoi(lhs_value);
				const auto rhs_stamina = std::stoi(rhs_value);
				delta = lhs_stamina - rhs_stamina;
				break;
			}
			case PropertyType::kLevel: {
				const auto lhs_level = std::stoi(lhs_value);
				const auto rhs_level = std::stoi(rhs_value);
				delta = lhs_level - rhs_level;
				break;
			}
			case PropertyType::kSpellCost: {
				const auto lhs_cost = std::stoi(lhs_value);
				const auto rhs_cost = std::stoi(rhs_value);
				delta = lhs_cost - rhs_cost;
				break;
			}
		}

		// Both have valid values, sort normally

		if (delta < 0)
			return m_ascending ? false : true;
		if (delta > 0)
			return m_ascending ? true : false;

		return false;
	}
}
