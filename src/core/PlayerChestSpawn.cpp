#include "PlayerChestSpawn.h"
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
		auto playerRef = player->AsReference();

		UIManager::CloseAllGameMenus();
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

		auto player = RE::PlayerCharacter::GetSingleton();
		auto playerRef = player->GetObjectReference();

		containerRef->formFlags |= RE::TESForm::RecordFlags::kTemporary;
		containerRef->data.objectReference = m_chestContainer;
		containerRef->extraList.SetOwner(playerRef);
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

	void PlayerChestSpawn::PopulateChestWithKit(const Modex::Kit& a_kit)
	{
		Reset();

		auto container = m_chestHandle.get();
		container->SetDisplayName(a_kit.GetName().c_str(), true);

		int _count = 0;
		for (auto& kitItem : a_kit.m_items) {
			auto boundObject = RE::TESForm::LookupByEditorID(kitItem->m_editorid);
			Trace("Adding Item '{}' from '{}' to PlayerChest container.", kitItem->m_editorid, a_kit.m_key);

			if (boundObject) {
				container->AddObjectToContainer(
					boundObject->As<RE::TESBoundObject>(),      // RE::TESBoundObject* a_item,
					nullptr,                                    // RE::ExtraDataList* a_extraData,
					static_cast<std::uint32_t>(kitItem->m_amount),// std::uint32_t a_count,
					nullptr                                     // RE::TESObjectREFR* a_fromRefr,
				);

				_count++;
			}
		}

		Debug("Populated PlayerChest with '{}/{}' items from kit: '{}'", _count, a_kit.m_items.size(), a_kit.m_key);
		OpenChest();
	}

	void PlayerChestSpawn::PopulateChestWithItems(const std::vector<std::unique_ptr<BaseObject>>& a_items)
	{
		if (a_items.empty()) { return; }

		Reset();

		auto container = m_chestHandle.get();
		container->SetDisplayName("Modex", true);

		int _count = 0;
		for (auto& item : a_items) {
			auto boundObject = RE::TESForm::LookupByEditorID(item->GetEditorID());
			Trace("Adding Item '{}' from Table to PlayerChest container", item->GetEditorID());

			if (boundObject) {
				container->AddObjectToContainer(
					boundObject->As<RE::TESBoundObject>(),      // RE::TESBoundObject* a_item,
					nullptr,                                    // RE::ExtraDataList* a_extraData,
					1,                                          // std::uint32_t a_count,
					nullptr                                     // RE::TESObjectREFR* a_fromRefr,
				);

				_count ++;
			}
		}

		Debug("Populated PlayerChest with '{}/{}' items from Table.", _count, a_items.size());
		OpenChest();
	}
}
