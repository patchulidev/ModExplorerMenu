#include "PapyrusAPI.h"

#include "RE/P/PlayerCharacter.h"
#include "SKSE/API.h"
#include "SKSE/Events.h"
#include "core/Commands.h"
#include "config/EquipmentConfig.h"
#include "data/Data.h"
#include "ui/core/UIManager.h"
#include "ui/modules/formselector/FormSelectorOptions.h"
#include "ui/modules/kitselector/KitSelectorModule.h"

namespace Modex::PapyrusAPI
{
	/// Menu Control

	static void OpenMenu(RE::StaticFunctionTag*)
	{
		auto* ui = UIManager::GetSingleton();
		if (!ui->IsMenuOpen()) {
			ui->QueueOpen();
		}
	}

	static void CloseMenu(RE::StaticFunctionTag*)
	{
		auto* ui = UIManager::GetSingleton();
		if (ui->IsMenuOpen()) {
			ui->Close();
		}
	}

	static bool IsMenuOpen(RE::StaticFunctionTag*)
	{
		return UIManager::GetSingleton()->IsMenuOpen();
	}

	/// Inventory Helpers

	static void AddItemToPlayer(RE::StaticFunctionTag*, RE::TESForm* a_form, int32_t a_count)
	{
		if (!a_form) {
			Warn("Modex.AddItemToPlayer: form is None");
			return;
		}

		if (a_count < 1) {
			a_count = 1;
		}

		auto* player = RE::PlayerCharacter::GetSingleton();
		if (!player) {
			return;
		}

		Commands::AddItemToInventory(Ownership::None, player->AsReference(), a_form->GetFormID(), static_cast<uint32_t>(a_count));
	}

	static void RemoveItemFromPlayer(RE::StaticFunctionTag*, RE::TESForm* a_form, int32_t a_count)
	{
		if (!a_form) {
			Warn("Modex.RemoveItemFromPlayer: form is None");
			return;
		}

		if (a_count < 1) {
			a_count = 1;
		}

		Commands::RemoveItemFromPlayerInventory(Ownership::None, a_form->GetFormID(), static_cast<uint32_t>(a_count));
	}

	static void AddSpellToPlayer(RE::StaticFunctionTag*, RE::SpellItem* a_spell)
	{
		if (!a_spell) {
			Warn("Modex.AddSpellToPlayer: spell is None");
			return;
		}

		auto* player = RE::PlayerCharacter::GetSingleton();
		if (!player) return;

		Commands::AddSpellToActor(Ownership::None, player->AsReference(), a_spell->GetFormID());
	}

	static void RemoveSpellFromPlayer(RE::StaticFunctionTag*, RE::SpellItem* a_spell)
	{
		if (!a_spell) {
			Warn("Modex.RemoveSpellFromPlayer: spell is None");
			return;
		}

		auto* player = RE::PlayerCharacter::GetSingleton();
		if (!player) return;

		Commands::RemoveSpellFromActor(Ownership::None, player->AsReference(), a_spell->GetFormID());
	}

	/// NPC / Reference Actions

	static void TeleportPlayerTo(RE::StaticFunctionTag*, RE::TESObjectREFR* a_ref)
	{
		if (!a_ref) {
			Warn("Modex.TeleportPlayerTo: reference is None");
			return;
		}

		Commands::TeleportPlayerToREFR(Ownership::None, a_ref);
	}

	static void TeleportToPlayer(RE::StaticFunctionTag*, RE::TESObjectREFR* a_ref)
	{
		if (!a_ref) {
			Warn("Modex.TeleportToPlayer: reference is None");
			return;
		}

		Commands::TeleportREFRToPlayer(Ownership::None, a_ref);
	}

	static void KillActor(RE::StaticFunctionTag*, RE::Actor* a_actor)
	{
		if (!a_actor) {
			Warn("Modex.KillActor: actor is None");
			return;
		}

		Commands::KillRefr(Ownership::None, a_actor->AsReference());
	}

	static void ResurrectActor(RE::StaticFunctionTag*, RE::Actor* a_actor)
	{
		if (!a_actor) {
			Warn("Modex.ResurrectActor: actor is None");
			return;
		}

		Commands::ResurrectRefr(Ownership::None, a_actor->AsReference());
	}

	static void EnableReference(RE::StaticFunctionTag*, RE::TESObjectREFR* a_ref)
	{
		if (!a_ref) {
			Warn("Modex.EnableReference: reference is None");
			return;
		}

		Commands::EnableRefr(Ownership::None, a_ref);
	}

	static void DisableReference(RE::StaticFunctionTag*, RE::TESObjectREFR* a_ref)
	{
		if (!a_ref) {
			Warn("Modex.DisableReference: reference is None");
			return;
		}

		Commands::DisableRefr(Ownership::None, a_ref);
	}

	// Placement Helpers -- Probably unecessary due to native PlaceAtMe?

	static RE::TESObjectREFR* PlaceAtPlayer(RE::StaticFunctionTag*, RE::TESForm* a_form, int32_t a_count)
	{
		if (!a_form) {
			Warn("Modex.PlaceAtPlayer: form is None");
			return nullptr;
		}

		if (a_count < 1) {
			a_count = 1;
		}

		auto* player = RE::PlayerCharacter::GetSingleton();
		if (!player) {
			return nullptr;
		}

		auto* playerRef = player->AsReference();
		if (!playerRef) {
			return nullptr;
		}

		auto* newRef = Commands::Papyrus_PlaceAtMe(playerRef, a_form, static_cast<uint32_t>(a_count), true, false);
		if (newRef) {
			UserData::SendEvent(ModexActionType::PlaceAtMe, newRef->GetFormID(), Ownership::None);
		}

		return newRef;
	}

	// Data / Cache Queries
	// Cache type indices mirror ModexAPI::CacheType (kItem=0, kNPC=1, kObject=2, kCell=3,
	// kOutfit=4, kKit=5, kSpell=6).
	static int32_t GetCachedFormCount(RE::StaticFunctionTag*, int32_t a_cacheType)
	{
		auto* data = Data::GetSingleton();

		switch (a_cacheType) {
		case 0: return static_cast<int32_t>(data->GetAddItemList().size());
		case 1: return static_cast<int32_t>(data->GetNPCList().size());
		case 2: return static_cast<int32_t>(data->GetObjectList().size());
		case 3: return static_cast<int32_t>(data->GetTeleportList().size());
		case 4: return static_cast<int32_t>(data->GetOutfitList().size());
		case 6: return static_cast<int32_t>(data->GetSpellList().size());
		default:
			Warn("Modex.GetCachedFormCount: invalid cache type {}", a_cacheType);
			return -1;
		}
	}

	static bool IsFormCached(RE::StaticFunctionTag*, RE::TESForm* a_form)
	{
		if (!a_form) {
			Warn("Modex.IsFormCached: form is None");
			return false;
		}

		auto formID = a_form->GetFormID();
		auto* data = Data::GetSingleton();

		// Search all
		auto search = [formID](const std::vector<BaseObject>& a_list) -> bool {
			for (const auto& obj : a_list) {
				if (obj.GetBaseFormID() == formID) {
					return true;
				}
			}
			return false;
		};

		return search(data->GetAddItemList()) ||
		       search(data->GetNPCList()) ||
		       search(data->GetObjectList()) ||
		       search(data->GetTeleportList()) ||
		       search(data->GetOutfitList()) ||
		       search(data->GetSpellList());
	}

	// Modex Data layer finished initialization and ready for queries. I.e. Form Selection / Cache
	static bool IsDataReady(RE::StaticFunctionTag*)
	{
		auto* data = Data::GetSingleton();
		return data && !data->GetAddItemList().empty();
	}

	/// Form Selector UI

	// Builder-pattern option buffers — one per selector so settings don't leak between them.
	// Populated by Set*Selector* calls, consumed and reset by the corresponding Open* call.
	static FormSelectorOptions s_pendingFormOptions;
	static FormSelectorOptions s_pendingKitOptions;

	// Selection buffer. Populated by the callback, consumed once by GetSelectedForms().
	// Both writes (via AddTask) and reads (via Papyrus VM) run on the game thread.
	static std::vector<uint32_t> s_selectedForms;

	// Static callback for the form selector. Fires an SKSE ModEvent so
	// Papyrus scripts can react to the selection via RegisterForModEvent.
	// Runs on the render thread, so we defer to the game thread via AddTask.
	static void FormSelectorModEventCallback(const uint32_t* a_formIDs, uint32_t a_count)
	{
		// Copy form IDs for deferred dispatch (pointer may not survive past this frame).
		std::vector<uint32_t> ids;
		if (a_formIDs && a_count > 0) {
			ids.assign(a_formIDs, a_formIDs + a_count);
		}

		SKSE::GetTaskInterface()->AddTask([ids = std::move(ids)]() {
			// Populate the selection buffer before firing the event,
			// so GetSelectedForms() is valid inside the event handler.
			s_selectedForms = ids;

			auto* eventSource = SKSE::GetModCallbackEventSource();
			if (!eventSource) {
				Warn("Modex.OpenFormSelector: ModCallbackEventSource unavailable");
				return;
			}

			RE::TESForm* form = !ids.empty() ? RE::TESForm::LookupByID(ids[0]) : nullptr;

			SKSE::ModCallbackEvent event{
				"Modex_OnFormSelected",                     // eventName
				"",                                         // strArg
				static_cast<float>(ids.size()),             // numArg (0 = cancelled, >0 = selected)
				form                                        // sender (first selected form, or nullptr)
			};

			eventSource->SendEvent(&event);
		});
	}

	// Form Selector Builder Setters

	static void SetFormSelectorSingleSelect(RE::StaticFunctionTag*, bool a_enable)
	{
		s_pendingFormOptions.singleSelect = a_enable;
	}

	static void SetFormSelectorShowTotalCost(RE::StaticFunctionTag*, bool a_enable)
	{
		s_pendingFormOptions.showTotalCost = a_enable;
	}

	static void SetFormSelectorRequireTotalCost(RE::StaticFunctionTag*, bool a_enable)
	{
		s_pendingFormOptions.requireTotalCost = a_enable;
	}

	static void SetFormSelectorMaxCost(RE::StaticFunctionTag*, int32_t a_maxCost)
	{
		s_pendingFormOptions.maxCost = a_maxCost;
	}

	static void SetFormSelectorMaxCount(RE::StaticFunctionTag*, int32_t a_maxCount)
	{
		s_pendingFormOptions.maxCount = a_maxCount;
	}

	static void SetFormSelectorCostMultiplier(RE::StaticFunctionTag*, float a_multiplier)
	{
		s_pendingFormOptions.costMultiplier = a_multiplier;
	}

	static void SetFormSelectorTitle(RE::StaticFunctionTag*, RE::BSFixedString a_title)
	{
		s_pendingFormOptions.title = a_title.c_str();
	}

	static void ResetFormSelectorOptions(RE::StaticFunctionTag*)
	{
		s_pendingFormOptions.Reset();
	}

	// Kit Selector Builder Setters

	static void SetKitSelectorSingleSelect(RE::StaticFunctionTag*, bool a_enable)
	{
		s_pendingKitOptions.singleSelect = a_enable;
	}

	static void SetKitSelectorShowTotalCost(RE::StaticFunctionTag*, bool a_enable)
	{
		s_pendingKitOptions.showTotalCost = a_enable;
	}

	static void SetKitSelectorRequireTotalCost(RE::StaticFunctionTag*, bool a_enable)
	{
		s_pendingKitOptions.requireTotalCost = a_enable;
	}

	static void SetKitSelectorMaxCost(RE::StaticFunctionTag*, int32_t a_maxCost)
	{
		s_pendingKitOptions.maxCost = a_maxCost;
	}

	static void SetKitSelectorMaxCount(RE::StaticFunctionTag*, int32_t a_maxCount)
	{
		s_pendingKitOptions.maxCount = a_maxCount;
	}

	static void SetKitSelectorCostMultiplier(RE::StaticFunctionTag*, float a_multiplier)
	{
		s_pendingKitOptions.costMultiplier = a_multiplier;
	}

	static void SetKitSelectorTitle(RE::StaticFunctionTag*, RE::BSFixedString a_title)
	{
		s_pendingKitOptions.title = a_title.c_str();
	}

	static void ResetKitSelectorOptions(RE::StaticFunctionTag*)
	{
		s_pendingKitOptions.Reset();
	}

	static void OpenFormSelector(RE::StaticFunctionTag*, int32_t a_cacheType)
	{
		auto* ui = UIManager::GetSingleton();

		Ownership ownership;
		switch (a_cacheType) {
		case 0: ownership = Ownership::Item;   break;
		case 1: ownership = Ownership::Actor;  break;
		case 2: ownership = Ownership::Object; break;
		case 3: ownership = Ownership::Cell;   break;
		case 4: ownership = Ownership::Outfit; break;
		case 6: ownership = Ownership::Spell;  break;
		default:
			Warn("Modex.OpenFormSelector: invalid cache type {}, defaulting to Item", a_cacheType);
			ownership = Ownership::Item;
			break;
		}

		// Consume pending options and reset for next use.
		FormSelectorOptions options = s_pendingFormOptions;
		s_pendingFormOptions.Reset();

		ui->OpenFormSelector(ownership, FormSelectorModEventCallback, options);
	}

	// Returns the forms from the last selection and clears the buffer.
	// Single-consumer: the first script to call this gets the data,
	// subsequent calls return an empty array until the next selection.
	static std::vector<RE::TESForm*> GetSelectedForms(RE::StaticFunctionTag*)
	{
		std::vector<RE::TESForm*> result;

		for (auto formID : s_selectedForms) {
			if (auto* form = RE::TESForm::LookupByID(formID)) {
				if (auto outfit = form->As<RE::BGSOutfit>(); outfit) {
					auto items = Commands::GetOutfitItems(outfit);

					for (auto& baseObject : items) {
						result.push_back(baseObject.GetTESForm());
					}
				} else {
					result.push_back(form);
				}
			}
		}

		s_selectedForms.clear();
		return result;
	}

	// Returns the number of forms in the selection buffer (0 if already consumed).
	static int32_t GetSelectedFormCount(RE::StaticFunctionTag*)
	{
		return static_cast<int32_t>(s_selectedForms.size());
	}

	// Returns the base gold cost of forms in the selection buffer (0 if already consumed).
	static int32_t GetSelectedFormCost(RE::StaticFunctionTag*)
	{
		if (s_selectedForms.size() <= 0) return 0;

		int32_t total = 0;
		for (auto formID : s_selectedForms) {
			if (auto* form = RE::TESForm::LookupByID(formID)) {
				if (form->GetFormType() == RE::FormType::Outfit) {
					total += Commands::GetOutfitValue(form->As<RE::BGSOutfit>());
				} else {
					total += form->GetGoldValue();
				}
			}
		}

		return total;
	}

	/// Kit Selector UI

	// Selection buffer for kit keys. Populated by the callback, consumed once by GetSelectedKits().
	static std::vector<std::string> s_selectedKits;

	// Static callback for the kit selector. Fires an SKSE ModEvent so
	// Papyrus scripts can react to the selection via RegisterForModEvent.
	// Runs on the render thread, so we defer to the game thread via AddTask.
	static void KitSelectorModEventCallback(const char* const* a_kitKeys, uint32_t a_count)
	{
		std::vector<std::string> keys;
		if (a_kitKeys && a_count > 0) {
			for (uint32_t i = 0; i < a_count; i++) {
				keys.emplace_back(a_kitKeys[i]);
			}
		}

		SKSE::GetTaskInterface()->AddTask([keys = std::move(keys)]() {
			s_selectedKits = keys;

			auto* eventSource = SKSE::GetModCallbackEventSource();
			if (!eventSource) {
				Warn("Modex.OpenKitSelector: ModCallbackEventSource unavailable");
				return;
			}

			SKSE::ModCallbackEvent event{
				"Modex_OnKitSelected",
				!keys.empty() ? RE::BSFixedString(keys[0]) : "",
				static_cast<float>(keys.size()),
				nullptr
			};

			eventSource->SendEvent(&event);
		});
	}

	static void OpenKitSelector(RE::StaticFunctionTag*)
	{
		auto* ui = UIManager::GetSingleton();

		FormSelectorOptions options = s_pendingKitOptions;
		s_pendingKitOptions.Reset();

		ui->OpenKitSelector(KitSelectorModEventCallback, options);
	}

	// Returns the kit keys from the last selection and clears the buffer.
	static std::vector<RE::BSFixedString> GetSelectedKits(RE::StaticFunctionTag*)
	{
		std::vector<RE::BSFixedString> result;
		result.reserve(s_selectedKits.size());

		for (const auto& key : s_selectedKits) {
			result.emplace_back(key);
		}

		s_selectedKits.clear();
		return result;
	}

	static int32_t GetSelectedKitCount(RE::StaticFunctionTag*)
	{
		return static_cast<int32_t>(s_selectedKits.size());
	}

	static int32_t GetSelectedKitCost(RE::StaticFunctionTag*)
	{
		int32_t total = 0;
		for (const auto& key : s_selectedKits) {
			total += KitSelectorModule::GetKitGoldValue(key);
		}
		return total;
	}

	/// Returns true if a kit exists in the cache for the given key.
	static bool IsKitCached(RE::StaticFunctionTag*, RE::BSFixedString a_key)
	{
		return EquipmentConfig::KitLookup(a_key.c_str()) != nullptr;
	}

	static int32_t GetKitItemCount(RE::StaticFunctionTag*, RE::BSFixedString a_key)
	{
		auto* kit = EquipmentConfig::KitLookup(a_key.c_str());
		return kit ? static_cast<int32_t>(kit->m_items.size()) : 0;
	}

	static int32_t GetKitSpellCount(RE::StaticFunctionTag*, RE::BSFixedString a_key)
	{
		auto* kit = EquipmentConfig::KitLookup(a_key.c_str());
		return kit ? static_cast<int32_t>(kit->m_spells.size()) : 0;
	}

	static int32_t GetKitGoldValue(RE::StaticFunctionTag*, RE::BSFixedString a_key)
	{
		return KitSelectorModule::GetKitGoldValue(a_key.c_str());
	}

	/// Apply a kit (items + spells) to an arbitrary actor reference. No UI involved.
	static void ApplyKitToActor(RE::StaticFunctionTag*, RE::BSFixedString a_key, RE::TESObjectREFR* a_target)
	{
		if (!a_target) {
			Warn("Modex.ApplyKitToActor: target is None");
			return;
		}

		auto* kit = EquipmentConfig::KitLookup(a_key.c_str());
		if (!kit) {
			Warn("Modex.ApplyKitToActor: kit '{}' not found", a_key.c_str());
			return;
		}

		for (const auto& item : kit->m_items) {
			if (item->m_equipped) {
				Commands::AddAndEquipItemToInventory(Ownership::Kit, a_target, item->m_formID);
			} else {
				Commands::AddItemToRefInventory(Ownership::Kit, a_target, item->m_formID, static_cast<uint32_t>(item->m_amount));
			}
		}

		for (const auto& spell : kit->m_spells) {
			Commands::AddSpellToActor(Ownership::Kit, a_target, spell->m_formID);
		}
	}

	static void ApplySelectedKitsToPlayer(RE::StaticFunctionTag*)
	{
		auto* player = RE::PlayerCharacter::GetSingleton();
		if (!player) return;

		auto* playerRef = player->AsReference();
		if (!playerRef) return;

		for (const auto& key : s_selectedKits) {
			auto* kit = EquipmentConfig::KitLookup(key);
			if (!kit) continue;

			for (const auto& item : kit->m_items) {
				if (item->m_equipped) {
					Commands::AddAndEquipItemToInventory(Ownership::Kit, playerRef, item->m_formID);
				} else {
					Commands::AddItemToRefInventory(Ownership::Kit, playerRef, item->m_formID, static_cast<uint32_t>(item->m_amount));
				}
			}

			for (const auto& spell : kit->m_spells) {
				Commands::AddSpellToActor(Ownership::Kit, playerRef, spell->m_formID);
			}
		}

		s_selectedKits.clear();
	}

	/// Papyrus Native Function Registration

	bool Register(RE::BSScript::IVirtualMachine* a_vm)
	{
		// Menu Control
		a_vm->RegisterFunction("OpenMenu"sv, SCRIPT_NAME, OpenMenu);
		a_vm->RegisterFunction("CloseMenu"sv, SCRIPT_NAME, CloseMenu);
		a_vm->RegisterFunction("IsMenuOpen"sv, SCRIPT_NAME, IsMenuOpen);

		// Inventory
		a_vm->RegisterFunction("AddItemToPlayer"sv, SCRIPT_NAME, AddItemToPlayer);
		a_vm->RegisterFunction("RemoveItemFromPlayer"sv, SCRIPT_NAME, RemoveItemFromPlayer);
		a_vm->RegisterFunction("AddSpellToPlayer"sv, SCRIPT_NAME, AddSpellToPlayer);
		a_vm->RegisterFunction("RemoveSpellFromPlayer"sv, SCRIPT_NAME, RemoveSpellFromPlayer);

		// NPC / Reference
		a_vm->RegisterFunction("TeleportPlayerTo"sv, SCRIPT_NAME, TeleportPlayerTo);
		a_vm->RegisterFunction("TeleportToPlayer"sv, SCRIPT_NAME, TeleportToPlayer);
		a_vm->RegisterFunction("KillActor"sv, SCRIPT_NAME, KillActor);
		a_vm->RegisterFunction("ResurrectActor"sv, SCRIPT_NAME, ResurrectActor);
		a_vm->RegisterFunction("EnableReference"sv, SCRIPT_NAME, EnableReference);
		a_vm->RegisterFunction("DisableReference"sv, SCRIPT_NAME, DisableReference);

		// Spawning
		a_vm->RegisterFunction("PlaceAtPlayer"sv, SCRIPT_NAME, PlaceAtPlayer);

		// Data Queries
		a_vm->RegisterFunction("GetCachedFormCount"sv, SCRIPT_NAME, GetCachedFormCount);
		a_vm->RegisterFunction("IsFormCached"sv, SCRIPT_NAME, IsFormCached);
		a_vm->RegisterFunction("IsDataReady"sv, SCRIPT_NAME, IsDataReady);

		// Form Selector Options (Builder Pattern)
		a_vm->RegisterFunction("SetFormSelectorSingleSelect"sv, SCRIPT_NAME, SetFormSelectorSingleSelect);
		a_vm->RegisterFunction("SetFormSelectorShowTotalCost"sv, SCRIPT_NAME, SetFormSelectorShowTotalCost);
		a_vm->RegisterFunction("SetFormSelectorRequireTotalCost"sv, SCRIPT_NAME, SetFormSelectorRequireTotalCost);
		a_vm->RegisterFunction("SetFormSelectorMaxCost"sv, SCRIPT_NAME, SetFormSelectorMaxCost);
		a_vm->RegisterFunction("SetFormSelectorMaxCount"sv, SCRIPT_NAME, SetFormSelectorMaxCount);
		a_vm->RegisterFunction("SetFormSelectorCostMultiplier"sv, SCRIPT_NAME, SetFormSelectorCostMultiplier);
		a_vm->RegisterFunction("SetFormSelectorTitle"sv, SCRIPT_NAME, SetFormSelectorTitle);
		a_vm->RegisterFunction("ResetFormSelectorOptions"sv, SCRIPT_NAME, ResetFormSelectorOptions);

		// Kit Selector Options (Builder Pattern)
		a_vm->RegisterFunction("SetKitSelectorSingleSelect"sv, SCRIPT_NAME, SetKitSelectorSingleSelect);
		a_vm->RegisterFunction("SetKitSelectorShowTotalCost"sv, SCRIPT_NAME, SetKitSelectorShowTotalCost);
		a_vm->RegisterFunction("SetKitSelectorRequireTotalCost"sv, SCRIPT_NAME, SetKitSelectorRequireTotalCost);
		a_vm->RegisterFunction("SetKitSelectorMaxCost"sv, SCRIPT_NAME, SetKitSelectorMaxCost);
		a_vm->RegisterFunction("SetKitSelectorMaxCount"sv, SCRIPT_NAME, SetKitSelectorMaxCount);
		a_vm->RegisterFunction("SetKitSelectorCostMultiplier"sv, SCRIPT_NAME, SetKitSelectorCostMultiplier);
		a_vm->RegisterFunction("SetKitSelectorTitle"sv, SCRIPT_NAME, SetKitSelectorTitle);
		a_vm->RegisterFunction("ResetKitSelectorOptions"sv, SCRIPT_NAME, ResetKitSelectorOptions);

		// Form Selector
		a_vm->RegisterFunction("OpenFormSelector"sv, SCRIPT_NAME, OpenFormSelector);
		a_vm->RegisterFunction("GetSelectedForms"sv, SCRIPT_NAME, GetSelectedForms);
		a_vm->RegisterFunction("GetSelectedFormCount"sv, SCRIPT_NAME, GetSelectedFormCount);
		a_vm->RegisterFunction("GetSelectedFormCost"sv, SCRIPT_NAME, GetSelectedFormCost);

		// Kit Selector
		a_vm->RegisterFunction("OpenKitSelector"sv, SCRIPT_NAME, OpenKitSelector);
		a_vm->RegisterFunction("GetSelectedKits"sv, SCRIPT_NAME, GetSelectedKits);
		a_vm->RegisterFunction("GetSelectedKitCount"sv, SCRIPT_NAME, GetSelectedKitCount);
		a_vm->RegisterFunction("GetSelectedKitCost"sv, SCRIPT_NAME, GetSelectedKitCost);
		a_vm->RegisterFunction("ApplySelectedKitsToPlayer"sv, SCRIPT_NAME, ApplySelectedKitsToPlayer);

		// Kit Queries / Apply
		a_vm->RegisterFunction("IsKitCached"sv, SCRIPT_NAME, IsKitCached);
		a_vm->RegisterFunction("GetKitItemCount"sv, SCRIPT_NAME, GetKitItemCount);
		a_vm->RegisterFunction("GetKitSpellCount"sv, SCRIPT_NAME, GetKitSpellCount);
		a_vm->RegisterFunction("GetKitGoldValue"sv, SCRIPT_NAME, GetKitGoldValue);
		a_vm->RegisterFunction("ApplyKitToActor"sv, SCRIPT_NAME, ApplyKitToActor);

		Info("Registered {} Papyrus native functions for script '{}'.", 47, SCRIPT_NAME);
		return true;
	}
}
