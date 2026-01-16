#include "PlayerChestSpawn.h"
#include "ui/core/UIManager.h"

namespace Modex
{
    void PlayerChestSpawn::OpenChest()
    {
        auto container = m_chestHandle.get();

        if (!container) {
            PrettyLog::Error("Could not open chest - invalid container reference handle");
            return;
        }

        auto player = RE::PlayerCharacter::GetSingleton();
		auto playerRef = player->AsReference();

        UIManager::GetSingleton()->Close();
		container->ActivateRef(playerRef, 0, nullptr, 0, false);
        UIManager::GetSingleton()->SetMenuListener(true);
        PrettyLog::Debug("Opened player chest container");
    }

    void PlayerChestSpawn::InitializeChest()
    {
        auto container = RE::IFormFactory::GetConcreteFormFactoryByType<RE::TESObjectCONT>();
		m_chestContainer = container ? container->Create() : nullptr;

		m_chestContainer->SetFormEditorID("ModexPlayerChestCONT");
		m_chestContainer->fullName = "ModexPlayerChest";
		m_chestContainer->boundData = { { 0, 0, 0 }, { 0, 0, 0 } };
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
    }

    void PlayerChestSpawn::Reset()
    {
        if (m_chestContainer == nullptr) {
            InitializeChest();

            if (m_chestContainer == nullptr) {
                PrettyLog::Error("Unable to create/reset chest container object");
                return;
            }
        }

        if (m_chestHandle.get() == nullptr) {
            InitializeChestHandle();

            if (!m_chestHandle) {
                PrettyLog::Error("Unable to create/reset chest reference handle");
                return;
            }
        }

        auto container = m_chestHandle.get();

        if (!container || container->IsActivationBlocked()) {
            PrettyLog::Error("Unable to reset chest container - invalid or blocked");
            return;
        }

        container->ResetInventory(false); // TODO: Compare against other method.

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

        for (auto& kitItem : a_kit.m_items) {
            auto boundObject = RE::TESForm::LookupByEditorID(kitItem->m_editorid);

            if (boundObject) {
                container->AddObjectToContainer(
                    boundObject->As<RE::TESBoundObject>(),      // RE::TESBoundObject* a_item,
                    nullptr,                                    // RE::ExtraDataList* a_extraData,
                    static_cast<std::uint32_t>(kitItem->m_amount),// std::uint32_t a_count,
                    nullptr                                     // RE::TESObjectREFR* a_fromRefr,
                );
            }
        }

        OpenChest();
    }

    void PlayerChestSpawn::PopulateChestWithItems(const std::vector<std::unique_ptr<BaseObject>>& a_items)
    {
        Reset();

        auto container = m_chestHandle.get();
        container->SetDisplayName("Modex Search Results", true);

        for (auto& item : a_items) {
            auto boundObject = RE::TESForm::LookupByEditorID(item->GetEditorID());

            if (boundObject) {
                container->AddObjectToContainer(
                    boundObject->As<RE::TESBoundObject>(),      // RE::TESBoundObject* a_item,
                    nullptr,                                    // RE::ExtraDataList* a_extraData,
                    1,                                          // std::uint32_t a_count,
                    nullptr                                     // RE::TESObjectREFR* a_fromRefr,
                );
            }
        }

        OpenChest();
    }
}