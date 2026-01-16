#include "Data.h"
#include "external/framework/DescriptionFrameworkImpl.h"

namespace Modex
{
	
	// Source: powerofthree | License: MIT
	// https://github.com/powerof3/po3-Tweaks
	using _GetFormEditorID = const char* (*)(std::uint32_t);
	std::string po3_GetEditorID(RE::FormID a_formID)
	{
		static auto tweaks = GetModuleHandleA("po3_Tweaks");
		static auto function = reinterpret_cast<_GetFormEditorID>(GetProcAddress(tweaks, "GetFormEditorID"));

		if (function) {
			return function(a_formID);
		}

		return {};
	}

	constexpr const char* ERROR_FILENAME = "Unable to Read";

	std::string ValidateTESFileName(const RE::TESFile* a_file)
	{
		if (a_file == nullptr) {
			return ERROR_FILENAME;
		}

		if (a_file->fileName[0] == '\0') {
			return ERROR_FILENAME;
		}

		return std::string(a_file->fileName);
	}

	template <class TESObject>
	const char* ValidateTESName(const TESObject* a_object)
	{
		if (a_object == nullptr) {
			return ERROR_FILENAME;
		}

		if (a_object->GetName() == nullptr) {
			return ERROR_FILENAME;
		}

		return a_object->GetName();
	}

	void Data::AddModToIndex(const RE::TESFile* a_mod, std::unordered_set<const RE::TESFile*>& a_out)
	{
		if (a_mod == nullptr) {
			return;
		}

		if (a_out.find(a_mod) == a_out.end()) {
			a_out.insert(a_mod);
			m_modList.insert(a_mod);
		}
	}

	// Helper method to assigning form flags to mod files when caching forms.
	// Allows us to track and filter what form types a mod file contains.
	void Data::ApplyModFileItemFlags(const RE::TESFile* a_mod, RE::FormType a_formType)
	{
		auto iter = m_itemListModFormTypeMap.find(a_mod);

		if (iter == m_itemListModFormTypeMap.end()) {
			m_itemListModFormTypeMap[a_mod] = ModFileItemFlags();
		}

		ModFileItemFlags& flags = m_itemListModFormTypeMap[a_mod];

		switch (a_formType) {
		case RE::FormType::Armor:
			flags.armor = true;
			break;
		case RE::FormType::Book:
			flags.book = true;
			break;
		case RE::FormType::Weapon:
			flags.weapon = true;
			break;
		case RE::FormType::Misc:
			flags.misc = true;
			break;
		case RE::FormType::SoulGem:
			flags.misc = true;
			break;
		case RE::FormType::KeyMaster:
			flags.key = true;
			break;
		case RE::FormType::Ammo:
			flags.ammo = true;
			break;
		case RE::FormType::AlchemyItem:
			flags.alchemy = true;
			break;
		case RE::FormType::Ingredient:
			flags.ingredient = true;
			break;
		case RE::FormType::Scroll:
			flags.scroll = true;
			break;
		case RE::FormType::Tree:
			flags.tree = true;
			break;
		case RE::FormType::Activator:
			flags.activator = true;
			break;
		case RE::FormType::Container:
			flags.container = true;
			break;
		case RE::FormType::Door:
			flags.door = true;
			break;
		case RE::FormType::Light:
			flags.light = true;
			break;
		case RE::FormType::Static:
			flags.staticObject = true;
			break;
		case RE::FormType::Furniture:
			flags.furniture = true;
			break;
		case RE::FormType::Flora:
			flags.flora = true;
			break;
		default:
			break;
		}
	}

	template <class T>
	void Data::CacheNPCs(RE::TESDataHandler* a_data)
	{
		for (RE::TESForm* form : a_data->GetFormArray<T>()) {
			if (!form) {
				continue;
			}

			const RE::TESFile* mod = form->GetFile(0);

			if (!mod) {
				continue;
			}

			if (RE::TESNPC* npc = form->As<RE::TESNPC>()) {
				if (npc->IsPlayerRef()) {
					continue;
				}
			}

			m_npcCache.push_back(BaseObject{ form });
			AddModToIndex(mod, m_npcModList);
			ApplyModFileItemFlags(mod, form->GetFormType());
		}
	}

	// Called Internally from Data::CacheNPCRefIds.
	// Merges newfound NPC references with the master list.
	void Data::MergeNPCRefIds(std::shared_ptr<std::unordered_map<RE::FormID, RE::FormID>> npc_ref_map)
	{
		if (npc_ref_map->empty()) {
			PrettyLog::Debug("No NPC references found.");
		} else {
			for (auto& npc : m_npcCache) {
				auto it = npc_ref_map->find(npc.GetBaseFormID());
				if (it != npc_ref_map->end()) {
					npc.m_refID = it->second;
				}
			}
		}
	}

	// Best I can currently do to cache NPC references for the time being.
	// Some unique NPCs are not captured until their cell (or world) is loaded.
	// Hroki in Markarth Silverblood-inn is an example of this.
	void Data::CacheNPCRefIds()
	{
		// This is shared so that it's lifetime persists until the SKSE task is complete.
		// Passing solely by reference does not seem to work, causes a CTD.
		auto npc_ref_map = std::make_shared<std::unordered_map<RE::FormID, RE::FormID>>();

		SKSE::GetTaskInterface()->AddTask([npc_ref_map]() {
			auto process = RE::ProcessLists::GetSingleton();

			// Captures most NPCs in the game.
			for (auto& handle : process->lowActorHandles) {
				if (!handle.get() || !handle.get().get()) {
					continue;
				}

				auto actor = handle.get().get();
				RE::FormID base = actor->GetBaseObject()->GetFormID();
				RE::FormID ref = actor->GetFormID();

				npc_ref_map->insert_or_assign(base, ref);
			}

			// Captures some unloaded NPCs based on cell.
			for (auto& handle : process->middleLowActorHandles) {
				if (!handle.get() || !handle.get().get()) {
					continue;
				}

				auto actor = handle.get().get();
				RE::FormID base = actor->GetBaseObject()->GetFormID();
				RE::FormID ref = actor->GetFormID();

				if (npc_ref_map->find(base) == npc_ref_map->end()) {
					npc_ref_map->insert_or_assign(base, ref);
				}
			}

			// Same as above.
			for (auto& handle : process->highActorHandles) {
				if (!handle.get() || !handle.get().get()) {
					continue;
				}

				auto actor = handle.get().get();
				RE::FormID base = actor->GetBaseObject()->GetFormID();
				RE::FormID ref = actor->GetFormID();

				if (npc_ref_map->find(base) == npc_ref_map->end()) {
					npc_ref_map->insert_or_assign(base, ref);
				}
			}

			// Callback to Data to merge with master list.
			Data::GetSingleton()->MergeNPCRefIds(npc_ref_map);
		});
	}

	template <class T>
	void Data::CacheItems(RE::TESDataHandler* a_data)
	{
		int count = 0;

		for (RE::TESForm* form : a_data->GetFormArray<T>()) {
			if (!form)
				continue;

			const RE::TESFile* mod = form->GetFile(0);

			if (!mod)
				continue;

			m_cache.push_back(BaseObject{ form });
			AddModToIndex(mod, m_itemModList);
			ApplyModFileItemFlags(mod, form->GetFormType());

			count++;
		}

		PrettyLog::Trace("Finished caching {} items of type: {}", count, RE::FormTypeToString(T::FORMTYPE).data());
	}

	template <class T>
	void Data::CacheStaticObjects(RE::TESDataHandler* a_data)
	{
		for (RE::TESForm* form : a_data->GetFormArray<T>()) {
			if (!form)
				continue;

			const RE::TESFile* mod = form->GetFile(0);

			if (!mod)
				continue;

			m_staticCache.push_back(BaseObject{ form });
			AddModToIndex(mod, m_staticModList);
			ApplyModFileItemFlags(mod, form->GetFormType());
		}
	}

	// https://github.com/shad0wshayd3-TES5/BakaHelpExtender | License : MIT
	// Absolute unit of code here. Super grateful for the author's work!
	void Data::CacheCells(RE::TESFile* a_file, std::map<std::tuple<std::uint32_t, const std::string, const std::string>, std::string_view>& out_map)
	{
		if (!a_file->OpenTES(RE::NiFile::OpenMode::kReadOnly, false)) {
			PrettyLog::Warn("Failed to open file: {:s}", a_file->fileName);
			return;
		}

		int count = 0;

		do {
			if (a_file->currentform.form == 'LLEC') {
				char edid[512]{ '\0' };
				bool gotEDID{ false };

				std::uint32_t cidx{ a_file->currentform.formID };
				cidx += a_file->compileIndex << 24;
				cidx += a_file->smallFileCompileIndex << 12;

				do {
					switch (a_file->GetCurrentSubRecordType()) {
					case 'DIDE':
						gotEDID = a_file->ReadData(edid, a_file->actualChunkSize);
						if (gotEDID) {
							count++;
							out_map.insert_or_assign(std::make_tuple(cidx, edid, "First Pass"), a_file->fileName);
							PrettyLog::Trace(" - {:s} -> {:s}", a_file->fileName, edid);
						}
						break;
					default:
						break;
					}
				} while (a_file->SeekNextSubrecord());
			}
		} while (a_file->SeekNextForm(true));

		// TODO: Bug, survivalmode has a single cell that isn't being loaded for some reason.
		PrettyLog::Debug("Found {} cells in mod: {:s}", count, a_file->fileName);

		if (!a_file->CloseTES(false)) {
			PrettyLog::Error("Failed to close file: {:s}", a_file->fileName);
		}
	}

	void Data::GenerateInventoryList()
	{
		m_inventory.clear();

		PrettyLog::Debug("Generating Inventory List...");

		auto player = RE::PlayerCharacter::GetSingleton();
		if (!player) {
			PrettyLog::Warn("Unable to get player character singleton!");
			return;
		}

		auto inventory = player->GetInventory();
		for (auto& [obj, data] : inventory) {
			auto& [count, entry] = data;
			if (count > 0 && entry) {
				uint32_t quantity = static_cast<std::uint32_t>(count);
				m_inventory.emplace_back(BaseObject{ entry->object, 0, 0, quantity });
			}
		}

		PrettyLog::Debug("Cached {} inventory items.", m_inventory.size());
	}

	void Data::GenerateItemList()
	{
		m_cache.clear();

		// PrettyLog::Assert(true, "Generating Item List called when item cache is not empty!");
		// ASSERT_MSG(true, "TEST");

		PrettyLog::Debug("Generating Item List...");

		if (auto dataHandler = RE::TESDataHandler::GetSingleton()) {
			CacheItems<RE::TESObjectARMO>(dataHandler);
			CacheItems<RE::TESObjectBOOK>(dataHandler);
			CacheItems<RE::TESObjectWEAP>(dataHandler);
			CacheItems<RE::TESObjectMISC>(dataHandler);
			CacheItems<RE::TESAmmo>(dataHandler);
			CacheItems<RE::AlchemyItem>(dataHandler);
			CacheItems<RE::IngredientItem>(dataHandler);
			CacheItems<RE::TESKey>(dataHandler);
			CacheItems<RE::ScrollItem>(dataHandler);
			CacheItems<RE::TESSoulGem>(dataHandler);
		}
	}

	void Data::GenerateNPCList()
	{
		m_npcCache.clear();

		PrettyLog::Debug("Generating NPC List...");

		if (auto dataHandler = RE::TESDataHandler::GetSingleton()) {
			CacheNPCs<RE::TESNPC>(dataHandler);
			CacheNPCRefIds();
		}
	}

	void Data::GenerateNPCClassList()
	{
		m_npcClassList.clear();

		PrettyLog::Debug("Populating NPCClassList for {} NPCs.", m_npcCache.size());

		for (auto& npc : m_npcCache) {
			auto className = npc.GetClass();
			m_npcClassList.insert(className);
		}
	}

	void Data::GenerateNPCRaceList()
	{
		m_npcRaceList.clear();

		PrettyLog::Debug("Populating NPCRaceList for {} NPCs.", m_npcCache.size());

		for (auto& npc : m_npcCache) {
			auto raceName = npc.GetRace();
			m_npcRaceList.insert(raceName);
		}
	}

	void Data::GenerateNPCFactionList()
	{
		m_npcFactionList.clear();

		PrettyLog::Debug("Populating NPCFactionList for {} NPCs.", m_npcCache.size());

		for (auto& npc : m_npcCache) {
			if (auto faction_names = npc.GetFactions(); faction_names.has_value()) {
				for (auto& faction : (*faction_names)) {
					std::string factionName = ValidateTESName(faction.faction);
					m_npcFactionList.insert(factionName);
				}
			}
		}
	}

	void Data::GenerateObjectList()
	{
		m_staticCache.clear();

		PrettyLog::Debug("Generating Object List...");

		if (auto dataHandler = RE::TESDataHandler::GetSingleton()) {
			CacheStaticObjects<RE::TESObjectTREE>(dataHandler);
			CacheStaticObjects<RE::TESObjectACTI>(dataHandler);
			CacheStaticObjects<RE::TESObjectDOOR>(dataHandler);
			CacheStaticObjects<RE::TESObjectSTAT>(dataHandler);
			CacheStaticObjects<RE::TESObjectCONT>(dataHandler);
			CacheStaticObjects<RE::TESObjectLIGH>(dataHandler);
			CacheStaticObjects<RE::TESFlora>(dataHandler);
			CacheStaticObjects<RE::TESFurniture>(dataHandler);
		}
	}

	// TODO: Outfit List
	// OTFT is the outfit form type / record header
	// Outfits contain EDID and INAM records
	// INAM is a leveled list of sorted forms of items
	
	// TESNPC.h
	// BGSOutfit.h
	// Actor.h


	void Data::GenerateCellList()
	{
		m_cellCache.clear();

		PrettyLog::Debug("Generating Cell List...");

		std::map<std::tuple<std::uint32_t, const std::string, const std::string>, std::string_view> rawCellMap;
		if (auto dataHandler = RE::TESDataHandler::GetSingleton()) {
			auto [forms, lock] = RE::TESForm::GetAllForms();
			for (auto& iter : *forms) {
				if (iter.second->GetFormType() == RE::FormType::Cell) {
					auto file = iter.second->GetFile(-1);

					if (!file || m_cellModList.contains(file)) {
						continue;
					}

					CacheCells(file, rawCellMap);
					m_cellModList.insert(file);
				}
			}

			PrettyLog::Debug("Found a total of {} mods containing CELL records", m_cellModList.size());
			if (rawCellMap.empty()) {
				PrettyLog::Debug("No cells found in loaded worldspaces.");
			} else {
				for (const auto& [key, value] : rawCellMap) {
					const std::string& editorID = std::get<1>(key);
					const RE::TESFile* modFile = dataHandler->LookupLoadedModByName(value);
					std::string full = "";

					// What is causing the cell formid to not be found using LookupByID?
					// const auto& form = RE::TESForm::LookupByID<RE::TESObjectCELL>(cidx);

					if (const auto& form = RE::TESForm::LookupByEditorID<RE::TESObjectCELL>(editorID)) {
						full = form->GetFullName();
					}

					m_cellCache.emplace_back(
						ValidateTESFileName(modFile),
						full,
						editorID,
						modFile);
				}
			}
		}
	}

	// Returns a reference to the CellData object with the specified editor ID for PersistentData
	CellData& Data::GetCellByEditorID(const std::string& a_editorid)
	{
		for (auto& cell : m_cellCache) {
			if (cell.GetEditorID() == a_editorid) {
				return cell;
			}
		}

		static CellData emptyCell(
			"MODEX_ERR",
			"MODEX_ERR",
			a_editorid,
			nullptr);

		return emptyCell;
	}

	// TODO: Optimize this by loading on demand rather than all at once.
	void Data::Run()
	{
		GenerateItemList();
		GenerateNPCList();
		GenerateNPCClassList();
		GenerateNPCRaceList();
		GenerateNPCFactionList();
		GenerateObjectList();
		GenerateCellList();
		

		PrettyLog::Info("Successfully registered data from {} mods.", m_modList.size());
		for (auto& file : m_modList) {
			m_modListSorted.insert(ValidateTESFileName(file));
			PrettyLog::Trace(" - \"{}\"", ValidateTESFileName(file));
		}
		
		DescriptionFramework_Impl::SetDescriptionFrameworkInterface(DescriptionFrameworkAPI::GetDescriptionFrameworkInterface001());
	}
}