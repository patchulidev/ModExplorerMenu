#include "PlayerChestSpawn.h"
#include "RE/T/TESBoundObject.h"
#include "data/BaseObject.h"
#include "ui/core/UIManager.h"
#include "core/Commands.h"

#include <cmath>

namespace Modex
{
	constexpr float kChestForwardDistance = 110.0f;
	constexpr float kChestHiddenDepth = 1000.0f;

	// NOTE: Should we expose the hidden depth in settings? O.o

	void PlayerChestSpawn::InitializeBaseContainer()
	{
		// chest container from skyrim.esm just to be safe, hopefully.
		auto* form = RE::TESForm::LookupByID(RE::FormID(0x000D762F));
		m_chestContainer = form ? form->As<RE::TESObjectCONT>() : nullptr;

		if (!m_chestContainer) {
			Error("Failed to lookup vanilla chest base form [0x000D762F]");
			return;
		}

		Trace("Modex PlayerChest using base container [{:08X}]", m_chestContainer->GetFormID());
	}

	RE::TESObjectREFR* PlayerChestSpawn::SpawnChestReference(Placement a_placement)
	{
		if (!m_chestContainer) {
			InitializeBaseContainer();

			if (!m_chestContainer)
				return nullptr;
		}

		auto player = RE::PlayerCharacter::GetSingleton();

		if (!player)
			return nullptr;

		auto* playerRef = player->AsReference();

		if (!playerRef)
			return nullptr;

		m_chestRefHandle.reset();

		// Tried to use create reference, but I think this is safer.
		// Do not flag as persistent for our use, potentially reason for save corruption
		auto* chestRef = Commands::Papyrus_PlaceAtMe(playerRef, m_chestContainer, 1, false, false);

		if (!chestRef) {
			Error("Failed to place PlayerChest reference in the world");
			return nullptr;
		}

		// Best guess at positioning the chest
		auto playerPos = player->GetPosition();
		float heading = player->GetAngle().z;

		RE::NiPoint3 spawnPos = playerPos;
		switch (a_placement) {
		case Placement::HiddenBelowPlayer:
			spawnPos.z -= kChestHiddenDepth;
			break;
		case Placement::InFrontOfPlayer:
		default:
			spawnPos.x += std::sin(heading) * kChestForwardDistance;
			spawnPos.y += std::cos(heading) * kChestForwardDistance;
			break;
		}

		RE::NiPoint3 uprightAngle = { 0.0f, 0.0f, heading };

		chestRef->SetPosition(spawnPos);
		chestRef->SetAngle(uprightAngle);
		chestRef->Update3DPosition(true);

		// assign ownership to circumvent stolen flag in interiors
		if (auto* playerBase = player->GetObjectReference()) {
			chestRef->extraList.SetOwner(playerBase);
		}

		// flag OnContainerMenuClosed so hidden chests don't accumulate.
		m_cleanupOnClose = (a_placement == Placement::HiddenBelowPlayer);

		m_chestRefHandle = chestRef->GetHandle();
		Debug("Spawned new PlayerChest reference [{:08X}]", chestRef->GetFormID());
		return chestRef;
	}

	void PlayerChestSpawn::QueueOrDispatch(std::function<void()> a_action)
	{
		auto* ui = UIManager::GetSingleton();

		if (ui->IsMenuOpen()) {
			{
				std::lock_guard<std::mutex> lock(m_pendingMutex);
				m_pendingAction = std::move(a_action);
			}
			ui->Close();
			ui->SetMenuListener(true);
		} else {
			SKSE::GetTaskInterface()->AddTask(std::move(a_action));
		}
	}

	void PlayerChestSpawn::OnContainerMenuClosed()
	{
		if (!m_cleanupOnClose) return;

		m_cleanupOnClose = false;

		// Be safe and do all this on game thread, not UI thread
		SKSE::GetTaskInterface()->AddTask([this]() {
			auto* chestRef = m_chestRefHandle.get().get();
			if (!chestRef) return;

			chestRef->Disable();
			chestRef->SetDelete(true);
			m_chestRefHandle.reset();
			Debug("Discarded hidden PlayerChest");
		});
	}

	void PlayerChestSpawn::OnModexClosed()
	{
		std::function<void()> action;
		{
			std::lock_guard<std::mutex> lock(m_pendingMutex);
			action = std::move(m_pendingAction);
			m_pendingAction = nullptr;
		}

		if (!action)
			return;

		SKSE::GetTaskInterface()->AddTask(std::move(action));
	}

	void PlayerChestSpawn::OpenChest()
	{
		auto* chestRef = m_chestRefHandle.get().get();

		if (!chestRef) {
			Error("Could not open chest - no active container reference");
			return;
		}

		auto player = RE::PlayerCharacter::GetSingleton();

		if (!player)
			return;

		auto playerRef = player->AsReference();

		if (!playerRef)
			return;

		if (UIManager::GetSingleton()->IsMenuOpen()) {
			UIManager::GetSingleton()->Close();
			UIManager::GetSingleton()->SetMenuListener(true);
		}

		chestRef->ActivateRef(playerRef, 0, nullptr, 0, false);
		Debug("Opened PlayerChest container");
	}

	void PlayerChestSpawn::PopulateChestWithOutfit(const RE::BGSOutfit* a_outfit, uint16_t a_level, Placement a_placement)
	{
		if (!a_outfit) return;

		QueueOrDispatch([this, a_outfit, a_level, a_placement]() {
			auto container = SpawnChestReference(a_placement);

			if (!container)
				return;

			auto displayName = po3_GetEditorID(a_outfit->GetFormID());
			container->SetDisplayName(displayName.c_str(), true);

			auto* playerRef = Commands::GetPlayerReference();
			auto resolved = Commands::ResolveOutfitItems(a_outfit, playerRef, a_level);

			for (auto& entry : resolved) {
				container->AddObjectToContainer(entry.object, nullptr, entry.count, nullptr);
			}

			SKSE::GetTaskInterface()->AddTask([this]() { OpenChest(); });
		});
	}

	void PlayerChestSpawn::PopulateChestWithKit(const Modex::Kit& a_kit, Placement a_placement)
	{
		auto kitName = a_kit.GetName();
		auto kitKey = a_kit.m_key;
		auto kitItems = a_kit.m_items;

		QueueOrDispatch([this, kitName, kitKey, kitItems, a_placement]() {
			auto container = SpawnChestReference(a_placement);

			if (!container)
				return;

			container->SetDisplayName(kitName.c_str(), true);

			int _count = 0;
			for (auto& kitItem : kitItems) {
				auto boundObject = RE::TESForm::LookupByEditorID(kitItem->m_editorid);
				Trace("Adding Item '{}' from '{}' to PlayerChest container.", kitItem->m_editorid, kitKey);

				if (boundObject) {
					if (auto bound = boundObject->As<RE::TESBoundObject>()) {
						container->AddObjectToContainer(
							bound,
							nullptr,
							static_cast<std::uint32_t>(kitItem->m_amount),
							nullptr);

						_count++;
					}
				}
			}

			Debug("Populated PlayerChest with '{}/{}' items from kit: '{}'", _count, kitItems.size(), kitKey);

			SKSE::GetTaskInterface()->AddTask([this]() { OpenChest(); });
		});
	}

	void PlayerChestSpawn::PopulateChestWithActorInventory(RE::TESObjectREFR* a_actor, Placement a_placement)
	{
		if (!a_actor) return;

		// Capture by handle in case reference dies between calls
		auto actorHandle = a_actor->GetHandle();

		QueueOrDispatch([this, actorHandle, a_placement]() {
			auto* actor = actorHandle.get().get();
			if (!actor) return;

			auto container = SpawnChestReference(a_placement);
			if (!container) return;

			container->SetDisplayName(actor->GetDisplayFullName(), true);

			auto inventory = actor->GetInventory();

			int _count = 0;
			for (auto& [bound, data] : inventory) {
				const auto count = data.first;
				if (bound && count > 0) {
					container->AddObjectToContainer(bound, nullptr, count, nullptr);
					_count++;
				}
			}

			Debug("Populated PlayerChest with '{}' unique items from actor: '{}'",
				_count, actor->GetDisplayFullName());

			SKSE::GetTaskInterface()->AddTask([this]() { OpenChest(); });
		});
	}

	void PlayerChestSpawn::PopulateChestWithItems(const std::vector<std::unique_ptr<BaseObject>>& a_items, Placement a_placement)
	{
		if (a_items.empty()) { return; }

		std::vector<std::string> editorIDs;
		editorIDs.reserve(a_items.size());
		for (auto& item : a_items) {
			editorIDs.push_back(item->GetEditorID());
		}

		QueueOrDispatch([this, items = std::move(editorIDs), a_placement]() {
			auto container = SpawnChestReference(a_placement);

			if (!container)
				return;

			container->SetDisplayName("Modex", true);

			int _count = 0;
			for (auto& editorID : items) {
				auto boundObject = RE::TESForm::LookupByEditorID(editorID);
				Trace("Adding Item '{}' from Table to PlayerChest container", editorID);

				if (boundObject) {
					if (auto bound = boundObject->As<RE::TESBoundObject>()) {
						container->AddObjectToContainer(bound, nullptr, 1, nullptr);
						_count++;
					}
				}
			}

			Debug("Populated PlayerChest with '{}/{}' items from Table.", _count, items.size());

			SKSE::GetTaskInterface()->AddTask([this]() { OpenChest(); });
		});
	}
}
