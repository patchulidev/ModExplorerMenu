#include "include/D/Data.h"
#include "include/D/DataTypes.h"
#include "include/P/Persistent.h"
#include "include/S/Settings.h"
#include "include/U/Util.h"

namespace Modex
{
	// https://github.com/Nightfallstorm/DescriptionFramework | License GPL-3.0
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

	// Need this to ensure CTD doesn't occur with weird file names being passed around.
	constexpr const char* ERROR_FILENAME = "MODEX_ERR";

	std::string ValidateTESFileName(const RE::TESFile* a_file)
	{
		if (a_file == nullptr) {
			return ERROR_FILENAME;
		}

		if (a_file->fileName == nullptr) {
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

	// Helper method to assigning form flags to mod files when caching forms.
	// Allows us to track and filter what form types a mod file contains.
	void Data::ApplyModFileItemFlags(const RE::TESFile* a_mod, RE::FormType a_formType)
	{
		auto iter = _itemListModFormTypeMap.find(a_mod);

		if (iter == _itemListModFormTypeMap.end()) {
			_itemListModFormTypeMap[a_mod] = ModFileItemFlags();
		}

		ModFileItemFlags& flags = _itemListModFormTypeMap[a_mod];

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

			_npcCache.push_back(NPCData{ form });

			if (!_npcModList.contains(mod)) {
				_npcModList.insert(mod);
				_modList.insert(mod);

				ApplyModFileItemFlags(mod, form->GetFormType());
			}
		}
	}

	// Called Internally from Data::CacheNPCRefIds.
	// Merges newfound NPC references with the master list.
	void Data::MergeNPCRefIds(std::shared_ptr<std::unordered_map<RE::FormID, RE::FormID>> npc_ref_map)
	{
		if (npc_ref_map->empty()) {
			PrettyLog::Debug("No NPC references found.");
		} else {
			for (auto& npc : _npcCache) {
				auto it = npc_ref_map->find(npc.GetBaseForm());
				if (it != npc_ref_map->end()) {
					npc.refID = it->second;
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

				if (npc_ref_map->find(base) != npc_ref_map->end()) {
					PrettyLog::Debug("Duplicate NPC reference found (middleLow): {}", actor->GetName());
				} else {
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

				if (npc_ref_map->find(base) != npc_ref_map->end()) {
					PrettyLog::Debug("Duplicate NPC reference found (highActorHandle): {}", actor->GetName());
				} else {
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
		for (RE::TESForm* form : a_data->GetFormArray<T>()) {
			if (!form)
				continue;

			const RE::TESFile* mod = form->GetFile(0);

			if (!mod)
				continue;

			_cache.push_back(ItemData{ form });

			if (!_itemModList.contains(mod)) {
				_itemModList.insert(mod);
				_modList.insert(mod);

				ApplyModFileItemFlags(mod, form->GetFormType());
			}
		}
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

			_staticCache.push_back(ObjectData{ form });

			if (!_staticModList.contains(mod)) {
				_staticModList.insert(mod);
				_modList.insert(mod);

				ApplyModFileItemFlags(mod, form->GetFormType());
			}
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
							out_map.insert_or_assign(std::make_tuple(cidx, edid, "First Pass"), a_file->fileName);
						}
						break;
					default:
						break;
					}
				} while (a_file->SeekNextSubrecord());
			}
		} while (a_file->SeekNextForm(true));

		if (!a_file->CloseTES(false)) {
			PrettyLog::Error("Failed to close file: {:s}", a_file->fileName);
		}
	}

	void Data::GenerateItemList()
	{
		_cache.clear();

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
		_npcCache.clear();

		if (auto dataHandler = RE::TESDataHandler::GetSingleton()) {
			CacheNPCs<RE::TESNPC>(dataHandler);
			CacheNPCRefIds();
		}
	}

	void Data::GenerateNPCClassList()
	{
		_npcClassList.clear();

		for (auto& npc : _npcCache) {
			auto className = npc.GetClass();
			_npcClassList.insert(className);
		}
	}

	void Data::GenerateNPCRaceList()
	{
		_npcRaceList.clear();

		for (auto& npc : _npcCache) {
			auto raceName = npc.GetRace();
			_npcRaceList.insert(raceName);
		}
	}

	void Data::GenerateNPCFactionList()
	{
		_npcFactionList.clear();

		for (auto& npc : _npcCache) {
			auto factionNames = npc.GetFactions();
			for (auto& faction : factionNames) {
				std::string factionName = ValidateTESName(faction.faction);
				_npcFactionList.insert(factionName);
			}
		}
	}

	void Data::GenerateObjectList()
	{
		_staticCache.clear();

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

	void Data::GenerateCellList()
	{
		_cellCache.clear();

		std::map<std::tuple<std::uint32_t, const std::string, const std::string>, std::string_view> rawCellMap;
		if (auto dataHandler = RE::TESDataHandler::GetSingleton()) {
			auto [forms, lock] = RE::TESForm::GetAllForms();
			for (auto& iter : *forms) {
				if (iter.second->GetFormType() == RE::FormType::Cell) {
					auto file = iter.second->GetFile(-1);

					if (!file || _cellModList.contains(file)) {
						continue;
					}

					CacheCells(file, rawCellMap);
					_cellModList.insert(file);
				}
			}

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

					_cellCache.emplace_back(
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
		for (auto& cell : _cellCache) {
			if (cell.GetEditorID() == a_editorid) {
				return cell;
			}
		}

		static CellData emptyCell(
			"MODEX_ERR",
			"MODEX_ERR",
			a_editorid,
			nullptr);
		PrettyLog::Debug("Cell with editor ID '{}' not found.", a_editorid);
		return emptyCell;
	}

	void Data::Run()
	{
		Settings::Config& config = Settings::GetSingleton()->GetConfig();

		if (config.showAddItemMenu == true) {
			GenerateItemList();
		}

		if (config.showNPCMenu) {
			GenerateNPCList();

			GenerateNPCClassList();
			GenerateNPCRaceList();
			GenerateNPCFactionList();
		}

		if (config.showObjectMenu) {
			GenerateObjectList();
		}

		if (config.showTeleportMenu) {
			GenerateCellList();
		}

		for (auto& file : _modList) {
			_modListSorted.insert(ValidateTESFileName(file));
		}

		Utils::SetDescriptionFrameworkInterface(DescriptionFrameworkAPI::GetDescriptionFrameworkInterface001());
	}
}