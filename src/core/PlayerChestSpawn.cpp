#include "PlayerChestSpawn.h"
#include "RE/T/TESBoundObject.h"
#include "data/BaseObject.h"
#include "ui/core/UIManager.h"
#include "core/Commands.h"

namespace Modex
{
	void PlayerChestSpawn::InitializeBaseContainer()
	{
		auto factory = RE::IFormFactory::GetConcreteFormFactoryByType<RE::TESObjectCONT>();
		m_chestContainer = factory ? factory->Create() : nullptr;

		if (!m_chestContainer) {
			Error("Failed to create PlayerChest base container form");
			return;
		}

		m_chestContainer->SetFormEditorID("ModexPlayerChestCONT");
		m_chestContainer->fullName = "ModexPlayerChest";
		m_chestContainer->boundData = { { 0, 0, 0 }, { 0, 0, 0 } };
		Trace("Modex PlayerChest base container initialized");
	}

	RE::TESObjectREFR* PlayerChestSpawn::SpawnChestReference()
	{
		if (!m_chestContainer) {
			InitializeBaseContainer();

			if (!m_chestContainer)
				return nullptr;
		}

		auto player = RE::PlayerCharacter::GetSingleton();

		if (!player)
			return nullptr;

		auto playerRef = player->AsReference();

		if (!playerRef)
			return nullptr;

		// Destroy previous reference if it exists.
		DestroyChestReference();

		// Place a fresh container reference in the world near the player.
		// This goes through the engine's full placement pipeline, ensuring
		// proper cell membership, ExtraContainerChanges, and extra data init.
		m_chestRef = Commands::Papyrus_PlaceAtMe(playerRef, m_chestContainer, 1, false, false);

		if (!m_chestRef) {
			Error("Failed to place PlayerChest reference in the world");
			return nullptr;
		}

		Debug("Spawned new PlayerChest reference [{:08X}]", m_chestRef->GetFormID());
		return m_chestRef;
	}

	void PlayerChestSpawn::DestroyChestReference()
	{
		if (!m_chestRef)
			return;

		m_chestRef->Disable();
		m_chestRef->SetDelete(true);
		Debug("Destroyed PlayerChest reference [{:08X}]", m_chestRef->GetFormID());
		m_chestRef = nullptr;
	}

	void PlayerChestSpawn::OpenChest()
	{
		if (!m_chestRef) {
			Error("Could not open chest - no active container reference");
			return;
		}

		auto player = RE::PlayerCharacter::GetSingleton();

		if (!player)
			return;

		auto playerRef = player->AsReference();

		if (!playerRef)
			return;

		UIManager::GetSingleton()->Close();
		m_chestRef->ActivateRef(playerRef, 0, nullptr, 0, false);
		UIManager::GetSingleton()->SetMenuListener(true);
		Debug("Opened PlayerChest container");
	}

	void PlayerChestSpawn::PopulateChestWithOutfit(const RE::BGSOutfit* a_outfit, uint16_t a_level)
	{
		if (!a_outfit) return;

		// Frame 1: Spawn fresh container and populate it.
		SKSE::GetTaskInterface()->AddTask([this, a_outfit, a_level]() {
			auto container = SpawnChestReference();

			if (!container)
				return;

			auto displayName = po3_GetEditorID(a_outfit->GetFormID());
			container->SetDisplayName(displayName.c_str(), true);

			auto resolved = Commands::ResolveOutfitItems(a_outfit, Commands::GetPlayerReference(), a_level);

			for (auto& entry : resolved) {
				container->AddObjectToContainer(entry.object, nullptr, entry.count, nullptr);
			}

			// Frame 2: Open after engine processes inventory.
			SKSE::GetTaskInterface()->AddTask([this]() {
				OpenChest();
			});
		});
	}

	void PlayerChestSpawn::PopulateChestWithKit(const Modex::Kit& a_kit)
	{
		auto kitName = a_kit.GetName();
		auto kitKey = a_kit.m_key;
		auto kitItems = a_kit.m_items;
		auto kitSize = a_kit.m_items.size();

		// Frame 1: Spawn fresh container and populate it.
		SKSE::GetTaskInterface()->AddTask([this, kitName, kitKey, kitItems, kitSize]() {
			auto container = SpawnChestReference();

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

			// Frame 2: Open after engine processes inventory.
			SKSE::GetTaskInterface()->AddTask([this]() {
				OpenChest();
			});
		});
	}

	void PlayerChestSpawn::PopulateChestWithItems(const std::vector<std::unique_ptr<BaseObject>>& a_items)
	{
		if (a_items.empty()) { return; }

		std::vector<std::string> editorIDs;
		editorIDs.reserve(a_items.size());
		for (auto& item : a_items) {
			editorIDs.push_back(item->GetEditorID());
		}

		// Frame 1: Spawn fresh container and populate it.
		SKSE::GetTaskInterface()->AddTask([this, editorIDs = std::move(editorIDs)]() {
			auto container = SpawnChestReference();

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

			// Frame 2: Open after engine processes inventory.
			SKSE::GetTaskInterface()->AddTask([this]() {
				OpenChest();
			});
		});
	}
}
