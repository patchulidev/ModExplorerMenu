#include "UITable.h" 

#include "core/Commands.h"
#include "data/BaseObject.h"
#include "external/icons/IconsLucide.h"
#include "imgui.h"
#include "imgui_internal.h"
#include "localization/Locale.h"
#include "ui/core/UIManager.h"
#include "localization/FontManager.h"
#include "ui/components/UICustom.h"
#include "ui/components/ItemPreview.h"
#include "ui/components/UINotification.h"
#include "config/EquipmentConfig.h"
#include "config/BlacklistConfig.h"
#include "config/UserConfig.h"
#include "config/UserData.h"
#include "config/ThemeConfig.h"

#include "ui/components/UIModule.h"
#include "ui/style/LayoutMetrics.h"

namespace Modex
{
	bool UITable::IsValidTargetReference(RE::TESObjectREFR* a_reference) {
		if (a_reference == nullptr) {
			a_reference = tableTargetRef;
		}

		if (a_reference && !a_reference->IsInitialized()) {
			return false;
		}

		const auto baseObject = a_reference != nullptr ? a_reference : nullptr; // huh
		return baseObject && (baseObject->IsActor() || baseObject->GetFormType() == RE::FormType::Container);
	}

	bool UITable::IsActionAllowed()
	{
		// Must have a valid selection.
		if (GetSelectionCount() <= 0) {
			return false;
		} else {
			for (const auto& item : GetSelection()) {
				if (item == nullptr || item->IsDummy()) {
					return false;
				}
			}
		}

		// Selection must be valid.
		if (!IsValidTargetReference(tableTargetRef)) {
			return false;
		}

		// Best guess at being in-game and not in main-menu
		if (const auto ui = RE::UI::GetSingleton(); ui == nullptr || ui->IsMenuOpen(RE::MainMenu::MENU_NAME)) {
			return false;
		}

		return true;
	}

	bool UITable::IsValidSelectionReference() const
	{
		if (GetSelectionCount() == 1) {
			return GetSelection().front() && !GetSelection().front()->IsDummy() && GetSelection().front()->GetRefID() != 0;
		} else {
			return itemPreview != nullptr && !itemPreview->IsDummy() && itemPreview->GetRefID() != 0;
		}
	}

	RE::TESObjectREFR* UITable::GetSelectedReference() const
	{
		if (GetSelectionCount() == 1) {
			if (GetSelection().front() && !GetSelection().front()->IsDummy()) {
				auto id = GetSelection().front()->GetRefID();
				return RE::TESForm::LookupByID<RE::TESObjectREFR>(id);
			}
		} else {
			if (itemPreview && !itemPreview->IsDummy()) {
				auto id = itemPreview->GetRefID();
				return RE::TESForm::LookupByID<RE::TESObjectREFR>(id);
			}
		}

		return nullptr;
	}

	bool UITable::IsMouseHoveringRect(const ImVec2& a_min, const ImVec2& a_max)
	{
		if (HasFlag(ModexTableFlag_EnableItemPreviewOnHover))
			return false;

		if (ImGui::IsMouseHoveringRect(a_min, a_max)) {
			return ImGui::IsItemHovered(ImGuiHoveredFlags_NoSharedDelay | ImGuiHoveredFlags_DelayNone);
		}

		return false;
	}

	UITable::UITable(const std::string& a_dataID, bool a_shared, Ownership a_owner, uint32_t a_flags)
		: data_id(a_dataID)
		, owner(a_owner)
		, flags(a_flags)
		, tableID()
		, tableMode(SHOWALL)
		, itemPreview(nullptr)
		, tableTargetRef(nullptr)
		, selectedKitPtr(nullptr)
		, updateKeyboardNav(false)
		, showEditorID(false)
		, showFormID(false)
		, useSharedTarget(a_shared)
		, navPositionID(0)
	{
		memset(pluginSearchBuffer, 0, sizeof(pluginSearchBuffer));

		auto timestamp = std::chrono::steady_clock::now();
		auto count = timestamp.time_since_epoch().count();
		tableID = static_cast<uint32_t>(std::hash<decltype(count)>{}(count));
		Trace("Unique TableID {}", tableID);

		InitializeSystems();
		LoadSystemState();
		Setup();
	}

	UITable::~UITable()
	{
		SaveSystemState();
		CleanupResources();
	}

	void UITable::Setup()
	{
		filterSystem->SetSystemCallback([this]() { Refresh(); });

		if (useSharedTarget) {
			auto target = UIModule::GetTargetReference();

			if (target == nullptr) {
				target = UIModule::LookupReferenceByFormID(20);
			}

			SetTargetByReference(target);
		} else {
			const auto formID = UserData::Get<RE::FormID>(data_id + "::LastTargetRef", 0);
			auto target = UIModule::LookupReferenceByFormID(formID);
			SetTargetByReference(target);
		}

		showEditorID = UserData::Get<bool>(data_id + "::ShowEditorID", false);
		showFormID = UserData::Get<bool>(data_id + "::ShowFormID", false);
		selectedPlugin = UserData::Get<std::string>(data_id + "::LastSelectedPlugin", Translate("SHOWALL"));
		tableMode = UserData::Get<uint32_t>(data_id + "::TableMode", SHOWALL);

		if (UserConfig::Get().developerMode) {
			this->flags |= ModexTableFlag_EnableDebugToolkit;
		}

		if (HasFlag(ModexTableFlag_Kit)) {
			sortSystem->SetupColumns({
					{0, PropertyType::kEditorID},
					{1, PropertyType::kArmorType},
					{2, PropertyType::kKitItemCount}
			});
		}

		BuildPluginList();
		Refresh();
	}

	void UITable::InitializeSystems()
	{
		const auto filename = ConfigManager::FILTER_DIRECTORY / (this->data_id + ".json");
		const bool forceCreate = HasFlag(ModexTableFlag_Inventory) || HasFlag(ModexTableFlag_Kit) ? false : true;

		filterSystem = std::make_unique<FilterSystem>(filename);
		filterSystem->Load(forceCreate);

		sortSystem = std::make_unique<SortSystem>(filename);
		sortSystem->Load(forceCreate);

		searchSystem = std::make_unique<SearchSystem>(filename);
		searchSystem->Load(forceCreate);
	}

	void UITable::SaveSystemState()
	{
		filterSystem->SaveState(data_id + "::FilterState");
		sortSystem->SaveState(data_id + "::SortState");
		searchSystem->SaveState(data_id + "::SearchState");

		UserData::Set<bool>(data_id + "::ShowEditorID", showEditorID);
		UserData::Set<bool>(data_id + "::ShowFormID", showFormID);
		UserData::Set<std::string>(data_id + "::LastSelectedPlugin", selectedPlugin);
		UserData::Set<uint32_t>(data_id + "::TableMode", tableMode);

		std::vector<uint32_t> selectedFormIDs;
		for (auto& item : tableList) {
			if (selectionStorage.Contains(item->m_tableID)) {
				selectedFormIDs.push_back(item->GetBaseFormID());
			}
		}
		UserData::Set<std::vector<uint32_t>>(data_id + "::Selection", selectedFormIDs);
	}

	void UITable::LoadSystemState()
	{
		filterSystem->LoadState(data_id + "::FilterState");
		sortSystem->LoadState(data_id + "::SortState");
		searchSystem->LoadState(data_id + "::SearchState");

		auto savedSelection = UserData::Get<std::vector<uint32_t>>(data_id + "::Selection");
		m_pendingSelection.insert(savedSelection.begin(), savedSelection.end());
	}

	void UITable::CleanupResources()
	{
		if (filterSystem) {
			filterSystem->SetSystemCallback(nullptr);
		}

		// Reset systems
		filterSystem.reset();
		sortSystem.reset();
		searchSystem.reset();

		// Clear containers
		tableList.clear();
		recentList.clear();
		pluginList.clear();
		pluginSet.clear();

		selectionStorage.Clear();
		dragDropSourceList.clear();

		// Clear raw pointers
		itemPreview = nullptr;
		tableTargetRef = nullptr;
		selectedKitPtr = nullptr;
	}
	
	// BUG: Called twice during instantiation for Inventories.
	void UITable::SetTargetByReference(RE::TESObjectREFR* a_reference)
	{
		if (a_reference) {
			tableTargetRef = a_reference;
		} else {
			return;
		}

		// Target-dependent property evaluators (kKnownByTarget, kSpellCost) read this global.
		const RE::FormID previousFilterTargetID = g_modexFilterTargetID;
		g_modexFilterTargetID = a_reference->GetFormID();

		// Inventory tables refresh because their data is the target's contents. Spell tables
		// refresh on target change so target-dependent filters/sorts re-evaluate. Other
		// modules deliberately preserve selection across target swaps.
		if (HasFlag(ModexTableFlag_Inventory)) {
			this->Refresh();
		} else if (owner == Ownership::Spell && previousFilterTargetID != g_modexFilterTargetID) {
			this->Refresh();
		}

		if (useSharedTarget)
		{
			UIModule::SetTargetReference(a_reference);

			// HACK: To avoid a higher level callback system.
			for (const auto source : dragDropSourceList) {
				if (source.second->useSharedTarget) {
					if (source.second->GetTableTargetRef() != this->GetTableTargetRef()) {
						source.second->SetTargetByReference(a_reference);
					}
				}
			}

			if (a_reference) {
				UserData::Set<RE::FormID>("LastSharedTargetFormID", a_reference->formID);
			}
		}

		if (!useSharedTarget)
		{
			RE::FormID formID = 0;

			if (tableTargetRef) {
				formID = tableTargetRef->formID;
				UserData::Set<RE::FormID>(data_id + "::LastTargetRef", formID);
			}
		}
	}

	// NOTE: Defer dirtying inventory tables using the game thread so that we always stay in sync.
	// Failure to do so results in race conditions between UI and Game thread and incorrect
	// inventory contents being displayed.

	void UITable::UpdateActiveInventoryTables()
	{
		if (this->HasFlag(ModexTableFlag_Inventory) || this->GetDragDropHandle() == DragDropHandle::Inventory) {
			SKSE::GetTaskInterface()->AddTask([this]() {
				this->AddFlag(ModexTableFlag_Dirty);
			});

			return;
		}

		for (auto& pair : dragDropSourceList) {
			const auto handle = pair.first;
			const auto ptr = pair.second;

			if (ptr->HasFlag(ModexTableFlag_Inventory) || handle == DragDropHandle::Inventory) {
				SKSE::GetTaskInterface()->AddTask([target = std::make_shared<UITable*>(ptr)] {
					auto table = *target;
					table->AddFlag(ModexTableFlag_Dirty);
				});
			}
		}
	}

	void UITable::AddKitToTargetInventory(const Kit& a_kit)
    {
		if (tableList.empty())
			return;

		if (a_kit.m_items.empty() && a_kit.m_spells.empty())
			return;

		if (!tableTargetRef)
			return;

		for (auto& kitItem : a_kit.m_items) {
			if (kitItem->m_equipped) {
				Commands::AddAndEquipItemToInventory(Ownership::Item, tableTargetRef, kitItem->m_formID);
			} else {
				Commands::AddItemToRefInventory(Ownership::Item, tableTargetRef, kitItem->m_formID, static_cast<std::uint32_t>(kitItem->m_amount));
			}
		}

		for (auto& kitSpell : a_kit.m_spells) {
			Commands::AddSpellToActor(Ownership::Spell, tableTargetRef, kitSpell->m_formID);
		}

		UpdateActiveInventoryTables();
    }

	void UITable::RemoveSelectionFromTargetInventory()
	{
		if (tableList.empty())
			return;


		if (!tableTargetRef)
			return;

		if (GetSelectionCount() == 0)
		{
			if (itemPreview && !itemPreview->IsDummy() && itemPreview->GetTESForm()->IsInventoryObject()) {
				Commands::RemoveItemFromInventory(owner, tableTargetRef, itemPreview->GetBaseFormID(), 1);
			}
		} 
		else {
			void* it = NULL;
			ImGuiID id = 0;

			while (selectionStorage.GetNextSelectedItem(&it, &id)) {
				if (id < std::ssize(tableList) && id >= 0) {
					const auto& item = tableList[id];
					if (!item->IsDummy()) {
						Commands::RemoveItemFromInventory(owner, tableTargetRef, item->GetBaseFormID(), 1);
					}
				}
			}
		}

		selectionStorage.Clear();
		UpdateActiveInventoryTables();
	}

	void UITable::AddSelectionToTargetInventory(uint32_t a_count)
	{
		if (tableList.empty()) 
			return;

		if (!tableTargetRef)
			return;

		if (GetSelectionCount() == 0) {
			if (itemPreview && !itemPreview->IsDummy() && itemPreview->GetTESForm()->IsInventoryObject()) {
				Commands::AddItemToRefInventory(owner, tableTargetRef, itemPreview->GetBaseFormID(), a_count);
			}
		} 
		else {
			void* it = NULL;
			ImGuiID id = 0;

			while (selectionStorage.GetNextSelectedItem(&it, &id)) {
				if (id < std::ssize(tableList) && id >= 0) {
					const auto& item = tableList[id];
					if (item && !item->IsDummy() && item->GetTESForm()->IsInventoryObject()) {
						Commands::AddItemToRefInventory(owner, tableTargetRef, item->GetBaseFormID(), a_count);
					}
				}
			}
		}

		UpdateActiveInventoryTables();
	}

	void UITable::EquipSelectionToTarget()
	{
		if (tableList.empty()) 
			return;

		if (!tableTargetRef)
			return;

		if (GetSelectionCount() == 0) {
			if (itemPreview && !itemPreview->IsDummy() && (itemPreview->IsArmor() || itemPreview->IsWeapon())) {
				Commands::AddAndEquipItemToInventory(owner, tableTargetRef, itemPreview->GetBaseFormID());
			}
		}
		else {
			void* it = NULL;
			ImGuiID id = 0;

			while (selectionStorage.GetNextSelectedItem(&it, &id)) {
				if (id < std::ssize(tableList) && id >= 0) {
					const auto& item = tableList[id];
					if (item && !item->IsDummy() && (item->IsArmor() || item->IsWeapon())) {
						Commands::AddAndEquipItemToInventory(owner, tableTargetRef, item->GetBaseFormID());
					}
				}
			}
		}

		selectionStorage.Clear();
		UpdateActiveInventoryTables();
	}


	void UITable::PlaceSelectionOnGround(uint32_t a_count)
	{
		if (tableList.empty()) 
			return;

		if (GetSelectionCount() == 0) {
			if (itemPreview && !itemPreview->IsDummy()) {
				Commands::PlaceAtMe(owner, itemPreview->GetBaseFormID(), a_count);
			}
		}
		else {
			void* it = NULL;
			ImGuiID id = 0;

			while (selectionStorage.GetNextSelectedItem(&it, &id)) {
				if (id < std::ssize(tableList) && id >= 0) {
					const auto& item = tableList[id];
					if (item && !item->IsDummy()) {
						Commands::PlaceAtMe(owner, item->GetBaseFormID(), a_count);
					}
				}
			}
		}
	}

	void UITable::ExecuteCommandOnSelection(const std::function<void(const std::unique_ptr<BaseObject>&)>& a_command)
	{
		if (tableList.empty())
			return;

		if (GetSelectionCount() == 0) {
			if (itemPreview && !itemPreview->IsDummy()) {
				a_command(itemPreview);
			}
		}
		else {
			void* it = NULL;
			ImGuiID id = 0;
			while (selectionStorage.GetNextSelectedItem(&it, &id)) {
				if (id < std::ssize(tableList) && id >= 0) {
					const auto& item = tableList[id];
					if (item && !item->IsDummy()) {
						a_command(item);
					}
				}
			}
		}

		selectionStorage.Clear();
	}

	void UITable::BringSelectionToPlayer()
	{
		if (GetSelectionCount() == 0) {
			if (itemPreview && !itemPreview->IsDummy()) {
				Commands::TeleportREFRToPlayer(owner, GetSelectedReference());
			}
		}
		else {
			void* it = NULL;
			ImGuiID id = 0;
			while (selectionStorage.GetNextSelectedItem(&it, &id)) {
				if (id < std::ssize(tableList) && id >= 0) {
					const auto& item = tableList[id];
					if (item && !item->IsDummy()) {
						if (auto refr = RE::TESForm::LookupByID<RE::TESObjectREFR>(item->GetRefID())) {
							Commands::TeleportREFRToPlayer(owner, refr);
						}
					}
				}
			}
		}

		selectionStorage.Clear();
	}

	void UITable::AddAll()
	{
		if (tableList.empty())
			return;

		if (!tableTargetRef)
			return;
		
		for (auto& item : tableList) {
			if (item && !item->IsDummy() && item->GetTESForm()->IsInventoryObject()) {
				Commands::AddItemToRefInventory(owner, tableTargetRef, item->GetBaseFormID(), 1);
			}
		}

		selectionStorage.Clear();
		UpdateActiveInventoryTables();
	}

	void UITable::PlaceAll()
	{
		if (tableList.empty())
			return;

		if (!tableTargetRef)
			return;

		for (auto& item : tableList) {
			if (item && !item->IsDummy() && item->GetTESForm()->HasWorldModel()) {
				Commands::PlaceAtMe(owner, item->GetBaseFormID(), 1);
			}
		}

		selectionStorage.Clear();
	}

	void UITable::SetDragDropTarget(DragDropHandle a_handle, UITable* a_view)
	{
		dragDropSourceList.clear();
		dragDropSourceList[a_handle] = a_view;
	}

	void UITable::AddDragDropTarget(DragDropHandle a_handle, UITable* a_view)
	{
		dragDropSourceList[a_handle] = a_view;
	}

	void UITable::RemoveDragDropTarget(DragDropHandle a_handle)
	{
		auto it = dragDropSourceList.find(a_handle);
		if (it != dragDropSourceList.end()) {
			dragDropSourceList.erase(it);
		}
	}

	void UITable::SetDragDropHandle(DragDropHandle a_id)
	{
		dragDropHandle = a_id;
	}

	const std::vector<std::unique_ptr<BaseObject>> UITable::GetSelection() const
	{
		std::vector<std::unique_ptr<BaseObject>> selectedItems;

		for (auto& item : tableList) {
			bool is_item_selected = selectionStorage.Contains(item->m_tableID);
			if (is_item_selected) {
				selectedItems.emplace_back(std::make_unique<BaseObject>(*item));
			}
		}

		return selectedItems;
	}

	uint32_t UITable::GetSelectionCount() const
	{
		return selectionStorage.Size;
	}

	// NOTE: Drag Drop specific helpers with additional inventory update callbacks.

	void UITable::AddPayloadToInventory(const std::unique_ptr<BaseObject>& a_item)
	{
		if (!tableTargetRef)
			return;

		if (a_item && !a_item->IsDummy() && a_item->GetTESForm()->IsInventoryObject()) {
			Commands::AddItemToRefInventory(owner, this->tableTargetRef, a_item->GetBaseFormID(), a_item->GetQuantity());
		}

		UpdateActiveInventoryTables();
	}

	void UITable::RemovePayloadFromInventory(const std::unique_ptr<BaseObject>& a_item)
	{
		if (!tableTargetRef)
			return;

		if (a_item && !a_item->IsDummy() && a_item->GetTESForm()->IsInventoryObject()) {
			Commands::RemoveItemFromInventory(owner, this->tableTargetRef, a_item->GetBaseFormID(), a_item->GetQuantity());
		}

		UpdateActiveInventoryTables();
	}

	// NOTE: Kit's are not inventories, they're virtual containers in memory. So we handle them
	// explcitly. We also can handle dummy forms here without worry.

	void UITable::AddPayloadToKit(const std::unique_ptr<BaseObject>& a_item)
	{
		// Spells are binary owned (HasSpell/AddSpell) — no quantity to merge.
		auto* form = a_item->GetTESForm();
		const bool is_spell = form && form->As<RE::SpellItem>();

		bool has_item = false;
		for (auto& item : tableList) {
			if (item->GetEditorID() == a_item->GetEditorID()) {
				if (!is_spell) item->m_quantity++;
				has_item = true;
				break;
			}
		}

		if (!has_item) {
			tableList.emplace_back(std::make_unique<BaseObject>(*a_item));
		}

		SyncChangesToKit();
		UpdateImGuiTableIDs();
	}

	void UITable::AddSelectionToActiveKit()
	{
		if (auto map = dragDropSourceList.find(DragDropHandle::Kit); map != this->dragDropSourceList.end()) {
			const auto& destination = map->second;

			if (GetSelectionCount() == 0)
			{
				// NOTE: This was a valid check, but should be used when iterating over a kit for
				// actions. Not when we're adding/removing. That way we support dummy objects. The
				// context of this was to prevent npc's from being added to kits lol.
				//
				// if (itemPreview && itemPreview->GetTESForm()->IsInventoryObject()) {

				if (itemPreview) {
					destination->AddPayloadToKit(itemPreview);
				}
			}
			else {
				void* it = NULL;
				ImGuiID id = 0;

				while (selectionStorage.GetNextSelectedItem(&it, &id)) {
					if (id < tableList.size() && id >= 0) {
						const auto& item = this->tableList[id];

						if (item) {
							destination->AddPayloadToKit(item);
						}
					}
				}
			}
		}
	}

	void UITable::RemoveSelectionFromKit()
	{
		if (this->tableList.empty()) {
			return;
		}

		if (GetSelectionCount() == 0)
		{
			if (itemPreview) {
				this->RemovePayloadItemFromKit(itemPreview);
			}
		}
		else {
			void* it = NULL;
			ImGuiID id = 0;

			std::vector<std::unique_ptr<BaseObject>> items_to_remove;

			// Alternative to remove_if to prevent iterator invalidation.
			while (selectionStorage.GetNextSelectedItem(&it, &id)) {
				if (id < tableList.size() && id >= 0) {
					items_to_remove.emplace_back(std::make_unique<BaseObject>(*this->tableList[id]));
				}
			}

			for (const auto& item : items_to_remove) {
				this->RemovePayloadItemFromKit(item);
			}
		}

		selectionStorage.Clear();
	}

	void UITable::RemovePayloadItemFromKit(const std::unique_ptr<BaseObject>& a_item)
	{
		if (this->tableList.empty()) {
			return;
		}

		m_markedForDelete.emplace_back(a_item->GetEditorID());

		if (selectedKitPtr) {
			selectedKitPtr->m_dirty = true;
		}
	}

	void UITable::AddSelectionToFavorites()
	{
		if (GetSelectionCount() == 0) {
			if (itemPreview && !itemPreview->GetEditorID().empty()) {
				UserData::SendEvent(ModexActionType::Favorited, itemPreview);
			}
		}
		else {
			void* it = NULL;
			ImGuiID id = 0;
			while (selectionStorage.GetNextSelectedItem(&it, &id)) {
				if (id < std::ssize(tableList) && id >= 0) {
					const auto& item = tableList[id];
					if (item) {
						UserData::SendEvent(ModexActionType::Favorited, item);
					}
				}
			}
		}

		if (tableMode == SHOWFAVORITE) {
			Refresh();
		}
	}

	void UITable::RemoveSelectionFromFavorites()
	{
		if (GetSelectionCount() == 0) {
			if (itemPreview && !itemPreview->GetEditorID().empty()) {
				UserData::SendEvent(ModexActionType::Unfavorited, itemPreview);
			}
		}
		else {
			void* it = NULL;
			ImGuiID id = 0;
			while (selectionStorage.GetNextSelectedItem(&it, &id)) {
				if (id < std::ssize(tableList) && id >= 0) {
					const auto& item = tableList[id];
					if (item) {
						UserData::SendEvent(ModexActionType::Unfavorited, item);
					}
				}
			}
		}

		if (tableMode == SHOWFAVORITE) {
			Refresh();
		}
	}

	void UITable::SyncChangesToKit()
	{
		if (this->HasFlag(ModexTableFlag_Kit)) {
			if (selectedKitPtr && !selectedKitPtr->empty()) {
				auto equipmentConfig = EquipmentConfig::GetSingleton();
				selectedKitPtr->m_items.clear();
				selectedKitPtr->m_spells.clear();

				for (auto& entry : this->tableList) {
					auto* form = entry->GetTESForm();
					if (form && form->As<RE::SpellItem>()) {
						selectedKitPtr->m_spells.emplace_back(EquipmentConfig::CreateKitSpell(*entry));
					} else {
						selectedKitPtr->m_items.emplace_back(EquipmentConfig::CreateKitItem(*entry));
					}
				}

				equipmentConfig->SaveKit(*selectedKitPtr);
			}
		}
	}

	void UITable::FlushPendingKitChanges()
	{
		if (!selectedKitPtr || selectedKitPtr->empty()) {
			return;
		}

		// Process pending removals.
		if (!m_markedForDelete.empty()) {
			std::unordered_set<std::string> to_remove(m_markedForDelete.begin(), m_markedForDelete.end());
			m_markedForDelete.clear();

			auto it = std::remove_if(this->tableList.begin(), this->tableList.end(),
				[&to_remove](const std::unique_ptr<BaseObject>& item) {
					return to_remove.contains(item->GetEditorID());
				});

			this->tableList.erase(it, this->tableList.end());
			selectionStorage.Clear();
			selectedKitPtr->m_dirty = true;
		}

		// Sync all changes to disk if dirty.
		if (selectedKitPtr->m_dirty) {
			SyncChangesToKit();
			selectedKitPtr->m_dirty = false;
		}
	}

	void UITable::SortListBySpecs()
	{
		std::sort(tableList.begin(), tableList.end(), [this](const std::unique_ptr<BaseObject>& a, const std::unique_ptr<BaseObject>& b) {
			return sortSystem->SortFn(a, b);
		});
	}

	void UITable::UpdateImGuiTableIDs()
	{
		for (int i = 0; i < std::ssize(tableList); i++) {
			tableList[i]->m_tableID = i;
		}

		if (!m_pendingSelection.empty()) {
			for (auto& item : tableList) {
				if (m_pendingSelection.contains(item->GetBaseFormID())) {
					selectionStorage.SetItemSelected(item->m_tableID, true);
				}
			}
			m_pendingSelection.clear();
		}
	}

	std::vector<BaseObject> UITable::GetReferenceInventory()
	{
		std::vector<BaseObject> m_inventory;

		if (!tableTargetRef)
			return m_inventory;

		auto inventory = tableTargetRef->GetInventory();
		for (auto& [obj, data] : inventory) {
			auto& [count, entry] = data;
			if (count > 0 && entry) {
				uint32_t quantity = static_cast<std::uint32_t>(count);
				m_inventory.emplace_back(entry->object, owner, 0, 0, quantity);
			}
		}

		Trace("Cached {} inventory items.", m_inventory.size());
		return m_inventory;
	}

	void UITable::Refresh()
	{
		selectionStorage.Clear();
		tableList.clear();

		if (HasFlag(ModexTableFlag_Dirty)) {
			RemoveFlag(ModexTableFlag_Dirty);
		}
		
		if (this->tableMode == SHOWRECENT) {
			return FilterRecentImpl();
		}

		if (this->tableMode == SHOWFAVORITE) {
			return FilterFavoriteImpl();
		}

		if (this->HasFlag(ModexTableFlag_Kit)) {
			return FilterKitImpl();
		}

		if (this->HasFlag(ModexTableFlag_Inventory)) {
			return FilterInventoryImpl();
		}

		if (tableList.empty())
		{
			if (owner == Ownership::Item)
				return Filter(Data::GetSingleton()->GetAddItemList());
			if (owner == Ownership::Actor)
				return Filter(Data::GetSingleton()->GetNPCList());
			if (owner == Ownership::Object)
				return Filter(Data::GetSingleton()->GetObjectList());
			if (owner == Ownership::Outfit)
				return Filter(Data::GetSingleton()->GetOutfitList());
			if (owner == Ownership::Cell)
				return Filter(Data::GetSingleton()->GetTeleportList());
			if (owner == Ownership::Spell)
				return Filter(Data::GetSingleton()->GetSpellList());
			if (owner == Ownership::Kit)
				return FilterKitListImpl();
		}
	}
	
	void UITable::FilterFavoriteImpl()
	{
		const auto favorites = UserData::GetFavoritesAsVector();
		auto temp = TableList{};

		for (const auto& favoriteItem : favorites) {
			if (favoriteItem.refid != 0) {
				if (RE::TESForm* form = RE::TESForm::LookupByID(favoriteItem.refid); form != nullptr) {
					if (const auto reference = form->As<RE::TESObjectREFR>(); reference != nullptr) {
						temp.emplace_back(std::make_unique<BaseObject>(reference->GetBaseObject()->As<RE::TESForm>(), favoriteItem.owner, 0, favoriteItem.refid));
						continue;
					}
				}
			} else {
				if (RE::TESForm* form = RE::TESForm::LookupByEditorID(favoriteItem.editorid); form != nullptr) {
					temp.emplace_back(std::make_unique<BaseObject>(form, favoriteItem.owner, 0, favoriteItem.refid));
					continue;
				}
			}

			temp.emplace_back(std::make_unique<BaseObject>(favoriteItem.editorid, favoriteItem.editorid, favoriteItem.plugin, favoriteItem.owner, favoriteItem.refid));
		}

		for (auto& item : temp) {
			if (item->GetOwnership() != owner) {
				continue;
			}

			if (filterSystem && !filterSystem->ShouldShowItem(item.get())) {
				continue;
			}

			this->tableList.emplace_back(std::move(item)); 
		}

		SortListBySpecs();
		UpdateImGuiTableIDs();
	}

	void UITable::FilterRecentImpl()
	{
		const auto recent = UserData::GetRecentAsVector();
		auto temp = TableList{};

		for (const auto& recentItem : recent) {
			if (recentItem.refid != 0) {
				if (RE::TESForm* form = RE::TESForm::LookupByID(recentItem.refid); form != nullptr) {
					if (const auto reference = form->As<RE::TESObjectREFR>(); reference != nullptr) {
						temp.emplace_back(std::make_unique<BaseObject>(reference->GetBaseObject()->As<RE::TESForm>(), recentItem.owner, 0, recentItem.refid));
						continue;
					}
				}
			} else {
				if (RE::TESForm* form = RE::TESForm::LookupByEditorID(recentItem.editorid); form != nullptr) {
					temp.emplace_back(std::make_unique<BaseObject>(form, recentItem.owner, 0, recentItem.refid));
					continue;
				}
			}

			temp.emplace_back(std::make_unique<BaseObject>(recentItem.editorid, recentItem.editorid, recentItem.plugin, recentItem.owner, recentItem.refid));
		}

		for (auto& item : temp) {
			if (item->GetOwnership() != owner) {
				continue;
			}

			if (filterSystem && !filterSystem->ShouldShowItem(item.get())) {
				continue;
			}

			this->tableList.emplace_back(std::move(item)); 
		}

		SortListBySpecs();
		UpdateImGuiTableIDs();
	}

	void UITable::FilterKitImpl()
	{
		if (!selectedKitPtr)
			return;

		for (const auto& item : selectedKitPtr->m_items) {
			RE::TESForm* form = RE::TESForm::LookupByEditorID(item->m_editorid);

			if (form) {
				tableList.emplace_back(std::make_unique<BaseObject>(form, Ownership::Item, 0, 0, item->m_amount, item->m_equipped));
			} else {
				tableList.emplace_back(std::make_unique<BaseObject>(item->m_name, item->m_editorid, item->m_plugin, Ownership::Item, 0, item->m_amount, item->m_equipped));
			}
		}

		for (const auto& spell : selectedKitPtr->m_spells) {
			RE::TESForm* form = RE::TESForm::LookupByEditorID(spell->m_editorid);

			if (form) {
				tableList.emplace_back(std::make_unique<BaseObject>(form, Ownership::Spell));
			} else {
				tableList.emplace_back(std::make_unique<BaseObject>(spell->m_name, spell->m_editorid, spell->m_plugin, Ownership::Spell));
			}
		}

		SortListBySpecs();
		UpdateImGuiTableIDs();
	}

	void UITable::FilterKitListImpl()
	{
		auto& cache = EquipmentConfig::GetEquipmentList();
		std::vector<BaseObject> kitObjects;
		kitObjects.reserve(cache.size());

		for (const auto& [key, kit] : cache) {
			kitObjects.emplace_back(kit.GetNameTail(), key, kit.m_collection, Ownership::Kit);
		}

		Filter(kitObjects);
	}

	void UITable::FilterInventoryImpl()
	{
		const auto inventory = GetReferenceInventory();

		for (const auto& item : inventory) {
			this->tableList.emplace_back(std::make_unique<BaseObject>(item.GetTESForm(), owner, 0, 0, item.GetQuantity()));
		}

		SortListBySpecs();
		UpdateImGuiTableIDs();
	}

	void UITable::Filter(const std::vector<BaseObject>& a_data)
	{
		if (a_data.empty()) {
			return;
		}

		for (const auto& item : a_data) {
			if (searchSystem->CompareInputToObject(&item) == false) {
				continue;
			}

			// All Mods vs Selected Mod
			if (this->selectedPlugin != Translate("SHOWALL") && item.GetPluginName() != this->selectedPlugin) {
				continue;
			}

			// Blacklist
			if (this->selectedPlugin == Translate("SHOWALL")) {
				if (const auto& fileOpt = item.GetFile(); fileOpt.has_value()) {
					const auto& file = fileOpt.value();
					if (BlacklistConfig::GetSingleton()->Has(file)) {
						continue;
					}
				}
			}

			// Filter Tree Node system
			if (filterSystem && !filterSystem->ShouldShowItem(&item)) {
				continue;
			}

			if (item.IsDummy()) {
				this->tableList.emplace_back(std::make_unique<BaseObject>(item));
			} else {
				this->tableList.emplace_back(std::make_unique<BaseObject>(item.GetTESForm(), owner, 0, item.m_refID));
			}
		}

		SortListBySpecs();
		UpdateImGuiTableIDs();
	}

	void UITable::BuildPluginList()
	{
		const auto& config = UserConfig::Get();
		const auto& sort = static_cast<PluginSort>(config.modListSort);

		this->pluginList = Data::GetSingleton()->GetFilteredListOfPluginNames(owner, sort); 
		this->pluginSet = Data::GetSingleton()->GetModulePluginList(owner);

		pluginList.insert(pluginList.begin(), Translate("SHOWALL"));
	}

	void UITable::DrawFormSearchBar(const ImVec2& a_size)
	{
		if (tableMode != SHOWALL) ImGui::BeginDisabled();

		const float input_width = a_size.x;
		const float key_width = a_size.x * ThemeConfig::GetWidgetStyle().table.searchKeyRatio;

		int current_idx = searchSystem->GetSearchKeyIndex();
		const std::string current_key_text = searchSystem->GetCurrentKeyString();

		ImGui::PushStyleColor(ImGuiCol_FrameBg, ThemeConfig::GetColor("BG_LIGHT"));
		ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ThemeConfig::GetHover("BG_LIGHT"));
		ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ThemeConfig::GetActive("BG_LIGHT"));
		ImGui::PushStyleVar(ImGuiStyleVar_SelectableTextAlign, ImVec2(0.5f, 0.5f));

		ImVec2 cursor_pos = ImGui::GetCursorScreenPos();
		const std::vector<std::string> available_keys = searchSystem->GetAvailableKeysVector();
		if (UICustom::FancyDropdown("##Search::Input::Key", "TABLE_KEY_TOOLTIP", current_idx, available_keys, key_width)) {
			searchSystem->SetSearchKeyByIndex(current_idx);
			Refresh();
		}

		ImGui::PopStyleVar();
		ImGui::PopStyleColor(3);
		
		{ // Dropdown Descriptor
			const auto draw_list = ImGui::GetWindowDrawList();
			const auto text = Translate("SEARCH_KEY");
			const auto text_pos_x = (cursor_pos.x + (key_width / 2.0f)) - (ImGui::CalcTextSize(text).x / 2.0f);
			const auto text_pos_y = cursor_pos.y - ImGui::GetFrameHeight();
			const auto alpha = ImGui::GetStyle().Alpha;
			draw_list->AddText(ImVec2(text_pos_x, text_pos_y), ThemeConfig::GetColorU32("TEXT_DISABLED", alpha), text);
		}

		ImGui::AlignTextToFramePadding();
		ImGui::Text(" " ICON_LC_ARROW_LEFT_RIGHT " ");
		ImGui::SameLine();
		
		const bool force_quick = HasFlag(ModexTableFlag_APIMode) ? true : useQuickSearch;
		const auto input_flags = force_quick ? ImGuiInputTextFlags_AutoSelectAll :
		ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_EnterReturnsTrue;
		const std::string& search_hint = TRUNCATE(Translate("TABLE_SEARCH_HINT"), input_width * 0.80f).c_str();

		static bool key_hovered;
		cursor_pos = ImGui::GetCursorScreenPos();
		ImGui::PushStyleColor(ImGuiCol_FrameBg, key_hovered ? ThemeConfig::GetHover("BG_LIGHT") : ThemeConfig::GetColor("BG_LIGHT"));
		if (UICustom::FancyInputText("##Search::Input::Compare", search_hint.c_str(), "TABLE_SEARCH_TOOLTIP", searchSystem->GetSearchBuffer(), input_width, input_flags)) {
			this->Refresh();
		}
		ImGui::PopStyleColor();

		key_hovered = ImGui::IsItemHovered();
		if (ImGui::Shortcut(ImGuiKey_Space, ImGuiInputFlags_RouteFromRootWindow)) {
			ImGui::SetKeyboardFocusHere(-1);
		}
		
		ImGui::SameLine();

		{ // Dropdown Descriptor
			const auto draw_list = ImGui::GetWindowDrawList();
			const auto text = Translate("SEARCH_PHRASE");
			const auto text_pos_x = (cursor_pos.x + (a_size.x / 2.0f)) - (ImGui::CalcTextSize(text).x / 2.0f);
			const auto text_pos_y = cursor_pos.y - ImGui::GetFrameHeight();
			const auto alpha = ImGui::GetStyle().Alpha;
			draw_list->AddText(ImVec2(text_pos_x, text_pos_y), ThemeConfig::GetColorU32("TEXT_DISABLED", alpha), text);
		}

		ImGui::AlignTextToFramePadding();
		ImGui::Text(" " ICON_LC_ARROW_RIGHT_TO_LINE " ");
		if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort | ImGuiHoveredFlags_NoSharedDelay)) {
			UICustom::FancyTooltip("TABLE_PLUGIN_TOOLTIP");
		}
		ImGui::SameLine();

		if (tableMode != SHOWALL) ImGui::EndDisabled();
	}

	void UITable::DrawPluginSearchBar(const ImVec2& a_size)
	{
		if (tableMode != SHOWALL) ImGui::BeginDisabled();

		static bool hovered;
		ImGui::PushStyleColor(ImGuiCol_FrameBg, hovered ? ThemeConfig::GetHover("BG_LIGHT") : ThemeConfig::GetColor("BG_LIGHT"));

		const auto cursor_pos = ImGui::GetCursorScreenPos();
		if (searchSystem->InputTextComboBox("##Search::Filter::PluginField", pluginSearchBuffer, selectedPlugin, IM_ARRAYSIZE(pluginSearchBuffer), pluginList, a_size.x)) {
			this->selectedPlugin = this->pluginSearchBuffer;
			this->pluginSearchBuffer[0] = '\0';
			
			this->selectionStorage.Clear();
			this->Refresh();
		}

		{ // Dropdown Descriptor
			const auto draw_list = ImGui::GetWindowDrawList();
			const auto text = Translate("SEARCH_PLUGIN");
			const auto text_pos_x = (cursor_pos.x + (a_size.x / 2.0f)) - (ImGui::CalcTextSize(text).x / 2.0f);
			const auto text_pos_y = cursor_pos.y - ImGui::GetFrameHeight();
			const auto alpha = ImGui::GetStyle().Alpha;
			draw_list->AddText(ImVec2(text_pos_x, text_pos_y), ThemeConfig::GetColorU32("TEXT_DISABLED", alpha), text);
		}

		hovered = ImGui::IsItemHovered();
		if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal | ImGuiHoveredFlags_NoSharedDelay)) {
			UICustom::FancyTooltip("TABLE_PLUGIN_TOOLTIP");
		}
		
		ImGui::PopStyleColor();
		if (tableMode != SHOWALL) ImGui::EndDisabled();
	}

	void UITable::UpdateLayout()
	{
		const auto& config = UserConfig::Get();

		styleHeight = UserData::Get<float>("Modex::Table::ItemHeight", 0.0f);
		styleWidth = UserData::Get<float>("Modex::Table::ItemWidth", 0.0f);
		styleSpacing = UserData::Get<float>("Modex::Table::ItemSpacing", 0.0f);
		styleFontSize = UserData::Get<float>("Modex::Table::FontSize", 0.0f); 
		showAltRowBG = UserData::Get<bool>("Modex::Table::ShowAltRowBG", true);
		showItemIcon = UserData::Get<bool>("Modex::Table::ShowItemIcon", true);
		useQuickSearch = UserData::Get<bool>("Modex::Table::UseQuickSearch", false);

		colors.alpha = ImGui::GetStyle().Alpha;
		colors.background = ThemeConfig::GetColorU32("TABLE_BG", colors.alpha);
		colors.backgroundAlt = ThemeConfig::GetColorU32("TABLE_BG_ALT", colors.alpha);
		colors.selected = ThemeConfig::GetColorU32("TABLE_SELECTED", colors.alpha);
		colors.outline = ThemeConfig::GetColorU32("TABLE_BORDER", colors.alpha);
		colors.hover = ThemeConfig::GetColorU32("TABLE_HOVER", colors.alpha);
		colors.text = ThemeConfig::GetColorU32("TEXT", colors.alpha);
		colors.textDisabled = ThemeConfig::GetColorU32("TEXT_DISABLED", colors.alpha);
		colors.textEnchanted = ThemeConfig::GetColorU32("TEXT_ENCHANTED", colors.alpha);
		colors.textUnique = ThemeConfig::GetColorU32("TEXT_UNIQUE", colors.alpha);
		colors.textEssential = ThemeConfig::GetColorU32("TEXT_ESSENTIAL", colors.alpha);
		colors.textUniqueEssential = ThemeConfig::GetColorU32("TEXT_UNIQUE_ESSENTIAL", colors.alpha);
		colors.error = ThemeConfig::GetColorU32("ERROR", colors.alpha * 0.1f);

		if (styleFontSize == 0.0f) {
			styleFontSize = static_cast<decltype(styleFontSize)>(config.globalFontSize);
		}

		// Constrained to the child window which we draw within.
		const float full_width = ImGui::GetContentRegionAvail().x;
		const float full_height = ImGui::GetContentRegionAvail().y;

		const auto& tbl = ThemeConfig::GetWidgetStyle().table;
		const float height = (styleFontSize * tbl.rowHeightScale) + styleHeight;

		// Spacing between each element.
		LayoutRowSpacing = tbl.rowSpacingBase + styleSpacing;
		LayoutHitSpacing = 0.0f;

		// Calculate whether a scrollbar is present based on which list we're viewing.
		const int table_size = tableMode == SHOWRECENT ? static_cast<int>(std::ssize(recentList)) : static_cast<int>(std::ssize(tableList));
		const float total_height = table_size * (height) + ((table_size + 2) * LayoutRowSpacing);

		// Offset Layout width if scrollbar is present
		const float scroll_bar = total_height > full_height ? ImGui::GetStyle().ScrollbarSize : 0.0f;
		const float width = full_width - scroll_bar;

		if (width <= 0.0f) {
			return;
		}

		// Each table item's width and height.
		LayoutItemSize = ImVec2(width, height);

		LayoutColumnCount = 1;
		LayoutColumnWidth = LayoutItemSize.x + styleWidth;

		// Used to increment position for next table item.
		LayoutItemStep = ImVec2(LayoutItemSize.x, LayoutItemSize.y + LayoutRowSpacing);

		// Includes measurement for outline thickness!
		// LayoutOuterPadding = floorf(LayoutRowSpacing * 0.5f);
		LayoutOuterPadding = 0.0f;
	}

	void UITable::DrawModeDropdown(const ImVec2& a_size)
	{
		ImGui::PushStyleColor(ImGuiCol_FrameBg, ThemeConfig::GetColor("BG_LIGHT"));
		ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ThemeConfig::GetHover("BG_LIGHT"));
		ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ThemeConfig::GetActive("BG_LIGHT"));
		ImGui::PushStyleVar(ImGuiStyleVar_SelectableTextAlign, ImVec2(0.5f, 0.5f));

		auto modes = magic_enum::enum_names<TableMode>();
		const std::vector<std::string> mode_strings(modes.begin(), modes.end());
		int current_idx = static_cast<int>(tableMode);
		const auto cursor_pos = ImGui::GetCursorScreenPos();
		if (UICustom::FancyDropdown("##Search::Input::Mode", "TABLE_MODE_TOOLTIP", current_idx, mode_strings, a_size.x)) {
			selectionStorage.Clear();
			tableMode = current_idx;
			Refresh();
		}

		ImGui::PopStyleColor(3);
		ImGui::PopStyleVar();

		// Draw dropdown descriptor
		const auto draw_list = ImGui::GetWindowDrawList();
		const auto text = Translate("SEARCH_LIST");
		const auto text_pos_x = (cursor_pos.x + (a_size.x / 2)) - (ImGui::CalcTextSize(text).x / 2.0f);
		const auto text_pos_y = cursor_pos.y - ImGui::GetFrameHeight();
		const auto alpha = ImGui::GetStyle().Alpha;
		draw_list->AddText(ImVec2(text_pos_x, text_pos_y), ThemeConfig::GetColorU32("TEXT_DISABLED", alpha), text);

		const auto icon = tableMode == SHOWALL ? " " ICON_LC_ARROW_RIGHT " " : " " ICON_LC_ARROW_DOWN " ";
		ImGui::AlignTextToFramePadding();
		ImGui::Text(icon);
		ImGui::SameLine();
	}

	void UITable::DrawSearchBar()
	{
		// Nullify horizontal spacing. Horizontal spacing between table widgets handled here.
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, ImGui::GetFrameHeight() / 2.0f));
		ImGui::Spacing();
		ImGui::Spacing();

		const auto& tbl = ThemeConfig::GetWidgetStyle().table;
		const float dropdown_width = ImGui::GetContentRegionAvail().x / tbl.searchModeDivisor;
		DrawModeDropdown(ImVec2(dropdown_width, 0.0f));

		const float search_width = ImGui::GetContentRegionAvail().x / tbl.searchFormDivisor;
		DrawFormSearchBar(ImVec2(search_width, 0.0f));

		
		const float plugin_width = ImGui::GetContentRegionAvail().x;
		DrawPluginSearchBar(ImVec2(plugin_width, 0.0f));
		ImGui::PopStyleVar();
	}

	void UITable::ResolvePayloadDrop(UITable* origin, UITable* destination, std::vector<std::unique_ptr<BaseObject>>& payload_items)
	{
		// Behavior based on source and destination types.
		for (const auto& item : payload_items) {
			if (destination->GetDragDropHandle() == DragDropHandle::Kit) {
				destination->AddPayloadToKit(item);
			}	

			if (destination->GetDragDropHandle() == DragDropHandle::Table) {
				if (origin->GetDragDropHandle() == DragDropHandle::Kit) {
					origin->RemovePayloadItemFromKit(item);
				}

				if (origin->GetDragDropHandle() == DragDropHandle::Inventory) {
					origin->RemovePayloadFromInventory(item);
				}
			}

			if (destination->tableID != origin->tableID) {
				if (destination->GetDragDropHandle() == DragDropHandle::Inventory) {
					destination->AddPayloadToInventory(item);

					if (origin->GetDragDropHandle() == DragDropHandle::Inventory) {
						origin->RemovePayloadFromInventory(item);
					}
				}
			}
		}

		if (destination->GetDragDropHandle() == DragDropHandle::Kit) {
			destination->Refresh();
		}

		if (origin->GetDragDropHandle() == DragDropHandle::Kit) {
			origin->Refresh();
		}

		// destination->Refresh();
		// origin->Refresh();
	}

	void UITable::HandleDragDropBehavior()
	{
		static std::string tooltip_icon = "";
		static std::string tooltip_string = "";
		static std::string tooltip_target = "";

		if (ImGui::GetDragDropPayload()) { // Handle Tooltips outside of widget targets!
			DrawDragDropPayload(tooltip_icon);
		}

		// The UITable Clipper/Child Widget rectangle acts as the drop target.
		if (ImGui::BeginDragDropTarget()) {
			for (auto& source : dragDropSourceList) {
				const auto ptr = source.second;

				const auto handle_string = std::to_string(ptr->tableID);
				const auto destination = this;
				const auto origin = ptr;
				const auto shift_down = ImGui::GetIO().KeyShift;

				if (destination->tableID == origin->tableID) {
					continue;
				}

				tooltip_icon = "";
				if (ImGui::AcceptDragDropPayload(handle_string.c_str(), ImGuiDragDropFlags_AcceptPeekOnly)) {
					if (destination->GetDragDropHandle() == DragDropHandle::Kit) {
						tooltip_icon = ICON_LC_PLUS;
						tooltip_string = Translate("KIT_ADD");
						tooltip_target = destination->selectedKitPtr ? destination->selectedKitPtr->m_key : "";
					}

					if (destination->GetDragDropHandle() == DragDropHandle::Inventory) {
						tooltip_icon = ICON_LC_PLUS;
						tooltip_string = Translate("ADD_SELECTION");
						tooltip_target = tableTargetRef ? tableTargetRef->GetName() : "";
					}

					if (destination->GetDragDropHandle() == DragDropHandle::Table) {
						if (origin->GetDragDropHandle() == DragDropHandle::Kit) {
							tooltip_icon = ICON_LC_X;
							tooltip_string = Translate("KIT_REMOVE");
							tooltip_target = origin->selectedKitPtr ? origin->selectedKitPtr->m_key : "";
						}

						if (origin->GetDragDropHandle() == DragDropHandle::Inventory) {
							tooltip_icon = ICON_LC_TRASH;
							tooltip_string = Translate("REMOVE_SELECTION");
							tooltip_target = tableTargetRef ? tableTargetRef->GetName() : "";
						}
					}

					// If the above wasn't resolved, either we're invalid state or empty target.
					if (tooltip_string.empty() && origin->tableTargetRef && destination->tableTargetRef) {
						if (origin->tableTargetRef->GetFormID() == destination->tableTargetRef->GetFormID()) {
							tooltip_icon = ICON_LC_X;
							tooltip_string = Translate("ERROR_SAME_REF");
							tooltip_target = destination->tableTargetRef ? destination->tableTargetRef->GetName() : "";
						}
					}

					// Edge case where on multi-table layout one table is empty, use as trash can.
					if (origin->tableTargetRef && !destination->tableTargetRef) {
						tooltip_icon = ICON_LC_TRASH;
						tooltip_string = Translate("REMOVE_SELECTION");
						tooltip_target = tableTargetRef ? tableTargetRef->GetName() : "";
					}

					if (!tooltip_string.empty()) {
						const auto& overlay = ThemeConfig::GetWidgetStyle().dragOverlay;
						const auto& DrawList = ImGui::GetForegroundDrawList();
						const auto table_size = ImGui::GetItemRectSize();
						const auto window_size = ImGui::GetWindowSize();
						const auto window_pos = ImGui::GetWindowPos();
						const ImVec2 min = window_pos + ImVec2(0.0f, window_size.y - table_size.y);
						const ImVec2 max = min + table_size;
						const float font_size = overlay.iconFontSize;
						const float label_font = font_size / overlay.labelFontDivisor;
						const float target_font = font_size / overlay.targetFontDivisor;
						const float center_offset = font_size / 4.0f;

						ImGui::PushFont(NULL, font_size);
						const ImVec2 icon_size = ImGui::CalcTextSize(tooltip_icon.c_str());
						ImGui::PopFont();

						DrawList->AddRectFilled(min, max, ThemeConfig::GetColorU32("BG"));

						DrawList->AddText(ImGui::GetFont(), font_size,
							min + (table_size / 2.0f) - (icon_size / 1.5f) - ImVec2(0, center_offset),
							ThemeConfig::GetColorU32("TEXT"),
							tooltip_icon.c_str()
						);

						ImGui::PushFont(NULL, label_font);
						const ImVec2 string_size = ImGui::CalcTextSize(tooltip_string.c_str());
						ImGui::PopFont();

						DrawList->AddText(ImGui::GetFont(), label_font,
							min + (table_size / 2.0f) - (string_size / 2.0f) + ImVec2(0, icon_size.y / 1.5f) - ImVec2(0, center_offset),
							ThemeConfig::GetColorU32("TEXT", 0.75f),
							tooltip_string.c_str()
						);

						if (!tooltip_target.empty()) {
							ImGui::PushFont(NULL, label_font);
							const ImVec2 target_size = ImGui::CalcTextSize(tooltip_target.c_str());
							ImGui::PopFont();

							DrawList->AddText(ImGui::GetFont(), target_font,
								min + (table_size / 2.0f) - (target_size / 2.0f) + ImVec2(0, icon_size.y * 1.25f) - ImVec2(0, center_offset),
								ThemeConfig::GetColorU32("TEXT", 0.5f),
								tooltip_target.c_str()
							);
						}

					}
				}

				// Payload Data is contained in other table sources.
				if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(handle_string.c_str()); payload && payload->IsDelivery()) {
					const int payloadCount = (int)payload->DataSize / (int)sizeof(ImGuiID);

					std::vector<std::unique_ptr<BaseObject>> payload_items;
					for (int payload_idx = 0; payload_idx < payloadCount; ++payload_idx) {
						const ImGuiID payloadID = ((ImGuiID*)payload->Data)[payload_idx];
						if (const auto& item = (*ptr->GetTableListPtr())[payloadID]; item != nullptr) {
							payload_items.emplace_back(std::make_unique<BaseObject>(*item));
						}
					}


					// If we're dragging into an empty / none kit, prompt the user to create one.
					if (destination->GetDragDropHandle() == DragDropHandle::Kit) {
						if (destination->selectedKitPtr && destination->selectedKitPtr->empty()) {

							// By wrapping payload_items as a shared pointer, we keep it alive long enough
							// to use and discard after the popup window is completed!
							auto items = std::make_shared<std::vector<std::unique_ptr<BaseObject>>>(std::move(payload_items));

							Kit* pointer = destination->selectedKitPtr;
							UIManager::GetSingleton()->ShowInputBox(
								Translate("POPUP_KIT_CREATE_TITLE"),
								Translate("POPUP_KIT_CREATE_DESC"),
								"",
								[items, pointer, destination](const std::string& a_input) {
									if (auto new_kit = EquipmentConfig::CreateKit(a_input); new_kit) {
										*pointer = std::move(new_kit);

										for (const auto& item : *items) {
											auto* form = item->GetTESForm();
											if (form && form->As<RE::SpellItem>()) {
												pointer->m_spells.emplace_back(EquipmentConfig::CreateKitSpell(*item));
											} else {
												pointer->m_items.emplace_back(EquipmentConfig::CreateKitItem(*item));
											}
											destination->Refresh();
										}
									}
								}
							);

							return;
						}
					}

					if (shift_down) {
						auto items = std::make_shared<std::vector<std::unique_ptr<BaseObject>>>(std::move(payload_items));
						UICustom::InputAmountHandler(shift_down, [items, this, destination, origin](uint32_t amount) {
							for (auto& item : *items) {
								item->m_quantity = amount;
							}

							ResolvePayloadDrop(origin, destination, *items);
						});
						return;
					} else {
						ResolvePayloadDrop(origin, destination, payload_items);
					}
				}
			}
		}
	}

	void UITable::HandleItemHoverPreview(const std::unique_ptr<BaseObject>& a_item)
	{
		itemPreview = std::make_unique<BaseObject>(*a_item);

		if (HasFlag(ModexTableFlag_EnableItemPreviewOnHover) && !a_item->IsDummy()) {
			if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort | ImGuiHoveredFlags_NoSharedDelay)) {
				const auto size = ImVec2(ImGui::GetWindowSize().x / 3.0f, 0.f);
				if (ImGui::BeginTooltip()) {
					ShowItemPreview(a_item, true);
					ImGui::EndTooltip();
				}
			}
		}

		if (a_item->IsDummy() && owner != Ownership::Cell) {
			UINotification::ShowTooltip(Translate("DUMMY_OBJECT_INFO"), ICON_LC_MESSAGE_CIRCLE_QUESTION);
		}
	}

	// NOTE:: APIMode Input Amount adds duplicates for consistent papyrus API usage. Avoids adding
	// another parameter we have to manage across instances (count).

	void UITable::HandleLeftClickBehavior(const std::unique_ptr<BaseObject>& a_item)
	{
		if (HasFlag(ModexTableFlag_APIMode)) {
			if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left) && (m_selectionChangedCallback || m_selectionChangedStringCallback)) {
				UICustom::InputAmountHandler(ImGui::GetIO().KeyShift, [&a_item, this](uint32_t amount = 1) {
					for (uint32_t i = 0; i < amount; i++) {
						if (m_selectionChangedCallback) {
							m_selectionChangedCallback({ a_item->GetBaseFormID() });
						}
						if (m_selectionChangedStringCallback) {
							m_selectionChangedStringCallback({ a_item->GetEditorID() });
						}
					}
				});

				ImGuiIO& io = ImGui::GetIO();
				io.MouseClickedTime[ImGuiMouseButton_Left] = -FLT_MAX;
				io.MouseClickedCount[ImGuiMouseButton_Left] = 0;
			}

			return;
		}

		if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
			if (tableTargetRef && a_item->IsItem() && !Commands::IsGameMenuOpen()) {
				UICustom::InputAmountHandler(ImGui::GetIO().KeyShift, [&a_item, this](uint32_t amount = 1) {
					if (auto targetRef = this->GetTableTargetRef(); targetRef) {
						Commands::AddItemToRefInventory(owner, targetRef, a_item->GetBaseFormID(), amount);
					}
				});
			}

			if (owner == Ownership::Actor && !Commands::IsGameMenuOpen()) {
				UICustom::InputAmountHandler(ImGui::GetIO().KeyShift, [&a_item, this](uint32_t amount = 1) {
					Commands::PlaceAtMe(owner, a_item->GetBaseFormID(), amount);
				});
			}

			if (owner == Ownership::Object && !Commands::IsGameMenuOpen()) {
				UICustom::InputAmountHandler(ImGui::GetIO().KeyShift, [&a_item, this](uint32_t amount = 1) {
					Commands::PlaceAtMe(owner, a_item->GetBaseFormID(), amount);
				});
			}

			if (owner == Ownership::Spell && IsValidTargetReference()) {
				Commands::AddSpellToActor(owner, GetTableTargetRef(), a_item->GetBaseFormID());
			}

			if (owner == Ownership::Outfit && tableTargetRef && !Commands::IsGameMenuOpen()) {
				if (auto outfit = a_item->GetTESOutfit(); outfit) {
					Commands::AddOutfitItemsToInventory(owner, tableTargetRef, outfit);
				}
			}

			// BUG: Returning to menu after double-click causes first left-click to not register?

			if (owner == Ownership::Cell) {
				Commands::CenterOnCell(Ownership::Cell, a_item->GetEditorID());
			}

			// Reset imgui click state to speed up double click registers
			ImGuiIO& io = ImGui::GetIO();
			io.MouseClickedTime[ImGuiMouseButton_Left] = -FLT_MAX;
			io.MouseClickedCount[ImGuiMouseButton_Left] = 0;

		}
	}

	void UITable::HandleRightClickBehavior(const std::unique_ptr<BaseObject>& a_item)
	{
		if (HasFlag(ModexTableFlag_APIMode)) {
			if (ImGui::IsItemClicked(ImGuiMouseButton_Right)) {
				if (!selectionStorage.Contains(a_item->m_tableID)) {
					selectionStorage.Clear();
				}

				if (!a_item->IsDummy() || m_selectionChangedStringCallback) {
					ImGui::OpenPopup("APIModeContextMenu");
				}
			}

			if (ImGui::BeginPopup("APIModeContextMenu")) {
				if (ImGui::MenuItem(Translate("ADD_SELECTION"))) {
					if (m_selectionChangedCallback || m_selectionChangedStringCallback) {
						UICustom::InputAmountHandler(ImGui::GetIO().KeyShift, [&](uint32_t amount = 1) {
							std::vector<RE::FormID> selectedIDs;
							std::vector<std::string> selectedStrings;
							void* it = NULL;
							ImGuiID id = 0;
							while (selectionStorage.GetNextSelectedItem(&it, &id)) {
								if (id < tableList.size()) {
									selectedIDs.push_back(tableList[id]->GetBaseFormID());
									selectedStrings.push_back(tableList[id]->GetEditorID());
								}
							}
							if (selectedIDs.empty()) {
								selectedIDs.push_back(a_item->GetBaseFormID());
								selectedStrings.push_back(a_item->GetEditorID());
							}

							for (uint32_t i = 0; i < amount; i++) {
								if (m_selectionChangedCallback) {
									m_selectionChangedCallback(selectedIDs);
								}
								if (m_selectionChangedStringCallback) {
									m_selectionChangedStringCallback(selectedStrings);
								}
							}
						});
					}
				}
				ImGui::EndPopup();
			}

			return;
		}

		static const int click_amount = 1; // TODO revisit this

		if (ImGui::IsItemClicked(ImGuiMouseButton_Right)) {
			if (!selectionStorage.Contains(a_item->m_tableID)) {
				selectionStorage.Clear();
			}

			if (!a_item->IsDummy() || a_item->GetOwnership() == Ownership::Cell) {
				ImGui::OpenPopup("TableViewContextMenu");
			}
		}

		if (ImGui::BeginPopup("TableViewContextMenu")) {
			const bool shift_down = ImGui::GetIO().KeyShift;

			if (owner == Ownership::Cell) {
				if (UserData::IsFavorited(a_item->GetEditorID())) {
					if (ImGui::MenuItem(Translate("REMOVE_FROM_FAVORITES"))) {
						RemoveSelectionFromFavorites();
					}
				} else {
					if (ImGui::MenuItem(Translate("ADD_TO_FAVORITES"))) {
						AddSelectionToFavorites();
					}
				}

				if (ImGui::MenuItem(Translate("CENTER_ON_CELL"))) {
					Commands::CenterOnCell(Ownership::Cell, a_item->GetEditorID());
					UserData::SendEvent(ModexActionType::CenterOnCell, a_item);
				}

				ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal);

				if (ImGui::BeginMenu(Translate("COPY"))) {
					if (ImGui::MenuItem(Translate("COPY_FORMID"))) {
						ImGui::SetClipboardText(a_item->GetFormID().c_str());
					}
					if (ImGui::MenuItem(Translate("COPY_EDITORID"))) {
						ImGui::SetClipboardText(a_item->GetEditorID().c_str());
					}
					if (ImGui::MenuItem(Translate("COPY_NAME"))) {
						ImGui::SetClipboardText(a_item->GetName().c_str());
					}
					if (ImGui::MenuItem(Translate("COPY_PLUGIN"))) {
						ImGui::SetClipboardText(a_item->GetPluginName().c_str());
					}
					ImGui::EndMenu();
				}

				ImGui::EndPopup();
				return;
			}

			if (!tableTargetRef || !IsValidTargetReference()) {
				ImGui::TextColored(ThemeConfig::GetColor("ERROR"), "%s", Translate("ERROR_INVALID_REFERENCE"));
				ImGui::EndPopup();
				return;
			}

			const auto color = tableTargetRef->IsPlayerRef() ? ThemeConfig::GetColor("SUCCESS") : ThemeConfig::GetColor("WARN");
			ImGui::TextColored(color, "%s %s", ICON_LC_ASTERISK " ", tableTargetRef->GetName());
			ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal);

			if (!HasFlag(ModexTableFlag_Inventory)) {
				if (a_item->m_refID == 0 ? UserData::IsFavorited(a_item->GetEditorID()) : UserData::IsFavorited(a_item->m_refID)) {
					if (ImGui::MenuItem(Translate("REMOVE_FROM_FAVORITES"))) {
						RemoveSelectionFromFavorites();
					}
				} else {
					if (ImGui::MenuItem(Translate("ADD_TO_FAVORITES"))) {
						AddSelectionToFavorites();
					}
				}
			}

			if (a_item->IsItem()) {
				if (HasFlag(ModexTableFlag_Inventory)) {
					if (ImGui::MenuItem(Translate("REMOVE_SELECTION"))) {
						this->RemoveSelectionFromTargetInventory();
					}

					if (ImGui::MenuItem(Translate("DROP_SELECTION"))) {
						this->PlaceSelectionOnGround(click_amount);
						this->RemoveSelectionFromTargetInventory();
					}
				}

				if (HasFlag(ModexTableFlag_Base) && !Commands::IsGameMenuOpen()) {
					ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal);

					if (ImGui::MenuItem(Translate("ADD_SELECTION"))) {
						UICustom::InputAmountHandler(shift_down, [&](uint32_t amount) {
							this->AddSelectionToTargetInventory(amount);
						});
					}

					if (a_item->IsArmor() || a_item->IsWeapon()) {
						if (ImGui::MenuItem(Translate("EQUIP_SELECTION"))) {
							this->EquipSelectionToTarget();
						}
					}

					if (ImGui::MenuItem(Translate("PLACE_SELECTION"))) {
						UICustom::InputAmountHandler(shift_down, [&](uint32_t amount) {
							this->PlaceSelectionOnGround(amount);
						});
					}


					if (a_item->GetFormType() == RE::FormType::Book) {
						if (GetSelectionCount() <= 1) {
							ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal);

							if (ImGui::MenuItem(Translate("READ"))) {
								Commands::ReadBook(owner, a_item->GetBaseFormID());
								UIManager::GetSingleton()->Close();
							}
						}
					}
				}

				if (HasFlag(ModexTableFlag_Kit)) { // Kit Table View Window
					ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal);

					if (ImGui::MenuItem(Translate("KIT_REMOVE"))) {
						this->RemoveSelectionFromKit();
					}
				}

				// We use dragDropSourceList to find paired Kit tables, since selectedKit is stored in the kit table.
				if (HasFlag(ModexTableFlag_Base)) {
					if (auto it = dragDropSourceList.find(DragDropHandle::Kit); it != this->dragDropSourceList.end()) {
						const auto destination = it->second;
						if (const auto selected_kit = destination->selectedKitPtr; selected_kit && !selected_kit->empty()) {
							if (ImGui::MenuItem(Translate("KIT_ADD"))) {
								this->AddSelectionToActiveKit();
							}
						}
					}
				}
			}

			if (a_item->IsNPC()) { // ModexTableFlag_Base (?)
				if (a_item->m_refID != 0) {
					ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal);
					if (ImGui::MenuItem(Translate("TABLE_SET_TARGET"))) {
						SetTargetByReference(RE::TESForm::LookupByID<RE::TESObjectREFR>(itemPreview->GetRefID()));
					}
				}

				if (!Commands::IsGameMenuOpen()) {
					if (ImGui::MenuItem(Translate("PLACE_SELECTION"))) {
						PlaceSelectionOnGround(click_amount);
					}
				}

				if (a_item->m_refID != 0) {
					if (ImGui::MenuItem(Translate("GOTO_NPC_REFERENCE"))) {
						Commands::TeleportPlayerToNPC(owner, a_item->m_refID);
						UIManager::GetSingleton()->Close();
					}
				}

				if (a_item->m_refID != 0 && !Commands::IsGameMenuOpen()) {
					if (ImGui::MenuItem(Translate("BRING_NPC_REFERENCE"))) {
						Commands::TeleportNPCToPlayer(owner, a_item->m_refID);
						UIManager::GetSingleton()->Close();
					}
				}

				if (a_item->m_refID != 0) {
					if (a_item->IsDisabled()) {
						if (ImGui::MenuItem(Translate("ENABLE_NPC_REFERENCE"))) {
							if (auto* target = RE::TESForm::LookupByID<RE::Actor>(a_item->m_refID); target != nullptr) {
								Commands::EnableRefr(owner, target, false);
							}
						}
					} else {
						if (ImGui::MenuItem(Translate("DISABLE_NPC_REFERENCE"))) {
							if (auto* target = RE::TESForm::LookupByID<RE::Actor>(a_item->m_refID); target != nullptr) {
								Commands::DisableRefr(owner, target);
							}
						}
					}
				}
			}

			if (owner == Ownership::Spell && this->GetTableTargetRef() != nullptr) {
				ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal);

				if (ImGui::MenuItem(Translate("ADD_SPELL_TO_TARGET"))) {
					ExecuteCommandOnSelection([this](const std::unique_ptr<BaseObject>& a_spell) {
						Commands::AddSpellToActor(owner, this->GetTableTargetRef(), a_spell->GetBaseFormID());
					});
				}

				if (ImGui::MenuItem(Translate("REMOVE_SPELL_FROM_TARGET"))) {
					ExecuteCommandOnSelection([this](const std::unique_ptr<BaseObject>& a_spell) {
						Commands::RemoveSpellFromActor(owner, this->GetTableTargetRef(), a_spell->GetBaseFormID());
					});
				}
			}

			if (a_item->IsOutfit()) {
				if (!Commands::IsGameMenuOpen() && this->GetTableTargetRef() != nullptr) {
					ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal);

					if (ImGui::MenuItem(Translate("ADD_OUTFIT_ITEMS"))) {
						ExecuteCommandOnSelection([this](const std::unique_ptr<BaseObject>& a_item) {
							Commands::AddOutfitItemsToInventory(owner, this->GetTableTargetRef(), a_item->GetTESOutfit()); 	
						});
					}

					if (ImGui::MenuItem(Translate("EQUIP_OUTFIT_ITEMS"))) {
						ExecuteCommandOnSelection([this](const std::unique_ptr<BaseObject>& a_item) {
							Commands::EquipOutfit(owner, this->GetTableTargetRef(), a_item->GetTESOutfit());
						});
					}

					if (GetSelectionCount() <= 1 && !this->GetTableTargetRef()->IsPlayerRef()) {
						ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal);
						if (ImGui::MenuItem(Translate("SET_DEFAULT_OUTFIT"))) {
							ExecuteCommandOnSelection([this](const std::unique_ptr<BaseObject>& a_item) {
								Commands::SetDefaultOutfitOnActor(owner, this->GetTableTargetRef(), a_item->GetTESOutfit());	
							});
						}

						if (ImGui::MenuItem(Translate("SET_SLEEP_OUTFIT"))) {
							ExecuteCommandOnSelection([this](const std::unique_ptr<BaseObject>& a_item) {
								Commands::SetSleepOutfitOnActor(owner, this->GetTableTargetRef(), a_item->GetTESOutfit());
							});
						}
					}
				}
			}

			ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal);

			if (ImGui::BeginMenu(Translate("COPY"))) {
				if (ImGui::MenuItem(Translate("COPY_FORMID"))) {
					ImGui::SetClipboardText(a_item->GetFormID().c_str());
				}

				if (ImGui::MenuItem(Translate("COPY_EDITORID"))) {
					ImGui::SetClipboardText(a_item->GetEditorID().c_str());
				}

				if (ImGui::MenuItem(Translate("COPY_NAME"))) {
					ImGui::SetClipboardText(a_item->GetName().c_str());
				}

				if (ImGui::MenuItem(Translate("COPY_PLUGIN"))) {
					ImGui::SetClipboardText(a_item->GetPluginName().c_str());
				}

				if (a_item->GetFormType() == RE::FormType::NPC) {
					if (ImGui::MenuItem(Translate("COPY_REFERENCEID"))) {
						ImGui::SetClipboardText(std::format("{:08x}", a_item->m_refID).c_str());
					}
				}

				ImGui::EndMenu();
			}

			ImGui::EndPopup();
		}
	}

	// This implementation is not great. Navigating within a clipper context with multi selection
	// is above my pay grade. I ended up with a good-e-enough result. Wouldn't recommend duplicating.
	void UITable::HandleKeyboardNavigation(const TableList& a_tableList)
	{
		if (ImGui::Shortcut(ImGuiKey_Escape, ImGuiInputFlags_RouteFromRootWindow)) {
			selectionStorage.Clear();
			itemPreview = nullptr;
			return;
		}

		if (HasFlag(ModexTableFlag_APIMode)) {
			if (ImGui::Shortcut(ImGuiKey_Enter, ImGuiInputFlags_RouteFromRootWindow) && (m_selectionChangedCallback || m_selectionChangedStringCallback)) {
				std::vector<RE::FormID> selectedIDs;
				std::vector<std::string> selectedStrings;
				void* it = NULL;
				ImGuiID id = 0;
				while (selectionStorage.GetNextSelectedItem(&it, &id)) {
					if (id < a_tableList.size()) {
						selectedIDs.push_back(a_tableList[id]->GetBaseFormID());
						selectedStrings.push_back(a_tableList[id]->GetEditorID());
					}
				}
				if (!selectedIDs.empty() && m_selectionChangedCallback) {
					m_selectionChangedCallback(selectedIDs);
				}
				if (!selectedStrings.empty() && m_selectionChangedStringCallback) {
					m_selectionChangedStringCallback(selectedStrings);
				}
			}
		}

		// HACK: This is a result of IMenu impl key behavior. Could do ControlMap fixes, but nty.
		if (ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_LeftArrow, ImGuiInputFlags_RouteFromRootWindow)) {
			if (selectionStorage.Size > 0) {
				selectionStorage.Clear();
			} else {
				for (auto& item : a_tableList) {
					if (item != nullptr) {
						selectionStorage.SetItemSelected(item->m_tableID, true);
					}
				}
			}
		}

		if (ImGui::Shortcut(ImGuiKey_F, ImGuiInputFlags_RouteFromRootWindow)) {
			if (selectionStorage.Size > 0) {
				void* it = NULL;
				ImGuiID id = 0;

				while (selectionStorage.GetNextSelectedItem(&it, &id)) {
					if (id < std::ssize(tableList) && id >= 0) {
						const auto& item = tableList[id];
						if (item) {
							const bool is_favorited = item->GetRefID() == 0 ? UserData::IsFavorited(item->GetEditorID()) : UserData::IsFavorited(item->GetRefID());
							UserData::SendEvent(is_favorited ? ModexActionType::Unfavorited : ModexActionType::Favorited, item);
						}
					}
				}
			} else {
				if (itemPreview) {
					const bool is_favorited = itemPreview->GetRefID() == 0 ? UserData::IsFavorited(itemPreview->GetEditorID()) : UserData::IsFavorited(itemPreview->GetRefID());
					UserData::SendEvent(is_favorited ? ModexActionType::Unfavorited : ModexActionType::Favorited, itemPreview);
				}
			}

			if (tableMode == SHOWFAVORITE) {
				Refresh();
			}
		}

		if (ImGui::Shortcut(ImGuiKey_DownArrow | ImGuiMod_Shift, ImGuiInputFlags_RouteFromRootWindow | ImGuiInputFlags_Repeat) ||
    		ImGui::Shortcut(ImGuiKey_DownArrow, ImGuiInputFlags_RouteFromRootWindow | ImGuiInputFlags_Repeat)) {

			void* it = NULL;
			ImGuiID id = 0;

			if (selectionStorage.GetNextSelectedItem(&it, &id)) {
				size_t current_index = navPositionID;

				if (current_index + 1 < a_tableList.size() && a_tableList[current_index + 1]) {
					if (!ImGui::IsKeyDown(ImGuiMod_Shift)) {
						selectionStorage.Clear();
					}

					navPositionID = a_tableList[current_index + 1]->m_tableID;
					bool is_previously_selected = selectionStorage.Contains(navPositionID);
					selectionStorage.SetItemSelected(navPositionID, true);
					itemPreview = std::make_unique<BaseObject>(*a_tableList[navPositionID]);
					updateKeyboardNav = true;

					if (is_previously_selected) {
						selectionStorage.SetItemSelected(navPositionID - 1, false);
					}

					// itemPreview = std::make_unique<BaseObject>(*a_tableList[current_index + 1]);
				}

			}
		}

		if (ImGui::Shortcut(ImGuiKey_UpArrow | ImGuiMod_Shift, ImGuiInputFlags_RouteFromRootWindow | ImGuiInputFlags_Repeat) ||
			ImGui::Shortcut(ImGuiKey_UpArrow, ImGuiInputFlags_RouteFromRootWindow | ImGuiInputFlags_Repeat)) {

			void* it = NULL;
			ImGuiID id = 0;

			if (selectionStorage.GetNextSelectedItem(&it, &id)) {
				size_t current_index = navPositionID;  // Use first index when navigating up

				if (current_index > 0 && a_tableList[current_index - 1]) {
					navPositionID = a_tableList[current_index - 1]->m_tableID;

					if (!ImGui::IsKeyDown(ImGuiMod_Shift)) {
						selectionStorage.Clear();
					}
					
					bool is_previously_selected = selectionStorage.Contains(navPositionID);
					selectionStorage.SetItemSelected(navPositionID, true);
					itemPreview = std::make_unique<BaseObject>(*a_tableList[navPositionID]);
					updateKeyboardNav = true;

					if (is_previously_selected) {
						selectionStorage.SetItemSelected(navPositionID + 1, false);
					}

					// itemPreview = std::make_unique<BaseObject>(*a_tableList[current_index - 1]);
				}
			}
		}
	}

	void UITable::DrawKitItem(const std::unique_ptr<BaseObject>& a_item, const ImVec2& a_pos, bool a_selected)
	{
		const auto& draw_list = ImGui::GetWindowDrawList();
		const float font_size = ImGui::GetFontSize();

		// Setup box and bounding box for positioning and drawing.
		const ImVec2 box_min(a_pos.x, a_pos.y);
		const ImVec2 box_max(box_min.x + LayoutItemSize.x, box_min.y + LayoutItemSize.y);
		ImRect bb(box_min, box_max);

		// Background
		if (!a_selected) {
			if (showAltRowBG) {
				if (a_item->m_tableID % 2 == 0) {
					draw_list->AddRectFilled(bb.Min, bb.Max, colors.backgroundAlt);
				} else {
					draw_list->AddRectFilled(bb.Min, bb.Max, colors.background);
				}
			} else {
				draw_list->AddRectFilled(bb.Min, bb.Max, colors.background);
			}
		} else {
			draw_list->AddRectFilled(bb.Min, bb.Max, colors.selected);
		}

		if (a_item->IsDummy() && owner != Ownership::Cell) {
			draw_list->AddRectFilled(bb.Min, bb.Max, colors.error);
		}

		// Outline
		draw_list->AddRect(bb.Min, bb.Max, colors.outline, 0.0f, 0, 1.0f);

		// Type Color Pillar Identifier
		const float type_pillar_width = Style::Metrics().pillarWidth;
		draw_list->AddRectFilled(
			ImVec2(bb.Min.x + LayoutOuterPadding, bb.Min.y + LayoutOuterPadding),
			ImVec2(bb.Min.x + LayoutOuterPadding + type_pillar_width, bb.Max.y - LayoutOuterPadding),
			UICustom::GetFormTypeColor(a_item->GetFormType()));

		// Type Pillar tooltip
		if (IsMouseHoveringRect(
			ImVec2(bb.Min.x + LayoutOuterPadding, bb.Min.y + LayoutOuterPadding),
			ImVec2(bb.Min.x + LayoutOuterPadding + type_pillar_width, bb.Max.y - LayoutOuterPadding))) {
			UINotification::ShowObjectTooltip(a_item);
		}

		// Adjust and Setup bounding box and layout spacing.
		bb.Min.x += type_pillar_width * 2.0f;
		const float spacing = (LayoutColumnWidth / std::ssize(sortSystem->GetColumns())) - ImGui::GetFrameHeight();

		// Text alignment calculations
		const float center_align = bb.Min.y + ((LayoutOuterPadding + LayoutItemSize.y) / 2) - (font_size / 2.0f);

		// Column 0: Name
		if (LayoutColumnScreenX.size() > 0) {
			const std::string quantity_string = a_item->GetQuantity() > 1 ? std::format(" ({})", std::to_string(a_item->GetQuantity())) : "";
			const float quantity_offset = ImGui::CalcTextSize(quantity_string.c_str()).x;

			const std::string item_icon = showItemIcon ? (a_item->GetItemIcon() + " ").c_str() : "";
			const std::string raw_name = showEditorID ? a_item->GetEditorID() : a_item->GetName();
			const std::string name_string = TRUNCATE(item_icon + raw_name, spacing - quantity_offset) + quantity_string;

			ImU32 item_color = colors.text;
			if (a_item->IsEnchanted()) {
				item_color = colors.textEnchanted;
			} else if (a_item->IsUnique()) {
				item_color = colors.textUnique;
			} else if (a_item->IsEssential()) {
				item_color = colors.textEssential;
			}

			const ImVec2 name_pos = ImVec2(LayoutColumnScreenX[0], center_align);
			draw_list->AddText(name_pos, item_color, name_string.c_str());
		}

		// Exit early since dummy forms can't be acted upon.
		if (a_item->IsDummy()) { return; }

		// Column 1: Equip Button
		if (LayoutColumnScreenX.size() > 1) {
			const float equip_x = LayoutColumnScreenX[1] - ImGui::GetWindowPos().x + ImGui::GetScrollX();
			ImGui::SameLine();
			ImGui::SetCursorPosX(equip_x);

			const auto icon = a_item->GetEquipped() ? ICON_LC_CHECK : ICON_LC_X;
			const auto equip_size = ImVec2(spacing, LayoutItemSize.y);

			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0.0f, 0.0f));
			ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f));

			if (a_item->GetFormType() == RE::FormType::Armor || a_item->GetFormType() == RE::FormType::Weapon) {
				const auto equip_color = a_item->GetEquipped() ? ThemeConfig::GetColor("PRIMARY") : ThemeConfig::GetColor("PRIMARY", 0.5f);

				ImGui::PushStyleColor(ImGuiCol_Button, equip_color);
				const auto text = a_item->GetFormType() == RE::FormType::Armor ? a_item->GetArmorSlots()[0] : a_item->GetWeaponType();
				if (ImGui::Button((std::string(icon) + text).c_str(), equip_size)) {
					a_item->m_equipped = !a_item->m_equipped;
					SyncChangesToKit();
				}
				ImGui::PopStyleColor();
			} else {
				const auto form_type_text = RE::FormTypeToString(a_item->GetFormType());
				ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ThemeConfig::GetColor("PRIMARY"));
				ImGui::PushStyleColor(ImGuiCol_ButtonActive, ThemeConfig::GetColor("PRIMARY"));
				ImGui::Button(form_type_text.data(), equip_size);
				ImGui::PopStyleColor(2);
			}

			ImGui::PopStyleVar(2);
		}

		// Column 2: Quantity Input
		if (LayoutColumnScreenX.size() > 2) {
			const float quantity_x = LayoutColumnScreenX[2] - ImGui::GetWindowPos().x + ImGui::GetScrollX();
			ImGui::SameLine();
			ImGui::SetCursorPosX(quantity_x);
			ImGui::SetCursorPosY(ImGui::GetCursorPosY() + LayoutOuterPadding);
			ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - LayoutOuterPadding);
			if (ImGui::InputInt("##EquipCount", &a_item->m_quantity, 1, 10)) {
				SyncChangesToKit();
			}
		}
	}

	void UITable::DrawItem(const std::unique_ptr<BaseObject>& a_item, const ImVec2& a_pos, bool a_selected)
	{
		const auto& draw_list = ImGui::GetWindowDrawList();
		const float font_size = ImGui::GetFontSize();

		// Setup box and bounding box for positioning and drawing.
		const ImVec2 box_min(a_pos.x, a_pos.y);
		const ImVec2 box_max(box_min.x + LayoutItemSize.x, box_min.y + LayoutItemSize.y);
		ImRect bb(box_min, box_max);

		// Outline & Background colors
		ImU32 text_color = colors.text;

		// Background
		if (!a_selected) {
			if (showAltRowBG) {
				if (a_item->m_tableID % 2 == 0) {
					draw_list->AddRectFilled(bb.Min, bb.Max, colors.backgroundAlt);
				} else {
					draw_list->AddRectFilled(bb.Min, bb.Max, colors.background);
				}
			} else {
				draw_list->AddRectFilled(bb.Min, bb.Max, colors.background);
			}
		} else {
			draw_list->AddRectFilled(bb.Min, bb.Max, colors.selected);
		}

		// Invalid / Missing plugin indicator
		if (a_item->IsDummy() && owner != Ownership::Cell) {
			draw_list->AddRectFilled(bb.Min, bb.Max, colors.error);
		}

		// Outline
		draw_list->AddRect(bb.Min, bb.Max, colors.outline, 0.0f, 0, 1.0f);

		// Type Color Pillar Identifier
		const float type_pillar_width = Style::Metrics().pillarWidth;
		draw_list->AddRectFilled(
			ImVec2(bb.Min.x + LayoutOuterPadding, bb.Min.y + LayoutOuterPadding),
			ImVec2(bb.Min.x + LayoutOuterPadding + type_pillar_width, bb.Max.y - LayoutOuterPadding),
			UICustom::GetFormTypeColor(a_item->GetFormType()));

		// Type Pillar tooltip
		if (IsMouseHoveringRect(
			ImVec2(bb.Min.x + LayoutOuterPadding, bb.Min.y + LayoutOuterPadding),
			ImVec2(bb.Min.x + LayoutOuterPadding + type_pillar_width, bb.Max.y - LayoutOuterPadding))) {
			UINotification::ShowObjectTooltip(a_item);
		}

		// Adjust and Setup bounding box and layout spacing.
		bb.Min.x += type_pillar_width * 2.0f;
		float spacing = (LayoutColumnWidth / 3.0f) - (ImGui::GetFrameHeight() * 2.0f);

		// Text alignment calculations
		const float center_align = bb.Min.y + ((LayoutOuterPadding + LayoutItemSize.y) / 2) - (font_size / 2.0f);
		const float left_align = bb.Min.x + LayoutOuterPadding;
		const float right_align = bb.Max.x - LayoutOuterPadding - font_size;
		const ImVec2 center_left_align = ImVec2(left_align, center_align);
		const ImVec2 center_right_align = ImVec2(right_align, center_align);

		const bool has_reference = a_item->m_refID != 0;
		const bool is_favorited = a_item->m_refID == 0 ? UserData::IsFavorited(a_item->GetEditorID()) : UserData::IsFavorited(a_item->m_refID);
		const ImVec2 favorite_pos = ImVec2(center_right_align.x - ImGui::GetFontSize(), center_right_align.y);

		if (is_favorited) {
			draw_list->AddText(favorite_pos, colors.text, ICON_LC_HEART);

			if (IsMouseHoveringRect(favorite_pos, ImVec2(favorite_pos.x + font_size, favorite_pos.y + font_size))) {
				UINotification::ShowTooltip(Translate("ADD_TO_FAVORITES_TOOLTIP"), ICON_LC_HEART);
			}
		}

		if (has_reference) {
			const ImVec2 reference_pos = is_favorited ? favorite_pos - ImVec2(ImGui::GetFrameHeight(), 0.f) : favorite_pos;

			draw_list->AddText(reference_pos, colors.text, ICON_LC_ASTERISK);

			if (IsMouseHoveringRect(reference_pos, ImVec2(reference_pos.x + font_size, reference_pos.y + font_size))) {
				UINotification::ShowPropertyTooltip(PropertyType::kReferenceID);
			}
		}

		// TODO: Somehow the reference ids on start vary heavily from after loading in?

		const auto columns = sortSystem->GetColumns();
		for (auto entry : columns) {
			if (entry.column < 0 || entry.column >= static_cast<int>(LayoutColumnScreenX.size()))
				continue;

			auto tooltip = entry.property.GetPropertyType();
			const auto value = a_item->GetPropertyValueWithIcon(entry.property.GetPropertyType());
			const auto text = TRUNCATE(value, spacing);
			const auto pos = ImVec2(LayoutColumnScreenX[entry.column], center_align);

			if (entry.property == PropertyType::kEditorID || entry.property == PropertyType::kName) {
				if (a_item->IsEnchanted()) {
					text_color = colors.textEnchanted;
					tooltip = PropertyType::kEnchanted;
				} else if (a_item->IsUnique()) {
					text_color = colors.textUnique;
					tooltip = PropertyType::kUnique;
				} else if (a_item->IsEssential()) {
					text_color = colors.textEssential;
					tooltip = PropertyType::kEssential;
				} else if (a_item->IsUnique() && a_item->IsEssential()) {
					text_color = colors.textUniqueEssential;
					tooltip = PropertyType::kUniqueEssential;
				}
			}

			draw_list->AddText(pos, text_color, text.c_str());

			const auto text_size = ImGui::CalcTextSize(text.c_str());
			if (ImGui::IsMouseHoveringRect(pos, pos + text_size)) {
				UINotification::ShowPropertyTooltip(tooltip);
			}
		}
	}

	void UITable::DrawDragDropPayload(const std::string& a_icon)
	{
		const auto payload = ImGui::GetDragDropPayload();
		if (payload && payload->IsDataType(std::to_string(tableID).c_str())) {
			const auto payloadCount = payload->DataSize / (int)sizeof(ImGuiID);
			const auto& overlay = ThemeConfig::GetWidgetStyle().dragOverlay;

			const float mult = ImGui::GetFrameHeightWithSpacing();
			const ImVec2 size_min = ImVec2(mult * overlay.payloadMinFrameH, mult * overlay.payloadMinFrameH);
			const ImVec2 size_max = ImVec2(mult * overlay.payloadMaxFrameH, mult * overlay.payloadMaxFrameH);

			ImGui::SetNextWindowSizeConstraints(size_min, size_max);
			if (ImGui::BeginTooltip()) {
				const ImVec2 start_pos = ImGui::GetCursorScreenPos();
				ImGui::PushFontBold(overlay.payloadCountFont);
				ImGui::SetCursorPosX(UICustom::GetCenterTextPosX(std::to_string(payloadCount).c_str()));
				ImGui::SetCursorPosY((ImGui::GetContentRegionAvail().y / 2.0f) - (overlay.payloadCountFont / 1.5f));
				ImGui::Text("%d", payloadCount);
				ImGui::PopFont();
				ImGui::SetCursorPosX(UICustom::GetCenterTextPosX(Translate("SELECTED")));
				ImGui::Text("%s", Translate("SELECTED"));

				const auto& DrawList = ImGui::GetWindowDrawList();
				const float icon_x = ImGui::GetWindowWidth() - ImGui::GetFrameHeight() * 1.5f;
				const float icon_y = -overlay.payloadIconSize * (5.0f / 24.0f);

				ImGui::PushFont(NULL, overlay.payloadIconSize);
				DrawList->AddText(start_pos + ImVec2(icon_x, icon_y), colors.text, a_icon.c_str());
				ImGui::PopFont();

				ImGui::EndTooltip();
			}
		}
	}

	void UITable::DrawTableSettingsPopup()
	{
		if (UICustom::Popup_MenuHeader(Translate("SETTINGS"))) {
			ImGui::CloseCurrentPopup();
		}
		
		const float width = ImGui::GetContentRegionAvail().x;
		ImGui::PushStyleVar(ImGuiStyleVar_SeparatorTextAlign, ImVec2(0.5f, 0.5f));

		float font_size = styleFontSize;
		ImGui::SeparatorText(Translate("TABLE_SETTINGS_FONT_SIZE"));
		ImGui::SetNextItemWidth(width);
		if (ImGui::SliderFloat("##Slider::FontSize", &font_size, 0.0f, 48.0f, "%.f")) {
			UserData::Set<float>("Modex::Table::FontSize", font_size);
			styleFontSize = font_size;
		}

		float item_height =  styleHeight; 
		ImGui::SeparatorText(Translate("TABLE_SETTINGS_HEIGHT"));
		ImGui::SetNextItemWidth(width);
		if (ImGui::SliderFloat("##Slider::ItemHeight", &item_height, -20.0f, 20.0f, "%.2f")) {
			UserData::Set<float>("Modex::Table::ItemHeight", item_height);
			styleHeight = item_height;
		}

		float item_width = styleWidth;
		ImGui::SeparatorText(Translate("TABLE_SETTINGS_WIDTH"));
		ImGui::SetNextItemWidth(width);
		if (ImGui::SliderFloat("##Slider::ItemWidth", &item_width, -300.0f, 300.0f, "%.2f")) {
			UserData::Set<float>("Modex::Table::ItemWidth", item_width);
			styleWidth = item_width;
		}

		float item_spacing = styleSpacing;
		ImGui::SeparatorText(Translate("TABLE_SETTINGS_SPACING"));
		ImGui::SetNextItemWidth(width);
		if (ImGui::SliderFloat("##Slider::ItemSpacing", &item_spacing, -20.0f, 20.0f, "%.2f")) {
			UserData::Set<float>("Modex::Table::ItemSpacing", item_spacing);
			styleSpacing = item_spacing;
		}

		ImGui::SeparatorText(Translate("TABLE_SETTINGS_SHOW_BG"));
		if (UICustom::ToggleButton("##Button::AltRowBG", showAltRowBG, width)) {
			UserData::Set<bool>("Modex::Table::ShowAltRowBG", showAltRowBG);
		}

		ImGui::SeparatorText(Translate("TABLE_SETTINGS_SHOW_ICON"));
		if (UICustom::ToggleButton("##Button::ItemIcon", showItemIcon, width)) {
			UserData::Set<bool>("Modex::Table::ShowItemIcon", showItemIcon);
		}

		ImGui::SeparatorText(Translate("TABLE_SETTINGS_QUICK_SEARCH"));
		if (UICustom::ToggleButton("##Button::QuickSearch", useQuickSearch, width)) {
			UserData::Set<bool>("Modex::Table::UseQuickSearch", useQuickSearch);
		}

		ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal);
		const bool reset = ImGui::Selectable("Reset");

		if (reset) {
			UserData::Set<float>("Modex::Table""FontSize", 0.0f); 
			UserData::Set<float>("Modex::Table::ItemHeight", 0.0f);
			UserData::Set<float>("Modex::Table::ItemWidth", 0.0f);
			UserData::Set<float>("Modex::Table::ItemSpacing", 0.0f);
			UserData::Set<bool>("Modex::Table::ShowAltRowBG", true);
			UserData::Set<bool>("Modex::Table::ShowItemIcon", true);
			UserData::Set<bool>("Modex::Table::UseQuickSearch", false);
			sortSystem->Reset();
			SortListBySpecs();
			UpdateImGuiTableIDs();
		}

		ImGui::PopStyleVar();
	}

	// yuck
	const char* GetTableLocale(Ownership a_owner)
	{
		switch (a_owner) {
			case Ownership::Actor: return "TABLE_ACTOR";
			case Ownership::All: return "TABLE_ALL";
			case Ownership::Cell: return "TABLE_TELEPORT";
			case Ownership::Item: return "TABLE_ITEM";
			case Ownership::Kit: return "TABLE_KIT";
			case Ownership::Object: return "TABLE_OBJECT";
			case Ownership::Outfit: return "TABLE_OUTFIT";
			case Ownership::Spell: return "TABLE_SPELL";
			case Ownership::None: return "TABLE_NONE";
		}

		return "MISSING_KEY";
	}

	void UITable::DrawStatusBar()
	{
		static constexpr const char* valid_icon = ICON_LC_ASTERISK;
		static constexpr const char* invalid_icon = ICON_LC_X;
		static constexpr const char* warning_icon = ICON_LC_TRIANGLE_ALERT;
		std::string table;
		std::string status;

		bool valid_target = tableTargetRef != nullptr;
		bool valid_type = valid_target && IsValidTargetReference(tableTargetRef);
		bool warning = !valid_target || !valid_type;
		
		if (!valid_target) {
			const auto lookup = useSharedTarget ? "LastSharedTargetFormID" : (data_id + "::LastTargetRef");
			const auto stored_ref = UserData::Get<RE::FormID>(lookup, 0);
			if (stored_ref != 0) {
				status = std::format("({:08X}) - {}", stored_ref, Translate("ERROR_LAST_REFERENCE"));
			} else {
				status = Translate("ERROR_MISSING_REFERENCE");
			}
		} else {
			const std::string name = tableTargetRef->GetName();
			const std::string editorid = po3_GetEditorID(tableTargetRef->GetBaseObject()->formID);

			if (name.empty()) {
				status = std::format("({:08X}) - '{}'", tableTargetRef->GetFormID(), editorid);
			} else {
				status = std::format("({:08X}) - '{}'", tableTargetRef->GetFormID(), name);
			}

			if (!valid_type) {
				status += " - ";
				if (tableTargetRef->IsHandleValid()) {
					status += Translate("ERROR_INVALID_REFERENCE");
				} else {
					status += Translate("ERROR_UNLOADED_REFERENCE");
				}
			} else if (HasFlag(ModexTableFlag_Kit)) {
				if (selectedKitPtr == nullptr || (selectedKitPtr && selectedKitPtr->m_key.empty())) {
					status = Translate("ERROR_NO_KIT_SELECTED");
					warning = true;
				} else {
					status = selectedKitPtr->GetNameTail();
				}
			}
		}

		const auto status_icon = !valid_target ? invalid_icon : (!valid_type ? warning_icon : valid_icon);

		const auto status_color = warning ? "ERROR" : 
			tableTargetRef->IsPlayerRef() ? "SUCCESS" : "WARN";

		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f));
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(ImGui::GetFontSize(), 3.0f));
		ImGui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign, ImVec2(0.5f, 0.5f));
		ImGui::PushStyleColor(ImGuiCol_Button, ThemeConfig::GetColor("NONE"));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ThemeConfig::GetColor(status_color));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ThemeConfig::GetColor("NONE"));
		
		ImGui::SameLine();

		if (valid_target && valid_type && !warning) {
			ImGui::PushFontBold(ImGui::GetFontSize());
		}

		const auto user_height = ImGui::GetFrameHeight() * ThemeConfig::GetWidgetStyle().table.statusBarHeightScale;
		const auto user_width = ImGui::GetContentRegionAvail().x;

		ImGui::SetNextItemAllowOverlap();
		bool clicked = ImGui::Button(TRUNCATE(status, user_width).c_str(), ImVec2(user_width, user_height));
		bool user_shift_clicked = clicked && ImGui::GetIO().KeyShift;
		bool user_ctrl_clicked  = clicked && ImGui::GetIO().KeyCtrl;
		bool user_console = ImGui::IsItemHovered() && ImGui::IsKeyPressed(ImGuiKey_C, false);
		bool user_default = ImGui::IsItemHovered() && ImGui::IsKeyPressed(ImGuiKey_T, false);

		ImGui::PopStyleVar(3);
		ImGui::PopStyleColor(3);

		if (ImGui::IsItemClicked(ImGuiMouseButton_Right)) {
			ImGui::OpenPopup("STATUS_BAR_CONTEXT_MENU");
		}

		if (valid_target && valid_type && !warning) {
			ImGui::PopFont();
		}

		ImGui::SameLine();

		const ImVec2 icon_position = ImVec2(
			(ImGui::GetCursorScreenPos().x - user_width),
			(ImGui::GetCursorScreenPos().y + 5.0f));

		const ImVec2 link_position = ImVec2(
			(ImGui::GetCursorScreenPos().x - ImGui::GetFrameHeight() * 1.50f),
			(ImGui::GetCursorScreenPos().y + 5.0f));
		
		const std::string item_count = std::format("[{}]", std::ssize(tableList));
		const ImVec2 count_position = ImVec2(
			(link_position.x - ImGui::CalcTextSize(item_count.c_str()).x - (ImGui::GetFrameHeight() / 2.0f)),
			(link_position.y));

		ImGui::PushFont(NULL, ImGui::GetFontSize() + 2.0f);
		const auto& DrawList = ImGui::GetWindowDrawList();
		DrawList->AddText(icon_position, colors.text, status_icon);
		DrawList->AddText(link_position, colors.text, useSharedTarget ? ICON_LC_LINK : ICON_LC_UNLINK);
		DrawList->AddText(count_position, colors.textDisabled, item_count.c_str());
		ImGui::PopFont();

		if (ImGui::IsMouseHoveringRect(link_position, link_position + ImVec2(ImGui::GetFrameHeight(), ImGui::GetFrameHeight()))) {
			UICustom::FancyTooltip("STATUS_BAR_LINK");
		} else if (ImGui::IsMouseHoveringRect(count_position, count_position + ImGui::CalcTextSize(item_count.c_str()))) {
			UICustom::FancyTooltip("STATUS_BAR_COUNT");
		} else if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort | ImGuiHoveredFlags_NoSharedDelay)) {
			UICustom::FancyTooltip("STATUS_BAR_TOOLTIP");
		}

		if (user_shift_clicked) {
			auto* reference = UIModule::GetTargetReference();
			this->SetTargetByReference(reference);
		}
			
		if (user_ctrl_clicked && tableTargetRef) {
			auto formID = tableTargetRef->GetFormID();
			std::string hex = std::format("{:08X}", formID);
			ImGui::SetClipboardText(hex.c_str());	
		}

		if (user_default) {
			this->SetTargetByReference(Commands::GetPlayerReference());
		}

		if (user_console) {
			this->SetTargetByReference(Commands::GetConsoleReference());
		}

		if (clicked && !user_shift_clicked && !user_ctrl_clicked) {
			UIManager::GetSingleton()->ShowReferenceLookup(
				Translate("STATUS_BAR_TITLE"),
				Translate("STATUS_BAR_DESC"),
				[this](RE::FormID formID) {
					if (const auto form = RE::TESForm::LookupByID(formID); form != nullptr) {
						if (const auto reference = form->As<RE::TESObjectREFR>(); reference != nullptr) {
							this->SetTargetByReference(reference);
							return;
						}
					}

					UIManager::GetSingleton()->ShowWarning(
						Translate("INVALID_REFERENCE_POPUP_TITLE") + std::format("{:08X}", formID),
						Translate("INVALID_REFERENCE_POPUP_DESC")
					);
				}
			);
		}

		ImGui::NewLine();
		ImGui::SetCursorPosY(ImGui::GetCursorPosY() - ImGui::GetStyle().ItemSpacing.y);
		ImGui::PushStyleColor(ImGuiCol_Separator, ThemeConfig::GetColor(status_color));
		ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal);
		ImGui::PopStyleColor();

		// Draw last to avoid unwanted styling
		if (ImGui::BeginPopup("STATUS_BAR_CONTEXT_MENU")) {
			if (ImGui::MenuItem(Translate("GOTO_NPC_REFERENCE"))) {
				if (IsValidTargetReference()) {
					Commands::TeleportPlayerToREFR(owner, tableTargetRef);
				}
			}

			if (ImGui::MenuItem(Translate("BRING_NPC_REFERENCE"))) {
				if (IsValidTargetReference()) {
					Commands::TeleportREFRToPlayer(owner, tableTargetRef);
				}
			}

			ImGui::EndPopup();
		}
	}

	void UITable::DrawFormFilterTree()
	{
		ImGui::PushID("##Modex::Table::CategoryTabs");
	
		if (const auto rootNode = this->filterSystem->FindNode("__root__")) {
			ASSERT_MSG(rootNode == nullptr, "Failed to find the \"FilterProperty\" root node in Table JSON configuration.");
			this->filterSystem->RenderNodeAndChildren(rootNode, ImGui::GetContentRegionAvail().x);
		}

		ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal);

		ImGui::PopID();
	}

	// a_valueWidth is to determine avaiable column space before EOL.
	void UITable::CustomSortColumn(const char* a_id, float a_spacing, bool a_sorted, int a_column)
	{
		ImGui::PushID(a_id);
		const static ImVec4 text_col = ThemeConfig::GetColor("TEXT");

		const auto combo_flags = HasFlag(ModexTableFlag_Kit) ? ImGuiComboFlags_NoArrowButton : ImGuiComboFlags_HeightLargest;
		ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0, 0, 0, 0));
		ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImGui::GetStyle().Colors[ImGuiCol_ButtonHovered]);
		ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0, 0, 0, 0));
		ImGui::PushStyleColor(ImGuiCol_Text, text_col);
		
		// const float sortby_size = ImGui::GetContentRegionAvail().x;
		const FilterProperty current_key = sortSystem->GetColumnPropertyType(a_column);
		const auto preview_text = current_key == PropertyType::kNone ? Translate("SET") : current_key.ToString();
		
		if (a_sorted) ImGui::PushFontBold();
		ImGui::SetNextItemWidth(a_spacing);
		if (ImGui::BeginCombo("##UITable::Sort::Combo", a_sorted ? (std::string(preview_text) + " * ").c_str() : preview_text.c_str(), combo_flags)) {
			const FilterPropertyList available_keys = sortSystem->GetAvailableFilters();

			ImGui::PushStyleColor(ImGuiCol_Text, colors.text);
			ImGui::PushFontRegular();
			for (auto& key : available_keys) {
				const bool is_selected = (key == current_key);
				const std::string key_text = key.ToString();

				if (key.GetPropertyType() == PropertyType::kImGuiSeparator) {
					ImGui::Separator();
					continue;
				}

				if (ImGui::Selectable(key_text.c_str(), is_selected)) {
					sortSystem->SetSortData({a_column, key});
				}
				
				if (is_selected) {
					ImGui::SetItemDefaultFocus();
				}
			}
			ImGui::PopFont();
			ImGui::PopStyleColor();
			ImGui::EndCombo();
		}
		if (a_sorted) ImGui::PopFont();

		ImGui::PopStyleColor(4);
		ImGui::PopID();
	}

	void UITable::DrawHeader()
	{

		ImGui::PushID("##Modex::Table::Header");
		ImGui::PushStyleColor(ImGuiCol_Separator, ThemeConfig::GetColor("PRIMARY"));

		std::string sort_icon = sortSystem->GetSortAscending()  == true ? ICON_LC_CHEVRON_DOWN : ICON_LC_CHEVRON_UP;
		constexpr float pillar_offset = 5.0f; // pillar offset.
		const float spacing = (LayoutColumnWidth / std::ssize(sortSystem->GetColumns())) - ImGui::GetFrameHeight();
		static const ImVec4 text_col = ThemeConfig::GetColor("TEXT_HEADER");

		auto headerSortButton = [&](const char* a_icon, int a_column) {
			ImGui::AlignTextToFramePadding();
			ImGui::TextColored(text_col, "%s", a_icon);

			if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
				sortSystem->SetSortedColumn(a_column);
				sortSystem->ToggleAscending();
				SortListBySpecs();
				UpdateImGuiTableIDs();
			}

			ImGui::SameLine();
		};

		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + pillar_offset + ImGui::GetStyle().WindowPadding.x);
		const auto columns = sortSystem->GetColumns();

		LayoutColumnScreenX.resize(columns.size());

		for (auto entry : columns) {
			bool is_sorted = sortSystem->IsColumnSorted(entry.column);
			const char* icon = is_sorted ? sort_icon.c_str() : ICON_LC_CHEVRONS_UP_DOWN;

			LayoutColumnScreenX[entry.column] = ImGui::GetCursorScreenPos().x;

			headerSortButton(icon, entry.column);
			const auto settings_padding = ImGui::GetContentRegionAvail().x - ImGui::GetFrameHeightWithSpacing();
			const auto size = entry.column == std::ssize(columns) - 1 ? settings_padding : spacing;
			if (HasFlag(ModexTableFlag_Kit)) ImGui::BeginDisabled();
			CustomSortColumn(("Column_" + std::to_string(entry.column)).c_str(), size, is_sorted, entry.column);
			if (HasFlag(ModexTableFlag_Kit)) ImGui::EndDisabled();
			ImGui::SameLine();
		}

		// Settings Button
		ImGui::SameLine();

		ImGui::AlignTextToFramePadding();
		ImGui::Text("%s", ICON_LC_SETTINGS);
		if (ImGui::IsItemClicked()) {
			ImGui::OpenPopup("TABLE_SETTINGS_POPUP");
		}

		ImGui::SetNextWindowSize(ImVec2(ImGui::GetWindowSize().x / 4.0f, 0.0f));
		if (ImGui::BeginPopup("TABLE_SETTINGS_POPUP")) {
			DrawTableSettingsPopup();
			ImGui::EndPopup();
		}
		
		ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal);
		ImGui::PopStyleColor();
		ImGui::PopID();
	}
	
	static inline bool test_selection = false;
	static inline bool test_filters = false;

	// TODO: Add instructions to empty table background panel

	void UITable::Draw(const TableList& _tableList)
	{
		if (HasFlag(ModexTableFlag_Kit)) {
			FlushPendingKitChanges();
		}

		if (HasFlag(ModexTableFlag_Dirty)) {
			Refresh();
		}

		UpdateLayout();

		if (!HasFlag(ModexTableFlag_APIMode)) {
			DrawStatusBar();
		}

		if (HasFlag(ModexTableFlag_EnableSearch)) {
			DrawSearchBar();
		}

		if (HasFlag(ModexTableFlag_EnableDebugToolkit)) {
			DrawDebugToolkit();
		}

		if (HasFlag(ModexTableFlag_EnableFilterTree)) {
			DrawFormFilterTree();
		}

		if (HasFlag(ModexTableFlag_EnableHeader)) {
			DrawHeader();
		}

		if (ImGui::BeginChild("##UITable::Draw", ImVec2(0.0f, 0.0f), 0, ImGuiWindowFlags_NoMove)) {
			if (test_selection) {
				Test_TableSelection();
				const float target_scroll = itemPreview ? this->itemPreview->m_tableID * LayoutItemStep.y : 0;
				const float current_scroll = ImGui::GetScrollY();
				const float lerp_factor = 0.15f; // Adjust for faster/slower scroll (0.0-1.0)
				const float smooth_scroll = current_scroll + (target_scroll - current_scroll) * lerp_factor;
				ImGui::SetScrollY(smooth_scroll);
			}

			if (test_filters) {
				Test_TableFilters();
			}

			// Calculate and store start position of table.
			ImVec2 start_pos = ImGui::GetCursorScreenPos();
			start_pos.y += 2.0f;

			// Build table meta data and structures
			const int COLUMN_COUNT = LayoutColumnCount; // 1
			const int ITEMS_COUNT = static_cast<int>(_tableList.size());

			ImGuiListClipper clipper;
			ImGuiMultiSelectIO* ms_io = ImGui::BeginMultiSelect(MULTI_SELECT_FLAGS, selectionStorage.Size, ITEMS_COUNT);
			selectionStorage.UserData = (void*)&_tableList;
			selectionStorage.AdapterIndexToStorageId = [](ImGuiSelectionBasicStorage* self, int idx) {
				TableList* a_items = (TableList*)self->UserData;
				return (*a_items)[idx]->m_tableID;  // Index -> TableID
			};
			selectionStorage.ApplyRequests(ms_io);
			
			// Start clipper and iterate through table's item list.
			clipper.Begin(ITEMS_COUNT, LayoutItemStep.y);
			if (ms_io->RangeSrcItem != -1) {
				clipper.IncludeItemByIndex((int)ms_io->RangeSrcItem);  // Ensure RangeSrc item is not clipped.
			}

			clipper.IncludeItemsByIndex(navPositionID - 1, navPositionID + 1);
			HandleKeyboardNavigation(_tableList);

			ImGui::PushFont(NULL, styleFontSize);

			while (clipper.Step()) {
				const int item_start = clipper.DisplayStart;
				const int item_end = clipper.DisplayEnd;

				for (int line_idx = item_start; line_idx < item_end; line_idx++) {
					const int item_min_idx_for_current_line = line_idx * COLUMN_COUNT;
					const int item_max_idx_for_current_line = (std::min)((line_idx + 1) * COLUMN_COUNT, ITEMS_COUNT);

					for (int item_idx = item_min_idx_for_current_line; item_idx < item_max_idx_for_current_line; ++item_idx) {
						if (item_idx >= static_cast<int>(_tableList.size())) {
							continue;
						}

						if (!_tableList[item_idx]) {
							continue;
						}

						auto& item_data = _tableList.at(item_idx);
						ImGui::PushID((int)item_data->m_tableID);

						// position item at start
						ImVec2 pos = ImVec2(start_pos.x, start_pos.y + line_idx * LayoutItemStep.y);
						ImGui::SetCursorScreenPos(pos);

						// set next item selection user data
						bool is_item_selected = selectionStorage.Contains(item_data->m_tableID);
						bool is_item_visible = ImGui::IsRectVisible(LayoutItemSize);

						ImGui::SetNextItemSelectionUserData(item_idx);
						ImGui::PushItemFlag(ImGuiItemFlags_NoNav, true);
						ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.f, 0.f));
						ImGui::PushStyleColor(ImGuiCol_Header, colors.hover);
						ImGui::PushStyleColor(ImGuiCol_HeaderHovered,  colors.hover);

						if (this->HasFlag(ModexTableFlag_Kit)) {
							ImGui::Selectable("", is_item_selected, ImGuiSelectableFlags_None, ImVec2(LayoutItemSize.x / std::ssize(sortSystem->GetColumns()) + ImGui::GetFrameHeight(), LayoutItemSize.y));
						} else {
							ImGui::Selectable("", is_item_selected, ImGuiSelectableFlags_None, LayoutItemSize);
						}
						ImGui::PopStyleColor(2);
						ImGui::PopItemFlag();
						ImGui::PopStyleVar();

						if (ImGui::IsItemHovered()) {
							HandleItemHoverPreview(item_data);
						}

						if (item_data != itemPreview) {
							if (!ImGui::IsAnyItemHovered()) {
								if (GetSelectionCount() > 0) {
									itemPreview = std::make_unique<BaseObject>(*GetSelection()[0]);
								} else {
									if (!ImGui::IsPopupOpen("", ImGuiPopupFlags_AnyPopup | ImGuiPopupFlags_AnyPopupLevel)) {
										itemPreview = nullptr;
									}
								}
							}
						}

						if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
							navPositionID = item_data->m_tableID;
						}

						if (updateKeyboardNav) {
							if (item_data->m_tableID == navPositionID) {
								const ImGuiID top_id = item_start;
								const ImGuiID bot_id = item_end;

								if (navPositionID <= top_id) 
									ImGui::SetScrollY((item_idx + 1) * LayoutItemStep.y - ImGui::GetWindowHeight());
								else if (navPositionID >= bot_id - 1) 
									ImGui::SetScrollY(item_idx * LayoutItemStep.y);
								
								updateKeyboardNav = false;
							}
						}

						if (ImGui::IsItemHovered()) {
							HandleLeftClickBehavior(item_data);
						}

						HandleRightClickBehavior(item_data);

						// Drag and drop
						if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceNoPreviewTooltip)) {
							// Create payload with full selection OR single unselected item.
							// (the later is only possible when using ImGuiMultiSelectFlags_SelectOnClickRelease)
							if (ImGui::GetDragDropPayload() == NULL) {
								ImVector<ImGuiID> payload_items;
								void* it = NULL;
								ImGuiID id = 0;
								if (!is_item_selected) {
									payload_items.push_back(item_data->m_tableID);
								} else {
									if (id < _tableList.size() && id >= 0) {
										while (selectionStorage.GetNextSelectedItem(&it, &id)) {
											payload_items.push_back(id);
										}
									}
								}
								// ImGui::SetDragDropPayload(GetDragDropHandleText(dragDropHandle), payload_items.Data, (size_t)payload_items.size_in_bytes());
								ImGui::SetDragDropPayload(std::to_string(tableID).c_str(), payload_items.Data, (size_t)payload_items.size_in_bytes());
							}

							ImGui::EndDragDropSource();
						}

						// if the item is visible, offload drawing item data to separate function
						if (is_item_visible && item_data != nullptr) {
							if (this->HasFlag(ModexTableFlag_Kit)) {
								DrawKitItem(item_data, pos, is_item_selected);
							} else {
								DrawItem(item_data, pos, is_item_selected);
							}
						}

						ImGui::PopID();
					}
				}
			}
			clipper.End();

			ImGui::PopFont();

			ms_io = ImGui::EndMultiSelect();
			selectionStorage.ApplyRequests(ms_io);
		}

		ImGui::EndChild();

		HandleDragDropBehavior();
	}

	void UITable::DrawDebugToolkit()
	{
		ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal);
		if (UICustom::Settings_ToggleButton("Autotest Table Selection", test_selection)) {
			// test_selection = !test_selection;
		}

		if (UICustom::Settings_ToggleButton("Autotest Table Filters", test_filters)) {
			// test_filters = !test_filters;
		}
	}

	// Use ImGuiIO delta to incrementally select table filter nodes one by one until completion
	void UITable::Test_TableFilters()
	{
		static float accumulator = 0.0f;
		static size_t current_index = 0;

		ImGuiIO& io = ImGui::GetIO();
		accumulator += io.DeltaTime;

		const std::list<FilterNode*>& allNodes = this->filterSystem->GetAllNodes();
		const size_t nodesSize = allNodes.size();

		if (accumulator >= 0.50f) {
			accumulator = 0.0f;

			// Clear previous selection
			this->filterSystem->ClearActiveNodes();

			for (auto it = allNodes.begin(); it != allNodes.end(); ++it) {
				if (std::distance(allNodes.begin(), it) == static_cast<std::ptrdiff_t>(current_index)) {
					FilterNode* node = *it;
					if (node) {
						this->filterSystem->ActivateNodeByID(node->id, true);
						this->Refresh();
					}
					break;
				}
			}

			// Move to the next index
			current_index = (current_index + 1) % nodesSize;

			// stop when we finish
			if (current_index == nodesSize) {
				test_filters = false;
			}
		}
	}

	// Use ImGuiIO delta to shift selection down by one every half second:
	void UITable::Test_TableSelection()
	{
		static float accumulator = 0.0f;
		static size_t current_index = 0;

		ImGuiIO& io = ImGui::GetIO();
		accumulator += io.DeltaTime;

		const TableList* selectionList = (TableList*)selectionStorage.UserData;
		const size_t tableSize = selectionList->size();

		if (tableSize == 0) {
			return;
		}

		if (accumulator >= 0.015f) {
			accumulator = 0.0f;

			// Clear previous selection
			selectionStorage.Clear();

			// Select the current index
			if (current_index < tableSize) {
				auto& item = (*selectionList)[current_index];
				if (item) {
					selectionStorage.SetItemSelected(item->m_tableID, true);
					this->itemPreview = std::make_unique<BaseObject>(*item);
				}
			}

			// Move to the next index
			current_index = (current_index + 1) % tableSize;

			// stop when we finish
			if (current_index == tableSize) {
				test_selection = false;
			}
		}
	}
}
