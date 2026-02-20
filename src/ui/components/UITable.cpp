#include "UITable.h" 

#include "SKSE/API.h"
#include "core/Commands.h"
#include "data/BaseObject.h"
#include "imgui.h"
#include "localization/FontManager.h"
#include "localization/Locale.h"
#include "ui/core/UIManager.h"
#include "ui/components/UICustom.h"
#include "ui/components/ItemPreview.h"
#include "ui/components/UINotification.h"
#include "config/EquipmentConfig.h"
#include "config/BlacklistConfig.h"
#include "config/UserConfig.h"
#include "config/UserData.h"
#include "config/ThemeConfig.h"

#include "ui/components/UIModule.h"

#include "pch.h"

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

	UITable::UITable(const std::string& a_dataID, bool a_shared, uint8_t a_type, uint32_t a_flags)
		: data_id(a_dataID)
		, pluginType(a_type)
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
	}

	void UITable::LoadSystemState()
	{
		filterSystem->LoadState(data_id + "::FilterState");
		sortSystem->LoadState(data_id + "::SortState");
		searchSystem->LoadState(data_id + "::SearchState");
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

		// Inventory has in-method generator.
		if (HasFlag(ModexTableFlag_Inventory)) {
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

	void UITable::UpdateActiveInventoryTables()
	{
		if (this->HasFlag(ModexTableFlag_Inventory) || this->GetDragDropHandle() == DragDropHandle::Inventory) {
			this->Refresh();
			return;
		}

		for (auto& pair : dragDropSourceList) {
			const auto handle = pair.first;
			const auto ptr = pair.second;

			if (ptr->HasFlag(ModexTableFlag_Inventory) || handle == DragDropHandle::Inventory) {
				ptr->Refresh();
			}
		}
	}

	void UITable::AddKitToTargetInventory(const Kit& a_kit)
    {
		if (tableList.empty())
			return;

		if (a_kit.m_items.empty())
			return;

		if (!tableTargetRef)
			return;

		for (auto& kitItem : a_kit.m_items) {
			if (kitItem->m_equipped) {
				Commands::AddAndEquipItemToInventory(tableTargetRef, kitItem->m_editorid);
			} else {
				Commands::AddItemToRefInventory(tableTargetRef, kitItem->m_editorid, static_cast<std::uint32_t>(kitItem->m_amount));
			}
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
				Commands::RemoveItemFromInventory(tableTargetRef, itemPreview->GetEditorID(), 1);
				UserData::AddRecent(itemPreview);
			}
		} 
		else {
			void* it = NULL;
			ImGuiID id = 0;

			while (selectionStorage.GetNextSelectedItem(&it, &id)) {
				if (id < std::ssize(tableList) && id >= 0) {
					const auto& item = tableList[id];
					if (!item->IsDummy()) {
						Commands::RemoveItemFromInventory(tableTargetRef, item->GetEditorID(), 1);
						UserData::AddRecent(item);
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
				Commands::AddItemToRefInventory(tableTargetRef, itemPreview->GetEditorID(), a_count);
				UserData::AddRecent(itemPreview);
			}
		} 
		else {
			void* it = NULL;
			ImGuiID id = 0;

			while (selectionStorage.GetNextSelectedItem(&it, &id)) {
				if (id < std::ssize(tableList) && id >= 0) {
					const auto& item = tableList[id];
					if (item && !item->IsDummy() && item->GetTESForm()->IsInventoryObject()) {
						Commands::AddItemToRefInventory(tableTargetRef, item->GetEditorID(), a_count);
						UserData::AddRecent(item);
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
				Commands::AddAndEquipItemToInventory(tableTargetRef, itemPreview->GetEditorID());
				UserData::AddRecent(itemPreview);
			}
		}
		else {
			void* it = NULL;
			ImGuiID id = 0;

			while (selectionStorage.GetNextSelectedItem(&it, &id)) {
				if (id < std::ssize(tableList) && id >= 0) {
					const auto& item = tableList[id];
					if (item && !itemPreview->IsDummy() && (item->IsArmor() || item->IsWeapon())) {
						Commands::AddAndEquipItemToInventory(tableTargetRef, item->GetEditorID());
						UserData::AddRecent(item);
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
				Commands::PlaceAtMe(itemPreview->GetEditorID(), a_count);
				UserData::AddRecent(itemPreview);
			}
		}
		else {
			void* it = NULL;
			ImGuiID id = 0;

			while (selectionStorage.GetNextSelectedItem(&it, &id)) {
				if (id < std::ssize(tableList) && id >= 0) {
					const auto& item = tableList[id];
					if (item && !item->IsDummy()) {
						Commands::PlaceAtMe(item->GetEditorID(), a_count);
						UserData::AddRecent(item);
					}
				}
			}
		}
	}

	bool UITable::SelectionContainsOnlyReferences()
	{
		if (GetSelectionCount() == 0) {
			if (itemPreview && !itemPreview->IsDummy()) {
				return itemPreview->GetRefID() != 0;
			}
		}
		else {
			void* it = NULL;
			ImGuiID id = 0;

			while (selectionStorage.GetNextSelectedItem(&it, &id)) {
				if (id < std::ssize(tableList) && id >= 0) {
					const auto& item = tableList[id];
					if (item->IsDummy()) return false;
					if (item->GetRefID() == 0) return false;
				}
			}
		}

		return false;
	}

	void UITable::ExecuteCommandOnSelection(const std::function<void(const std::unique_ptr<BaseObject>&)>& a_command)
	{
		if (tableList.empty())
			return;

		if (GetSelectionCount() == 0) {
			if (itemPreview && !itemPreview->IsDummy()) {
				a_command(itemPreview);
				UserData::AddRecent(itemPreview);
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
						UserData::AddRecent(item);
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
				Commands::TeleportREFRToPlayer(GetSelectedReference());
				UserData::AddRecent(itemPreview);
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
							Commands::TeleportREFRToPlayer(refr);
						}

						UserData::AddRecent(item);
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
				Commands::AddItemToRefInventory(tableTargetRef, item->GetEditorID(), 1);
				UserData::AddRecent(item);
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
				Commands::PlaceAtMe(item->GetEditorID(), 1);
				UserData::AddRecent(item);
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
			Commands::AddItemToRefInventory(this->tableTargetRef, a_item->GetEditorID(), a_item->GetQuantity());
			UserData::AddRecent(a_item);
		}

		UpdateActiveInventoryTables();
	}

	void UITable::RemovePayloadFromInventory(const std::unique_ptr<BaseObject>& a_item)
	{
		if (!tableTargetRef)
			return;

		if (a_item && !a_item->IsDummy() && a_item->GetTESForm()->IsInventoryObject()) {
			Commands::RemoveItemFromInventory(this->tableTargetRef, a_item->GetEditorID(), a_item->GetQuantity());
			UserData::AddRecent(a_item);
		}

		UpdateActiveInventoryTables();
	}

	// NOTE: Kit's are not inventories, they're virtual containers in memory. So we handle them
	// explcitly. We also can handle dummy forms here without worry.

	void UITable::AddPayloadToKit(const std::unique_ptr<BaseObject>& a_item)
	{
		bool has_item = false;
		if (!tableList.empty()) {
			for (auto& item : tableList) {
				if (item->GetEditorID() == a_item->GetEditorID()) {
					item->m_quantity++;
					has_item = true;
				}
			}
		}

		if (!has_item) {
			tableList.emplace_back(std::make_unique<BaseObject>(*a_item));
		}

		UserData::AddRecent(a_item);
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
		
		auto it = std::remove_if(this->tableList.begin(), this->tableList.end(),
			[&a_item](const std::unique_ptr<BaseObject>& item) {
				return item->GetEditorID() == a_item->GetEditorID();
			});

		this->tableList.erase(it, this->tableList.end());
	}

	void UITable::SyncChangesToKit()
	{
		if (this->HasFlag(ModexTableFlag_Kit)) {
			if (selectedKitPtr && !selectedKitPtr->empty()) {
				auto equipmentConfig = EquipmentConfig::GetSingleton();
				selectedKitPtr->m_items.clear();

				if (!this->tableList.empty()) {

					for (auto& item : this->tableList) {
						selectedKitPtr->m_items.emplace_back(EquipmentConfig::CreateKitItem(*item));
					}
				}

				equipmentConfig->SaveKit(*selectedKitPtr);
			}
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
				m_inventory.emplace_back(entry->object, 0, 0, quantity);
			}
		}

		Trace("Cached {} inventory items.", m_inventory.size());
		return m_inventory;
	}

	void UITable::Refresh()
	{
		selectionStorage.Clear();
		tableList.clear();
		
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
			if (pluginType == 0)
				return Filter(Data::GetSingleton()->GetAddItemList());
			if (pluginType == 1)
				return Filter(Data::GetSingleton()->GetNPCList());
			if (pluginType == 2)
				return Filter(Data::GetSingleton()->GetObjectList());
			// if (pluginType == 3)
		}
	}
	
	void UITable::FilterFavoriteImpl()
	{
		const auto favorites = UserData::GetFavoritesAsVector();
		auto temp = std::vector<BaseObject>{};

		for (const auto& edid : favorites) {
			RE::TESForm* form = RE::TESForm::LookupByEditorID(edid);
			temp.emplace_back(BaseObject(form, 0, 0));
		}

		for (const auto& item : temp) {
			// Disabled: Might be confusing to users if search parameter is carried over.
			// if (searchSystem->CompareInputToObject(&item) == false) {
				// continue;
			// }

			if (filterSystem && !filterSystem->ShouldShowItem(&item)) {
				continue;
			}

			this->tableList.emplace_back(std::make_unique<BaseObject>(item));
		}

	}

	void UITable::FilterRecentImpl()
	{
		const auto recent = UserData::GetRecentAsVector();
		auto temp = std::vector<BaseObject>{};

		// NOTE: This looks stinky, but unless we cache recent items as BaseObjects to begin with,
		// We have to rebuild them everytime we refresh. Something to think of down the line.

		for (const auto& edid : recent) {
			RE::TESForm* form = RE::TESForm::LookupByEditorID(edid);
			if (form) {
				temp.emplace_back(BaseObject(form, 0, 0));
			} else {
				if (UserConfig::Get().showMissing) {
					temp.emplace_back(BaseObject(edid, edid, Translate("ERROR_MISSING_PLUGIN")));
				}
			}
		}

		for (const auto& item : temp) {
			// Disabled: Might be confusing to users if search parameter is carried over.
			// if (searchSystem->CompareInputToObject(&item) == false) {
				// continue;
			// }

			if (filterSystem && !filterSystem->ShouldShowItem(&item)) {
				continue;
			}

			this->tableList.emplace_back(std::make_unique<BaseObject>(item));
		}

		SortListBySpecs();
		UpdateImGuiTableIDs();
	}

	void UITable::FilterKitImpl()
	{
		if (!selectedKitPtr)
			return;

		const auto& kit = selectedKitPtr->m_items;

		for (const auto& item : kit) {
			RE::TESForm* form = RE::TESForm::LookupByEditorID(item->m_editorid);

			if (form) {
				tableList.emplace_back(std::make_unique<BaseObject>(form, 0, 0, item->m_amount, item->m_equipped));
			} else {
				tableList.emplace_back(std::make_unique<BaseObject>(item->m_name, item->m_editorid, item->m_plugin, 0, item->m_amount, item->m_equipped));
			}
		}

		SortListBySpecs();
		UpdateImGuiTableIDs();
	}

	void UITable::FilterInventoryImpl()
	{
		const auto inventory = GetReferenceInventory();

		for (const auto& item : inventory) {
			this->tableList.emplace_back(std::make_unique<BaseObject>(item.GetTESForm(), 0, 0, item.GetQuantity()));
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

			this->tableList.emplace_back(std::make_unique<BaseObject>(item.GetTESForm(), 0, item.m_refID));
		}

		SortListBySpecs();
		UpdateImGuiTableIDs();
	}


	void UITable::BuildPluginList()
	{
		const auto& config = UserConfig::Get();
		const auto type = static_cast<Data::PluginType>(this->pluginType);
		const auto sort = static_cast<Data::PluginSort>(config.modListSort);

		this->pluginList = Data::GetSingleton()->GetFilteredListOfPluginNames(type, sort); 
		this->pluginSet = Data::GetSingleton()->GetModulePluginList(type);

		pluginList.insert(pluginList.begin(), Translate("SHOWALL"));
	}

	void UITable::DrawFormSearchBar(const ImVec2& a_size)
	{
		if (tableMode != SHOWALL) ImGui::BeginDisabled();

		const float input_width = a_size.x;
		const float key_width = a_size.x * 0.45f;

		int current_idx = searchSystem->GetSearchKeyIndex();
		const std::string current_key_text = searchSystem->GetCurrentKeyString();

		ImGui::PushStyleColor(ImGuiCol_FrameBg, ThemeConfig::GetColor("BG_LIGHT"));
		ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ThemeConfig::GetHover("BG_LIGHT"));
		ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ThemeConfig::GetActive("BG_LIGHT"));
		ImGui::PushStyleVar(ImGuiStyleVar_SelectableTextAlign, ImVec2(0.5f, 0.5f));

		const std::vector<std::string> available_keys = searchSystem->GetAvailableKeysVector();
		if (UICustom::FancyDropdown("##Search::Input::Key", "TABLE_KEY_TOOLTIP", current_idx, available_keys, key_width)) {
			searchSystem->SetSearchKeyByIndex(current_idx);
			Refresh();
		}

		ImGui::PopStyleVar();
		ImGui::PopStyleColor(3);
		
		ImGui::AlignTextToFramePadding();
		ImGui::Text(" " ICON_LC_ARROW_LEFT_RIGHT " ");
		ImGui::SameLine();
		
		const auto input_flags = useQuickSearch ? ImGuiInputTextFlags_AutoSelectAll :
		ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_EnterReturnsTrue;

		static bool key_hovered;
		ImGui::PushStyleColor(ImGuiCol_FrameBg, key_hovered ? ThemeConfig::GetHover("BG_LIGHT") : ThemeConfig::GetColor("BG_LIGHT"));
		if (UICustom::FancyInputText("##Search::Input::Compare", "TABLE_SEARCH_HINT", "TABLE_SEARCH_TOOLTIP", searchSystem->GetSearchBuffer(), input_width, input_flags)) {
			this->Refresh();
		}
		ImGui::PopStyleColor();

		key_hovered = ImGui::IsItemHovered();
		if (ImGui::Shortcut(ImGuiKey_Space, ImGuiInputFlags_RouteFromRootWindow)) {
			ImGui::SetKeyboardFocusHere(-1);
		}
		
		ImGui::SameLine();

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

		if (searchSystem->InputTextComboBox("##Search::Filter::PluginField", pluginSearchBuffer, selectedPlugin, IM_ARRAYSIZE(pluginSearchBuffer), pluginList, a_size.x)) {
			this->selectedPlugin = this->pluginSearchBuffer;
			this->pluginSearchBuffer[0] = '\0';
			
			this->selectionStorage.Clear();
			this->Refresh();
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

		// Magic number + user offset.
		const float height = (styleFontSize * 1.75f) + styleHeight;
		
		// Spacing between each element.
		LayoutRowSpacing = 3.0f + styleSpacing;
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
		if (UICustom::FancyDropdown("##Search::Input::Mode", "TABLE_MODE_TOOLTIP", current_idx, mode_strings, a_size.x)) {
			selectionStorage.Clear();
			tableMode = current_idx;
			Refresh();
		}

		ImGui::PopStyleColor(3);
		ImGui::PopStyleVar();

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

		const float dropdown_width = ImGui::GetContentRegionAvail().x / 7.5f;
		DrawModeDropdown(ImVec2(dropdown_width, 0.0f));

		const float search_width = ImGui::GetContentRegionAvail().x / 2.5f;
		DrawFormSearchBar(ImVec2(search_width, 0.0f));

		
		const float plugin_width = ImGui::GetContentRegionAvail().x;
		DrawPluginSearchBar(ImVec2(plugin_width, 0.0f));
		ImGui::PopStyleVar();
	}

	void UITable::DrawKit(const Kit& a_kit, const ImVec2& a_pos)
	{
		const auto& DrawList = ImGui::GetWindowDrawList();
		const float fontSize = ImGui::GetFontSize(); 

		// Setup box and bounding box for positioning and drawing.
		const ImVec2 box_min(a_pos.x - 1, a_pos.y - 1);
		const ImVec2 box_max(box_min.x + LayoutItemSize.x + 2, box_min.y + LayoutItemSize.y + 2);  // Dubious
		ImRect bb(box_min, box_max);

		// Outline & Background
		const float global_alpha = ImGui::GetStyle().Alpha;
		const ImU32 bg_color = ThemeConfig::GetColorU32("BG", global_alpha);
		const ImU32 bg_color_alt = ThemeConfig::GetColorU32("BG_LIGHT", global_alpha);
		const ImU32 outline_color = ThemeConfig::GetColorU32("BG", global_alpha);
		const ImU32 text_color = ThemeConfig::GetColorU32("TEXT", global_alpha);

		// Background
		if (a_kit.m_tableID % 2 == 0) {
			DrawList->AddRectFilled(bb.Min, bb.Max, bg_color);
		} else {
			DrawList->AddRectFilled(bb.Min, bb.Max, bg_color_alt);
		}

		// Outline
		DrawList->AddRect(bb.Min, bb.Max, outline_color, 0.0f, 0, 1.0f);

		const float spacing = LayoutColumnWidth / 3.0f;
		const float top_align = bb.Min.y + LayoutOuterPadding;
		const float bot_align = bb.Max.y - LayoutOuterPadding - fontSize;
		const float center_align = bb.Min.y + ((LayoutOuterPadding + LayoutItemSize.y) / 2) - (fontSize / 2.0f);
		const float left_align = bb.Min.x + LayoutOuterPadding;
		const float right_align = bb.Max.x - LayoutOuterPadding - fontSize;
		const ImVec2 top_left_align = ImVec2(left_align, top_align);
		const ImVec2 top_right_align = ImVec2(right_align, top_align);
		const ImVec2 bot_left_align = ImVec2(left_align, bot_align);
		const ImVec2 bot_right_align = ImVec2(right_align, bot_align);
		const ImVec2 center_left_align = ImVec2(left_align, center_align);
		const ImVec2 center_right_align = ImVec2(right_align, center_align);

		// Draw the kit name for now
		const std::string name_string = TRUNCATE(a_kit.GetName(), spacing);
		DrawList->AddText(center_left_align, text_color, name_string.c_str());
		
		const std::string weaponCount = a_kit.m_weaponCount == 0 ? Translate("None") : std::to_string(a_kit.m_weaponCount);
		const std::string armorCount = a_kit.m_armorCount == 0 ? Translate("None") : std::to_string(a_kit.m_armorCount);
		const std::string miscCount = a_kit.m_miscCount == 0 ? Translate("None") : std::to_string(a_kit.m_miscCount);
		const std::string totalCount = std::to_string(a_kit.m_weaponCount + a_kit.m_armorCount + a_kit.m_miscCount);

		// Draw the kit meta data
		const ImVec2 total_count_pos = ImVec2(left_align + spacing, center_align);
		const std::string total_count_string = ICON_LC_BOX + totalCount;
		DrawList->AddText(total_count_pos, text_color, total_count_string.c_str());
	

		const std::string desc_string = a_kit.m_desc;

		if (ImGui::CalcTextSize(desc_string.c_str()).x > spacing * 1.5f) {
			std::string first_half = desc_string.substr(0, desc_string.size() / 2);
			std::string second_half = desc_string.substr(desc_string.size() / 2);

			DrawList->AddText(center_left_align, text_color, first_half.c_str());
			DrawList->AddText(bot_left_align, text_color, second_half.c_str());
		} else {
			DrawList->AddText(bot_left_align, text_color, desc_string.c_str());
		}
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
					origin->RemovePayloadFromInventory(item);
				}
			}
		}

		if (destination->GetDragDropHandle() == DragDropHandle::Kit) {
			destination->SyncChangesToKit();
			destination->Refresh();
		}

		if (origin->GetDragDropHandle() == DragDropHandle::Kit) {
			origin->SyncChangesToKit();
			origin->Refresh();
		}

		Refresh();
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
						const auto& DrawList = ImGui::GetForegroundDrawList();
						const auto table_size = ImGui::GetItemRectSize();
						const auto window_size = ImGui::GetWindowSize();
						const auto window_pos = ImGui::GetWindowPos();
						const ImVec2 min = window_pos + ImVec2(0.0f, window_size.y - table_size.y);
						const ImVec2 max = min + table_size;
						const float font_size = 72.0f;
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

						ImGui::PushFont(NULL, font_size / 2.0f);
						const ImVec2 string_size = ImGui::CalcTextSize(tooltip_string.c_str());
						ImGui::PopFont();

						DrawList->AddText(ImGui::GetFont(), font_size / 2.0f,
							min + (table_size / 2.0f) - (string_size / 2.0f) + ImVec2(0, icon_size.y / 1.5f) - ImVec2(0, center_offset),
							ThemeConfig::GetColorU32("TEXT", 0.75f),
							tooltip_string.c_str()
						);

						if (!tooltip_target.empty()) {
							ImGui::PushFont(NULL, font_size / 2.0f);
							const ImVec2 target_size = ImGui::CalcTextSize(tooltip_target.c_str());
							ImGui::PopFont();

							DrawList->AddText(ImGui::GetFont(), font_size / 2.5f,
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
						const auto& item = (*ptr->GetTableListPtr())[payloadID];
						payload_items.emplace_back(std::make_unique<BaseObject>(*item));
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
									if (auto new_kit = EquipmentConfig::CreateKit(a_input); new_kit.has_value()) {
										*pointer = std::move(new_kit.value());

										for (const auto& item : *items) {
											// pointer->m_items.emplace_back(EquipmentConfig::CreateKitItem(*item));
											destination->AddPayloadToKit(item);
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
	}

	// TODO: Migrate all additions to Recent list to Command methods.

	void UITable::HandleLeftClickBehavior(const std::unique_ptr<BaseObject>& a_item)
	{		
		if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
			if (a_item->IsItem()) {
				UICustom::InputAmountHandler(ImGui::GetIO().KeyShift, [&a_item](uint32_t amount = 1) {
					Commands::AddItemToPlayerInventory(a_item->GetEditorID(), amount);
				});
			}

			if (tableTargetRef && a_item->IsNPC()) {
				UICustom::InputAmountHandler(ImGui::GetIO().KeyShift, [&a_item](uint32_t amount = 1) {
					Commands::PlaceAtMe(a_item->GetEditorID(), amount);
				});
			}
		}
	}

	void UITable::HandleRightClickBehavior(const std::unique_ptr<BaseObject>& a_item)
	{

		static const int click_amount = 1; // TODO revisit this

		if (ImGui::IsItemClicked(ImGuiMouseButton_Right)) {
			if (!selectionStorage.Contains(a_item->m_tableID)) {
				selectionStorage.Clear();
			}

			if (!a_item->IsDummy()) {
				ImGui::OpenPopup("TableViewContextMenu");
			}
		}

		if (ImGui::BeginPopup("TableViewContextMenu")) {
			const bool shift_down = ImGui::GetIO().KeyShift;
			
			if (!tableTargetRef || !IsValidTargetReference()) {
				ImGui::TextColored(ThemeConfig::GetColor("ERROR"), "%s", Translate("ERROR_INVALID_REFERENCE"));
				ImGui::EndPopup();
				return;
			}

			const auto color = tableTargetRef->IsPlayerRef() ? ThemeConfig::GetColor("SUCCESS") : ThemeConfig::GetColor("WARN");
			ImGui::TextColored(color, "%s %s", ICON_LC_ASTERISK " ", tableTargetRef->GetName());
			ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal);

			if (a_item->IsItem()) {
				if (HasFlag(ModexTableFlag_Inventory)) {
					if (ImGui::MenuItem(Translate("REMOVE_SELECTION"))) {
						this->RemoveSelectionFromTargetInventory();
					}

					if (ImGui::MenuItem(Translate("DROP_SELECTION"))) {
						this->PlaceSelectionOnGround(click_amount);
						this->RemoveSelectionFromTargetInventory();
					}

					ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal);
				}

				if (HasFlag(ModexTableFlag_Base)) {
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

					ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal);

					if (a_item->GetFormType() == RE::FormType::Book) {
						if (GetSelectionCount() <= 1) {
							if (ImGui::MenuItem(Translate("GENERAL_READ_ME"))) {
								Commands::ReadBook(a_item->GetEditorID());
								UIManager::GetSingleton()->Close();
								UserData::AddRecent(a_item);
							}
						}

						ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal);
					}
				}

				if (HasFlag(ModexTableFlag_Kit)) { // Kit Table View Window
					if (ImGui::MenuItem(Translate("KIT_REMOVE"))) {
						this->RemoveSelectionFromKit();
						this->SyncChangesToKit();
					}

					ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal);
				}

				// We use dragDropSourceList to find paired Kit tables, since selectedKit is stored in the kit table.
				if (HasFlag(ModexTableFlag_Base)) {
					if (auto it = dragDropSourceList.find(DragDropHandle::Kit); it != this->dragDropSourceList.end()) {
						const auto destination = it->second;
						if (const auto selected_kit = destination->selectedKitPtr; selected_kit && !selected_kit->empty()) {
							if (ImGui::MenuItem(Translate("KIT_ADD"))) {
								this->AddSelectionToActiveKit();
								destination->SyncChangesToKit();
							}

							ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal);
						}
					}
				}
			}

			if (a_item->IsNPC()) { // ModexTableFlag_Base (?)
				if (ImGui::MenuItem(Translate("PLACE_SELECTION"))) {
					PlaceSelectionOnGround(click_amount);
				}

				if (a_item->m_refID != 0) {
					if (ImGui::MenuItem(Translate("GOTO_NPC_REFERENCE"))) {
						Commands::TeleportPlayerToNPC(a_item->m_refID);
						UserData::AddRecent(a_item);
						UIManager::GetSingleton()->Close();
					}

					if (ImGui::MenuItem(Translate("BRING_NPC_REFERENCE"))) {
						Commands::TeleportNPCToPlayer(a_item->m_refID);
						UserData::AddRecent(a_item);
						UIManager::GetSingleton()->Close();
					}

					if (a_item->IsDisabled()) {
						if (ImGui::MenuItem(Translate("ENABLE_NPC_REFERENCE"))) {
							if (auto* target = RE::TESForm::LookupByID<RE::Actor>(a_item->m_refID); target != nullptr) {
								Commands::EnableRefr(target, false);
								UserData::AddRecent(a_item);
							}
						}
					} else {
						if (ImGui::MenuItem(Translate("DISABLE_NPC_REFERENCE"))) {
							if (auto* target = RE::TESForm::LookupByID<RE::Actor>(a_item->m_refID); target != nullptr) {
								Commands::DisableRefr(target);
								UserData::AddRecent(a_item);
							}
						}
					}

				}

				ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal);
			}

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
		const auto& font_size = ImGui::GetFontSize();

		// Setup box and bounding box for positioning and drawing.
		const ImVec2 box_min(a_pos.x - 1, a_pos.y - 1);
		const ImVec2 box_max(box_min.x + LayoutItemSize.x + 2, box_min.y + LayoutItemSize.y + 2);
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

		if (a_item->IsDummy()) {
			draw_list->AddRectFilled(bb.Min, bb.Max, colors.error);
		}

		draw_list->AddRect(bb.Min, bb.Max, colors.outline, 0.0f, 0, 1.0f);
		

		// Type Color Identifier
		const float type_pillar_width = 5.0f;
		draw_list->AddRectFilled(
			ImVec2(bb.Min.x, bb.Min.y),
			ImVec2(bb.Min.x + type_pillar_width, bb.Max.y),
			UICustom::GetFormTypeColor(a_item->GetFormType())
		);

		// Type Pillar tooltip
		if (IsMouseHoveringRect(
			ImVec2(bb.Min.x + LayoutOuterPadding, bb.Min.y + LayoutOuterPadding),
			ImVec2(bb.Min.x - LayoutOuterPadding + type_pillar_width, bb.Max.y - LayoutOuterPadding))) {
			UINotification::ShowObjectTooltip(a_item);
		}

		// Pad bounding box pre-alignment.
		bb.Min.x += type_pillar_width * 2.0f;
		float spacing = LayoutItemSize.x / 4.0f;

		// Text alignment calculations
		const float center_align = bb.Min.y + ((LayoutOuterPadding + LayoutItemSize.y) / 2) - (font_size / 2.0f);
		const float left_align = bb.Min.x + LayoutOuterPadding;
		const ImVec2 center_left_align = ImVec2(left_align, center_align);

		// Dynamically concenate quantity to string.
		const std::string quantity_string = a_item->GetQuantity() > 1 ? std::format(" ({})", std::to_string(a_item->GetQuantity())) : "";
		const float quantity_offset = ImGui::CalcTextSize(quantity_string.c_str()).x;

		// Resolve item naming string based on editorid, icon, and truncation.
		const std::string item_icon = showItemIcon ? (a_item->GetItemIcon() + " ").c_str() : "";
		const std::string raw_name = showEditorID ? a_item->GetEditorID() : a_item->GetName();
		const std::string name_string = TRUNCATE(item_icon + raw_name, (spacing * 1.75f) - quantity_offset) + quantity_string;

		bool is_enchanted = false;
		if (const auto& weapon = a_item->GetTESWeapon(); weapon.has_value()) {
			is_enchanted = weapon.value()->formEnchanting;
		}

		if (const auto& armor = a_item->GetTESArmor(); armor.has_value()) {
			is_enchanted = armor.value()->formEnchanting;
		}

		const ImU32 item_color = is_enchanted ? colors.textEnchanted : colors.text;
		draw_list->AddText(center_left_align, item_color, name_string.c_str());

		// Exit early since dummy forms can't be acted upon.
		if (a_item->IsDummy()) { return; }
		
		// Toggelable Equip Button
		ImVec2 equippable_pos = ImVec2((LayoutItemSize.x / 2.0f), a_pos.y);
		ImGui::SameLine();
		ImGui::SetCursorPosX(LayoutItemSize.x / 2.0f);

		const auto icon = a_item->GetEquipped() ? ICON_LC_CHECK : ICON_LC_X;
		const auto equip_size = ImVec2(LayoutItemSize.x / 4.0f, LayoutItemSize.y);

		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0.0f, 0.0f));
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f));

		if (a_item->GetFormType() == RE::FormType::Armor || a_item->GetFormType() == RE::FormType::Weapon) {
			const auto equip_color = a_item->GetEquipped() ? ThemeConfig::GetColor("PRIMARY") : ThemeConfig::GetColor("PRIMARY", 0.5f);

			ImGui::PushStyleColor(ImGuiCol_Button, equip_color);
			const auto text = a_item->GetFormType() == RE::FormType::Armor ? a_item->GetArmorSlots()[0] : a_item->GetWeaponType();
			if (ImGui::Button((std::string(icon) + text).c_str(), equip_size)) {
				a_item->m_equipped = !a_item->m_equipped;
				this->SyncChangesToKit();
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

		ImGui::SameLine();
		ImGui::SetCursorPosX(LayoutItemSize.x / 1.25f);
		// Input Amount Widget
		ImGui::SetCursorPosY(ImGui::GetCursorPosY() + LayoutOuterPadding);
		ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - LayoutOuterPadding);
		if (ImGui::InputInt("##EquipCount", &a_item->m_quantity, 1, 10)) {
			this->SyncChangesToKit();
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
		if (a_item->IsDummy()) {
			draw_list->AddRectFilled(bb.Min, bb.Max, colors.error);
		}

		// Outline
		draw_list->AddRect(bb.Min, bb.Max, colors.outline, 0.0f, 0, 1.0f);

		// Type Color Pillar Identifier
		const float type_pillar_width = 5.0f;
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
		float spacing = (LayoutColumnWidth / 4.0f);
		float padding = ImGui::GetFrameHeight();

		// Text alignment calculations
		const float center_align = bb.Min.y + ((LayoutOuterPadding + LayoutItemSize.y) / 2) - (font_size / 2.0f);
		const float left_align = bb.Min.x + LayoutOuterPadding;
		const float right_align = bb.Max.x - LayoutOuterPadding - font_size;
		const ImVec2 center_left_align = ImVec2(left_align, center_align);
		const ImVec2 center_right_align = ImVec2(right_align, center_align);

		// Name string construction with dynamic truncation based on available space.
		const std::string quantity_string = a_item->GetQuantity() > 1 ? std::format(" ({})", std::to_string(a_item->GetQuantity())) : "";
		const std::string item_icon = showItemIcon ? (a_item->GetItemIcon() + " ").c_str() : "";
		const float quantity_offset = ImGui::CalcTextSize(quantity_string.c_str()).x;

		const std::string raw_name = showEditorID ? a_item->GetEditorID() : a_item->GetName();
		const std::string name_string = TRUNCATE(item_icon + raw_name, (spacing * 1.5f) - padding - quantity_offset) + quantity_string;

		if (a_item->GetFormType() == RE::FormType::NPC) {
			if (const auto& npc = a_item->GetTESNPC(); npc.has_value()) {
				const auto& npcData = npc.value();
				if (npcData != nullptr) {
					if (a_item->GetRefID() != 0) {
						constexpr std::string icon = ICON_LC_ASTERISK;
						const ImVec2 icon_pos = ImVec2(center_right_align.x - ImGui::GetFontSize(), center_right_align.y);
						draw_list->AddText(icon_pos, colors.text, icon.c_str());

						if (IsMouseHoveringRect(icon_pos, ImVec2(icon_pos.x + font_size, icon_pos.y + font_size))) {
							UINotification::ShowPropertyTooltip(PropertyType::kReferenceID);
						}
					}

					const bool is_essential = a_item->IsEssential();
					const bool is_unique = a_item->IsUnique();

					if (is_essential && !is_unique) {
						text_color = colors.textEssential;
					} else if (is_unique && !is_essential) {
						text_color = colors.textUnique;
					} else if (is_essential && is_unique) {
						text_color = colors.textUniqueEssential;
					}
				}
			}
		}

		const PropertyType& sort_property = this->sortSystem->GetSecondarySortFilter().GetPropertyType();
		const ImVec2 sort_pos = ImVec2(bb.Min.x + spacing * 2.5f, center_align);
		const float sort_text_cutoff = spacing * 1.5f;

		if (const auto& item = a_item; item->IsItem()) {
			if (const auto& armor = item->GetTESArmor(); armor.has_value()) {
				const auto& armorData = armor.value();

				if (armorData != nullptr) {
					const std::string rating_string = item->GetPropertyValueWithIcon(PropertyType::kArmorRating);
					const std::string type_string = TRUNCATE(item->GetPropertyValueWithIcon(PropertyType::kArmorType), sort_text_cutoff);

					if (armorData->formEnchanting != nullptr) {
						text_color = colors.textEnchanted;
					}

					if (sort_property == PropertyType::kNone) {
						draw_list->AddText(sort_pos, colors.text, rating_string.c_str());
						if (IsMouseHoveringRect(sort_pos, ImVec2(sort_pos.x + font_size, sort_pos.y + font_size))) {
							UINotification::ShowPropertyTooltip(PropertyType::kArmorRating);
						}
					}
				}
			}

			if (item->GetFormType() == RE::FormType::Book) {
				if (const auto& book = item->GetTESForm()->As<RE::TESObjectBOOK>(); book != nullptr) {
					const auto teaches_skill = book->TeachesSkill();
					const auto teaches_spell = book->TeachesSpell();


					if (sort_property == PropertyType::kNone) {
						if (teaches_skill || book->GetSkill() != RE::ActorValue::kNone) {
							const std::string skill_string = TRUNCATE(item->GetPropertyValueWithIcon(PropertyType::kTomeSkill), sort_text_cutoff);
							draw_list->AddText(sort_pos, colors.text, skill_string.c_str());
						}
						
						if (teaches_spell || book->GetSpell() != nullptr) {
							const std::string spell_string = TRUNCATE(item->GetPropertyValueWithIcon(PropertyType::kTomeSpell), sort_text_cutoff);
							draw_list->AddText(sort_pos, colors.text, spell_string.c_str());
						}
						
						if (IsMouseHoveringRect(sort_pos, ImVec2(sort_pos.x + font_size, sort_pos.y + font_size))) {
							if (teaches_skill) {
								UINotification::ShowPropertyTooltip(PropertyType::kTomeSkill);
							} else if (teaches_spell) {
								UINotification::ShowPropertyTooltip(PropertyType::kTomeSpell);
							}
						}
					}
				}
			}

			if (const auto& weapon = item->GetTESWeapon(); weapon.has_value()) {
				const auto& weaponData = weapon.value();

				if (weaponData != nullptr) {
					const std::string damage_string = item->GetPropertyValueWithIcon(PropertyType::kWeaponDamage);
					const std::string skill_string = TRUNCATE(item->GetPropertyValueWithIcon(PropertyType::kWeaponSkill), sort_text_cutoff);
					// const std::string type_string = item.GetPropertyValueWithIcon(PropertyType::kWeaponType);

					if (weaponData->formEnchanting != nullptr) {
						text_color = colors.textEnchanted;
					}
					
					if (sort_property == PropertyType::kNone) {
						draw_list->AddText(sort_pos, colors.text, damage_string.c_str());
						if (IsMouseHoveringRect(sort_pos, ImVec2(sort_pos.x + font_size, sort_pos.y + font_size))) {
							UINotification::ShowPropertyTooltip(PropertyType::kWeaponDamage);
						}
					}
				}
			}
		}

		const std::string plugin_name = TRUNCATE(a_item->GetPluginName(), spacing - padding);
		draw_list->AddText(center_left_align, colors.text, plugin_name.c_str());

		const ImVec2 name_pos = ImVec2(bb.Min.x + spacing - 5.0f, center_align);
		draw_list->AddText(name_pos, text_color, name_string.c_str());

		if (IsMouseHoveringRect(name_pos, ImVec2(name_pos.x + font_size, name_pos.y + font_size))) {
			UINotification::ShowObjectTooltip(a_item);
		}

		// Display tooltips relevant to name colors (essential, unique, enchanted)
		if (IsMouseHoveringRect(name_pos + ImVec2(font_size, 0.0f), ImVec2(name_pos.x + ImGui::CalcTextSize(name_string.c_str()).x, name_pos.y + font_size))) {
			if (a_item->IsEssential() && !a_item->IsUnique()) {
				UINotification::ShowPropertyTooltip(PropertyType::kEssential);
			} else if (a_item->IsUnique() && !a_item->IsEssential()) {
				UINotification::ShowPropertyTooltip(PropertyType::kUnique);
			} else if (a_item->IsUnique() && a_item->IsEssential()) {
				UINotification::ShowPropertyTooltip(PropertyType::kUniqueEssential);
			} else if (a_item->IsEnchanted()) {
				UINotification::ShowPropertyTooltip(PropertyType::kEnchanted);
			}
		}

		if (sort_property != PropertyType::kPlugin and sort_property != PropertyType::kName and sort_property != PropertyType::kGoldValue) {
			const std::string sort_text = TRUNCATE(a_item->GetPropertyValueWithIcon(sort_property), spacing * 0.75f);
			draw_list->AddText(sort_pos, colors.text, sort_text.c_str());
			
			if (IsMouseHoveringRect(sort_pos, ImVec2(sort_pos.x + ImGui::CalcTextSize(sort_text.c_str()).x, sort_pos.y + font_size))) {
				UINotification::ShowPropertyTooltip(sort_property);
			}
		}
	}

	void UITable::DrawDragDropPayload(const std::string& a_icon)
	{
		const auto payload = ImGui::GetDragDropPayload();
		if (payload && payload->IsDataType(std::to_string(tableID).c_str())) {
			const auto payloadCount = payload->DataSize / (int)sizeof(ImGuiID);

			const float mult = ImGui::GetFrameHeightWithSpacing();
			const ImVec2 size_min = ImVec2(mult * 5.0f, mult * 5.0f);
			const ImVec2 size_max = ImVec2(mult * 10.0f, mult * 10.0f);

			ImGui::SetNextWindowSizeConstraints(size_min, size_max);
			if (ImGui::BeginTooltip()) {
				const ImVec2 start_pos = ImGui::GetCursorScreenPos();
				ImGui::PushFontBold(36.0f);
				ImGui::SetCursorPosX(UICustom::GetCenterTextPosX(std::to_string(payloadCount).c_str()));
				ImGui::SetCursorPosY((ImGui::GetContentRegionAvail().y / 2.0f) - 24.0f); // 6px
				ImGui::Text("%d", payloadCount);
				ImGui::PopFont();
				ImGui::SetCursorPosX(UICustom::GetCenterTextPosX(Translate("SELECTED")));
				ImGui::Text("%s", Translate("SELECTED"));

				const auto& DrawList = ImGui::GetWindowDrawList();
				float size = 24.0f;
				float icon_x = ImGui::GetWindowWidth() - ImGui::GetFrameHeight() * 1.5f;
				float icon_y = -5.0f;

				ImGui::PushFont(NULL, size);
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
		}

		ImGui::PopStyleVar();
	}

	void UITable::DrawStatusBar()
	{
		static constexpr const char* valid_icon = ICON_LC_ASTERISK;
		static constexpr const char* invalid_icon = ICON_LC_X;
		static constexpr const char* warning_icon = ICON_LC_TRIANGLE_ALERT;
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

		const auto user_height = ImGui::GetFrameHeight() * 1.25f;
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
		
		if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort | ImGuiHoveredFlags_NoSharedDelay)) {
			UICustom::FancyTooltip("STATUS_BAR_TOOLTIP");
		}

		ImGui::SameLine();

		const ImVec2 icon_position = ImVec2(
				(ImGui::GetCursorScreenPos().x - user_width),
				(ImGui::GetCursorScreenPos().y + 5.0f));

		const ImVec2 link_position = ImVec2(
				(ImGui::GetCursorScreenPos().x - ImGui::GetFrameHeightWithSpacing() * 2.75f),
				(ImGui::GetCursorScreenPos().y + 5.0f));

		ImGui::PushFont(NULL, ImGui::GetFontSize() + 2.0f);
		const auto& DrawList = ImGui::GetWindowDrawList();
		DrawList->AddText(icon_position, colors.text, status_icon);
		DrawList->AddText(link_position, colors.text, useSharedTarget ? ICON_LC_LINK : ICON_LC_UNLINK);
		ImGui::PopFont();

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
			UIManager::GetSingleton()->ShowInputBox(
				Translate("STATUS_BAR_TITLE"),
				Translate("STATUS_BAR_DESC"),
				"",
				[this](const std::string& a_input) {
					auto reference = UIModule::LookupReferenceBySearch(a_input);

					if (reference != nullptr) {
						this->SetTargetByReference(reference);
					} else {
						UIManager::GetSingleton()->ShowWarning(
							Translate("INVALID_REFERENCE_POPUP_TITLE") + a_input,
							Translate("INVALID_REFERENCE_POPUP_DESC")
						);
					}
				}
			);
		}

		// Settings Button
		ImGui::PushFont(NULL, ImGui::GetFontSize() + 2.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.f, 0.f));
		ImGui::PushStyleColor(ImGuiCol_Button, ThemeConfig::GetColor(status_color));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ThemeConfig::GetHover(status_color));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ThemeConfig::GetActive(status_color));
		ImGui::SameLine();

		ImGui::SetCursorPosX(ImGui::GetCursorPosX() - ImGui::GetFrameHeightWithSpacing() * 1.5f);
		// ImGui::SetNextItemAllowOverlap();
		const ImVec2 button_size = ImVec2(ImGui::GetContentRegionAvail().x, user_height);
		if (ImGui::Button(ICON_LC_SETTINGS, button_size)) {
			ImGui::OpenPopup("TABLE_SETTINGS_POPUP");
		}

		ImGui::PopStyleVar();
		ImGui::PopStyleColor(3);
		ImGui::PopFont();

		ImGui::SetNextWindowSize(ImVec2(ImGui::GetWindowSize().x / 4.0f, 0.0f));
		if (ImGui::BeginPopup("TABLE_SETTINGS_POPUP")) {
			DrawTableSettingsPopup();
			ImGui::EndPopup();
		}

		ImGui::PushStyleColor(ImGuiCol_Separator, ThemeConfig::GetColor(status_color));
		ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal);
		ImGui::PopStyleColor();

		// Draw last to avoid unwanted styling
		if (ImGui::BeginPopup("STATUS_BAR_CONTEXT_MENU")) {
			if (ImGui::MenuItem(Translate("GOTO_NPC_REFERENCE"))) {
				if (IsValidTargetReference()) {
					Commands::TeleportPlayerToREFR(tableTargetRef);
				}
			}

			if (ImGui::MenuItem(Translate("BRING_NPC_REFERENCE"))) {
				if (IsValidTargetReference()) {
					Commands::TeleportREFRToPlayer(tableTargetRef);
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
	void UITable::CustomSortColumn()
	{
		const static ImVec4 text_col = ThemeConfig::GetColor("TEXT_HEADER");

		constexpr auto combo_flags = ImGuiComboFlags_HeightLarge;
		ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0, 0, 0, 0));
		ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0, 0, 0, 0));
		ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0, 0, 0, 0));
		ImGui::PushStyleColor(ImGuiCol_Text, text_col);
		
		const float sortby_size = ImGui::GetContentRegionAvail().x;
		const FilterProperty current_key = sortSystem->GetSecondarySortFilter();
		const auto preview_text = current_key == PropertyType::kNone ? Translate("SORT_BY") : current_key.ToString();
		
		ImGui::SetNextItemWidth(sortby_size);
		if (ImGui::BeginCombo("##UITable::Sort::Combo", preview_text.c_str(), combo_flags)) {
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
					sortSystem->SetSecondarySortFilter(key);

					this->SortListBySpecs();
					this->UpdateImGuiTableIDs();
					
					UserData::Set<int>(data_id + "::SortBy", static_cast<int>(key.GetPropertyType()));
				}
				
				if (is_selected) {
					ImGui::SetItemDefaultFocus();
				}
			}
			ImGui::PopFont();
			ImGui::PopStyleColor();
			ImGui::EndCombo();
		}

		ImGui::PopStyleColor(4);
	}

	void UITable::DrawHeader()
	{
		static const ImVec4 text_col = ThemeConfig::GetColor("TEXT_HEADER");

		ImGui::PushID("##Modex::Table::Header");

		ImGui::PushStyleColor(ImGuiCol_Separator, ThemeConfig::GetColor("PRIMARY"));

		std::string header_plugin = this->showFormID ? Translate("FORMID") : Translate("PLUGIN");
		std::string header_name = this->showEditorID ? Translate("EDITORID") : Translate("NAME");
		std::string header_custom = this->sortSystem->GetSecondarySortFilter().ToString();
		std::string sort_icon = sortSystem->GetSortAscending()  == true ? ICON_LC_ARROW_DOWN_A_Z : ICON_LC_ARROW_UP_A_Z;
		
		// primary = name, plugin, editorid, formid.
		// secondary = any custom property type.
		PropertyType primary_sort = sortSystem->GetUsePrimary() ? sortSystem->GetPrimarySortFilter().GetPropertyType() : PropertyType::kNone;
		PropertyType secondary_sort = sortSystem->GetUsePrimary() ? PropertyType::kNone : sortSystem->GetSecondarySortFilter().GetPropertyType();

		constexpr float name_offset = 4.0f; // pillar offset.
		const float spacing = name_offset + (LayoutColumnWidth / 4.0f);
		const float sort_spacing = spacing * 2.50f;

		if (HasFlag(ModexTableFlag_Kit)) {
			bool is_first_sorted = showEditorID ? primary_sort == PropertyType::kEditorID : primary_sort == PropertyType::kName;
			auto first_icon = is_first_sorted ? sort_icon : ICON_LC_ARROW_UP_DOWN;
			ImGui::AlignTextToFramePadding();
			ImGui::TextColored(text_col, "%s", first_icon.c_str());

			if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
				this->sortSystem->SetPrimarySortFilter(this->showEditorID ? PropertyType::kEditorID : PropertyType::kName);
				this->sortSystem->ToggleAscending();
				this->SortListBySpecs();
				this->UpdateImGuiTableIDs();
			}

			ImGui::SameLine();
			ImGui::AlignTextToFramePadding();
			if (is_first_sorted) ImGui::PushFontBold();
			ImGui::TextColored(text_col, "%s", header_name.c_str());
			if (is_first_sorted) ImGui::PopFont();

			if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
				this->showEditorID = !this->showEditorID;

				if (primary_sort == PropertyType::kEditorID || primary_sort == PropertyType::kName) {
					this->sortSystem->SetPrimarySortFilter(this->showEditorID ? PropertyType::kEditorID : PropertyType::kName);
					this->SortListBySpecs();
					this->UpdateImGuiTableIDs();
				}
			}

			bool is_second_sorted = primary_sort == PropertyType::kArmorType;
			auto second_icon = is_second_sorted ? sort_icon : ICON_LC_ARROW_UP_DOWN;

			ImGui::SameLine(LayoutItemSize.x / 2.0f);
			ImGui::AlignTextToFramePadding();
			ImGui::TextColored(text_col, "%s", second_icon.c_str());

			if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
				this->sortSystem->SetPrimarySortFilter(PropertyType::kArmorType);
				this->sortSystem->ToggleAscending();
				this->SortListBySpecs();
				this->UpdateImGuiTableIDs();
			}

			ImGui::SameLine();
			ImGui::AlignTextToFramePadding();
			if (is_second_sorted) ImGui::PushFontBold();
			ImGui::TextColored(text_col, "%s", Translate("EQUIPPED"));
			if (is_second_sorted) ImGui::PopFont();

			bool is_third_sorted = primary_sort == PropertyType::kKitItemCount;
			auto third_icon = is_third_sorted ? sort_icon : ICON_LC_ARROW_UP_DOWN;

			ImGui::SameLine(LayoutItemSize.x / 1.25f);
			ImGui::AlignTextToFramePadding();
			ImGui::TextColored(text_col, "%s", third_icon.c_str());

			if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
				this->sortSystem->SetPrimarySortFilter(PropertyType::kKitItemCount);
				this->sortSystem->ToggleAscending();
				this->SortListBySpecs();
				this->UpdateImGuiTableIDs();
			}

			ImGui::SameLine();
			ImGui::AlignTextToFramePadding();
			if (is_third_sorted) ImGui::PushFontBold();
			ImGui::TextColored(text_col, "%s", Translate("AMOUNT"));
			if (is_third_sorted) ImGui::PopFont();
		}
		
		if (!HasFlag(ModexTableFlag_Kit)) {
				bool is_first_sorted = showFormID ? primary_sort == PropertyType::kFormID : primary_sort == PropertyType::kPlugin;
				auto first_icon = is_first_sorted ? sort_icon : ICON_LC_ARROW_UP_DOWN;

				ImGui::AlignTextToFramePadding();
				ImGui::TextColored(text_col, "%s", first_icon.c_str());

				if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
					this->sortSystem->SetPrimarySortFilter(this->showFormID ? PropertyType::kFormID : PropertyType::kPlugin);
					this->sortSystem->ToggleAscending();
					this->SortListBySpecs();
					this->UpdateImGuiTableIDs();
				}

				ImGui::SameLine();
				ImGui::AlignTextToFramePadding();
				if (is_first_sorted) ImGui::PushFontBold();
				ImGui::TextColored(text_col, "%s", header_plugin.c_str());
				if (is_first_sorted) ImGui::PopFont();

				if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
					this->showFormID = !this->showFormID;

					if (primary_sort == PropertyType::kFormID || primary_sort == PropertyType::kPlugin) {
						this->sortSystem->SetPrimarySortFilter(this->showFormID ? PropertyType::kFormID : PropertyType::kPlugin);
						this->SortListBySpecs();
						this->UpdateImGuiTableIDs();
					}
				}

				bool is_second_sorted = showEditorID ? primary_sort == PropertyType::kEditorID : primary_sort == PropertyType::kName;
				auto second_icon = is_second_sorted ? sort_icon : ICON_LC_ARROW_UP_DOWN;

				ImGui::SameLine(spacing);
				ImGui::AlignTextToFramePadding();
				ImGui::TextColored(text_col, "%s", second_icon.c_str());

				if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
					this->sortSystem->SetPrimarySortFilter(this->showEditorID ? PropertyType::kEditorID : PropertyType::kName);
					this->sortSystem->ToggleAscending();
					this->SortListBySpecs();
					this->UpdateImGuiTableIDs();
				}

				ImGui::SameLine();
				ImGui::AlignTextToFramePadding();
				if (is_second_sorted) ImGui::PushFontBold();
				ImGui::TextColored(text_col, "%s", header_name.c_str());
				if (is_second_sorted) ImGui::PopFont();

				if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
					this->showEditorID = !this->showEditorID;
					if (primary_sort == PropertyType::kEditorID || primary_sort == PropertyType::kName) {
						this->sortSystem->SetPrimarySortFilter(this->showEditorID ? PropertyType::kEditorID : PropertyType::kName);
						this->SortListBySpecs();
						this->UpdateImGuiTableIDs();
					}
				}

				bool is_custom_sorted = (!this->sortSystem->GetUsePrimary());
				auto custom_icon = secondary_sort == PropertyType::kNone ? ICON_LC_ARROW_UP_DOWN : is_custom_sorted ? sort_icon : ICON_LC_ARROW_UP_DOWN;

				ImGui::SameLine(sort_spacing - name_offset);
				ImGui::AlignTextToFramePadding();
				ImGui::TextColored(text_col, "%s", custom_icon.c_str());

				if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
					this->sortSystem->UsePrimary(false);
					this->sortSystem->ToggleAscending();
					this->SortListBySpecs();
					this->UpdateImGuiTableIDs();
				}

				ImGui::SameLine();
				if (is_custom_sorted) ImGui::PushFontBold();
				CustomSortColumn();
				if (is_custom_sorted) ImGui::PopFont();
		}
		
		ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal);
		ImGui::PopStyleColor();
		ImGui::PopID();
	}
	
	// TODO: move this
	static inline bool test_selection = false;
	static inline bool test_filters = false;

	void UITable::Draw(const TableList& _tableList)
	{
		UpdateLayout();
		DrawStatusBar();

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
							ImGui::Selectable("", is_item_selected, ImGuiSelectableFlags_None, ImVec2(LayoutItemSize.x / 2.0f, LayoutItemSize.y));
						} else {
							ImGui::Selectable("", is_item_selected, ImGuiSelectableFlags_None, LayoutItemSize);
						}
						ImGui::PopStyleColor(2);
						ImGui::PopItemFlag();
						ImGui::PopStyleVar();

						if (ImGui::IsItemHovered()) {
							HandleItemHoverPreview(item_data);
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
