#include "Data.h"
#include "config/BlacklistConfig.h"
#include "core/PrettyLog.h"
#include "pch.h"

#include "external/magic_enum.hpp"

namespace Modex
{
	// Sort Function for Case Insensitive comparing used for Plugin Lists.
	bool CaseInsensitiveCompareTESFile(const RE::TESFile* a, const RE::TESFile* b)
	{
		std::string filenameA = a->GetFilename().data();
		std::string filenameB = b->GetFilename().data();

		return std::lexicographical_compare(
			filenameA.begin(), filenameA.end(),
			filenameB.begin(), filenameB.end(),
			[](unsigned char c1, unsigned char c2) {
				return std::tolower(c1) < std::tolower(c2);
			});
	}

	// Sort function for compileindex (Load Order) - Ascending.
	bool CompileIndexCompareTESFileAsc(const RE::TESFile* a, const RE::TESFile* b)
	{
		if (!a or !b) {
			return false;
		}

		return a->GetCombinedIndex() < b->GetCombinedIndex();
	}

	// Sort function for compileIndex (Load Order) - Descending.
	bool CompileIndexCompareTESFileDesc(const RE::TESFile* a, const RE::TESFile* b)
	{
		if (!a or !b) {
			return false;
		}

		return a->GetCombinedIndex() > b->GetCombinedIndex();
	}

	std::vector<std::string> Data::GetTypeString()
	{
		auto enumNames = magic_enum::enum_names<PLUGIN_TYPE>();
		std::vector<std::string> strings;

		for (auto& name : enumNames) {
			if (name == "kTotal") {
				continue;
			}

			std::string prettyName = name.data();
			std::replace(prettyName.begin(), prettyName.end(), '_', ' ');
			strings.push_back(prettyName);
		}

		return strings;
	}

	std::vector<std::string> Data::GetSortStrings()
	{
		auto enumNames = magic_enum::enum_names<SORT_TYPE>();
		std::vector<std::string> strings;

		for (auto& name : enumNames) {
			if (name == "kTotal") {
				continue;
			}

			std::string prettyName = name.data();
			std::replace(prettyName.begin(), prettyName.end(), '_', ' ');
			strings.push_back(prettyName);
		}

		return strings;
	}

	// Returns an unordered set of TESFile pointers cached at startup based on PLUGIN_TYPE.
	std::unordered_set<const RE::TESFile*> Data::GetModulePluginList(PLUGIN_TYPE a_type)
	{
		switch (a_type) {
		case PLUGIN_TYPE::Item:
			return m_itemModList;
		case PLUGIN_TYPE::Actor:
			return m_npcModList;
		case PLUGIN_TYPE::Object:
			return m_staticModList;
		case PLUGIN_TYPE::Cell:
			return m_cellModList;
		case PLUGIN_TYPE::kTotal:
		case PLUGIN_TYPE::All:
			return m_modList;
		}

		ASSERT_MSG(true, "Invalid Data::PLUGIN_TYPE arg passed to Data::GetModulePluginList: '{}'", static_cast<uint32_t>(a_type));

		return m_modList;
	}

	// Returns an alphabetically sorted vector of plugin names cached at startup.
	std::vector<std::string> Data::GetSortedListOfPluginNames()
	{
		std::vector<std::string> modList;

		for (auto& mod : m_modListSorted) {
			modList.push_back(mod);
		}

		return modList;
	}

	// Returns a sorted vector of TESFile pointers cached at startup based on PLUGIN_TYPE.
	std::vector<const RE::TESFile*> Data::GetModulePluginListSorted(PLUGIN_TYPE a_type, SORT_TYPE a_sortType)
	{
		std::vector<const RE::TESFile*> copy;

		auto safeCopy = [&copy](const std::unordered_set<const RE::TESFile*>& a_set) {
			for (const auto& mod : a_set) {
				if (mod != nullptr) {
					copy.push_back(mod);
				}
			}
		};

		switch (a_type) 
		{
		case PLUGIN_TYPE::Item:
			safeCopy(m_itemModList);
			break;
		case PLUGIN_TYPE::Actor:
			safeCopy(m_npcModList);
			break;
		case PLUGIN_TYPE::Object:
			safeCopy(m_staticModList);
			break;
		case PLUGIN_TYPE::Cell:
			safeCopy(m_cellModList);
			break;
		case PLUGIN_TYPE::kTotal:
		case PLUGIN_TYPE::All:
			safeCopy(m_modList);
			break;
		}

		switch (a_sortType) 
		{
			case SORT_TYPE::Alphabetical:
				std::sort(copy.begin(), copy.end(), CaseInsensitiveCompareTESFile);
				break;
			case SORT_TYPE::Load_Order_Ascending:
				std::sort(copy.begin(), copy.end(), CompileIndexCompareTESFileAsc);
				break;
			case SORT_TYPE::Load_Order_Descending:
				std::sort(copy.begin(), copy.end(), CompileIndexCompareTESFileDesc);
				break;
			case SORT_TYPE::kTotal:
				break;
		}

		return copy;
	}

	// TODO: Verify that we are properly populating m_itemListModFormTypeMap in Data.cpp 
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
			ASSERT_MSG(true, "Unhandled 'RE::FormType' passed to Data::IsFormTypeInPlugin: '{}'", static_cast<int>(a_formType));
			return false;
		}
	}

	// BUG: Previously crashing to desktop, called from BuildPluginList. Might have been a weird
	// build artifact. Test a_type and a_sort parameters. Doesn't seem like a runtime issue.
	// 1245198 error code?

	// Returns a sorted vector of plugin names filtered out by global blacklist config.
	std::vector<std::string> Data::GetFilteredListOfPluginNames(PLUGIN_TYPE a_type, SORT_TYPE a_sort)
	{
		const auto& masterlist = GetModulePluginListSorted(a_type, a_sort);
		const auto& blacklist = BlacklistConfig::Get();

		std::vector<std::string> pluginList;

		for (const auto& plugin : masterlist) {
			const std::string name = plugin->GetFilename().data();

			if (blacklist.contains(plugin)) {
				continue;
			}

			pluginList.push_back(name);
		}

		return pluginList;
	}
}
