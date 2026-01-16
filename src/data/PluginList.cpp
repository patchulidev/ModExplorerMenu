#pragma once

#include "Data.h"
#include "config/BlacklistConfig.h"

namespace Modex
{
	// Sort Function for Case Insensitive comparing used for Plugin Lists.
	// Doing additional checks to ensure the TESFile* pointer is not null.
	// Due to reports of random crashing. #19 for example.
	bool CaseInsensitiveCompareTESFile(const RE::TESFile* a, const RE::TESFile* b)
	{
		std::string filenameA = ValidateTESFileName(a);
		std::string filenameB = ValidateTESFileName(b);

		return std::lexicographical_compare(
			filenameA.begin(), filenameA.end(),
			filenameB.begin(), filenameB.end(),
			[](unsigned char c1, unsigned char c2) {
				return std::tolower(c1) < std::tolower(c2);
			});
	}

	// Sort Function for compileIndex aka Load Order used for Plugin Lists.
	bool CompileIndexCompareTESFileAsc(const RE::TESFile* a, const RE::TESFile* b)
	{
		if (!a or !b) {
			return false;
		}

		return a->GetCombinedIndex() < b->GetCombinedIndex();
	}

	bool CompileIndexCompareTESFileDesc(const RE::TESFile* a, const RE::TESFile* b)
	{
		if (!a or !b) {
			return false;
		}

		return a->GetCombinedIndex() > b->GetCombinedIndex();
	}

	// Returns a copy of the specified plugin list containing TESFile* pointers. as an unordered_set.
	// This is primarily used as a quicker lookup than GetModulePluginListSorted (more expensive).
	//
	// @param a_type - The type of plugin list to return.
	// @return std::unordered_set<const RE::TESFile*> - A set of TESFile* pointers.
	std::unordered_set<const RE::TESFile*> Data::GetModulePluginList(PLUGIN_TYPE a_type)
	{
		switch (a_type) {
		case PLUGIN_TYPE::ITEM:
			return m_itemModList;
		case PLUGIN_TYPE::NPC:
			return m_npcModList;
		case PLUGIN_TYPE::OBJECT:
			return m_staticModList;
		case PLUGIN_TYPE::CELL:
			return m_cellModList;
		case PLUGIN_TYPE::ALL:
			return m_modList;
		default:
			PrettyLog::Error("Invalid PLUGIN_TYPE argument passed to GetModulePluginList");
			return m_modList;
		}
	}

	// Returns a copy of the specified plugin list as a sorted vector.
	//
	// @param a_type - The type of plugin list to return.
	// @return std::vector<const RE::TESFile*> - A list of alphabetically sorted TESFile* pointers.
	std::vector<const RE::TESFile*> Data::GetModulePluginListSorted(PLUGIN_TYPE a_type, SORT_TYPE a_sortType)
	{
		std::vector<const RE::TESFile*> copy;

		auto safeCopy = [&copy](const std::unordered_set<const RE::TESFile*>& a_set) {
			// copy.reserve(std::ssize(a_set));
			for (const auto& mod : a_set) {
				if (mod != nullptr) {
					copy.push_back(mod);
				}
			}
		};

		switch (a_type) {
		case PLUGIN_TYPE::ITEM:
			safeCopy(m_itemModList);
			break;
		case PLUGIN_TYPE::NPC:
			safeCopy(m_npcModList);
			break;
		case PLUGIN_TYPE::OBJECT:
			safeCopy(m_staticModList);
			break;
		case PLUGIN_TYPE::CELL:
			safeCopy(m_cellModList);
			break;
		case PLUGIN_TYPE::ALL:
			safeCopy(m_modList);
			break;
		default:
			PrettyLog::Error("Invalid PLUGIN_TYPE argument passed to GetModulePluginListSorted");
			safeCopy(m_modList);
			break;
		}

		if (a_sortType == SORT_TYPE::ALPHABETICAL) {
			std::sort(copy.begin(), copy.end(), CaseInsensitiveCompareTESFile);
		} else if (a_sortType == SORT_TYPE::COMPILEINDEX_ASC) {
			std::sort(copy.begin(), copy.end(), CompileIndexCompareTESFileAsc);
		} else if (a_sortType == SORT_TYPE::COMPILEINDEX_DESC) {
			std::sort(copy.begin(), copy.end(), CompileIndexCompareTESFileDesc);
		} else {
			PrettyLog::Error("Invalid SORT_TYPE argument passed to GetModulePluginListSorted");
		}

		return copy;
	}

	// Returns an alphabetically sorted vector of plugin names.
	//
	// @return std::vector<std::string> - A vector of plugin names.
	std::vector<std::string> Data::GetSortedListOfPluginNames()
	{
		std::vector<std::string> modList;

		for (auto& mod : m_modListSorted) {
			modList.push_back(mod);
		}

		return modList;
	}

	bool Data::IsFormTypeInPlugin(const RE::TESFile* a_plugin, RE::FormType a_formType)
	{
		if (a_plugin == nullptr) {
			return false;
		}

		switch (a_formType) {
		case RE::FormType::Armor:
			return m_itemListModFormTypeMap[a_plugin].armor;
		case RE::FormType::Book:
			return m_itemListModFormTypeMap[a_plugin].book;
		case RE::FormType::Weapon:
			return m_itemListModFormTypeMap[a_plugin].weapon;
		case RE::FormType::Misc:
			return m_itemListModFormTypeMap[a_plugin].misc;
		case RE::FormType::KeyMaster:
			return m_itemListModFormTypeMap[a_plugin].key;
		case RE::FormType::Ammo:
			return m_itemListModFormTypeMap[a_plugin].ammo;
		case RE::FormType::AlchemyItem:
			return m_itemListModFormTypeMap[a_plugin].alchemy;
		case RE::FormType::Ingredient:
			return m_itemListModFormTypeMap[a_plugin].ingredient;
		case RE::FormType::Scroll:
			return m_itemListModFormTypeMap[a_plugin].scroll;
		case RE::FormType::SoulGem:
			return m_itemListModFormTypeMap[a_plugin].misc;
		case RE::FormType::Static:
			return m_itemListModFormTypeMap[a_plugin].staticObject;
		case RE::FormType::Tree:
			return m_itemListModFormTypeMap[a_plugin].tree;
		case RE::FormType::Activator:
			return m_itemListModFormTypeMap[a_plugin].activator;
		case RE::FormType::Container:
			return m_itemListModFormTypeMap[a_plugin].container;
		case RE::FormType::Door:
			return m_itemListModFormTypeMap[a_plugin].door;
		case RE::FormType::Light:
			return m_itemListModFormTypeMap[a_plugin].light;
		case RE::FormType::Furniture:
			return m_itemListModFormTypeMap[a_plugin].furniture;
		case RE::FormType::NPC:
			return m_itemListModFormTypeMap[a_plugin].npc;
		case RE::FormType::Cell:
			return m_itemListModFormTypeMap[a_plugin].cell;
		default:
			PrettyLog::Error("Invalid FormType argument passed to IsFormTypeInPlugin");
			return false;
		}
	}

	// Returns an alphabetically sorted vector of plugin names that are cross-compared to the supplied
	// ItemFilterType filter selected. Primarily used to populate the "Filter By Pluginlist" list in the
	// table-view modules.
	//
	// @param a_selectedFilter - The selected filter to cross-compare against. (`RE::FormType`)
	// @param a_sortType - The sort type to use when sorting the list. (`SORT_TYPE`)
	// @param a_primaryFilter - The primary filter to use when selecting the list. (`RE::FormType`)
	// @return std::vector<std::string> - A vector of plugin names that match the selected filter.
	std::vector<std::string> Data::GetFilteredListOfPluginNames(PLUGIN_TYPE a_primaryFilter, SORT_TYPE a_sortType, RE::FormType a_secondaryFilter)
	{
		const auto& masterList = GetModulePluginListSorted(a_primaryFilter, a_sortType);
		const auto& blacklist = BlacklistConfig::Get();
		std::vector<std::string> pluginList;

		for (auto& mod : masterList) {
			auto modName = Modex::ValidateTESFileName(mod);

			if (blacklist.contains(mod)) {
				continue;
			}

			// We only step through the secondary filter if it's not None. This allows us to polymorphically
			// filter the list based on whether the module supports a secondary filter or not. Which most do by now.
			if (a_secondaryFilter == RE::FormType::None) {
				pluginList.push_back(modName);
			} else {
				auto plugin = RE::TESDataHandler::GetSingleton()->LookupModByName(modName.c_str());
				auto pluginFormTypeFlag = m_itemListModFormTypeMap[plugin];

				switch (a_secondaryFilter) {
				case RE::FormType::Armor:
					if (pluginFormTypeFlag.armor) {
						pluginList.push_back(modName);
					}
					break;
				case RE::FormType::Book:
					if (pluginFormTypeFlag.book) {
						pluginList.push_back(modName);
					}
					break;
				case RE::FormType::Weapon:
					if (pluginFormTypeFlag.weapon) {
						pluginList.push_back(modName);
					}
					break;
				case RE::FormType::Misc:
					if (pluginFormTypeFlag.misc) {
						pluginList.push_back(modName);
					}
					break;
				case RE::FormType::KeyMaster:
					if (pluginFormTypeFlag.key) {
						pluginList.push_back(modName);
					}
					break;
				case RE::FormType::Ammo:
					if (pluginFormTypeFlag.ammo) {
						pluginList.push_back(modName);
					}
					break;
				case RE::FormType::AlchemyItem:
					if (pluginFormTypeFlag.alchemy) {
						pluginList.push_back(modName);
					}
					break;
				case RE::FormType::Ingredient:
					if (pluginFormTypeFlag.ingredient) {
						pluginList.push_back(modName);
					}
					break;
				case RE::FormType::Scroll:
					if (pluginFormTypeFlag.scroll) {
						pluginList.push_back(modName);
					}
					break;
				case RE::FormType::SoulGem:
					if (pluginFormTypeFlag.misc) {
						pluginList.push_back(modName);
					}
					break;
				case RE::FormType::Static:
					if (pluginFormTypeFlag.staticObject) {
						pluginList.push_back(modName);
					}
					break;
				case RE::FormType::Tree:
					if (pluginFormTypeFlag.tree) {
						pluginList.push_back(modName);
					}
					break;
				case RE::FormType::Activator:
					if (pluginFormTypeFlag.activator) {
						pluginList.push_back(modName);
					}
					break;
				case RE::FormType::Container:
					if (pluginFormTypeFlag.container) {
						pluginList.push_back(modName);
					}
					break;
				case RE::FormType::Door:
					if (pluginFormTypeFlag.door) {
						pluginList.push_back(modName);
					}
					break;
				case RE::FormType::Light:
					if (pluginFormTypeFlag.light) {
						pluginList.push_back(modName);
					}
				case RE::FormType::Flora:
					if (pluginFormTypeFlag.flora) {
						pluginList.push_back(modName);
					}
					break;
				default:
					PrettyLog::Error("Invalid FormType argument passed to GetFilteredListOfPluginNames");
					pluginList.push_back(modName);
					break;
				}
			}
		}

		return pluginList;
	}
}