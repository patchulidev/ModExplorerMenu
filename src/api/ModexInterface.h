#pragma once

#include "include/ModexAPI.h"
#include "data/Data.h"

namespace Modex
{

	static_assert(static_cast<uint32_t>(Modex::PropertyType::kTotal) == 83,
		"Internal PropertyType changed - update ModexAPI::PropertyType to match");

	static_assert(static_cast<uint32_t>(Modex::Ownership::All) == 7,
		"Internal Ownership changed - update ModexAPI::CacheType to match");

	class ModexInterface : public ModexAPI::IModexInterface001
	{
	public:
		static ModexInterface* GetSingleton()
		{
			static ModexInterface singleton;
			return std::addressof(singleton);
		}

		void UpdateSettings() override;

		void AddItemToPlayer(uint32_t a_formID, uint32_t a_amount = 1) override;
		void AddItemToInventory(uint32_t a_formID, uint32_t a_targetReference, uint32_t a_amount = 1) override;

		void OpenModexContainer() override;
		void OpenInventory(uint32_t a_targetReference) override;

		void AddLeveledListToInventory(uint32_t a_formID, uint32_t a_targetReference, uint16_t a_amount = 1) override;

		void RemoveAllItems(uint32_t a_targetReference) override;
		void RemoveItem(uint32_t a_formID, uint32_t a_targetReference, uint32_t a_amount = 1) override;

		void PlaceAtMe(uint32_t a_formID, uint32_t a_count = 1, bool a_persistent = true, bool a_disabled = false) override;

		void TeleportPlayerToNPC(uint32_t a_refID) override;
		void TeleportNPCToPlayer(uint32_t a_refID) override;
		void CenterOnCell(const char* a_cellEditorID) override;

		void KillActor(uint32_t a_refID) override;
		void ResurrectActor(uint32_t a_refID) override;
		void DisableReference(uint32_t a_refID) override;
		void EnableReference(uint32_t a_refID, bool a_resetInventory = false) override;

		bool IsDataReady() override;
		unsigned int GetCachedFormCount(ModexAPI::CacheType a_type) override;
		bool IsFormCached(uint32_t a_formID, ModexAPI::CacheType a_type) override;
		void OpenMenu() override;
		void CloseMenu() override;
		bool IsMenuOpen() override;
		void OpenFormSelector(ModexAPI::CacheType a_type, void (*a_callback)(const uint32_t* a_formIDs, uint32_t a_count)) override;
		void OpenFormSelector(ModexAPI::CacheType a_type, const ModexAPI::FormSelectorOptions& a_options, void (*a_callback)(const uint32_t* a_formIDs, uint32_t a_count)) override;
		uint32_t GetCachedForms(ModexAPI::CacheType a_type, ModexAPI::FormEntry* a_outBuffer, uint32_t a_maxCount) override;
		bool GetFormProperty(uint32_t a_formID, ModexAPI::CacheType a_type, ModexAPI::PropertyType a_property, char* a_outBuffer, uint32_t a_bufferSize) override;

		void AddOutfitToInventory(uint32_t a_outfitFormID, uint32_t a_targetReference, uint16_t a_level = 0) override;
		void EquipOutfit(uint32_t a_outfitFormID, uint32_t a_targetReference, uint16_t a_level = 0) override;
		void SetDefaultOutfit(uint32_t a_outfitFormID, uint32_t a_targetReference) override;
		void SetSleepOutfit(uint32_t a_outfitFormID, uint32_t a_targetReference) override;

		// Called after Data::Run() completes to signal the API is ready.
		static void SetDataReady(bool a_ready);

		// Provider-side: handle incoming SKSE messages requesting our interface.
		static void HandleInterfaceRequest(SKSE::MessagingInterface::Message* a_msg);

	private:
		static void* GetApiFunction(unsigned int a_revisionNumber);
	};
}
