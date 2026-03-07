#include "PlayerChestSpawn.h"
#include "data/BaseObject.h"
#include "ui/core/UIManager.h"
#include "core/Commands.h"

namespace Modex
{
	void PlayerChestSpawn::OpenChest()
	{
		auto container = m_chestHandle.get();

		if (!container) {
			Error("Could not open chest - invalid container reference handle");
			return;
		}

		auto player = RE::PlayerCharacter::GetSingleton();

		if (!player)
			return;

		auto playerRef = player->AsReference();

		if (!playerRef)
			return;

		UIManager::GetSingleton()->Close();
		container->ActivateRef(playerRef, 0, nullptr, 0, false);
		UIManager::GetSingleton()->SetMenuListener(true);
		Debug("Opened PlayerChest container");
	}

	void PlayerChestSpawn::InitializeChest()
	{
		auto container = RE::IFormFactory::GetConcreteFormFactoryByType<RE::TESObjectCONT>();
		m_chestContainer = container ? container->Create() : nullptr;

		m_chestContainer->SetFormEditorID("ModexPlayerChestCONT");
		m_chestContainer->fullName = "ModexPlayerChest";
		m_chestContainer->boundData = { { 0, 0, 0 }, { 0, 0, 0 } };
		Trace("Modex PlayerChest container Initialized");
	}

	void PlayerChestSpawn::InitializeChestHandle()
	{
		auto reference = RE::IFormFactory::GetConcreteFormFactoryByType<RE::TESObjectREFR>();
		auto containerRef = reference ? reference->Create() : nullptr;

		if (!containerRef) {
			Error("Unable to create chest reference via IFormFactory");
			return;
		}

		auto player = RE::PlayerCharacter::GetSingleton();

		if (!player)
			return;

		containerRef->formFlags |= RE::TESForm::RecordFlags::kTemporary;
		containerRef->data.objectReference = m_chestContainer;
		containerRef->extraList.SetOwner(player);
		containerRef->SetStartingPosition({ 0, 0, 0 });

		m_chestHandle = containerRef->CreateRefHandle();
		Trace("Modex PlayerChest container handle set.");
	}

	void PlayerChestSpawn::Reset()
	{
		if (m_chestContainer == nullptr) {
			InitializeChest();

			if (m_chestContainer == nullptr) {
				Error("Unable to create/reset chest container object");
				return;
			}
		}

		if (m_chestHandle.get() == nullptr) {
			InitializeChestHandle();

			if (!m_chestHandle) {
				Error("Unable to create/reset chest reference handle");
				return;
			}
		}

		auto container = m_chestHandle.get();

		if (!container || container->IsActivationBlocked()) {
			Error("Unable to reset chest container - invalid or blocked");
			return;
		}

		// TEST: Compare against commented method below.
		container->ResetInventory(false);

		// auto inventory = container->GetInventory();
		// for (auto& [obj, data] : inventory) {
		// 	auto& [count, entry] = data;
		// 	if (count > 0 && entry) {
		// 		container->RemoveItem(obj, count, RE::ITEM_REMOVE_REASON::kRemove, nullptr, nullptr);
		// 	}
		// }
	}

	void PlayerChestSpawn::PopulateChestWithOutfit(const RE::BGSOutfit* a_outfit, uint16_t a_level)
	{
		if (!a_outfit) return;

		SKSE::GetTaskInterface()->AddTask([this, a_outfit, a_level]() {
			Reset();

			auto container = m_chestHandle.get();

			if (!container)
				return;

			auto displayName = po3_GetEditorID(a_outfit->GetFormID());

			container->SetDisplayName(displayName.c_str(), true);

			auto resolved = Commands::ResolveOutfitItems(a_outfit, Commands::GetPlayerReference(), a_level);

			for (auto& entry : resolved) {
				container->AddObjectToContainer(entry.object, nullptr, entry.count, nullptr);
			}

			OpenChest();
		});
	}

	void PlayerChestSpawn::PopulateChestWithKit(const Modex::Kit& a_kit)
	{
		// Copy kit data for safe capture across threads.
		auto kitName = a_kit.GetName();
		auto kitKey = a_kit.m_key;
		auto kitItems = a_kit.m_items;
		auto kitSize = a_kit.m_items.size();

		SKSE::GetTaskInterface()->AddTask([this, kitName, kitKey, kitItems, kitSize]() {
			Reset();

			auto container = m_chestHandle.get();

			if (!container)
				return;

			container->SetDisplayName(kitName.c_str(), true);

			int _count = 0;
			for (auto& kitItem : kitItems) {
				auto boundObject = RE::TESForm::LookupByEditorID(kitItem->m_editorid);
				Trace("Adding Item '{}' from '{}' to PlayerChest container.", kitItem->m_editorid, kitKey);

				if (boundObject) {
					container->AddObjectToContainer(
						boundObject->As<RE::TESBoundObject>(),
						nullptr,
						static_cast<std::uint32_t>(kitItem->m_amount),
						nullptr
					);

					_count++;
				}
			}

			Debug("Populated PlayerChest with '{}/{}' items from kit: '{}'", _count, kitSize, kitKey);
			OpenChest();
		});
	}

	void PlayerChestSpawn::PopulateChestWithItems(const std::vector<std::unique_ptr<BaseObject>>& a_items)
	{
		if (a_items.empty()) { return; }

		// Copy editor IDs for safe capture across threads.
		std::vector<std::string> editorIDs;
		editorIDs.reserve(a_items.size());
		for (auto& item : a_items) {
			editorIDs.push_back(item->GetEditorID());
		}

		SKSE::GetTaskInterface()->AddTask([this, editorIDs = std::move(editorIDs)]() {
			Reset();

			auto container = m_chestHandle.get();

			if (!container)
				return;

			container->SetDisplayName("Modex", true);

			int _count = 0;
			for (auto& editorID : editorIDs) {
				auto boundObject = RE::TESForm::LookupByEditorID(editorID);
				Trace("Adding Item '{}' from Table to PlayerChest container", editorID);

				if (boundObject) {
					container->AddObjectToContainer(
						boundObject->As<RE::TESBoundObject>(),
						nullptr,
						1,
						nullptr
					);

					_count++;
				}
			}

			Debug("Populated PlayerChest with '{}/{}' items from Table.", _count, editorIDs.size());
			OpenChest();
		});
	}
}
