#pragma once

#include <functional>

#include "RE/B/BSContainer.h"
#include "RE/B/BSTArray.h"
#include "RE/P/PlayerCharacter.h"
#include "RE/T/TESLeveledList.h"
#include "data/BaseObject.h"
#include "localization/Locale.h"
#include "ui/core/UIManager.h"
#include "config/UserData.h"

// TODO: Implement requested Papyrus API for some handlers.

namespace Modex::Commands
{
	// Source: Qudix in xSE RE discord <3
	// https://discord.com/channels/535508975626747927/535530099475480596/1213998301314031686
	//
	// Only serves as a shortcut for users to remotely open containers. Doesn't resolve issues with
	// items flagged as stolen, crime, etc. Will require a lot more implementation for such actions.
	static inline void TESObjectREFR_OpenContainer(RE::TESObjectREFR* a_this, RE::ContainerMenu::ContainerMode a_openType = RE::ContainerMenu::ContainerMode::kLoot)
	{
		using func_t = decltype(&TESObjectREFR_OpenContainer);
		REL::Relocation<func_t> func{ RELOCATION_ID(50211, 51140) };
		func(a_this, a_openType);
	}

    // Source: Meridiano on Skyrim REx Discord! :)
    // https://discord.com/channels/535508975626747927/535530099475480596/1434769252094705725
	//
	// Check for explosion and initially disabled flag to safely pass NULL into first two arguments
	// which are suspected for logging. Added the *a_count* argument for my implementation.
	static inline RE::TESObjectREFR* Papyrus_PlaceAtMe(RE::TESObjectREFR* a_target, RE::TESForm* a_form, uint32_t a_count, bool a_persistent, bool a_initiallyDisabled) {
		if (a_form->GetFormType() != RE::FormType::Explosion || !a_initiallyDisabled) {
			using func_t = RE::TESObjectREFR*(*)(int64_t, int32_t, RE::TESObjectREFR*, RE::TESForm*, int32_t, bool, bool);
			static REL::Relocation<func_t> func{ REL::RelocationID(55672, 56203, 55672) };
			return func(NULL, NULL, a_target, a_form, a_count, a_persistent, a_initiallyDisabled);
		}

		return nullptr;
	}

	static inline void OpenActorInventory(RE::TESObjectREFR* a_actorRef)
	{
		if (a_actorRef && UIManager::CloseAllGameMenus()) {
			TESObjectREFR_OpenContainer(a_actorRef, RE::ContainerMenu::ContainerMode::kLoot);
			UIManager::GetSingleton()->SetMenuListener(true);
		}
	}

	static inline bool IsGameMenuOpen()
	{
		if (const auto GameUI = RE::UI::GetSingleton(); GameUI) {
			return  GameUI->IsMenuOpen(RE::MainMenu::MENU_NAME)    ||
					GameUI->IsMenuOpen(RE::StatsMenu::MENU_NAME)   ||
					GameUI->IsMenuOpen(RE::JournalMenu::MENU_NAME) ||
					GameUI->IsMenuOpen(RE::MapMenu::MENU_NAME);
		}

		return false;
	}

	static inline RE::TESObjectREFR* GetConsoleReference()
	{
		if (auto consoleRefr = RE::Console::GetSelectedRef().get(); consoleRefr != nullptr) {
			return consoleRefr;
		}

		return nullptr;
	}

	static inline RE::TESObjectREFR* GetPlayerReference()
	{
		auto player = RE::PlayerCharacter::GetSingleton();

		if (!player)
			return nullptr;

		auto playerReference = player->AsReference();

		if (!playerReference)
			return nullptr;

		return playerReference;
	}

	// Walks a LeveledList's direct entries, calling a_onItem for concrete forms
	// and a_onLeveledList for nested LeveledList entries (without resolving them).
	// a_onLeveledList receives the original TESForm* (for GetFormID) and the cast TESLeveledList*.
	static inline void ForEachLeveledEntry(RE::TESLeveledList* a_list,
		const std::function<void(int a_index, RE::TESForm*, uint16_t, int32_t)>& a_onItem,
		const std::function<void(int a_index, RE::TESForm*, RE::TESLeveledList*)>& a_onLeveledList)
	{
		int index = 0;
		for (auto& entry : a_list->entries) {
			index++;

			if (!entry.form) continue;
			if (entry.form->GetFormType() == RE::FormType::LeveledItem) {
				if (auto nested = entry.form->As<RE::TESLeveledList>()) {
					a_onLeveledList(index, entry.form, nested);
				}
			} else {
				a_onItem(index, entry.form, entry.level, entry.count);
			}
		}
	}

	struct ResolvedItem {
		RE::TESBoundObject* object;
		uint16_t       count;
	};

	// Resolves a leveled list using the engine's native CalculateCurrentFormList.
	// Respects flags (UseAll, AllLevels, ForEach), chanceNone, custom level, and nested lists.
	static inline std::vector<ResolvedItem> ResolveLeveledList(
		RE::TESLeveledList* a_list,
		RE::TESObjectREFR*  a_targetRef = nullptr,
		int16_t        a_count = 1,
		uint16_t       a_level = 0)
	{
		std::vector<ResolvedItem> result;

		if (!a_list)
			return result;

		// Determine the level to resolve at. 0 = target
		if (a_level == 0) {
			if (a_targetRef) {
				a_level = a_targetRef->GetCalcLevel(true);
			} else if (auto player = RE::PlayerCharacter::GetSingleton()) {
				a_level = player->AsReference()->GetCalcLevel(true);
			}
		}

		RE::BSScrapArray<RE::CALCED_OBJECT> calcedObjects;
		a_list->CalculateCurrentFormList(a_level, a_count, calcedObjects, 0, false);

		for (auto& calced : calcedObjects) {
			if (!calced.form)
				continue;

			if (auto bound = calced.form->As<RE::TESBoundObject>()) {
				result.push_back({ bound, calced.count });
			}
		}

		return result;
	}

	// Resolves all items in an outfit, using engine-native leveled list resolution.
	// Concrete items are passed through; leveled lists are resolved via ResolveLeveledList.
	// a_level > 0 overrides the resolved level; 0 falls back to target/player level.
	static inline std::vector<ResolvedItem> ResolveOutfitItems(
		const RE::BGSOutfit* a_outfit,
		RE::TESObjectREFR*   a_targetRef = nullptr,
		uint16_t             a_level = 0)
	{
		std::vector<ResolvedItem> result;

		if (!a_outfit || a_outfit->outfitItems.size() == 0)
			return result;

		a_outfit->ForEachItem([&](RE::TESForm* a_form) {
			if (!a_form)
				return RE::BSContainer::ForEachResult::kContinue;

			if (a_form->GetFormType() == RE::FormType::LeveledItem) {
				if (auto leveledList = a_form->As<RE::TESLeveledList>()) {
					auto resolved = ResolveLeveledList(leveledList, a_targetRef, 1, a_level);
					result.insert(result.end(), resolved.begin(), resolved.end());
				}
			} else {
				if (auto bound = a_form->As<RE::TESBoundObject>()) {
					result.push_back({ bound, 1 });
				}
			}

			return RE::BSContainer::ForEachResult::kContinue;
		});

		return result;
	}

	static inline const std::vector<BaseObject> GetOutfitItems(const RE::BGSOutfit* a_outfit, uint16_t a_level = 0)
	{
		if (!a_outfit || a_outfit->outfitItems.size() == 0) return {};

		auto resolved = ResolveOutfitItems(a_outfit, GetPlayerReference(), a_level);

		std::vector<BaseObject> items;
		for (auto& entry : resolved) {
			items.emplace_back(entry.object, Ownership::Outfit, 0, 0, entry.count);
		}

		return items;
	}

	static inline const std::vector<BaseObject> GetInventoryList(RE::TESObjectREFR* reference = nullptr) // deprecated
	{
		Trace("Generating Inventory List...");

		reference = GetConsoleReference();
		std::vector<BaseObject> m_inventory;

		auto inventory = reference->GetInventory();
		for (auto& [obj, data] : inventory) {
			auto& [count, entry] = data;
			if (count > 0 && entry) {
				uint32_t quantity = static_cast<std::uint32_t>(count);
				m_inventory.emplace_back(entry->object, Ownership::Item, 0, 0, quantity);
			}
		}

		Trace("Cached {} inventory items.", m_inventory.size());
		return m_inventory;
	}

	// Called repeatedly in a loop
	static inline void AddItemToInventory(Ownership a_owner, RE::TESObjectREFR* a_targetRef, const std::string& a_editorID, uint32_t a_amount = 1)
	{
		if (!a_targetRef)
			return;

		auto form = RE::TESForm::LookupByEditorID(a_editorID);

		if (!form)
			return;

		auto boundObject = form->As<RE::TESBoundObject>();

		if (!boundObject)
			return;
		
		a_targetRef->AddObjectToContainer(boundObject, nullptr, a_amount, nullptr);
		UserData::SendEvent(ModexActionType::AddItem, a_editorID, a_owner);
	}

	static inline void AddItemToRefInventory(Ownership a_owner, RE::TESObjectREFR* a_targetRef, const std::string& a_editorID, uint32_t a_amount = 1)
	{
		if (!a_targetRef)
			return;

		Commands::AddItemToInventory(a_owner, a_targetRef, a_editorID, a_amount);
	}

	static inline void AddLeveledListToRefInventory(Ownership a_owner, RE::TESObjectREFR* a_targetRef, const std::string& a_editorID, int16_t a_amount = 1)
	{
		if (!a_targetRef || a_editorID.empty())
			return;

		if (auto leveled = RE::TESForm::LookupByEditorID<RE::TESLeveledList>(a_editorID)) {
			auto resolved = ResolveLeveledList(leveled, a_targetRef, a_amount);

			for (auto& entry : resolved) {
				a_targetRef->AddObjectToContainer(entry.object, nullptr, entry.count, nullptr);

				auto editorid = po3_GetEditorID(entry.object->GetFormID());
				UserData::SendEvent(ModexActionType::AddItem, editorid, a_owner);
			}
		}
	}

	static inline void RemoveAllItemsFromInventory(Ownership a_owner, RE::TESObjectREFR* a_targetRef)
	{
		if (!a_targetRef)
			return;

		// TEST: Compare results across save files.
		// a_targetRef->ResetInventory(false);

		auto inventory = a_targetRef->GetInventory();
		for (auto& [obj, data] : inventory) {
		    auto& [count, entry] = data;
		    if (count > 0 && entry) {
		        a_targetRef->RemoveItem(obj, count, RE::ITEM_REMOVE_REASON::kRemove, nullptr, nullptr);
				UserData::SendEvent(ModexActionType::RemoveItem, po3_GetEditorID(obj->GetFormID()), a_owner);
		    }
		}
	}

	static inline void RemoveItemFromInventory(Ownership a_owner, RE::TESObjectREFR* a_targetRef, const std::string& a_editorID, uint32_t a_amount = 1)
	{
		if (!a_targetRef)
			return;

		auto form = RE::TESForm::LookupByEditorID(a_editorID);

		if (!form)
			return;

		auto boundObject = form->As<RE::TESBoundObject>();

		if (!boundObject)
			return;
		
		a_targetRef->RemoveItem(boundObject, a_amount, RE::ITEM_REMOVE_REASON::kRemove, nullptr, nullptr);
		UserData::SendEvent(ModexActionType::RemoveItem, a_editorID, a_owner);
	}

	static inline void RemoveItemFromPlayerInventory(Ownership a_owner, const std::string& a_editorID, uint32_t a_amount = 1)
	{
		auto player = RE::PlayerCharacter::GetSingleton();

		if (!player)
			return;

		auto playerRef = player->AsReference();

		if (!playerRef)
			return;

		RemoveItemFromInventory(a_owner, playerRef, a_editorID, a_amount);
	}

	static inline void ResetTargetInventory(Ownership a_owner, RE::TESObjectREFR* a_targetRef)
	{
		if (!a_targetRef)
			return;

		a_targetRef->ResetInventory(false);
		UserData::SendEvent(ModexActionType::ResetInventory, po3_GetEditorID(a_targetRef->GetFormID()), a_owner);
	}

	static inline void UnequipAllItemsFromInventory(Ownership a_owner, RE::TESObjectREFR* a_targetRef)
	{
		(void)a_owner;

		if (!a_targetRef)
			return;

		auto actor = a_targetRef->As<RE::Actor>();

		if (!actor)
			return;

		auto inventory = a_targetRef->GetInventory();
		for (auto& [obj, data] : inventory) {
			auto& [count, entry] = data;
			if (count > 0 && entry) {
				RE::ActorEquipManager::GetSingleton()->UnequipObject(actor, obj, entry->extraLists ? entry->extraLists->front() : nullptr, count, nullptr);
			}
		}
	}

    // Helper function for inventory item binding
    inline int InventoryBoundObjects(RE::TESObjectREFR* a_targetRef, const RE::TESForm* a_form, RE::TESBoundObject*& out_object, RE::ExtraDataList*& out_extra)
	{
		// auto player = RE::PlayerCharacter::GetSingleton();
		RE::TESBoundObject* foundObject = nullptr;
		std::vector<RE::ExtraDataList*> extraDataCopy;
		RE::FormType a_type = a_form->GetFormType();

		std::map<RE::TESBoundObject*, std::pair<int, std::unique_ptr<RE::InventoryEntryData>>> candidates = 
			a_targetRef->GetInventory([a_type](const RE::TESBoundObject& a_object) { return a_object.Is(a_type); });

		auto count = 0;
		for (const auto& [item, inventoryData] : candidates) {
			const auto& [num_items, entry] = inventoryData;
			if (entry->object->formID == a_form->formID) {
				foundObject = item;
				count = num_items;
				auto simpleList = entry->extraLists;

				if (simpleList) {
					for (auto* extraData : *simpleList) {
						extraDataCopy.push_back(extraData);
					}
				}
				break;
			}
		}

		if (!foundObject) {
			return 0;
		}

		if (extraDataCopy.size() > 0) {
			out_extra = extraDataCopy.back();
		}

		out_object = foundObject;
		return count;
	}

	static inline void AddAndEquipItemToInventory(Ownership a_owner, RE::TESObjectREFR* a_targetRef, const std::string& a_editorID, uint32_t a_amount = 1)
	{
		if (!a_targetRef)
			return;

		AddItemToInventory(a_owner, a_targetRef, a_editorID, a_amount);
		
		SKSE::GetTaskInterface()->AddTask([a_owner, a_targetRef, a_editorID]() {
			auto actor = a_targetRef->As<RE::Actor>();
			RE::TESForm* form = RE::TESForm::LookupByEditorID(a_editorID);

			if (!form)
				return;

			RE::BGSEquipSlot* equipSlot = nullptr;
			RE::TESBoundObject* equipObject = nullptr;
			RE::ExtraDataList* extraData = nullptr;

			InventoryBoundObjects(a_targetRef, form, equipObject, extraData);

			if (RE::TESObjectWEAP* weapon = form->As<RE::TESObjectWEAP>()) {
				equipSlot = weapon->GetEquipSlot();
			} else if (RE::TESObjectARMO* armor = form->As<RE::TESObjectARMO>()) {
				equipSlot = armor->GetEquipSlot();
			}

			// TEST: Compare against commented out method.
			// RE::ActorEquipManager::GetSingleton()->EquipObject(actor, equipObject, extraData, 1, equipSlot);

			actor->AddWornItem(equipObject, 1, false, 0, 0);
			UserData::SendEvent(ModexActionType::EquipItem, a_editorID, a_owner);
		});
		
	}

	static inline void AddAndEquipItemToPlayerInventory(Ownership a_owner, const std::string& a_editorID)
	{
		auto player = RE::PlayerCharacter::GetSingleton();

		if (!player)
			return;

		AddAndEquipItemToInventory(a_owner, player->AsReference(), a_editorID, 1);
	}

	static inline void ReadBook(Ownership a_owner, const std::string& a_editorID)
	{
		SKSE::GetTaskInterface()->AddTask([a_owner, a_editorID]() {
			RE::TESForm* form = RE::TESForm::LookupByEditorID(a_editorID);

			const auto player = RE::PlayerCharacter::GetSingleton();

			if (!player)
				return;

			auto playerRef = player->AsReference();

			if (!playerRef)
				return;

			Commands::AddItemToRefInventory(a_owner, playerRef, a_editorID);

			if (!form) 
				return;

			RE::TESObjectBOOK* book = form->As<RE::TESObjectBOOK>();

			if (!book) 
				return;

			RE::NiPoint3 defaultPos{};
			RE::BSString buf;
			book->GetDescription(buf, nullptr);

			RE::TESBoundObject* equipObject = nullptr;
			RE::ExtraDataList* extraData = nullptr;
			int found = InventoryBoundObjects(player->AsReference(), form, equipObject, extraData);

			if (found == 0)
				return;

			RE::TESObjectREFR* bookRef = equipObject->As<RE::TESObjectREFR>();

			if (equipObject) {
				RE::BookMenu::OpenBookMenu(buf, extraData, bookRef, book, defaultPos, defaultPos, 1.0f, true);
			}
		});
	}

	static inline void TeleportNPCToPlayer(Ownership a_owner, uint32_t a_refID)
	{
		if (a_refID != 0) {
			if (auto player = RE::PlayerCharacter::GetSingleton(); player) {
				if (auto playerRefr = player->AsReference()) {
					if (auto ref = RE::TESForm::LookupByID<RE::TESObjectREFR>(a_refID)) {
						ref->MoveTo(playerRefr);
						UserData::SendEvent(ModexActionType::BringReference, ref->GetFormID(), a_owner); 
						UIManager::CloseAllGameMenus();
					}
				}
			}
		}
	}

	static inline void TeleportREFRToPlayer(Ownership a_owner, RE::TESObjectREFR* a_ref)
	{
		if (a_ref) {
			if (auto player = RE::PlayerCharacter::GetSingleton(); player) {
				if (auto playerRefr = player->AsReference()) {
					a_ref->MoveTo(playerRefr);
					UserData::SendEvent(ModexActionType::BringReference, a_ref->GetFormID(), a_owner); 
					UIManager::CloseAllGameMenus();
				}
			}
		}
	}

	static inline void TeleportPlayerToREFR(Ownership a_owner, RE::TESObjectREFR* a_ref)
	{
		if (a_ref) {
			if (auto player = RE::PlayerCharacter::GetSingleton(); player) {
				if (auto playerRefr = player->AsReference()) {
					playerRefr->MoveTo(a_ref);
					UserData::SendEvent(ModexActionType::GotoReference, a_ref->GetFormID(), a_owner); 
					UIManager::CloseAllGameMenus();
				}
			}
		}
	}

	static inline void TeleportPlayerToNPC(Ownership a_owner, uint32_t a_refID)
	{
		if (a_refID != 0) {
			if (auto player = RE::PlayerCharacter::GetSingleton(); player) {
				if (auto playerRefr = player->AsReference()) {
					if (auto ref = RE::TESForm::LookupByID<RE::TESObjectREFR>(a_refID)) {
						playerRefr->MoveTo(ref);
						UserData::SendEvent(ModexActionType::GotoReference, ref->GetFormID(), a_owner);
						UIManager::CloseAllGameMenus();
					}
				}
			}
		}
	}

	static inline void AddOutfitItemsToInventory(Ownership a_owner, RE::TESObjectREFR* a_targetRef, RE::BGSOutfit* a_outfit, uint16_t a_level = 0)
	{
		if (!a_targetRef || !a_outfit)
			return;

		auto resolved = ResolveOutfitItems(a_outfit, a_targetRef, a_level);

		for (auto& entry : resolved) {
			a_targetRef->AddObjectToContainer(entry.object, nullptr, entry.count, nullptr);
		}

		UserData::SendEvent(ModexActionType::AddItem, po3_GetEditorID(a_outfit->GetFormID()), a_owner);
	}

	// RE::Actor::AddWornOutfit doesn't behave as expected. Using alternative implementation with
	// existing methods provided in this header file.
	static inline void EquipOutfit(Ownership a_owner, RE::TESObjectREFR* a_targetRef, RE::BGSOutfit* a_outfit, uint16_t a_level = 0)
	{
		if (!a_targetRef || !a_outfit)
			return;

		// Resolve once, add to inventory, then equip from the resolved list.
		auto resolved = ResolveOutfitItems(a_outfit, a_targetRef, a_level);

		for (auto& entry : resolved) {
			a_targetRef->AddObjectToContainer(entry.object, nullptr, entry.count, nullptr);
		}

		// Queued task to allow game to catch up with inventory events (unsure).
		SKSE::GetTaskInterface()->AddTask([a_targetRef, resolved]() {
			auto actor = a_targetRef->As<RE::Actor>();
			if (!actor)
				return;

			for (auto& entry : resolved) {
				RE::TESBoundObject* equipObject = nullptr;
				RE::ExtraDataList* extraData = nullptr;

				InventoryBoundObjects(a_targetRef, entry.object, equipObject, extraData);
				if (equipObject) {
					actor->AddWornItem(equipObject, 1, false, 0, 0);
				}
			}
		});

		UserData::SendEvent(ModexActionType::EquipOutfit, po3_GetEditorID(a_outfit->GetFormID()), a_owner);

	}

	// TEST: What happens when we call this on Player?
	static inline void SetSleepOutfitOnActor(Ownership a_owner, RE::TESObjectREFR* a_targetRef, RE::BGSOutfit* a_outfit)
	{
		(void) a_owner;

		if (!a_targetRef || !a_outfit)
			return;

		if (auto npc = a_targetRef->As<RE::Actor>()) {
			if (auto base = npc->GetActorBase()) {
				base->sleepOutfit = a_outfit;
			}
		}

		UserData::SendEvent(ModexActionType::SetSleepOutfit, po3_GetEditorID(a_outfit->GetFormID()), a_owner);
	}

	// TEST: Is this okay being applied to the actor base?
	static inline void SetDefaultOutfitOnActor(Ownership a_owner, RE::TESObjectREFR* a_targetRef, RE::BGSOutfit* a_outfit)
	{
		(void)a_owner;

		if (!a_targetRef || !a_outfit)
			return;

		if (auto npc = a_targetRef->As<RE::Actor>()) {
			if (auto base = npc->GetActorBase()) {
				base->defaultOutfit = a_outfit;
			}
		}

		UserData::SendEvent(ModexActionType::SetDefaultOutfit, po3_GetEditorID(a_outfit->GetFormID()), a_owner);
	}

	static inline void PlaceAtMe(Ownership a_owner, const std::string& a_editorID, uint32_t a_count = 1, bool persistent = true, bool disabled = false) 
	{
		auto player = RE::PlayerCharacter::GetSingleton();
		if (!player) 
			return;

		auto target = player->AsReference();
		if (!target) 
			return;

		if (a_editorID.empty())
			return;
		
		if (RE::TESForm* object = RE::TESForm::LookupByEditorID(a_editorID); object) {
			auto newObject = Papyrus_PlaceAtMe(target, object, a_count, persistent, disabled);
			if (newObject) {
				UserData::SendEvent(ModexActionType::PlaceAtMe, newObject->GetFormID(), a_owner);
			} else {
				Error("Failed to resolve new object from Papyrus_PlaceAtMe func: {}", a_editorID);
			}
		}
	}

	static inline void KillRefr(Ownership a_owner, RE::TESObjectREFR* a_targetRef)
	{
		if (!a_targetRef)
			return;

		auto actor = a_targetRef->As<RE::Actor>();

		if (!actor)
			return;

		actor->KillImpl(nullptr, actor->GetActorValueMax(RE::ActorValue::kHealth), true, true);
		UserData::SendEvent(ModexActionType::KillActor, a_targetRef->GetFormID(), a_owner); 
	}

	static inline void ResurrectRefr(Ownership a_owner, RE::TESObjectREFR* a_targetRef)
	{
		if (!a_targetRef)
			return;

		auto actor = a_targetRef->As<RE::Actor>();

		if (!actor)
			return;

		actor->Resurrect(false, true);
		UserData::SendEvent(ModexActionType::ReviveActor, a_targetRef->GetFormID(), a_owner); 
	}

	static inline void DisableRefr(Ownership a_owner, RE::TESObjectREFR* a_targetRef)
	{
		if (!a_targetRef)
			return;

		a_targetRef->Disable();
		UserData::SendEvent(ModexActionType::DisableReference, a_targetRef->GetFormID(), a_owner); 
	}

	static inline void EnableRefr(Ownership a_owner, RE::TESObjectREFR* a_targetRef, bool a_resetInventory = false)
	{
		if (!a_targetRef)
			return;

		a_targetRef->Enable(a_resetInventory);
		UserData::SendEvent(ModexActionType::EnableReference, a_targetRef->GetFormID(), a_owner);
	}

	static inline void CenterOnCell(Ownership a_owner, const std::string& a_cellEditorID)
	{
		(void)a_owner;
		if (a_cellEditorID.empty())
			return;

		auto player = RE::PlayerCharacter::GetSingleton();

		if (!player)
			return;

		auto cell = RE::TESForm::LookupByEditorID<RE::TESObjectCELL>(a_cellEditorID);

		if (!cell)
			return;

		SKSE::GetTaskInterface()->AddTask([player, cell]() {
			player->CenterOnCell(cell);
		});

		// UserData::SendEvent(ModexActionType::CenterOnCell, a_cellEditorID);
	}
}
