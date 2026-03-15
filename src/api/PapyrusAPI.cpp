#include "PapyrusAPI.h"

#include "RE/P/PlayerCharacter.h"
#include "SKSE/API.h"
#include "SKSE/Events.h"
#include "core/Commands.h"
#include "data/Data.h"
#include "ui/core/UIManager.h"
#include "ui/modules/formselector/FormSelectorOptions.h"

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

		auto editorID = po3_GetEditorID(a_form->GetFormID());
		if (editorID.empty()) {
			Warn("Modex.RemoveItemFromPlayer: could not resolve editor ID for form {:08X}", a_form->GetFormID());
			return;
		}

		Commands::RemoveItemFromPlayerInventory(Ownership::None, editorID, static_cast<uint32_t>(a_count));
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
	static int32_t GetCachedFormCount(RE::StaticFunctionTag*, int32_t a_cacheType)
	{
		auto* data = Data::GetSingleton();

		switch (a_cacheType) {
		case 0: return static_cast<int32_t>(data->GetAddItemList().size());
		case 1: return static_cast<int32_t>(data->GetNPCList().size());
		case 2: return static_cast<int32_t>(data->GetObjectList().size());
		case 3: return static_cast<int32_t>(data->GetTeleportList().size());
		case 4: return static_cast<int32_t>(data->GetOutfitList().size());
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
		       search(data->GetOutfitList());
	}

	// Modex Data layer finished initialization and ready for queries. I.e. Form Selection / Cache
	static bool IsDataReady(RE::StaticFunctionTag*)
	{
		auto* data = Data::GetSingleton();
		return data && !data->GetAddItemList().empty();
	}

	/// Form Selector UI

	// Pending options accumulator for the builder pattern.
	// Populated by SetFormSelector* calls, consumed and reset by OpenFormSelector.
	static FormSelectorOptions s_pendingOptions;

	// Selection buffer. Populated by the callback, consumed once by GetSelectedForms().
	// Both writes (via AddTask) and reads (via Papyrus VM) run on the game thread.
	static std::vector<RE::FormID> s_selectedForms;

	// Static callback for the form selector. Fires an SKSE ModEvent so
	// Papyrus scripts can react to the selection via RegisterForModEvent.
	// Runs on the render thread, so we defer to the game thread via AddTask.
	static void FormSelectorModEventCallback(const RE::FormID* a_formIDs, uint32_t a_count)
	{
		// Copy form IDs for deferred dispatch (pointer may not survive past this frame).
		std::vector<RE::FormID> ids;
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
		s_pendingOptions.singleSelect = a_enable;
	}

	static void SetFormSelectorShowTotalCost(RE::StaticFunctionTag*, bool a_enable)
	{
		s_pendingOptions.showTotalCost = a_enable;
	}

	static void SetFormSelectorRequireTotalCost(RE::StaticFunctionTag*, bool a_enable)
	{
		s_pendingOptions.requireTotalCost = a_enable;
	}

	static void SetFormSelectorMaxCost(RE::StaticFunctionTag*, int32_t a_maxCost)
	{
		s_pendingOptions.maxCost = a_maxCost;
	}

	static void SetFormSelectorMaxCount(RE::StaticFunctionTag*, int32_t a_maxCount)
	{
		s_pendingOptions.maxCount = a_maxCount;
	}

	static void SetFormSelectorCostMultiplier(RE::StaticFunctionTag*, float a_multiplier)
	{
		s_pendingOptions.costMultiplier = a_multiplier;
	}

	static void SetFormSelectorTitle(RE::StaticFunctionTag*, RE::BSFixedString a_title)
	{
		s_pendingOptions.title = a_title.c_str();
	}

	static void ResetFormSelectorOptions(RE::StaticFunctionTag*)
	{
		s_pendingOptions.Reset();
	}

	static void OpenFormSelector(RE::StaticFunctionTag*, int32_t a_cacheType)
	{
		auto* ui = UIManager::GetSingleton();

		Ownership ownership;
		switch (a_cacheType) {
		case 0: ownership = Ownership::Item; break;
		case 1: ownership = Ownership::Actor; break;
		case 2: ownership = Ownership::Object; break;
		case 3: ownership = Ownership::Cell; break;
		case 4: ownership = Ownership::Outfit; break;
		default:
			Warn("Modex.OpenFormSelector: invalid cache type {}, defaulting to Item", a_cacheType);
			ownership = Ownership::Item;
			break;
		}

		// Consume pending options and reset for next use.
		FormSelectorOptions options = s_pendingOptions;
		s_pendingOptions.Reset();

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

		// Form Selector
		a_vm->RegisterFunction("OpenFormSelector"sv, SCRIPT_NAME, OpenFormSelector);
		a_vm->RegisterFunction("GetSelectedForms"sv, SCRIPT_NAME, GetSelectedForms);
		a_vm->RegisterFunction("GetSelectedFormCount"sv, SCRIPT_NAME, GetSelectedFormCount);
		a_vm->RegisterFunction("GetSelectedFormCost"sv, SCRIPT_NAME, GetSelectedFormCost);

		Info("Registered {} Papyrus native functions for script '{}'.", 27, SCRIPT_NAME);
		return true;
	}
}
