#include "UITable.h" 

#include "core/Commands.h"
#include "localization/FontManager.h"
#include "localization/Locale.h"
#include "ui/core/UIManager.h"
#include "ui/components/UICustom.h"
#include "ui/components/ItemPreview.h"
#include "config/EquipmentConfig.h"
#include "config/BlacklistConfig.h"
#include "config/UserConfig.h"
#include "config/UserData.h"
#include "config/ThemeConfig.h"

#include "ui/components/UIModule.h"

#include "pch.h"


// BUG: Kit items are resorted every time the table is refreshed.
// BUG: Inventory table doesn't initialize properly.

namespace Modex
{
	float Pulse(float a_time, float a_frequency, float a_amplitude)
	{
		return a_amplitude * sin(a_time * a_frequency);
	}

	float PulseMinMax(float a_time, float a_frequency, float a_amplitude, float a_min, float a_max)
	{
		return a_min + (a_max - a_min) * (1.0f + Pulse(a_time, a_frequency, a_amplitude)) * 0.5f;
	}

	bool UITable::IsMouseHoveringRectDelayed(const ImVec2& a_min, const ImVec2& a_max)
	{
		if (HasFlag(ModexTableFlag_EnableItemPreviewOnHover))
			return false;

		const auto& g = *GImGui;
		if (ImGui::IsMouseHoveringRect(a_min, a_max)) {
			return (g.MouseStationaryTimer>= ImGui::GetStyle().HoverDelayNormal);
		}

		return false;
	}

	ImU32 GetFormTypeColor(const RE::FormType& a_type)
	{
		auto alpha = ImGui::GetStyle().Alpha;
		switch (a_type) {
		case RE::FormType::Armor:
			return ThemeConfig::GetColorU32("ARMO", alpha);
		case RE::FormType::AlchemyItem:
			return ThemeConfig::GetColorU32("ALCH", alpha);
		case RE::FormType::Ammo:
			return ThemeConfig::GetColorU32("AMMO", alpha);
		case RE::FormType::Book:
			return ThemeConfig::GetColorU32("BOOK", alpha);
		case RE::FormType::Ingredient:
			return ThemeConfig::GetColorU32("INGR", alpha);
		case RE::FormType::KeyMaster:
			return ThemeConfig::GetColorU32("KEYM", alpha);
		case RE::FormType::Misc:
			return ThemeConfig::GetColorU32("MISC", alpha);
		case RE::FormType::Scroll:
			return ThemeConfig::GetColorU32("SCRL", alpha);
		case RE::FormType::Weapon:
			return ThemeConfig::GetColorU32("WEAP", alpha);
		case RE::FormType::NPC:
			return ThemeConfig::GetColorU32("NPC_", alpha);
		case RE::FormType::Tree:
			return ThemeConfig::GetColorU32("TREE", alpha);
		case RE::FormType::Static:
			return ThemeConfig::GetColorU32("STAT", alpha);
		case RE::FormType::Container:
			return ThemeConfig::GetColorU32("CONT", alpha);
		case RE::FormType::Activator:
			return ThemeConfig::GetColorU32("ACTI", alpha);
		case RE::FormType::Light:
			return ThemeConfig::GetColorU32("LIGH", alpha);
		case RE::FormType::Door:
			return ThemeConfig::GetColorU32("DOOR", alpha);
		case RE::FormType::Furniture:
			return ThemeConfig::GetColorU32("FURN", alpha);
		default:
			return IM_COL32(169, 169, 169, 100 * alpha);  // Dark Gray
		}
	}

	std::string TRUNCATE(const std::string& a_text, const float& a_width)
	{
		const float textWidth = ImGui::CalcTextSize(a_text.c_str()).x;

		if (textWidth > a_width) {
			const float ellipsisWidth = ImGui::CalcTextSize("...").x;

			std::string result = "";
			float resultWidth = 0.0f;

			for (const auto& c : a_text) {
				const float charWidth = ImGui::CalcTextSize(std::string(1, c).c_str()).x;

				if (resultWidth + charWidth + ellipsisWidth > a_width) {
					result += "...";
					break;
				}

				result += c;
				resultWidth += charWidth;
			}

			return result;
		}

		return a_text;
	}

	void UITable::Init()
	{
		const auto filename = ConfigManager::FILTER_DIRECTORY / (this->data_id + ".json");

		filterSystem = std::make_unique<FilterSystem>(filename);
		filterSystem->Load(true);
		filterSystem->SetSystemCallback([this]() {
			Refresh();
		});

		sortSystem = std::make_unique<SortSystem>(filename);
		sortSystem->Load(true);

		searchSystem = std::make_unique<SearchSystem>(filename);
		searchSystem->Load(true);
		searchSystem->SetupDefaultKey();

		selectedPlugin = Translate("SHOW_ALL");
	}

	void UITable::Unload()
	{
		tableList.clear();
		recentList.clear();

		pluginList.clear();
		pluginSet.clear();
		selectionStorage.Clear();
		dragDropSourceList.clear();

		itemPreview = nullptr;
		tableTargetRef = nullptr;
	}

	// TEST: Need extreme nullptr safety when operating on references since we can't guarantee
	// lifetime throughout Modex being open. Test and verify that methods calling tableTargetRef
	// are always doing nullptr checks before release!!!
	
	void UITable::Load()
	{
		
		if (useSharedTarget)
		{
			if (auto reference = UIModule::GetTargetReference(); reference) {
				this->SetTargetByReference(reference);
			}
		}

		if (!useSharedTarget) 
		{
			const auto formID = UserData::User().Get<RE::FormID>(data_id + "::LastTargetRef", 0);
			if (const auto reference = UIModule::LookupReferenceByFormID(formID); reference.has_value()) {
				if (*reference) {
					this->SetTargetByReference(*reference);
				}
			}
		}

		BuildPluginList();
		LoadRecentList();
		Refresh();
	}
	
	void UITable::SetTargetByReference(RE::TESObjectREFR* a_reference)
	{
		tableTargetRef = a_reference;

		if (HasFlag(ModexTableFlag_Inventory)) {
			this->Refresh();
		}

		if (useSharedTarget)
		{
			UIModule::SetTargetReference(a_reference);

			// HACK: Cheap. Doesn't work if tables aren't linked. Although currently
			// there is no instance of tables co-existing without drag linkage. This
			// hack was used as an alternative to propogating changes through a
			// hierarchy of Menu, UIModule, and UIManager instances.

			for (const auto source : dragDropSourceList) {
				if (source.second->useSharedTarget) {
					source.second->SetTargetByReference(a_reference);
				}
			}
		}

		if (!useSharedTarget)
		{
			RE::FormID formID = 0;

			if (tableTargetRef) {
				formID = tableTargetRef->formID;
			}

			UserData::User().Set<RE::FormID>(data_id + "::LastTargetRef", formID);
		}
	}

	void UITable::AddItemToRecent(const std::unique_ptr<BaseObject>& a_item)
	{
		UserData::Recent().Add(a_item->GetEditorID());
		updateRecentList = true;
	}

	void UITable::LoadRecentList()
	{
		std::vector<std::string> recentItems;
		recentList.clear();

		UserData::Recent().GetAsList(recentItems);

		if (recentItems.empty())
			return;

		// TEST: Should we keep dummy objects in recent list?
		// BUG: This is still broken. CTD if a dummy object is read due to icon.
		for (const auto& editorID : recentItems) {
			if (RE::TESForm* form = RE::TESForm::LookupByEditorID(editorID)) {
				recentList.emplace_back(std::make_unique<BaseObject>(form, 0, 0));
			} else {
				recentList.emplace_back(std::make_unique<BaseObject>(editorID, editorID, "Unknown", 0));
			}
		}

		for (int i = 0; i < std::ssize(recentList); i++) {
			recentList[i]->m_tableID = i;
		}

		updateRecentList = false;
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
			Commands::AddItemToRefInventory(tableTargetRef, kitItem->m_editorid, static_cast<std::uint32_t>(kitItem->m_amount));
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
			if (itemPreview && itemPreview->GetTESForm()->IsInventoryObject()) {
				Commands::RemoveItemFromInventory(tableTargetRef, itemPreview->GetEditorID(), 1);
				AddItemToRecent(itemPreview);
			}
		} 
		else {
			void* it = NULL;
			ImGuiID id = 0;

			while (selectionStorage.GetNextSelectedItem(&it, &id)) {
				if (id < std::ssize(tableList) && id >= 0) {
					const auto& item = tableList[id];
					Commands::RemoveItemFromInventory(tableTargetRef, item->GetEditorID(), 1);
					AddItemToRecent(item);
				}
			}
		}

		selectionStorage.Clear();
		UpdateActiveInventoryTables();
	}

	void UITable::AddSelectionToTargetInventory(int a_count)
	{
		if (tableList.empty()) 
			return;

		if (!tableTargetRef)
			return;

		if (GetSelectionCount() == 0) {
			if (itemPreview && itemPreview->GetTESForm()->IsInventoryObject()) {
				Commands::AddItemToRefInventory(tableTargetRef, itemPreview->GetEditorID(), a_count);
				AddItemToRecent(itemPreview);
			}
		} 
		else {
			void* it = NULL;
			ImGuiID id = 0;

			while (selectionStorage.GetNextSelectedItem(&it, &id)) {
				if (id < std::ssize(tableList) && id >= 0) {
					const auto& item = tableList[id];
					if (item && item->GetTESForm()->IsInventoryObject()) {
						Commands::AddItemToRefInventory(tableTargetRef, item->GetEditorID(), a_count);
						AddItemToRecent(item);
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
			if (itemPreview && (itemPreview->IsArmor() || itemPreview->IsWeapon())) {
				Commands::AddAndEquipItemToInventory(tableTargetRef, itemPreview->GetEditorID());
				AddItemToRecent(itemPreview);
			}
		}
		else {
			void* it = NULL;
			ImGuiID id = 0;

			while (selectionStorage.GetNextSelectedItem(&it, &id)) {
				if (id < std::ssize(tableList) && id >= 0) {
					const auto& item = tableList[id];
					if (item && (item->IsArmor() || item->IsWeapon())) {
						Commands::AddAndEquipItemToInventory(tableTargetRef, item->GetEditorID());
						AddItemToRecent(item);
					}
				}
			}
		}

		selectionStorage.Clear();
		UpdateActiveInventoryTables();
	}


	void UITable::PlaceSelectionOnGround(int a_count)
	{
		if (tableList.empty()) 
			return;

		if (!tableTargetRef)
			return;

		if (GetSelectionCount() == 0) {
			if (itemPreview && itemPreview->GetTESForm()->HasWorldModel()) {
				Commands::PlaceAtMe(tableTargetRef, itemPreview->GetEditorID(), a_count);
				AddItemToRecent(itemPreview);
			}
		}
		else {
			void* it = NULL;
			ImGuiID id = 0;

			while (selectionStorage.GetNextSelectedItem(&it, &id)) {
				if (id < std::ssize(tableList) && id >= 0) {
					const auto& item = tableList[id];
					if (item && item->GetTESForm()->HasWorldModel()) {
						Commands::PlaceAtMe(tableTargetRef, item->GetEditorID(), a_count);
						AddItemToRecent(item);
					}
				}
			}
		}
	}

	void UITable::AddAll()
	{
		if (tableList.empty())
			return;

		if (!tableTargetRef)
			return;
		
		for (auto& item : tableList) {
			if (item && item->GetTESForm()->IsInventoryObject()) {
				Commands::AddItemToRefInventory(tableTargetRef, item->GetEditorID(), 1);
				AddItemToRecent(item);
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
			if (item && item->GetTESForm()->HasWorldModel()) {
				Commands::PlaceAtMe(tableTargetRef, item->GetEditorID(), 1);
				AddItemToRecent(item);
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

	// OPTIMIZE: Is std::vector<BaseObject> the best fit here?
	void UITable::SetGenerator(std::function<std::vector<BaseObject>()> a_generator)
	{
		generator = a_generator;
	}

	std::vector<std::unique_ptr<BaseObject>> UITable::GetSelection()
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

		if (a_item && a_item->GetTESForm()->IsInventoryObject()) {
			Commands::AddItemToRefInventory(this->tableTargetRef, a_item->GetEditorID(), 1);
			AddItemToRecent(a_item);
		}

		UpdateActiveInventoryTables();
	}

	void UITable::RemovePayloadFromInventory(const std::unique_ptr<BaseObject>& a_item)
	{
		if (!tableTargetRef)
			return;

		if (a_item && a_item->GetTESForm()->IsInventoryObject()) {
			Commands::RemoveItemFromInventory(this->tableTargetRef, a_item->GetEditorID());
			AddItemToRecent(a_item);
		}

		UpdateActiveInventoryTables();
	}

	// NOTE: Kit's are not inventories, they're virtual containers in memory. So we handle them
	// outside the use of Add/RemovePayloadFromTable. Although, maybe they should be containers?

	void UITable::AddPayloadToKit(const std::unique_ptr<BaseObject>& a_item)
	{
		bool has_item = false;
		for (auto& item : tableList) {
			if (item->GetEditorID() == a_item->GetEditorID()) {
				item->kitAmount++;
				has_item = true;
			}
		}

		if (!has_item) {
			tableList.emplace_back(std::make_unique<BaseObject>(*a_item));
		}

		AddItemToRecent(a_item);
	}

	void UITable::AddSelectionToActiveKit()
	{
		if (auto map = dragDropSourceList.find(DragDropHandle::Kit); map != this->dragDropSourceList.end()) {
			const auto& destination = map->second;

			if (GetSelectionCount() == 0)
			{
				if (itemPreview && itemPreview->GetTESForm()->IsInventoryObject()) {
					destination->AddPayloadToKit(itemPreview);
				}
			}
			else {
				void* it = NULL;
				ImGuiID id = 0;

				while (selectionStorage.GetNextSelectedItem(&it, &id)) {
					if (id < tableList.size() && id >= 0) {
						const auto& item = this->tableList[id];

						destination->AddPayloadToKit(item);
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

	// OPTIMIZE: Potential room for optimization if we can perform lookups or something.

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
				this->Refresh();
			}
		}
	}

	void UITable::Refresh()
	{
		selectionStorage.Clear();
		
		if (HasFlag(ModexTableFlag_Inventory)) {
			this->Filter(GetReferenceInventory());
		} else {
			this->Filter(this->generator());
		}

		this->SortListBySpecs();
		this->UpdateImGuiTableIDs();
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

		if (selectedKitPtr && !selectedKitPtr->empty() && HasFlag(ModexTableFlag_Kit)) {
			for (const auto& kit_item : selectedKitPtr->m_items) {
				for (const auto& table_item : tableList) {
					if (table_item->GetEditorID() == kit_item->m_editorid) {
						table_item->kitAmount = kit_item->m_amount;
						table_item->kitEquipped = kit_item->m_equipped;
					}
				}
			}
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

	void UITable::Filter(const std::vector<BaseObject>& a_data)
	{
		tableList.clear();
		tableList.reserve(std::ssize(a_data));
		
		this->generalSearchDirty = false;
		ImFormatString(searchSystem->GetLastSearchBuffer(), IM_ARRAYSIZE(searchSystem->GetLastSearchBuffer()), "%s", searchSystem->GetSearchBuffer());

		if (filterSystem && filterSystem->ShowRecent()) {
			std::vector<std::string> recently_used;
			UserData::Recent().GetAsList(recently_used);
			
			for (const auto& recent_item : recently_used) {
				RE::TESForm* form = RE::TESForm::LookupByEditorID(recent_item);
				tableList.emplace_back(std::make_unique<BaseObject>(form, 0, 0));
			}

			return;
		}

		for (const auto& item : a_data) {
			if (this->HasFlag(ModexTableFlag_Kit) || this->HasFlag(ModexTableFlag_Inventory)) {
				if (auto form = item.GetTESForm(); form) {
					this->tableList.emplace_back(
						std::make_unique<BaseObject>(
							form, 
							item.GetTableID(), 
							item.GetRefID(), 
							item.GetQuantity()
						));	
				} else {
					this->tableList.emplace_back(
						std::make_unique<BaseObject>(
							item.GetName(), 
							item.GetEditorID(), 
							item.GetPluginName(), 
							item.GetTableID(), 
							item.GetQuantity()
						));
				}
				
				continue;
			}

			if (searchSystem->CompareInputToObject(&item) == false) {
				continue;
			}

			// All Mods vs Selected Mod
			if (this->selectedPlugin != Translate("SHOW_ALL") && item.GetPluginName() != this->selectedPlugin) {
				continue;
			}

			// Blacklist
			if (this->selectedPlugin == Translate("SHOW_ALL")) {
				if (const auto& fileOpt = item.GetFile(); fileOpt.has_value()) {
					const auto& file = fileOpt.value();
					if (BlacklistConfig::GetSingleton()->Has(file)) {
						continue;
					}

					// if (BlacklistConfig::GetBlacklist().contains(file)) {
					// 	continue;
					// }
				}
			}

			if (filterSystem && !filterSystem->ShouldShowItem(&item)) {
				continue;
			}

			this->tableList.emplace_back(std::make_unique<BaseObject>(item.GetTESForm(), 0, item.m_refID));
		}
	}

	// TODO: Take another look at this post 2.0 changes. We're not really using the full features of Filtered plugin
	// lists since we changed to the dynamic tree filter node system.

	void UITable::BuildPluginList()
	{
		if (pluginType != Data::PLUGIN_TYPE::kTotal) {
			const auto& config = UserConfig::Get();
			this->pluginList = Data::GetSingleton()->GetFilteredListOfPluginNames(this->pluginType, (Data::SORT_TYPE)config.modListSort);
			this->pluginSet = Data::GetSingleton()->GetModulePluginList(this->pluginType);
			pluginList.insert(pluginList.begin(), Translate("SHOW_ALL"));
		}
	}

	// TODO: Swap over to new Fancy rounded widgets with Icons.
	void UITable::DrawFormSearchBar(const ImVec2& a_size)
	{
		const float input_width = a_size.x;
		const float key_width = a_size.x * 0.30f;

		ImGui::PushStyleColor(ImGuiCol_FrameBg, ThemeConfig::GetColor("TABLE_FIELD_BG"));
		ImGui::PushStyleVar(ImGuiStyleVar_SelectableTextAlign, ImVec2(0.5f, 0.5f));

		const SearchItem current_key = searchSystem->GetSearchKey();
		const std::string current_key_text = searchSystem->GetSearchKeyString(current_key);

		ImGui::SetNextItemWidth(key_width);
		if (ImGui::BeginCombo("##Search::Input::SearchKey", current_key_text.c_str(), ImGuiComboFlags_NoArrowButton)) {
			for (SearchItem key : searchSystem->GetAvailableKeys()) {
				const std::string& key_text = searchSystem->GetSearchKeyString(key);
				if (ImGui::Selectable(key_text.c_str(), current_key == key)) {
					searchSystem->SetSearchKey(key);
					this->Refresh();
				}
				
				if (current_key == key) {
					ImGui::SetItemDefaultFocus();
				}
			}
			
			ImGui::EndCombo();
		}

		// ImGui::SetItemTooltip(Translate("TABLE_KEY_TOOLTIP"));
		if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal | ImGuiHoveredFlags_NoSharedDelay)) {
			UICustom::FancyTooltip(Translate("TABLE_KEY_TOOLTIP"));
		}
		
		ImGui::SameLine();
		ImGui::AlignTextToFramePadding();
		ImGui::Text(" " ICON_LC_ARROW_LEFT_RIGHT " ");
		if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort | ImGuiHoveredFlags_NoSharedDelay)) {
			UICustom::FancyTooltip("TABLE_SEARCH_TOOLTIP")	;
		}
		ImGui::SameLine();
		
		bool _change = false;
		ImGui::SetNextItemWidth(input_width);
		if (ImGui::InputTextWithHint("##Search::Input::CompareField", Translate("TABLE_SEARCH_HINT"), searchSystem->GetSearchBuffer(), 256, ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_EnterReturnsTrue)) {
			this->Refresh();
			_change = true;
		}

		if (ImGui::IsItemDeactivated() && !_change) {
			const char* searchBuffer = searchSystem->GetSearchBuffer();
			if (searchBuffer && std::strcmp(searchSystem->GetLastSearchBuffer(), searchBuffer) != 0) {
				ImFormatString(searchSystem->GetSearchBuffer(), IM_ARRAYSIZE(searchSystem->GetLastSearchBuffer()), "%s", searchSystem->GetLastSearchBuffer());
			}
		}
		
		if (ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_F, ImGuiInputFlags_RouteFromRootWindow)) {
			ImGui::SetKeyboardFocusHere(-1);
		}
		
		ImGui::SameLine();
		
		ImGui::PopStyleColor();
		ImGui::PopStyleVar();
	}

	void UITable::DrawPluginSearchBar(const ImVec2& a_size)
	{
		ImGui::PushStyleColor(ImGuiCol_FrameBg, ThemeConfig::GetColor("TABLE_FIELD_BG"));

		if (searchSystem->InputTextComboBox("##Search::Filter::PluginField", this->pluginSearchBuffer, this->selectedPlugin, IM_ARRAYSIZE(this->pluginSearchBuffer), this->pluginList, a_size.x, false)) {
			this->selectedPlugin = this->pluginSearchBuffer;
			this->pluginSearchBuffer[0] = '\0';
			
			this->selectionStorage.Clear();
			this->Refresh();
		}
		
		ImGui::PopStyleColor();
	}

	void UITable::UpdateLayout()
	{
		const auto& userdata = UserData::User();
		const auto& config = UserConfig::Get();

		styleHeight = userdata.Get<float>("Modex::Table::ItemHeight", 0.0f);
		styleWidth = userdata.Get<float>("Modex::Table::ItemWidth", 0.0f);
		styleSpacing = userdata.Get<float>("Modex::Table::ItemSpacing", 0.0f);
		styleFontSize = userdata.Get<float>("Modex::Table::FontSize", 0.0f); 
		showAltRowBG = userdata.Get<bool>("Modex::Table::ShowAltRowBG", true);
		showItemIcon = userdata.Get<bool>("Modex::Table::ShowItemIcon", true);

		if (styleFontSize == 0.0f) {
			styleFontSize = config.globalFontSize;
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
		const int table_size = filterSystem->ShowRecent() ? recentList.size() : tableList.size();
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

	void UITable::DrawSearchBar()
	{
		// ImGui::SameLine();
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
		const ImVec2 window_padding = ImGui::GetStyle().WindowPadding;
		const float frame_height = ImGui::GetFrameHeight();

		const float search_width = ImGui::GetContentRegionAvail().x / 3.0f;
		DrawFormSearchBar(ImVec2(search_width, 0.0f));

		ImGui::SameLine();

		ImGui::AlignTextToFramePadding();
		ImGui::Text(" " ICON_LC_ARROW_RIGHT_TO_LINE " ");
		if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort | ImGuiHoveredFlags_NoSharedDelay)) {
			UICustom::FancyTooltip("TABLE_PLUGIN_TOOLTIP");
		}
		ImGui::SameLine();
		
		const float plugin_width = ImGui::GetContentRegionAvail().x - window_padding.x - frame_height;
		DrawPluginSearchBar(ImVec2(plugin_width, 0.0f));
		ImGui::PopStyleVar();


	}

	void UITable::DrawKit(const Kit& a_kit, const ImVec2& a_pos, const bool& a_selected)
	{
		(void)a_selected;  // If we want to handle selection visuals manually.

		const auto& DrawList = ImGui::GetWindowDrawList();
		const float fontSize = ImGui::GetFontSize(); 

		// Setup box and bounding box for positioning and drawing.
		const ImVec2 box_min(a_pos.x - 1, a_pos.y - 1);
		const ImVec2 box_max(box_min.x + LayoutItemSize.x + 2, box_min.y + LayoutItemSize.y + 2);  // Dubious
		ImRect bb(box_min, box_max);

		// Outline & Background
		const float global_alpha = ImGui::GetStyle().Alpha;
		const ImU32 bg_color = ThemeConfig::GetColorU32("TABLE_ROW_BG", global_alpha);
		const ImU32 bg_color_alt = ThemeConfig::GetColorU32("TABLE_ROW_BG_ALT", global_alpha);
		const ImU32 outline_color = ThemeConfig::GetColorU32("TABLE_ROW_BORDER", global_alpha);
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

	// TODO: Try adding that + or - or Trash icon background thing again.
	void UITable::HandleDragDropBehavior()
	{

		static std::string tooltip_icon = "";
		static std::string tooltip_string = "";

		// The UITable Widget rectangle acts as the drop target.
		if (ImGui::BeginDragDropTarget()) {
			for (auto& source : dragDropSourceList) {
				const auto handle = source.first;
				const auto ptr = source.second;

				const auto handle_string = GetDragDropHandleText(handle);
				const auto destination = this;
				const auto origin = ptr;

				// Peak and assign tooltip icon. Handle payload dropping next.
				tooltip_icon = "";
				if (ImGui::AcceptDragDropPayload(handle_string, ImGuiDragDropFlags_AcceptPeekOnly)) {
					if (destination->GetDragDropHandle() == DragDropHandle::Kit) {
						tooltip_icon = ICON_LC_PLUS;
						tooltip_string = Translate("KIT_ADD");
					}

					if (destination->GetDragDropHandle() == DragDropHandle::Inventory) {
						tooltip_icon = ICON_LC_PLUS;
						tooltip_string = Translate("ADD_SELECTION");
					}

					if (destination->GetDragDropHandle() == DragDropHandle::Table) {
						tooltip_icon = ICON_LC_X;

						if (origin->GetDragDropHandle() == DragDropHandle::Kit) {
							tooltip_string = Translate("KIT_REMOVE");
						}

						if (origin->GetDragDropHandle() == DragDropHandle::Inventory) {
							tooltip_string = Translate("REMOVE_SELECTION");
						}
					}

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

					DrawList->AddRectFilled(min, max, ThemeConfig::GetColorU32("TABLE_FIELD_BG"));

					DrawList->AddText(ImGui::GetFont(), font_size,
							min + (table_size / 2.0f) - (icon_size / 2.0f) - ImVec2(0, center_offset),
							ThemeConfig::GetColorU32("TEXT"),
							tooltip_icon.c_str());

					ImGui::PushFont(NULL, font_size / 2.0f);
					const ImVec2 string_size = ImGui::CalcTextSize(tooltip_string.c_str());
					ImGui::PopFont();

					DrawList->AddText(ImGui::GetFont(), font_size / 2.0f,
							min + (table_size / 2.0f) - (string_size / 2.0f) + ImVec2(0, icon_size.y / 1.5f) - ImVec2(0, center_offset),
							ThemeConfig::GetColorU32("TEXT", 0.75f),
							tooltip_string.c_str());
				}

				// Payload Data is contained in other table sources.
				if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(handle_string); payload && payload->IsDelivery()) {
					const int payloadCount = (int)payload->DataSize / (int)sizeof(ImGuiID);

					std::vector<std::unique_ptr<BaseObject>> payload_items;
					for (int payload_idx = 0; payload_idx < payloadCount; ++payload_idx) {
						const ImGuiID payloadID = ((ImGuiID*)payload->Data)[payload_idx];
						const auto& item = (*ptr->GetTableListPtr())[payloadID];
						payload_items.emplace_back(std::make_unique<BaseObject>(*item));
					}


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

						if (destination->GetDragDropHandle() == DragDropHandle::Inventory) {
							destination->AddPayloadToInventory(item);
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
			}
		}

		if (ImGui::GetDragDropPayload()) { // Handle Tooltips outside of widget targets!
			DrawDragDropPayload(tooltip_icon);
		}
	}

	// BUG: Issue with auto sizing item preview card, or just item preview card in general.
	void UITable::HandleItemHoverPreview(const std::unique_ptr<BaseObject>& a_item)
	{
		itemPreview = std::make_unique<BaseObject>(*a_item);
		if (HasFlag(ModexTableFlag_EnableItemPreviewOnHover)) {
			if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort | ImGuiHoveredFlags_NoSharedDelay)) {
				ImGui::SetNextWindowSize(ImVec2(300.0f, 0));
				ImGui::SetNextWindowPos(ImVec2(ImGui::GetMousePos().x - 300.0f, ImGui::GetMousePos().y));
				if (ImGui::BeginTooltip()) {
					if (a_item->IsDummy()) {
						ShowMissingPlugin(a_item);
					} else {
						ShowItemPreview(a_item);
					}

					ImGui::EndTooltip();
				}
			}
		}
	}

	void UITable::HandleLeftClickBehavior(const std::unique_ptr<BaseObject>& a_item)
	{		
		if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
			if (a_item->IsItem()) {
				Commands::AddItemToPlayerInventory(a_item->GetEditorID(), 1);
			}

			// TODO: Poll whether this is expected behavior or not. Should we always spawn on the
			// player, or spawn on the targeted ref if available?

			if (tableTargetRef && a_item->IsNPC()) {
				Commands::PlaceAtMe(tableTargetRef, a_item->GetEditorID(), 1);
			}

			this->AddItemToRecent(a_item);
		}
	}

	void UITable::HandleRightClickBehavior(const std::unique_ptr<BaseObject>& a_item)
	{

		static const int click_amount = 1; // TODO revisit this

		if (ImGui::IsItemClicked(ImGuiMouseButton_Right)) {
			if (!selectionStorage.Contains(a_item->m_tableID)) {
				selectionStorage.Clear();
			}
			ImGui::OpenPopup("TableViewContextMenu");
		}

		if (ImGui::BeginPopup("TableViewContextMenu")) {
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
						this->AddSelectionToTargetInventory(click_amount); 
					}

					if (a_item->IsArmor() || a_item->IsWeapon()) {
						if (ImGui::MenuItem(Translate("EQUIP_SELECTION"))) {
							this->EquipSelectionToTarget();
						}
					}

					if (ImGui::MenuItem(Translate("PLACE_SELECTION"))) {
						this->PlaceSelectionOnGround(click_amount);
					}

					ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal);

					if (a_item->GetFormType() == RE::FormType::Book) {
						if (GetSelectionCount() <= 1) {
							if (ImGui::MenuItem(Translate("GENERAL_READ_ME"))) {
								Commands::ReadBook(a_item->GetEditorID());
								UIManager::GetSingleton()->Close();
								AddItemToRecent(a_item);
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
						AddItemToRecent(a_item);
						UIManager::GetSingleton()->Close();
					}

					if (ImGui::MenuItem(Translate("BRING_NPC_REFERENCE"))) {
						Commands::TeleportNPCToPlayer(a_item->m_refID);
						AddItemToRecent(a_item);
						UIManager::GetSingleton()->Close();
					}

					if (a_item->IsDisabled()) {
						if (ImGui::MenuItem(Translate("ENABLE_NPC_REFERENCE"))) {
							if (auto* target = RE::TESForm::LookupByID<RE::Actor>(a_item->m_refID); target != nullptr) {
								Commands::EnableRefr(target, false);
								AddItemToRecent(a_item);
							}
						}
					} else {
						if (ImGui::MenuItem(Translate("DISABLE_NPC_REFERENCE"))) {
							if (auto* target = RE::TESForm::LookupByID<RE::Actor>(a_item->m_refID); target != nullptr) {
								Commands::DisableRefr(target);
								AddItemToRecent(a_item);
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
		// TODO why the fuck is ImGuiEscape causing tab bug?
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

	void UITable::DrawKitItem(const std::unique_ptr<BaseObject>& a_item, const ImVec2& a_pos)
	{
		if (!a_item || a_item.get() == nullptr) {
			return;
		}

		const auto& DrawList = ImGui::GetWindowDrawList();

		// Setup box and bounding box for positioning and drawing.
		const ImVec2 box_min(a_pos.x - 1, a_pos.y - 1);
		const ImVec2 box_max(box_min.x + LayoutItemSize.x + 2, box_min.y + LayoutItemSize.y + 2);  // Dubious
		ImRect bb(box_min, box_max);

		// Outline & Background
		const float global_alpha = ImGui::GetStyle().Alpha;
		const ImU32 bg_color = ThemeConfig::GetColorU32("TABLE_ROW_BG", global_alpha);
		const ImU32 bg_color_alt = ThemeConfig::GetColorU32("TABLE_ROW_BG_ALT", global_alpha);
		const ImU32 outline_color = ThemeConfig::GetColorU32("TABLE_ROW_BORDER", global_alpha);
		const ImU32 text_color = ThemeConfig::GetColorU32("TEXT", global_alpha);
		const ImU32 err_color = ThemeConfig::GetColorU32("ERROR", global_alpha);

		if (a_item->m_tableID % 2 == 0) {
			DrawList->AddRectFilled(bb.Min, bb.Max, bg_color);
		} else {
			DrawList->AddRectFilled(bb.Min, bb.Max, bg_color_alt);
		}

		if (a_item->IsDummy()) {
			DrawList->AddRectFilled(bb.Min, bb.Max, err_color);
		}

		// Outline
		DrawList->AddRect(bb.Min, bb.Max, outline_color, 0.0f, 0, 1.0f);

		// Type Color Identifier
		const float type_pillar_width = 5.0f;
		DrawList->AddRectFilled(
			ImVec2(bb.Min.x + LayoutOuterPadding, bb.Min.y + LayoutOuterPadding),
			ImVec2(bb.Min.x + LayoutOuterPadding + type_pillar_width, bb.Max.y - LayoutOuterPadding),
			GetFormTypeColor(a_item->GetFormType()));

		// Type Pillar tooltip
		if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows)) {
			if (IsMouseHoveringRectDelayed(
				ImVec2(bb.Min.x + LayoutOuterPadding, bb.Min.y + LayoutOuterPadding),
				ImVec2(bb.Min.x + LayoutOuterPadding + type_pillar_width, bb.Max.y - LayoutOuterPadding))) {
				ImGui::SetTooltip("%s", RE::FormTypeToString(a_item->GetFormType()).data());
			}
		}
	

		// We need to adjust the bounding box to account for the type pillar.
		bb.Min.x += type_pillar_width * 2.0f;
		float spacing = LayoutItemSize.x / 6.0f;

		const float fontSize = ImGui::GetFontSize();
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

		std::string name_string = a_item->GetName().empty() ? a_item->GetEditorID() : a_item->GetName();
		float name_offset = 5.0f;  // initial offset from pillar

		if (showEditorID) {
			name_string = TRUNCATE(a_item->GetEditorID(), spacing * 1.5f);
		} else {
			name_string = TRUNCATE(name_string, spacing * 1.5f);
		}

		const std::string plugin_icon = showItemIcon ? a_item->GetItemIcon() : "";
		const std::string plugin_name = TRUNCATE(plugin_icon + a_item->GetPluginName(), ((spacing * 1.5f) - fontSize * 2.0f) - name_offset);
		DrawList->AddText(center_left_align, text_color, plugin_name.c_str());

		const ImVec2 name_pos = ImVec2(bb.Min.x + (spacing * 1.5f) - name_offset, center_align);
		DrawList->AddText(name_pos, text_color, name_string.c_str());

		if (a_item->IsDummy()) {
			return;
		}
		
		ImVec2 equippable_pos = ImVec2(a_pos.x + (LayoutItemSize.x / 1.50f), a_pos.y);

		ImGui::SetCursorScreenPos(equippable_pos);

		const auto alpha = a_item->kitEquipped ? global_alpha * 1.0f : global_alpha * 0.25f;
		const auto text = a_item->kitEquipped ? Translate("EQUIPPED") : Translate("EQUIP");
		const auto icon = a_item->kitEquipped ? ICON_LC_CHECK : ICON_LC_X;
		const auto equip_size = ImVec2(LayoutItemSize.x / 7.0f, LayoutItemSize.y);
		const auto color = ThemeConfig::GetColorU32("BUTTON", alpha);
		const auto padding = 5.0f;

		ImGui::PushStyleColor(ImGuiCol_Button, color);
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0.0f, 0.0f));
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f));

		if (a_item->GetFormType() == RE::FormType::Armor || a_item->GetFormType() == RE::FormType::Weapon) {
			if (ImGui::Button((std::string(icon) + text).c_str(), equip_size)) {
				a_item->kitEquipped = !a_item->kitEquipped;
				this->SyncChangesToKit();
			}
			
			ImGui::SameLine(0.0f, padding);
		} else {
			const auto form_type_text = RE::FormTypeToString(a_item->GetFormType());
			ImGui::BeginDisabled();
			ImGui::Button(form_type_text.data(), equip_size);
			ImGui::EndDisabled();
			ImGui::SameLine(0.0f, padding);
		}


		ImGui::PopStyleColor();
		ImGui::PopStyleVar(2);

		ImGui::SetCursorPosY(ImGui::GetCursorPosY() + LayoutOuterPadding);

		if (!a_item->IsDummy()) {
			ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - LayoutOuterPadding);
			if (ImGui::InputInt("##EquipCount", &a_item->kitAmount, 1, 10)) {
				this->SyncChangesToKit();
			}
			
			if (ImGui::IsItemHovered(ImGuiHoveredFlags_Stationary)) {
				ImGui::SetTooltip("%s", Translate("ADD_AMOUNT_TOOLTIP"));
			}
		}
	}

	void UITable::DrawItem(const std::unique_ptr<BaseObject>& a_item, const ImVec2& a_pos)
	{
		const auto& DrawList = ImGui::GetWindowDrawList();
		const float fontSize = ImGui::GetFontSize();

		// Setup box and bounding box for positioning and drawing.
		const ImVec2 box_min(a_pos.x, a_pos.y);
		const ImVec2 box_max(box_min.x + LayoutItemSize.x, box_min.y + LayoutItemSize.y);
		ImRect bb(box_min, box_max);

		// Outline & Background
		const float global_alpha = ImGui::GetStyle().Alpha;
		const ImU32 bg_color = ThemeConfig::GetColorU32("TABLE_ROW_BG", global_alpha);
		const ImU32 bg_color_alt = ThemeConfig::GetColorU32("TABLE_ROW_BG_ALT", global_alpha);
		const ImU32 outline_color = ThemeConfig::GetColorU32("TABLE_ROW_BORDER", global_alpha);
		const ImU32 text_color = ThemeConfig::GetColorU32("TEXT", global_alpha);
		const ImU32 ench_color = ThemeConfig::GetColorU32("TEXT_ENCHANTED", global_alpha);
		const ImU32 err_color = ThemeConfig::GetColorU32("ERROR", global_alpha);
		bool is_enchanted = false;

		// Background
		if (showAltRowBG) {
			if (a_item->m_tableID % 2 == 0) {
				DrawList->AddRectFilled(bb.Min, bb.Max, bg_color_alt);
			} else {
				DrawList->AddRectFilled(bb.Min, bb.Max, bg_color);
			}
		} else {
			DrawList->AddRectFilled(bb.Min, bb.Max, bg_color);
		}

		// Invalid / Missing plugin indicator
		if (a_item->IsDummy()) {
			DrawList->AddRectFilled(bb.Min, bb.Max, err_color);
		}

		// Outline
		DrawList->AddRect(bb.Min, bb.Max, outline_color, 0.0f, 0, 1.0f);

		// Type Color Pillar Identifier
		const float type_pillar_width = 5.0f;
		DrawList->AddRectFilled(
			ImVec2(bb.Min.x + LayoutOuterPadding, bb.Min.y + LayoutOuterPadding),
			ImVec2(bb.Min.x + LayoutOuterPadding + type_pillar_width, bb.Max.y - LayoutOuterPadding),
			GetFormTypeColor(a_item->GetFormType()));

		// Type Pillar tooltip
		if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows)) {
			if (IsMouseHoveringRectDelayed(
				ImVec2(bb.Min.x + LayoutOuterPadding, bb.Min.y + LayoutOuterPadding),
				ImVec2(bb.Min.x + LayoutOuterPadding + type_pillar_width, bb.Max.y - LayoutOuterPadding))) {
				ImGui::SetTooltip("%s", RE::FormTypeToString(a_item->GetFormType()).data());
			}
		}

		// We need to adjust the bounding box to account for the type pillar.
		bb.Min.x += type_pillar_width * 2.0f;

		float spacing = (LayoutColumnWidth / 4.0f);
		float padding = ImGui::GetFrameHeight();
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

		const std::string quantity_string = a_item->GetQuantity() > 1 ? std::format(" ({})", std::to_string(a_item->GetQuantity())) : "";
		const std::string item_icon = showItemIcon ? (a_item->GetItemIcon() + " ").c_str() : "";
		const float quantity_offset = ImGui::CalcTextSize(quantity_string.c_str()).x;

		const std::string raw_name = showEditorID ? a_item->GetEditorID() : a_item->GetName();
		const std::string name_string = TRUNCATE(item_icon + raw_name, (spacing * 1.5f) - padding - quantity_offset) + quantity_string;

		if (a_item->GetFormType() == RE::FormType::NPC) {
			if (const auto& npc = a_item->GetTESNPC(); npc.has_value()) {
				const auto& npcData = npc.value();

				if (npcData != nullptr) {
					// const ImU32 unique_color = IM_COL32(255, 179, 102, 20);
					// const ImU32 essential_color = IM_COL32(204, 153, 255, 20);

					const ImVec2 unique_pos = ImVec2(bb.Min.x + spacing + spacing, center_align);
					const ImVec2 essential_pos = ImVec2(unique_pos.x + fontSize + 2.0f, unique_pos.y);
					// ImVec2 disabled_pos = unique_pos;

					// TODO: Pending re-write
					// const std::string unique_string = Utils::IconMap["UNIQUE"];
					// const std::string essential_string = Utils::IconMap["ESSENTIAL"];
					// const std::string disabled_string = Utils::IconMap["FAILURE"];

					// if (npcData->IsUnique() && !npcData->IsEssential()) {
					// 	disabled_pos = ImVec2(unique_pos.x + fontSize + 2.0f, unique_pos.y);
					// 	DrawList->AddText(unique_pos, text_color, unique_string.c_str());
					// 	DrawList->AddRectFilledMultiColor(
					// 		ImVec2(bb.Min.x, bb.Min.y),
					// 		ImVec2(bb.Max.x, bb.Max.y),
					// 		unique_color,
					// 		unique_color,
					// 		IM_COL32(0, 0, 0, 0),
					// 		IM_COL32(0, 0, 0, 0));
					// } else if (npcData->IsEssential() && !npcData->IsUnique()) {
					// 	disabled_pos = ImVec2(unique_pos.x + fontSize + 2.0f, unique_pos.y);
					// 	DrawList->AddText(unique_pos, text_color, essential_string.c_str());
					// 	DrawList->AddRectFilledMultiColor(
					// 		ImVec2(bb.Min.x, bb.Min.y),
					// 		ImVec2(bb.Max.x, bb.Max.y),
					// 		essential_color,
					// 		essential_color,
					// 		IM_COL32(0, 0, 0, 0),
					// 		IM_COL32(0, 0, 0, 0));
					// } else if (npcData->IsEssential() && npcData->IsUnique()) {
					// 	disabled_pos = ImVec2(essential_pos.x + fontSize + 2.0f, unique_pos.y);
					// 	DrawList->AddText(unique_pos, text_color, unique_string.c_str());
					// 	DrawList->AddText(essential_pos, text_color, essential_string.c_str());
					// 	DrawList->AddRectFilledMultiColor(
					// 		ImVec2(bb.Min.x, bb.Min.y),
					// 		ImVec2(bb.Max.x, bb.Max.y),
					// 		unique_color,
					// 		unique_color,
					// 		essential_color,
					// 		essential_color);
					// }

					// if (a_item.IsDisabled()) {
					// 	DrawList->AddText(disabled_pos, text_color, disabled_string.c_str());
						
					// 	if (ImGui::IsMouseHoveringRectDelayed(disabled_pos, ImVec2(disabled_pos.x + fontSize, disabled_pos.y + fontSize))) {
					// 		// ImGui::SetTooltip(Translate("TOOLTIP_DISABLED"));
					// 		UICustom::AddFancyTooltip(Translate("TOOLTIP_DISABLED"));
					// 	}
					// }

					if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows)) {
						if (IsMouseHoveringRectDelayed(unique_pos, ImVec2(unique_pos.x + fontSize, unique_pos.y + fontSize))) {
							if (npcData->IsUnique()) {
								// ImGui::SetTooltip(Translate("TOOLTIP_UNIQUE"));
								UICustom::FancyTooltip(Translate("TOOLTIP_UNIQUE"));
							} else if (npcData->IsEssential()) {
								// ImGui::SetTooltip(Translate("TOOLTIP_ESSENTIAL"));
								UICustom::FancyTooltip(Translate("TOOLTIP_ESSENTIAL"));
							}
						}

						if (IsMouseHoveringRectDelayed(essential_pos, ImVec2(essential_pos.x + fontSize, essential_pos.y + fontSize))) {
							if (npcData->IsEssential()) {
								// ImGui::SetTooltip(Translate("TOOLTIP_ESSENTIAL"));
								UICustom::FancyTooltip(Translate("TOOLTIP_ESSENTIAL"));
							}
						}
					}

					// if (a_item.m_refID != 0) {
					// 	name_string = TRUNCATE(Utils::IconMap["REFID"] + name_string, spacing);
					// }
				}
			}
		}

		const PropertyType& sort_property = this->sortSystem->GetCurrentSortFilter().GetPropertyType();
		const ImVec2 sort_pos = ImVec2(bb.Min.x + spacing * 2.5f, center_align);
		const float sort_text_cutoff = spacing * 1.5f;


		if (const auto& item = a_item; item->IsItem()) {
			const std::string value_string = std::to_string(item->GetGoldValue()) + " " + ICON_LC_COINS;
			const float value_width = ImGui::CalcTextSize(value_string.c_str()).x;
			const ImVec2 value_pos = ImVec2(center_right_align.x - value_width, center_right_align.y);
			DrawList->AddText(value_pos, text_color, value_string.c_str());

			if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows)) {
				if (IsMouseHoveringRectDelayed(value_pos, ImVec2(value_pos.x + value_width, value_pos.y + fontSize))) {
					UICustom::FancyTooltip(FilterProperty::GetIconTooltipKey(PropertyType::kGoldValue).c_str());
				}
			}

			if (const auto& armor = item->GetTESArmor(); armor.has_value()) {
				const auto& armorData = armor.value();

				if (armorData != nullptr) {
					const std::string rating_string = item->GetPropertyValueWithIcon(PropertyType::kArmorRating);
					const std::string type_string = TRUNCATE(item->GetPropertyValueWithIcon(PropertyType::kArmorType), sort_text_cutoff);

					if (armorData->formEnchanting != nullptr) {
						is_enchanted = true;
					}

					if (sort_property == PropertyType::kNone) {
						DrawList->AddText(sort_pos, text_color, rating_string.c_str());
						if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows)) {
							if (IsMouseHoveringRectDelayed(sort_pos, ImVec2(sort_pos.x + fontSize, sort_pos.y + fontSize))) {
								UICustom::FancyTooltip(FilterProperty::GetIconTooltipKey(PropertyType::kArmorRating).c_str());
							}
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
							DrawList->AddText(sort_pos, text_color, skill_string.c_str());
						}
						
						if (teaches_spell || book->GetSpell() != nullptr) {
							const std::string spell_string = TRUNCATE(item->GetPropertyValueWithIcon(PropertyType::kTomeSpell), sort_text_cutoff);
							DrawList->AddText(sort_pos, text_color, spell_string.c_str());
						}
						
						if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows)) {
							if (IsMouseHoveringRectDelayed(sort_pos, ImVec2(sort_pos.x + fontSize, sort_pos.y + fontSize))) {
								if (teaches_skill) {
									UICustom::FancyTooltip(FilterProperty::GetIconTooltipKey(PropertyType::kTomeSkill).c_str());
								} else if (teaches_spell) {
									UICustom::FancyTooltip(FilterProperty::GetIconTooltipKey(PropertyType::kTomeSpell).c_str());
								}
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
						is_enchanted = true;
					}
					
					if (sort_property == PropertyType::kNone) {
						DrawList->AddText(sort_pos, text_color, damage_string.c_str());
						if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows)) {
							if (IsMouseHoveringRectDelayed(sort_pos, ImVec2(sort_pos.x + fontSize, sort_pos.y + fontSize))) {
								UICustom::FancyTooltip(FilterProperty::GetIconTooltipKey(PropertyType::kWeaponDamage).c_str());
							}
						}
					}
				}
			}
		}

		const std::string plugin_name = TRUNCATE(a_item->GetPluginName(), spacing - padding);
		DrawList->AddText(center_left_align, text_color, plugin_name.c_str());

		const ImVec2 name_pos = ImVec2(bb.Min.x + spacing - 5.0f, center_align);
		DrawList->AddText(name_pos, is_enchanted ? ench_color : text_color, name_string.c_str());

		if (sort_property != PropertyType::kPlugin and sort_property != PropertyType::kName and sort_property != PropertyType::kGoldValue) {
			const std::string sort_text = TRUNCATE(a_item->GetPropertyValueWithIcon(sort_property), spacing * 0.75f);
			DrawList->AddText(sort_pos, text_color, sort_text.c_str());
			
			if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows)) {
				if (IsMouseHoveringRectDelayed(sort_pos, ImVec2(sort_pos.x + ImGui::CalcTextSize(sort_text.c_str()).x, sort_pos.y + fontSize))) {
					UICustom::FancyTooltip(Translate(FilterProperty::GetIconTooltipKey(sort_property).c_str()));
				}
			}
		}
	}

	void UITable::DrawDragDropPayload(const std::string& a_icon)
	{
		const auto payload = ImGui::GetDragDropPayload();
		if (payload && payload->IsDataType(GetDragDropHandleText(dragDropHandle))) {
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
				DrawList->AddText(start_pos + ImVec2(icon_x, icon_y), ThemeConfig::GetColorU32("TEXT"), a_icon.c_str());
				ImGui::PopFont();

				ImGui::EndTooltip();
			}
		}
	}

	void UITable::DrawTableSettingsPopup()
	{
		auto& user = UserData::User();

		if (UICustom::Popup_MenuHeader(Translate("SETTINGS"))) {
			ImGui::CloseCurrentPopup();
		}
		
		const float width = ImGui::GetContentRegionAvail().x;
		ImGui::PushStyleVar(ImGuiStyleVar_SeparatorTextAlign, ImVec2(0.5f, 0.5f));

		float font_size = styleFontSize;
		ImGui::SeparatorText(Translate("TABLE_SETTINGS_FONT_SIZE"));
		ImGui::SetNextItemWidth(width);
		if (ImGui::SliderFloat("##Slider::FontSize", &font_size, 0.0f, 48.0f, "%.f")) {
			user.Set<float>("Modex::Table::FontSize", font_size);
			styleFontSize = font_size;
		}

		float item_height =  styleHeight; 
		ImGui::SeparatorText(Translate("TABLE_SETTINGS_HEIGHT"));
		ImGui::SetNextItemWidth(width);
		if (ImGui::SliderFloat("##Slider::ItemHeight", &item_height, -20.0f, 20.0f, "%.2f")) {
			user.Set<float>("Modex::Table::ItemHeight", item_height);
			styleHeight = item_height;
		}

		float item_width = styleWidth;
		ImGui::SeparatorText(Translate("TABLE_SETTINGS_WIDTH"));
		ImGui::SetNextItemWidth(width);
		if (ImGui::SliderFloat("##Slider::ItemWidth", &item_width, -300.0f, 300.0f, "%.2f")) {
			user.Set<float>("Modex::Table::ItemWidth", item_width);
			styleWidth = item_width;
		}

		float item_spacing = styleSpacing;
		ImGui::SeparatorText(Translate("TABLE_SETTINGS_SPACING"));
		ImGui::SetNextItemWidth(width);
		if (ImGui::SliderFloat("##Slider::ItemSpacing", &item_spacing, -20.0f, 20.0f, "%.2f")) {
			user.Set<float>("Modex::Table::ItemSpacing", item_spacing);
			styleSpacing = item_spacing;
		}

		ImGui::SeparatorText(Translate("TABLE_SETTINGS_SHOW_BG"));
		if (UICustom::ToggleButton("##Button::AltRowBG", showAltRowBG, width)) {
			user.Set<bool>("Modex::Table::ShowAltRowBG", showAltRowBG);
		}

		ImGui::SeparatorText(Translate("TABLE_SETTINGS_SHOW_ICON"));
		if (UICustom::ToggleButton("##Button::ItemIcon", showItemIcon, width)) {
			user.Set<bool>("Modex::Table::ShowItemIcon", showItemIcon);
		}

		ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal);
		const bool reset = ImGui::Selectable("Reset");

		if (reset) {
			user.Set<float>("Modex::Table""FontSize", 0.0f); 
			user.Set<float>("Modex::Table::ItemHeight", 0.0f);
			user.Set<float>("Modex::Table::ItemWidth", 0.0f);
			user.Set<float>("Modex::Table::ItemSpacing", 0.0f);
			user.Set<bool>("Modex::Table::ShowAltRowBG", true);
			user.Set<bool>("Modex::Table::ShowItemIcon", true);
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
			status = Translate("ERROR_MISSING_REFERENCE");
		} else {
			if (!tableTargetRef) {
				warn_text = Translate("ERROR_INVALID_REFERENCE");
			} else {
				user_text = std::format("({:08X}) - '{}'", tableTargetRef->GetFormID(), tableTargetRef->GetName());

				if (tableTargetRef->IsPlayerRef()) {
					user_color = ThemeConfig::GetColor("CONTAINER_BUTTON");
				} else {
					user_color = ThemeConfig::GetColor("BUTTON");
				}
			}
		}

		bool user_clicked   = false;
		bool search_clicked = false;
		bool warn_clicked   = false;

		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f));
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(ImGui::GetFontSize(), 3.0f));
		ImGui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign, ImVec2(0.0f, 0.5f));
		ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal, 2.0f);
		
		ImGui::SameLine();

		ImGui::PushFontBold(ImGui::GetFontSize());
		ImGui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign, ImVec2(0.5f, 0.5f));
		const auto user_height = ImGui::GetFrameHeightWithSpacing();
		const float padding = ImGui::GetFrameHeightWithSpacing() * 3.0f;
		const float text_width = ImGui::CalcTextSize(user_text.c_str()).x;
		const auto user_width = max(ImGui::GetContentRegionAvail().x / 4.0f, text_width + padding);

		user_clicked = ImGui::Button(user_text.c_str(), ImVec2(user_width, user_height));
		bool user_shift_clicked = user_clicked && ImGui::GetIO().KeyShift;
		bool user_ctrl_clicked  = user_clicked && ImGui::GetIO().KeyCtrl;
		bool user_default = ImGui::IsItemHovered() && ImGui::IsKeyPressed(ImGuiKey_T, false);

		auto DrawList = ImGui::GetWindowDrawList();
		

		ImGui::PopStyleVar();
		ImGui::PopFont();
		
		if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort | ImGuiHoveredFlags_NoSharedDelay)) {
			UICustom::FancyTooltip(Translate("STATUS_BAR_TARGET_TOOLTIP"));
		}

		ImGui::SameLine();

		const ImVec2 user_icon_offset = ImVec2(user_width - ImGui::GetStyle().WindowPadding.x / 2.0f, -3.0f);
		ImGui::PushFont(NULL, ImGui::GetFontSize() + 2.0f);
		DrawList->AddText(ImGui::GetCursorScreenPos() - user_icon_offset, ThemeConfig::GetColorU32("TEXT"), user_icon.c_str());
		ImGui::PopFont();

		const float search_width = ImGui::GetContentRegionAvail().x;
		ImGui::SetNextItemWidth(search_width);
		ImGui::PushStyleColor(ImGuiCol_Button, ThemeConfig::GetColor("BUTTON_CONFIRM"));
		if (!warn_text.empty()) {
			warn_clicked = ImGui::Button(TRUNCATE(warn_text, search_width).c_str(), ImVec2(search_width, user_height));

			if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort | ImGuiHoveredFlags_NoSharedDelay)) {
				UICustom::FancyTooltip(Translate("STATUS_BAR_WARN_TOOLTIP"));
			}
		} else {
			search_clicked = ImGui::Button(search_text.c_str(), ImVec2(search_width, user_height));

			if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort | ImGuiHoveredFlags_NoSharedDelay)) {
				UICustom::FancyTooltip(Translate("STATUS_BAR_AUX_TOOLTIP"));
			}
		}
		ImGui::PopStyleColor();
		ImGui::PopStyleVar(3);

		if (user_shift_clicked) {
			auto* reference = UIModule::GetTargetReference();
			this->SetTargetByReference(reference);
		}
			
		if (user_ctrl_clicked && tableTargetRef) {
			auto formID = tableTargetRef->GetFormID();
			std::string hex = std::format("{:08X}", formID);
			ImGui::SetClipboardText(hex.c_str());	
		}

		// BUG: Please investigate why calling this commented func crashes.
		// It might be due to pointer lifetime.

		if (user_default) {
			tableTargetRef = Commands::GetConsoleReference();
			// this->SetTargetByReference(tableTargetRef);
		}

		if (user_clicked && !user_shift_clicked && !user_ctrl_clicked) {
			UIManager::GetSingleton()->ShowInputBox(
				Translate("STATUS_BAR_TARGET_TITLE"),
				Translate("STATUS_BAR_TARGET_DESC"),
				"",
				[this](const std::string& a_input) {
					bool success = false;

					if (auto reference = UIModule::LookupReferenceBySearch(a_input); reference.has_value()) {
						this->SetTargetByReference(reference.value());


						success = true;
					}

					if (!success) {
						UIManager::GetSingleton()->ShowWarning(
							Translate("INVALID_REFERENCE_POPUP_TITLE"),
							Translate("INVALID_REFERENCE_POPUP_DESC")
						);
					}
				}
			);
		}

		// Settings Button
		ImGui::PushFont(NULL, ImGui::GetFontSize() + 2.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.f, 0.f));
		ImGui::SameLine();
		const ImVec2 button_size = ImVec2(ImGui::GetContentRegionAvail().x, user_height);
		if (ImGui::Button(ICON_LC_SETTINGS, button_size)) {
			ImGui::OpenPopup("TABLE_SETTINGS_POPUP");
		}
		ImGui::PopStyleVar();
		ImGui::PopFont();

		ImGui::SetNextWindowSize(ImVec2(ImGui::GetWindowSize().x / 4.0f, 0.0f));
		if (ImGui::BeginPopup("TABLE_SETTINGS_POPUP")) {
			DrawTableSettingsPopup();
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

		ImGui::PopID();
	}

	// a_valueWidth is to determine avaiable column space before EOL.
	void UITable::CustomSortColumn(std::string a_header, float a_valueWidth, bool a_sorted)
	{
		const static ImVec4 text_col = ThemeConfig::GetColor("HEADER_TEXT");
		
		if (!a_header.empty()) {
			ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 4.0f);
			ImGui::AlignTextToFramePadding();
			ImGui::TextColored(text_col, "%s", a_header.c_str());
			if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
				if (a_sorted) {
					this->sortSystem->ToggleAscending();
					this->SortListBySpecs();
					this->UpdateImGuiTableIDs();
				}
			}
			ImGui::SameLine();
			ImGui::SetCursorPosX(ImGui::GetCursorPosX() - ImGui::GetFontSize() / 1.5f); // Adjust for icon size
		}

		constexpr auto combo_flags = ImGuiComboFlags_NoArrowButton | ImGuiComboFlags_HeightLarge;
		const ImVec4 prev_text_color = ThemeConfig::GetColor("TEXT");
		ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0, 0, 0, 0));
		ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0, 0, 0, 0));
		ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0, 0, 0, 0));
		ImGui::PushStyleColor(ImGuiCol_Text, text_col);
		
		const float sortby_size = ImGui::GetContentRegionAvail().x - a_valueWidth - ImGui::GetStyle().ItemSpacing.x;				
		const FilterProperty current_key = sortSystem->GetCurrentSortFilter();
		
		ImGui::SetNextItemWidth(sortby_size);
		if (ImGui::BeginCombo("##UITable::Sort::Combo", current_key.ToString().c_str(), combo_flags)) {
			const FilterPropertyList available_keys = sortSystem->GetAvailableFilters();

			ImGui::PushStyleColor(ImGuiCol_Text, prev_text_color);
			for (auto& key : available_keys) {
				const bool is_selected = (key == current_key);
				const std::string key_text = key.ToString();

				if (key.GetPropertyType() == PropertyType::kImGuiSeparator) {
					ImGui::Separator();
					continue;
				}

				if (ImGui::Selectable(key_text.c_str(), is_selected)) {
					sortSystem->SetCurrentSortFilter(key);

					this->SortListBySpecs();
					this->UpdateImGuiTableIDs();
					
					UserData::User().Set<int>(this->data_id + "::SortBy", static_cast<int>(key.GetPropertyType()));
				}
				
				if (is_selected) {
					ImGui::SetItemDefaultFocus();
				}
			}
			ImGui::PopStyleColor();

			ImGui::EndCombo();
		}

		ImGui::PopStyleColor(4);
	}

	void UITable::DrawHeader()
	{
		static const ImVec4 text_col = ThemeConfig::GetColor("HEADER_TEXT");

		ImGui::PushID("##Modex::Table::Header");

		ImGui::PushStyleColor(ImGuiCol_Separator, ThemeConfig::GetColor("HEADER_SEPARATOR"));
		ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal, 1.0f);

		std::string header_plugin = Translate("PLUGIN");
		std::string header_name = this->showEditorID ? Translate("EDITORID") : Translate("NAME");
		std::string header_custom = this->sortSystem->GetCurrentSortFilter().ToString();
		std::string header_value = Translate("VALUE");
		std::string sort_icon = sortSystem->GetSortAscending()  == true ? ICON_LC_ARROW_DOWN_A_Z : ICON_LC_ARROW_UP_A_Z;
		PropertyType current_sort = sortSystem->GetCurrentSortFilter().GetPropertyType();

		const bool is_plugin_sorted = current_sort == PropertyType::kPlugin;
		const bool is_name_sorted = current_sort == PropertyType::kName || current_sort == PropertyType::kEditorID;
		const bool is_value_sorted = current_sort == PropertyType::kGoldValue;
		const bool is_custom_sorted = !is_plugin_sorted && !is_name_sorted && !is_value_sorted && current_sort != PropertyType::kNone;

		header_plugin = is_plugin_sorted ? sort_icon + " " + header_plugin : header_plugin;
		header_name = is_name_sorted ? sort_icon + " " + header_name : header_name;
		header_value = is_value_sorted ? sort_icon + " " + header_value : header_value;
		header_custom = is_custom_sorted ? sort_icon + " " : "";

		constexpr float name_offset = 4.0f; // pillar offset.
		const float spacing = name_offset + (LayoutColumnWidth / 4.0f);
		const float sort_spacing = spacing * 2.50f;
		const float value_width = ImGui::CalcTextSize(header_value.c_str()).x + ImGui::GetFontSize();

		if (HasFlag(ModexTableFlag_Kit)) {
			const std::string header_equip = Translate("EQUIP");

			std::string header_amount = Translate("AMOUNT");
			const bool is_amount_sorted = current_sort == PropertyType::kKitItemCount;
			header_amount = is_amount_sorted ? sort_icon + " " + header_amount : header_amount;

			ImGui::AlignTextToFramePadding();
			ImGui::TextColored(text_col, "%s", header_plugin.c_str());
			if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
				this->sortSystem->SetCurrentSortFilter(PropertyType::kPlugin);
				this->sortSystem->ToggleAscending();
				this->SortListBySpecs();
				this->UpdateImGuiTableIDs();
			}

			ImGui::SameLine(4.0f + LayoutItemSize.x / 4.0f);
			ImGui::AlignTextToFramePadding();
			ImGui::TextColored(text_col, "%s", header_name.c_str());
			if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
				this->sortSystem->ToggleAscending();

				if (this->sortSystem->GetClicks() == 2) {
					this->showEditorID = !this->showEditorID;
					this->sortSystem->ResetSort();
				} else {
					this->sortSystem->SetCurrentSortFilter(this->showEditorID ? PropertyType::kEditorID : PropertyType::kName);
				}

				this->sortSystem->SetCurrentSortFilter(this->showEditorID ? PropertyType::kEditorID : PropertyType::kName);
				this->SortListBySpecs();
				this->UpdateImGuiTableIDs();
			}

			ImGui::SameLine(LayoutItemSize.x / 1.50f);
			ImGui::AlignTextToFramePadding();
			ImGui::TextColored(text_col, "%s", header_equip.c_str());
			{

			}

			ImGui::SameLine(LayoutItemSize.x / 1.50f + (LayoutItemSize.x / 7.0f) + 5.0f); // + equip_size
			ImGui::AlignTextToFramePadding();
			ImGui::TextColored(text_col, "%s", header_amount.c_str());
			if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
				this->sortSystem->SetCurrentSortFilter(PropertyType::kKitItemCount);
				this->sortSystem->ToggleAscending();
				this->SortListBySpecs();
				this->UpdateImGuiTableIDs();
			}

		}
		
		if (!HasFlag(ModexTableFlag_Kit)) {
			ImGui::AlignTextToFramePadding();
			ImGui::TextColored(text_col, "%s", header_plugin.c_str());
			if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
				this->sortSystem->SetCurrentSortFilter(PropertyType::kPlugin);
				this->sortSystem->ToggleAscending();
				this->SortListBySpecs();
				this->UpdateImGuiTableIDs();
			}

			ImGui::SameLine(spacing);
			ImGui::AlignTextToFramePadding();
			ImGui::TextColored(text_col, "%s", header_name.c_str());
			if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
				this->sortSystem->ToggleAscending();

				if (this->sortSystem->GetClicks() == 2) {
					this->showEditorID = !this->showEditorID;
					this->sortSystem->ResetSort();
				} else {
					this->sortSystem->SetCurrentSortFilter(this->showEditorID ? PropertyType::kEditorID : PropertyType::kName);
				}

				this->SortListBySpecs();
				this->UpdateImGuiTableIDs();
			}

			ImGui::SameLine(sort_spacing - name_offset);

			CustomSortColumn(header_custom, value_width, is_custom_sorted);

			ImGui::SameLine();
			ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x - value_width - (ImGui::GetFrameHeight() / 2.0f));
			ImGui::AlignTextToFramePadding();
			ImGui::TextColored(text_col, "%s", header_value.c_str());
			if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
				this->sortSystem->SetCurrentSortFilter(PropertyType::kGoldValue);
				this->sortSystem->ToggleAscending();
				this->SortListBySpecs();
				this->UpdateImGuiTableIDs();
			}
		}
		
		ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal);
		ImGui::PopStyleColor();
		ImGui::PopID();
	}
	
	// TODO: move this
	static inline bool test_table_selection = false;
	static inline bool test_table_filters = false;

	void UITable::Draw(const TableList& _tableList)
	{
		if (!this->generalSearchDirty) {
			if (strcmp(searchSystem->GetSearchBuffer(), searchSystem->GetLastSearchBuffer()) != 0) {
				this->generalSearchDirty = true;
			}
		}

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
			// FIX: PluginList isn't deterministic of a module failing to load anymore due to
			// polymorphic use of UITable's. Need to redefine what a failure to load looks like.

			if (test_table_selection) {
				Test_TableSelection();
				const float target_scroll = this->itemPreview->m_tableID * LayoutItemStep.y;
				const float current_scroll = ImGui::GetScrollY();
				const float lerp_factor = 0.15f; // Adjust for faster/slower scroll (0.0-1.0)
				const float smooth_scroll = current_scroll + (target_scroll - current_scroll) * lerp_factor;
				ImGui::SetScrollY(smooth_scroll);
			}

			if (test_table_filters) {
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

						// Double-click add amount behavior.
						// TODO: Renable this
						// const int click_amount = HasFlag(ModexTableFlag_Kit) ? item_data->kitAmount : clickAmount;

						// position item at start
						ImVec2 pos = ImVec2(start_pos.x, start_pos.y + line_idx * LayoutItemStep.y);
						ImGui::SetCursorScreenPos(pos);

						// set next item selection user data
						bool is_item_selected = selectionStorage.Contains(item_data->m_tableID);
						bool is_item_visible = ImGui::IsRectVisible(LayoutItemSize);


						ImGui::SetNextItemSelectionUserData(item_idx);
						ImGui::PushItemFlag(ImGuiItemFlags_NoNav, true);
						ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.f, 0.f));
						if (this->HasFlag(ModexTableFlag_Kit)) {
							ImGui::Selectable("", is_item_selected, ImGuiSelectableFlags_None, ImVec2(LayoutItemSize.x / 1.75f, LayoutItemSize.y));
						} else {
							ImGui::Selectable("", is_item_selected, ImGuiSelectableFlags_None, LayoutItemSize);
						}
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

						// TODO: Can we bypass default selection backend so that when we click on an item
						// it will stay selected? Right now there is this intermediary state where there is an
						// outline but the item is not selected.

						// instantly toggle, bypassing multi selection backend
						// if (ImGui::IsItemToggledSelection()) {
						// 	is_item_selected = true;
						// 	// is_item_selected = !is_item_selected;
						// }

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
								ImGui::SetDragDropPayload(GetDragDropHandleText(dragDropHandle), payload_items.Data, (size_t)payload_items.size_in_bytes());
							}

							ImGui::EndDragDropSource();
						}

						// if the item is visible, offload drawing item data to separate function
						if (is_item_visible && item_data != nullptr) {
							if (this->HasFlag(ModexTableFlag_Kit)) {
								DrawKitItem(item_data, pos);
							} else {
								DrawItem(item_data, pos);
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

			if (this->updateRecentList) {
				this->LoadRecentList();
			}
		}
		
		ImGui::EndChild();

		HandleDragDropBehavior();
	}

	void UITable::DrawDebugToolkit()
	{
		ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal);
		// if (UICustom::AddToggleButton("Autotest Table Selection", test_table_selection)) {
			// test_table_selection = !test_table_selection;
		// }

		ImGui::SameLine();

		// if (UICustom::AddToggleButton("Autotest Table Filters", test_table_filters)) {
			// test_table_filters = !test_table_filters;
		// }
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
					}
					break;
				}
			}


			// Move to the next index
			current_index = (current_index + 1) % nodesSize;

			// stop when we finish
			if (current_index == nodesSize) {
				test_table_filters = false;
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
				test_table_selection = false;
			}
		}
	}
}
