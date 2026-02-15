#pragma once

#include "RE/T/TrainingMenu.h"
#include "data/BaseObject.h"
#include "localization/Locale.h"
#include "ui/core/UIManager.h"


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
			using func_t = RE::TESObjectREFR*(*)(std::int64_t, std::int32_t, RE::TESObjectREFR*, RE::TESForm*, std::int32_t, bool, bool);
			static REL::Relocation<func_t> func{ REL::RelocationID(55672, 56203, 55672) };
			return func(NULL, NULL, a_target, a_form, a_count, a_persistent, a_initiallyDisabled);
		}

		return nullptr;
	}

	static inline bool CloseAllGameMenus()
	{
		if (const auto messagingQueue = RE::UIMessageQueue::GetSingleton(); messagingQueue) {
			messagingQueue->AddMessage(ModexGUIMenu::MENU_NAME, RE::UI_MESSAGE_TYPE::kHide, nullptr);
			messagingQueue->AddMessage(RE::Console::MENU_NAME, RE::UI_MESSAGE_TYPE::kHide, nullptr);
			messagingQueue->AddMessage(RE::ContainerMenu::MENU_NAME, RE::UI_MESSAGE_TYPE::kHide, nullptr);
			messagingQueue->AddMessage(RE::TrainingMenu::MENU_NAME, RE::UI_MESSAGE_TYPE::kHide, nullptr);
			messagingQueue->AddMessage(RE::CraftingMenu::MENU_NAME, RE::UI_MESSAGE_TYPE::kHide, nullptr);
			messagingQueue->AddMessage(RE::DialogueMenu::MENU_NAME, RE::UI_MESSAGE_TYPE::kHide, nullptr);
			messagingQueue->AddMessage(RE::StatsMenu::MENU_NAME, RE::UI_MESSAGE_TYPE::kHide, nullptr);
			messagingQueue->AddMessage(RE::TweenMenu::MENU_NAME, RE::UI_MESSAGE_TYPE::kHide, nullptr);
			messagingQueue->AddMessage(RE::BarterMenu::MENU_NAME, RE::UI_MESSAGE_TYPE::kHide, nullptr);
			messagingQueue->AddMessage(RE::SleepWaitMenu::MENU_NAME, RE::UI_MESSAGE_TYPE::kHide, nullptr);
			return true;
		}

		Error("Failed to get UIMessageQueue to close all menus. Discarding last Request!");
		return false;
	}

	static inline void OpenActorInventory(RE::TESObjectREFR* a_actorRef)
	{
		if (a_actorRef && CloseAllGameMenus()) {
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

	static inline const std::vector<BaseObject> GetInventoryList(RE::TESObjectREFR* reference = nullptr) 
	{
		Trace("Generating Inventory List...");

		reference = GetConsoleReference();
		std::vector<BaseObject> m_inventory;

		auto inventory = reference->GetInventory();
		for (auto& [obj, data] : inventory) {
			auto& [count, entry] = data;
			if (count > 0 && entry) {
				uint32_t quantity = static_cast<std::uint32_t>(count);
				m_inventory.emplace_back(entry->object, 0, 0, quantity);
			}
		}

		Trace("Cached {} inventory items.", m_inventory.size());
		return m_inventory;
	}

	static inline void AddItemToInventory(RE::TESObjectREFR* a_targetRef, const std::string& a_editorID, uint32_t a_amount = 1)
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
	}

	static inline void AddItemToRefInventory(RE::TESObjectREFR* a_targetRef, const std::string& a_editorID, uint32_t a_amount = 1)
	{
		if (!a_targetRef)
			return;

		Commands::AddItemToInventory(a_targetRef, a_editorID, a_amount);
	}

	static inline void AddItemToPlayerInventory(const std::string& a_editorID, uint32_t a_amount = 1)
	{
		auto player = RE::PlayerCharacter::GetSingleton();

		if (!player)
			return;

		AddItemToInventory(player->AsReference(), a_editorID, a_amount);
	}

	static inline void RemoveAllItemsFromInventory(RE::TESObjectREFR* a_targetRef)
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
		    }
		}
	}

	static inline void RemoveItemFromInventory(RE::TESObjectREFR* a_targetRef, const std::string& a_editorID, uint32_t a_amount = 1)
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
	}

	static inline void RemoveItemFromPlayerInventory(const std::string& a_editorID, uint32_t a_amount = 1)
	{
		auto player = RE::PlayerCharacter::GetSingleton();

		if (!player)
			return;

		auto playerRef = player->AsReference();

		if (!playerRef)
			return;

		RemoveItemFromInventory(playerRef, a_editorID, a_amount);
	}

	static inline void ResetTargetInventory(RE::TESObjectREFR* a_targetRef)
	{
		if (!a_targetRef)
			return;

		a_targetRef->ResetInventory(false);
	}

	static inline void UnequipAllItemsFromInventory(RE::TESObjectREFR* a_targetRef)
	{
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
    inline int InventoryBoundObjects(RE::TESObjectREFR* a_targetRef, const RE::TESForm* a_form, RE::TESBoundObject*& out_object, RE::ExtraDataList* out_extra)
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

	static inline void AddAndEquipItemToInventory(RE::TESObjectREFR* a_targetRef, const std::string& a_editorID, uint32_t a_amount = 1)
	{
		if (!a_targetRef)
			return;

		AddItemToInventory(a_targetRef, a_editorID, a_amount);
		
		SKSE::GetTaskInterface()->AddTask([a_targetRef, a_editorID]() {
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
			actor->AddWornItem(equipObject, 1, false, 0, 0);
			// RE::ActorEquipManager::GetSingleton()->EquipObject(actor, equipObject, extraData, 1, equipSlot);
		});
		
	}

	static inline void AddAndEquipItemToPlayerInventory(const std::string& a_editorID)
	{
		auto player = RE::PlayerCharacter::GetSingleton();

		if (!player)
			return;

		AddAndEquipItemToInventory(player->AsReference(), a_editorID, 1);
	}

	static inline void ReadBook(const std::string& a_editorID)
	{
		SKSE::GetTaskInterface()->AddTask([a_editorID]() {
			RE::TESForm* form = RE::TESForm::LookupByEditorID(a_editorID);
			Commands::AddItemToPlayerInventory(a_editorID, 1); // Ensure the book is in inventory

			if (!form) 
				return;

			RE::TESObjectBOOK* book = form->As<RE::TESObjectBOOK>();

			if (!book) 
				return;

			RE::PlayerCharacter* player = RE::PlayerCharacter::GetSingleton();

			if (!player)
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

	static inline void TeleportNPCToPlayer(uint32_t a_refID)
	{
		if (a_refID != 0) {
			if (auto player = RE::PlayerCharacter::GetSingleton(); player) {
				if (auto playerRefr = player->AsReference()) {
					if (auto ref = RE::TESForm::LookupByID<RE::TESObjectREFR>(a_refID)) {
						ref->MoveTo(playerRefr);
						CloseAllGameMenus();
					}
				}
			}
		}
	}

	static inline void TeleportREFRToPlayer(RE::TESObjectREFR* a_ref)
	{
		if (a_ref) {
			if (auto player = RE::PlayerCharacter::GetSingleton(); player) {
				if (auto playerRefr = player->AsReference()) {
					a_ref->MoveTo(playerRefr);
					CloseAllGameMenus();
				}
			}
		}
	}

	static inline void TeleportPlayerToREFR(RE::TESObjectREFR* a_ref)
	{
		if (a_ref) {
			if (auto player = RE::PlayerCharacter::GetSingleton(); player) {
				if (auto playerRefr = player->AsReference()) {
					playerRefr->MoveTo(a_ref);
					CloseAllGameMenus();
				}
			}
		}
	}

	static inline void TeleportPlayerToNPC(uint32_t a_refID)
	{
		if (a_refID != 0) {
			if (auto player = RE::PlayerCharacter::GetSingleton(); player) {
				if (auto playerRefr = player->AsReference()) {
					if (auto ref = RE::TESForm::LookupByID<RE::TESObjectREFR>(a_refID)) {
						playerRefr->MoveTo(ref);
						CloseAllGameMenus();
					}
				}
			}
		}
	}

	static inline void PlaceAtMe(const std::string& a_editorID, uint32_t a_count = 1, bool persistent = true, bool disabled = false) 
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
			Papyrus_PlaceAtMe(target, object, a_count, persistent, disabled);
		}
	}

	static inline void KillRefr(RE::TESObjectREFR* a_targetRef)
	{
		if (!a_targetRef)
			return;

		auto actor = a_targetRef->As<RE::Actor>();

		if (!actor)
			return;

		actor->KillImpl(nullptr, actor->GetActorValueMax(RE::ActorValue::kHealth), true, true);
	}

	static inline void ResurrectRefr(RE::TESObjectREFR* a_targetRef)
	{
		if (!a_targetRef)
			return;

		auto actor = a_targetRef->As<RE::Actor>();

		if (!actor)
			return;

		// TEST: Verify works
		// Do we event need this?
		actor->Resurrect(false, true);
	}

	static inline void FreezeRefr(RE::TESObjectREFR* a_targetRef, bool a_freeze)
	{
		if (!a_targetRef)
			return;

		auto actor = a_targetRef->As<RE::Actor>();

		if (!actor)
			return;

		// TEST: Verify this actually freezes an actor reliably.
		actor->SetMotionType(a_freeze ? RE::hkpMotion::MotionType::kFixed : RE::hkpMotion::MotionType::kDynamic, true);
	}

	static inline void DisableRefr(RE::TESObjectREFR* a_targetRef)
	{
		if (!a_targetRef)
			return;

		a_targetRef->Disable();
	}

	static inline void EnableRefr(RE::TESObjectREFR* a_targetRef, bool a_resetInventory = false)
	{
		if (!a_targetRef)
			return;

		a_targetRef->Enable(a_resetInventory);
	}

	static inline void CenterOnCell(const std::string& a_cellEditorID)
	{
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
	}
}
