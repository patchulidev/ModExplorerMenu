#include "ModexInterface.h"

#include "RE/P/PlayerCharacter.h"
#include "SKSE/API.h"
#include "data/Data.h"
#include "core/Commands.h"
#include "core/PlayerChestSpawn.h"
#include "config/UserConfig.h"
#include "ui/core/UIManager.h"
#include "ui/modules/formselector/FormSelectorOptions.h"

// Revision number is the contract for a specific version of the API. Calling `GetApi(1)` will
// always return the v001 interface. Breaking changes are introduced in later subsequent revisions.
// API users will be required to rebuild if they want to target later API versons.

namespace Modex
{
	static std::atomic<bool> g_dataReady{ false };

	static std::vector<BaseObject>* GetCacheList(ModexAPI::CacheType a_type)
	{
		auto* data = Data::GetSingleton();
		switch (a_type) {
		case ModexAPI::CacheType::kItem:
			return &data->GetAddItemList();
		case ModexAPI::CacheType::kNPC:
			return &data->GetNPCList();
		case ModexAPI::CacheType::kObject:
			return &data->GetObjectList();
		case ModexAPI::CacheType::kCell:
			return &data->GetTeleportList();
		case ModexAPI::CacheType::kOutfit:
			return &data->GetOutfitList();
		default:
			return nullptr;
		}
	}

	void ModexInterface::SetDataReady(bool a_ready)
	{
		g_dataReady.store(a_ready);
	}

	bool ModexInterface::IsDataReady()
	{
		return g_dataReady.load();
	}

	void ModexInterface::UpdateSettings()
	{
		UserConfig::GetSingleton()->LoadSettings();
	}

	unsigned int ModexInterface::GetCachedFormCount(ModexAPI::CacheType a_type)
	{
		if (!IsDataReady()) {
			return 0;
		}

		auto* list = GetCacheList(a_type);
		return list ? static_cast<unsigned int>(list->size()) : 0;
	}

	void ModexInterface::AddItemToPlayer(RE::FormID a_formID, uint32_t a_amount)
	{
		if (auto* target = RE::PlayerCharacter::GetSingleton(); target->AsReference()) {
			Commands::AddItemToInventory(Ownership::None, target->AsReference(), a_formID, a_amount);
		}
	}

	void ModexInterface::AddItemToPlayer(const char* a_editorID, uint32_t a_amount)
	{
		if (auto* target = RE::PlayerCharacter::GetSingleton(); target->AsReference()) {
			Commands::AddItemToInventory(Ownership::None, target->AsReference(), a_editorID, a_amount);
		}
	}

	void ModexInterface::AddItemToInventory(RE::FormID a_formID, RE::FormID a_targetReference, uint32_t a_amount)
	{
		if (auto* target = RE::TESForm::LookupByID<RE::TESObjectREFR>(a_targetReference); target) {
			Commands::AddItemToInventory(Ownership::None, target, a_formID, a_amount);
		}
	}

	void ModexInterface::AddItemToInventory(const char* a_editorID, RE::FormID a_targetReference, uint32_t a_amount)
	{
		if (auto* target = RE::TESForm::LookupByID<RE::TESObjectREFR>(a_targetReference); target) {
			Commands::AddItemToInventory(Ownership::None, target, a_editorID, a_amount);
		}
	}

	void ModexInterface::OpenModexContainer()
	{
		PlayerChestSpawn::GetSingleton()->OpenChest();
	}

	void ModexInterface::OpenInventory(RE::FormID a_targetReference)
	{
		if (auto* target = RE::TESForm::LookupByID<RE::TESObjectREFR>(a_targetReference); target) {
			if (target->GetBaseObject()->IsActor() || target->GetBaseObject()->GetFormType() == RE::FormType::Container) {
				Commands::OpenActorInventory(target);
			}
		}
	}

	void ModexInterface::AddLeveledListToInventory(RE::FormID a_formID, RE::FormID a_targetReference, uint16_t a_amount)
	{
		if (auto* target = RE::TESForm::LookupByID<RE::TESObjectREFR>(a_targetReference); target) {
			if (target->GetBaseObject()->IsActor()) {
				Commands::AddLeveledListToRefInventory(Ownership::None, target, a_formID, a_amount);
			}
		}
	}

	void ModexInterface::AddLeveledListToInventory(const char* a_editorID, RE::FormID a_targetReference, uint16_t a_amount)
	{
		if (auto* target = RE::TESForm::LookupByID<RE::TESObjectREFR>(a_targetReference); target) {
			if (target->GetBaseObject()->IsActor()) {
				Commands::AddLeveledListToRefInventory(Ownership::None, target, a_editorID, a_amount);
			}
		}
	}

	void ModexInterface::RemoveAllItems(RE::FormID a_targetReference)
	{
		if (auto* target = RE::TESForm::LookupByID<RE::TESObjectREFR>(a_targetReference); target) {
			if (target->GetBaseObject()->IsActor() || target->GetBaseObject()->GetFormType() == RE::FormType::Container) {
				Commands::RemoveAllItemsFromInventory(Ownership::None, target);
			}
		}
	}

	void ModexInterface::RemoveItem(const char* a_editorID, RE::FormID a_targetReference, uint32_t a_amount)
	{
		if (auto* target = RE::TESForm::LookupByID<RE::TESObjectREFR>(a_targetReference); target) {
			Commands::RemoveItemFromInventory(Ownership::None, target, a_editorID, a_amount);
		}
	}

	void ModexInterface::PlaceAtMe(const char* a_editorID, uint32_t a_count, bool a_persistent, bool a_disabled)
	{
		Commands::PlaceAtMe(Ownership::None, a_editorID, a_count, a_persistent, a_disabled);
	}

	void ModexInterface::TeleportPlayerToNPC(RE::FormID a_targetReference)
	{
		Commands::TeleportPlayerToNPC(Ownership::None, a_targetReference);
	}

	void ModexInterface::TeleportNPCToPlayer(RE::FormID a_targetReference)
	{
		Commands::TeleportNPCToPlayer(Ownership::None, a_targetReference);
	}

	void ModexInterface::CenterOnCell(const char* a_cellEditorID)
	{
		Commands::CenterOnCell(Ownership::None, a_cellEditorID);
	}

	void ModexInterface::KillActor(RE::FormID a_targetReference)
	{
		if (auto* target = RE::TESForm::LookupByID<RE::TESObjectREFR>(a_targetReference); target) {
			Commands::KillRefr(Ownership::None, target);
		}
	}

	void ModexInterface::ResurrectActor(RE::FormID a_targetReference)
	{
		if (auto* target = RE::TESForm::LookupByID<RE::TESObjectREFR>(a_targetReference); target) {
			Commands::ResurrectRefr(Ownership::None, target);
		}
	}

	void ModexInterface::DisableReference(RE::FormID a_targetReference)
	{
		if (auto* target = RE::TESForm::LookupByID<RE::TESObjectREFR>(a_targetReference); target) {
			Commands::DisableRefr(Ownership::None, target);
		}
	}

	void ModexInterface::EnableReference(RE::FormID a_targetReference, bool a_resetInventory)
	{
		if (auto* target = RE::TESForm::LookupByID<RE::TESObjectREFR>(a_targetReference); target) {
			Commands::EnableRefr(Ownership::None, target, a_resetInventory);
		}
	}

	void ModexInterface::AddOutfitToInventory(RE::FormID a_outfitFormID, RE::FormID a_targetReference, uint16_t a_level)
	{
		auto* outfit = RE::TESForm::LookupByID<RE::BGSOutfit>(a_outfitFormID);
		auto* target = RE::TESForm::LookupByID<RE::TESObjectREFR>(a_targetReference);

		if (outfit && target) {
			Commands::AddOutfitItemsToInventory(Ownership::None, target, outfit, a_level);
		}
	}

	void ModexInterface::EquipOutfit(RE::FormID a_outfitFormID, RE::FormID a_targetReference, uint16_t a_level)
	{
		auto* outfit = RE::TESForm::LookupByID<RE::BGSOutfit>(a_outfitFormID);
		auto* target = RE::TESForm::LookupByID<RE::TESObjectREFR>(a_targetReference);

		if (outfit && target) {
			Commands::EquipOutfit(Ownership::None, target, outfit, a_level);
		}
	}

	void ModexInterface::SetDefaultOutfit(RE::FormID a_outfitFormID, RE::FormID a_targetReference)
	{
		auto* outfit = RE::TESForm::LookupByID<RE::BGSOutfit>(a_outfitFormID);
		auto* target = RE::TESForm::LookupByID<RE::TESObjectREFR>(a_targetReference);

		if (outfit && target) {
			Commands::SetDefaultOutfitOnActor(Ownership::None, target, outfit);
		}
	}

	void ModexInterface::SetSleepOutfit(RE::FormID a_outfitFormID, RE::FormID a_targetReference)
	{
		auto* outfit = RE::TESForm::LookupByID<RE::BGSOutfit>(a_outfitFormID);
		auto* target = RE::TESForm::LookupByID<RE::TESObjectREFR>(a_targetReference);

		if (outfit && target) {
			Commands::SetSleepOutfitOnActor(Ownership::None, target, outfit);
		}
	}

	bool ModexInterface::IsFormCached(RE::FormID a_formID, ModexAPI::CacheType a_type)
	{
		if (!IsDataReady()) {
			return false;
		}

		auto* list = GetCacheList(a_type);
		if (!list) {
			return false;
		}

		for (const auto& obj : *list) {
			if (obj.GetBaseFormID() == a_formID) {
				return true;
			}
		}

		return false;
	}

	void ModexInterface::OpenMenu()
	{
		auto* ui = UIManager::GetSingleton();
		if (!ui->IsMenuOpen()) {
			ui->QueueOpen();
		}
	}

	void ModexInterface::CloseMenu()
	{
		auto* ui = UIManager::GetSingleton();
		if (ui->IsMenuOpen()) {
			ui->QueueOpen();
		}
	}

	bool ModexInterface::IsMenuOpen()
	{
		return UIManager::GetSingleton()->IsMenuOpen();
	}

	static Ownership CacheTypeToOwnership(ModexAPI::CacheType a_type)
	{
		switch (a_type) {
		case ModexAPI::CacheType::kItem:   return Ownership::Item;
		case ModexAPI::CacheType::kNPC:    return Ownership::Actor;
		case ModexAPI::CacheType::kCell:   return Ownership::Cell;
		case ModexAPI::CacheType::kOutfit: return Ownership::Outfit;
		case ModexAPI::CacheType::kObject: return Ownership::Object;
		default:                           return Ownership::Item;
		}
	}

	static FormSelectorOptions ConvertOptions(const ModexAPI::FormSelectorOptions& a_options)
	{
		FormSelectorOptions opts;
		opts.singleSelect = a_options.singleSelect;
		opts.showTotalCost = a_options.showTotalCost;
		opts.requireTotalCost = a_options.requireTotalCost;
		opts.maxCost = a_options.maxCost;
		opts.maxCount = a_options.maxCount;
		opts.costMultiplier = a_options.costMultiplier;
		if (a_options.title) {
			opts.title = a_options.title;
		}
		return opts;
	}

	void ModexInterface::OpenFormSelector(ModexAPI::CacheType a_type, void (*a_callback)(const RE::FormID* a_formIDs, uint32_t a_count))
	{
		if (!IsDataReady() || !a_callback) {
			return;
		}

		UIManager::GetSingleton()->OpenFormSelector(CacheTypeToOwnership(a_type), a_callback);
	}

	void ModexInterface::OpenFormSelector(ModexAPI::CacheType a_type, const ModexAPI::FormSelectorOptions& a_options, void (*a_callback)(const RE::FormID* a_formIDs, uint32_t a_count))
	{
		if (!IsDataReady() || !a_callback) {
			return;
		}

		UIManager::GetSingleton()->OpenFormSelector(CacheTypeToOwnership(a_type), a_callback, ConvertOptions(a_options));
	}

	uint32_t ModexInterface::GetCachedForms(ModexAPI::CacheType a_type, ModexAPI::FormEntry* a_outBuffer, uint32_t a_maxCount)
	{
		if (!IsDataReady()) {
			return 0;
		}

		auto* list = GetCacheList(a_type);
		if (!list) {
			return 0;
		}

		uint32_t total = static_cast<uint32_t>(list->size());

		if (a_outBuffer) {
			uint32_t count = (std::min)(total, a_maxCount);
			for (uint32_t i = 0; i < count; ++i) {
				auto& obj = (*list)[i];
				a_outBuffer[i].formID = obj.GetBaseFormID();
				a_outBuffer[i].refID = obj.GetRefID();
				a_outBuffer[i].formType = obj.GetFormType();
				a_outBuffer[i].name = obj.GetName().c_str();
				a_outBuffer[i].editorID = obj.GetEditorID().c_str();
				a_outBuffer[i].plugin = obj.GetPluginName().c_str();
			}
		}

		return total;
	}

	bool ModexInterface::GetFormProperty(RE::FormID a_formID, ModexAPI::CacheType a_type, ModexAPI::PropertyType a_property, char* a_outBuffer, uint32_t a_bufferSize)
	{
		if (!IsDataReady() || !a_outBuffer || a_bufferSize == 0) {
			return false;
		}

		auto* list = GetCacheList(a_type);
		if (!list) {
			return false;
		}

		for (auto& obj : *list) {
			if (obj.GetBaseFormID() == a_formID) {
				auto internalProp = static_cast<Modex::PropertyType>(static_cast<uint32_t>(a_property));
				std::string value = obj.GetPropertyByValue(internalProp);

				size_t copyLen = (std::min)(value.size(), static_cast<size_t>(a_bufferSize - 1));
				std::memcpy(a_outBuffer, value.c_str(), copyLen);
				a_outBuffer[copyLen] = '\0';
				return true;
			}
		}

		return false;
	}

	// Specify which struct a user is requesting as API revisions may differ.
	void* ModexInterface::GetApiFunction(unsigned int a_revisionNumber)
	{
		switch (a_revisionNumber) {
		case 1:
			return static_cast<ModexAPI::IModexInterface001*>(GetSingleton());
		default:
			return nullptr;
		}
	}

	// Handle incoming SKSE messages from other plugins requesting our interface.
	void ModexInterface::HandleInterfaceRequest(SKSE::MessagingInterface::Message* a_msg)
	{
		if (a_msg->type != ModexAPI::ModexMessage::kMessage_QueryInterface) {
			return;
		}

		auto* request = static_cast<ModexAPI::ModexMessage*>(a_msg->data);
		request->GetApiFunction = GetApiFunction;
		Info("Provided Modex API interface to {}", a_msg->sender);
	}
}
