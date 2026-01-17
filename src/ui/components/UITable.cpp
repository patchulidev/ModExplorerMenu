#include "UITable.h" 

#include "core/Commands.h"
#include "imgui.h"
#include "imgui_internal.h"
#include "localization/Locale.h"
#include "ui/core/UIManager.h"
#include "ui/components/UICustom.h"
#include "ui/components/ItemPreview.h"
#include "config/EquipmentConfig.h"
#include "config/BlacklistConfig.h"
#include "config/UserConfig.h"
#include "config/UserData.h"
#include "config/ThemeConfig.h"


namespace Modex
{
	// TODO: Maybe add a (?) or something that will show a tooltip explaining the table view and colors

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

	// Does this really belong here?
	ImU32 GetFormTypeColor(const RE::FormType& a_type)
	{
		auto alpha = ImGui::GetStyle().Alpha;
		switch (a_type) {
		case RE::FormType::Armor:
			return IM_COL32(0, 102, 204, 100 * alpha);  // Medium Blue
		case RE::FormType::AlchemyItem:
			return IM_COL32(0, 204, 204, 100 * alpha);  // Cyan
		case RE::FormType::Ammo:
			return IM_COL32(204, 204, 0, 100 * alpha);  // Yellow
		case RE::FormType::Book:
			return IM_COL32(153, 76, 0, 100 * alpha);  // Brown
		case RE::FormType::Ingredient:
			return IM_COL32(0, 153, 0, 100 * alpha);  // Green
		case RE::FormType::KeyMaster:
			return IM_COL32(255, 153, 51, 100 * alpha);  // Orange
		case RE::FormType::Misc:
			return IM_COL32(153, 0, 153, 100 * alpha);  // Purple
		case RE::FormType::Scroll:
			return IM_COL32(255, 204, 153, 100 * alpha);  // Light Orange
		case RE::FormType::Weapon:
			return IM_COL32(255, 51, 51, 100 * alpha);  // Bright Red
		case RE::FormType::NPC:
			return IM_COL32(102, 178, 255, 100 * alpha);  // Light Blue
		case RE::FormType::Tree:
			return IM_COL32(0, 102, 0, 100 * alpha);  // Dark Green
		case RE::FormType::Static:
			return IM_COL32(102, 0, 204, 100 * alpha);  // Violet
		case RE::FormType::Container:
			return IM_COL32(204, 0, 0, 100 * alpha);  // Dark Red
		case RE::FormType::Activator:
			return IM_COL32(255, 102, 0, 100 * alpha);  // Orange Red
		case RE::FormType::Light:
			return IM_COL32(255, 204, 0, 100 * alpha);  // Gold
		case RE::FormType::Door:
			return IM_COL32(0, 204, 0, 100 * alpha);  // Bright Green
		case RE::FormType::Furniture:
			return IM_COL32(153, 0, 153, 100 * alpha);  // Dark Magenta
		default:
			return IM_COL32(169, 169, 169, 100 * alpha);  // Dark Gray
		}
	}


	// Should we newline instead of ellipsis?
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

			// ImGui::SetItemTooltip("%s", a_text.c_str());

			return result;
		}

		return a_text;
	}

	void UITable::AddItemToRecent(const std::unique_ptr<BaseObject>& a_item)
	{
		UserData::Recent().Add(a_item->GetEditorID());
		this->updateRecentList = true;
	}

	void UITable::LoadRecentList()
	{
		std::vector<std::string> recentItems;
		recentList.clear();

		UserData::Recent().GetAsList(recentItems);

		if (recentItems.empty()) {
			return;
		}

		for (const auto& editorID : recentItems) {
			if (RE::TESForm* form = RE::TESForm::LookupByEditorID(editorID)) {
				recentList.emplace_back(std::make_unique<BaseObject>(form, 0, 0));
			} else {
				// TODO: Should we include dummy objects in recent list?
				// recentList.emplace_back(std::make_unique<BaseObject>(editorID, editorID, "Unknown", 0));
			}
		}

		for (int i = 0; i < std::ssize(recentList); i++) {
			recentList[i]->m_tableID = i;
		}

		this->updateRecentList = false;
	}

	void UITable::AddKitItemsToInventory(const Kit& a_kit)
    {
		if (this->tableList.empty())
			return;

        if (a_kit.m_items.empty())
            return;

        for (auto& kitItem : a_kit.m_items) {
            Commands::AddItemToRefInventory(tableTargetRef, kitItem->m_editorid, static_cast<std::uint32_t>(kitItem->m_amount));
        }
    }

	// operates on tableTargetRef
	void UITable::RemoveSelectedFromInventory()
	{
		if (this->tableList.empty())
			return;

		void* it = NULL;
		ImGuiID id = 0;

		if (!tableTargetRef)
			return;

		while (selectionStorage.GetNextSelectedItem(&it, &id)) {
			if (id < std::ssize(tableList) && id >= 0) {
				const auto& item = tableList[id];
				Commands::RemoveItemFromInventory(tableTargetRef, item->GetEditorID(), 1);
				AddItemToRecent(item);
			}
		}

		selectionStorage.Clear();
	}

	void UITable::AddSelectionToInventory(int a_count)
	{
		if (this->tableList.empty()) 
			return;
		
		void* it = NULL;
		ImGuiID id = 0;

		if (!tableTargetRef)
			return;

		while (selectionStorage.GetNextSelectedItem(&it, &id)) {
			if (id < std::ssize(tableList) && id >= 0) {
				const auto& item = tableList[id];
				Commands::AddItemToRefInventory(tableTargetRef, item->GetEditorID(), a_count);
				AddItemToRecent(item);
			}
		}

		selectionStorage.Clear();
	}

	// void UITable::AddSelectionToNPC(int a_count)
	// {
	// 	if (this->tableList.empty())
	// 		return;

	// 	void* it = NULL;
	// 	ImGuiID id = 0;

	// 	while (selectionStorage.GetNextSelectedItem(&it, &id)) {
	// 		if (id < std::ssize(tableList) && id >= 0) {
	// 			const auto& item = tableList[id];
	// 			Commands::AddItemToNPC(item->GetEditorID(), a_count);
	// 			this->AddItemToRecent(item);
	// 		}
	// 	}

	// 	selectionStorage.Clear();
	// }


	void UITable::EquipSelection()
	{
		if (this->tableList.empty()) 
			return;
		
		void* it = NULL;
		ImGuiID id = 0;

		if (!tableTargetRef)
			return;

		while (selectionStorage.GetNextSelectedItem(&it, &id)) {
			if (id < std::ssize(tableList) && id >= 0) {
				const auto& item = tableList[id];
				Commands::AddAndEquipItemToInventory(tableTargetRef, item->GetEditorID());
				AddItemToRecent(item);
			}
		}

		selectionStorage.Clear();
	}

	void UITable::PlaceSelectionOnGround(int a_count)
	{
		if (this->tableList.empty()) 
			return;
		
		void* it = NULL;
		ImGuiID id = 0;

		while (selectionStorage.GetNextSelectedItem(&it, &id)) {
			if (id < std::ssize(tableList) && id >= 0) {
				const auto& item = tableList[id];
				Commands::PlaceAtMe(item->GetEditorID(), a_count);
				AddItemToRecent(item);
			}
		}

		selectionStorage.Clear();
	}

	void UITable::AddAll()
	{
		if (this->tableList.empty()) {
			return;
		}
		if (!tableTargetRef)
			return;
		
		for (auto& item : this->tableList) {
			Commands::AddItemToRefInventory(tableTargetRef, item->GetEditorID(), 1);
			AddItemToRecent(item);
		}

		selectionStorage.Clear();
	}

	void UITable::PlaceAll()
	{
		if (this->tableList.empty()) {
			return;
		}

		for (auto& item : this->tableList) {
			Commands::PlaceAtMe(item->GetEditorID(), 1);
			AddItemToRecent(item);
		}

		selectionStorage.Clear();
	}

	void UITable::Init()
	{
		// FIXME: Why is this called (x) amount of times on launch?

		const auto filename = ConfigManager::FILTER_DIRECTORY / (this->data_id + ".json");

		this->filterSystem = std::make_unique<FilterSystem>(filename);
		this->filterSystem->Load(true);
		this->filterSystem->SetSystemCallback([this]() {
			this->Refresh();
		});

		this->sortSystem = std::make_unique<SortSystem>(filename);
		this->sortSystem->Load(true);

		this->searchSystem = std::make_unique<SearchSystem>(filename);
		this->searchSystem->Load(true);
		this->searchSystem->SetupDefaultKey();

		this->BuildPluginList();
		this->LoadRecentList();
	}

	void UITable::Unload()
	{
		this->tableList.clear();
		this->pluginList.clear();
		this->selectionStorage.Clear();
		this->dragDropSourceList.clear();
	}

	void UITable::Load()
	{
		this->Reset();
	}

	void UITable::SetDragDropTarget(const std::string a_id, UITable* a_view)
	{
		this->dragDropSourceList.clear();
		this->dragDropSourceList[a_id] = a_view;
	}

	void UITable::AddDragDropTarget(const std::string a_id, UITable* a_view)
	{
		this->dragDropSourceList[a_id] = a_view;
	}

	void UITable::RemoveDragDropTarget(const std::string a_id)
	{
		auto it = this->dragDropSourceList.find(a_id);
		if (it != this->dragDropSourceList.end()) {
			this->dragDropSourceList.erase(it);
		}
	}

	void UITable::SetDragDropID(const std::string& a_id)
	{
		this->dragDropSourceID = a_id;
	}

	void UITable::SetGenerator(std::function<std::vector<BaseObject>()> a_generator)
	{
		this->generator = a_generator;
	}

	std::vector<std::unique_ptr<BaseObject>> UITable::GetSelection()
	{
		std::vector<std::unique_ptr<BaseObject>> selectedItems;

		for (auto& item : this->tableList) {
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

	void UITable::AddPayloadItemToKit(const std::unique_ptr<BaseObject>& a_item)
	{
		for (auto& item : this->tableList) {
			if (item->GetEditorID() == a_item->GetEditorID()) {
				return;
			}
		}

		this->tableList.emplace_back(std::make_unique<BaseObject>(*a_item));
	}

	// TODO: Need a better way to manage inventory changes
	void UITable::AddPayloadItemToInventory(const std::unique_ptr<BaseObject>& a_item)
	{
		for (auto& item : this->tableList) {
			if (item->GetEditorID() == a_item->GetEditorID()) {
				return;
			}
		}

		this->tableList.emplace_back(std::make_unique<BaseObject>(*a_item));

		Data::GetSingleton()->GenerateInventoryList();
	}

	void UITable::RemovePayloadItemFromInventory(const std::unique_ptr<BaseObject>& a_item)
	{
		if (this->tableList.empty()) {
			return;
		}

		auto player = RE::PlayerCharacter::GetSingleton();
		if (!player) {
			return;
		}

		auto playerRef = player->AsReference();
		if (!playerRef) {
			return;
		}

		Commands::RemoveItemFromInventory(playerRef, a_item->GetEditorID());
		Data::GetSingleton()->GenerateInventoryList();
	}

	void UITable::AddSelectedToKit()
	{
		if (auto map = this->dragDropSourceList.find("FROM_KIT"); map != this->dragDropSourceList.end()) {
			const auto dragDropSourceTable = map->second->GetTableListPtr();

			void* it = NULL;
			ImGuiID id = 0;

			while (selectionStorage.GetNextSelectedItem(&it, &id)) {
				if (id < tableList.size() && id >= 0) {
					const auto& item = this->tableList[id];

					auto iter = std::find_if(dragDropSourceTable->begin(), dragDropSourceTable->end(),
						[&item](const std::unique_ptr<BaseObject>& a_item) {
							return a_item->GetEditorID() == item->GetEditorID();
						});

					if (iter == dragDropSourceTable->end()) {
						dragDropSourceTable->emplace_back(std::make_unique<BaseObject>(*item));
					}
				}
			}
		}
	}

	void UITable::RemoveSelectedFromKit()
	{
		if (this->tableList.empty()) {
			return;
		}

		void* it = NULL;
		ImGuiID id = 0;

		std::vector<std::unique_ptr<BaseObject>> items_to_remove;

		while (selectionStorage.GetNextSelectedItem(&it, &id)) {
			if (id < tableList.size() && id >= 0) {
				items_to_remove.emplace_back(std::make_unique<BaseObject>(*this->tableList[id]));
			}
		}

		for (const auto& item : items_to_remove) {
			this->RemovePayloadItemFromKit(item);
		}
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
			// TODO: REFACTOR pending kit shit
			// if (this->selectedKit) {
			// 	auto& collection = EquipmentConfig::GetLoadedKits();
			// 	if (auto it = collection.find(*this->selectedKit); it != collection.end()) {
			// 		auto& kit = it->second;

			// 		kit.m_items.clear();

			// 		if (!this->tableList.empty()) {
			// 			for (auto& item : this->tableList) {
			// 				kit.m_items.emplace(CreateKitItem(*item));
			// 			}
			// 		}

			// 		EquipmentConfig::GetSingleton()->SaveKitToJSON(kit);
			// 	}
			// }

			// if (selectedKitPtr && !selectedKitPtr->empty()) {
			// 	auto equipmentConfig = EquipmentConfig::GetSingleton();
			// 	if (auto kitOpt = equipmentConfig->LoadKit(*selectedKitPtr); kitOpt.has_value()) {
			// 		auto& kit = kitOpt.value();
			// 		kit.m_items.clear();

			// 		if (!this->tableList.empty()) {
			// 			for (auto& item : this->tableList) {
			// 				kit.m_items.emplace(EquipmentConfig::CreateKitItem(*item));
			// 			}
			// 		}

			// 		equipmentConfig->SaveKit(kit);
			// 	}
			// }

			if (selectedKitPtr && !selectedKitPtr->empty()) {
				auto equipmentConfig = EquipmentConfig::GetSingleton();
				selectedKitPtr->m_items.clear();

				if (!this->tableList.empty()) {
					for (auto& item : this->tableList) {
						selectedKitPtr->m_items.emplace(EquipmentConfig::CreateKitItem(*item));
					}
				}

				equipmentConfig->SaveKit(*selectedKitPtr);
			}
		}
	}

	void UITable::Refresh()
	{
		// assert(this->generator);

		selectionStorage.Clear();

		if (this->HasFlag(ModexTableFlag_Kit)) {
			this->Filter(this->generator());

			if (this->sortSystem->GetCurrentSortFilter().GetPropertyType() != PropertyType::kNone) {
				this->SortListBySpecs();
			}
			
			this->UpdateImGuiTableIDs();
			this->UpdateKitItemData();
		}
		else if (this->HasFlag(ModexTableFlag_Inventory)) {
			this->Filter(this->generator());

			if (this->sortSystem->GetCurrentSortFilter().GetPropertyType() != PropertyType::kNone) {
				this->SortListBySpecs();
			}

			this->UpdateImGuiTableIDs();
		} 
		else {
			this->Filter(this->generator());

			if (this->sortSystem->GetCurrentSortFilter().GetPropertyType() != PropertyType::kNone) {
				this->SortListBySpecs();
			}

			this->UpdateImGuiTableIDs();
			
			// In-table kit view is disabled if there are no kits in the plugin.
			if (this->pluginKitList.empty()) {
				this->showPluginKitView = false;
			}
		}
	}

	void UITable::Reset()
	{
		this->tableList.clear();
		this->itemPreview = nullptr;
		this->selectedPlugin = Translate("SHOW_ALL");

		this->tableTargetRef = Commands::GetConsoleReference();
		ImFormatString(this->pluginSearchBuffer, IM_ARRAYSIZE(this->pluginSearchBuffer), "");

		this->BuildPluginList();
		this->Refresh();
		this->LoadRecentList();
	}

	void UITable::Filter(const std::vector<BaseObject>& a_data)
	{
		tableList.clear();
		tableList.reserve(std::ssize(a_data));
		
		this->generalSearchDirty = false;
		ImFormatString(searchSystem->GetLastSearchBuffer(), IM_ARRAYSIZE(searchSystem->GetLastSearchBuffer()), "%s", searchSystem->GetSearchBuffer());

		if (filterSystem && filterSystem->ShowRecent()) {
			// std::vector<std::pair<std::string, std::uint32_t>> recently_used;
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
		const auto& config = UserConfig::Get();
		this->pluginList = Data::GetSingleton()->GetFilteredListOfPluginNames(this->pluginType, (Data::SORT_TYPE)config.modListSort, RE::FormType::None);
		this->pluginSet = Data::GetSingleton()->GetModulePluginList(this->pluginType);
		pluginList.insert(pluginList.begin(), Translate("SHOW_ALL"));
	}

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
		ImGui::Text(" " ICON_LC_ARROW_LEFT_RIGHT);
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
			this->selectedPlugin = Translate("SHOW_ALL");

			if (this->selectedPlugin.find(this->pluginSearchBuffer) != std::string::npos) {
				ImFormatString(this->pluginSearchBuffer, IM_ARRAYSIZE(this->pluginSearchBuffer), "");
			} else {
				for (auto& plugin : this->pluginSet) {
					if (BlacklistConfig::GetSingleton()->Has(plugin)) {
						continue;
					}

					std::string pluginName = Modex::ValidateTESFileName(plugin);

					if (pluginName == Translate("SHOW_ALL")) {
						ImFormatString(this->pluginSearchBuffer, IM_ARRAYSIZE(this->pluginSearchBuffer), "");
						break;
					}

					if (pluginName.find(this->pluginSearchBuffer) != std::string::npos) {
						this->selectedPlugin = pluginName;
						ImFormatString(this->pluginSearchBuffer, IM_ARRAYSIZE(this->pluginSearchBuffer), "");
						break;
					}
				}
			}

			this->LoadKitsFromSelectedPlugin();
			this->selectionStorage.Clear();
			this->Refresh();
		}
		
		ImGui::PopStyleColor();
	}

	void UITable::UpdateLayout()
	{
		const auto& config = UserConfig::Get();
		const auto& user = UserData::User();
		const float _width = ImGui::GetContentRegionAvail().x + user.Get<float>("Modex::Table::ItemWidth", 0.0f);
		const float _height = (config.globalFontSize * 1.75f) + user.Get<float>("Modex::Table::ItemHeight", 0.0f); 

		if (_width <= 0.0f) {
			return;
		}

		// Each table item's width and height.
		LayoutItemSize = ImVec2(_width, _height);

		// Spacing
		LayoutRowSpacing = 5.0f;
		LayoutHitSpacing = 0.0f;

		// Not used currently
		LayoutColumnCount = 1;

		// Used to increment position for next table item.
		LayoutItemStep = ImVec2(LayoutItemSize.x, LayoutItemSize.y + LayoutRowSpacing);

		// Includes measurement for outline thickness!
		LayoutOuterPadding = floorf(LayoutRowSpacing * 0.5f);
	}

	// bool UITable::SortFnKit(const std::unique_ptr<Kit>& lhs, const std::unique_ptr<Kit>& rhs)
	// {
	// 	int delta = 0;
	// 	switch (sortBy) {
	// 	case SortType::Name:
	// 		delta = lhs->name.compare(rhs->name);
	// 		break;
	// 	}

	// 	if (delta > 0)
	// 		return sortAscending ? false : true;
	// 	if (delta < 0)
	// 		return sortAscending ? true : false;

	// 	return false;
	// }

	void UITable::SortListBySpecs()
	{
		if (!this->showPluginKitView) {
			std::sort(tableList.begin(), tableList.end(), [this](const std::unique_ptr<BaseObject>& a, const std::unique_ptr<BaseObject>& b) {
				return sortSystem->SortFn(a, b);
			});
		} else {
			std::sort(pluginKitList.begin(), pluginKitList.end(), [this](const std::unique_ptr<Kit>& a, const std::unique_ptr<Kit>& b) {
				return sortSystem->SortFnKit(a, b);
			});
		}
	}

	void UITable::UpdateImGuiTableIDs()
	{
		if (!this->showPluginKitView) {
			for (int i = 0; i < std::ssize(tableList); i++) {
				tableList[i]->m_tableID = i;
			}
		} else {
			for (int i = 0; i < std::ssize(pluginKitList); i++) {
				pluginKitList[i]->m_tableID = i;
			}
		}
	}

	void UITable::UpdateKitItemData()
	{
		if (!selectedKitPtr || selectedKitPtr->empty()) {
			return;
		}

		if (!this->HasFlag(ModexTableFlag_Kit)) {
			return;
		}

		for (const auto& kit_item : selectedKitPtr->m_items) {
			for (const auto& table_item : tableList) {
				if (table_item->GetEditorID() == kit_item->m_editorid) {
					table_item->kitAmount = kit_item->m_amount;
					table_item->kitEquipped = kit_item->m_equipped;
				}
			}
		}

		// if (auto kitOpt = EquipmentConfig::GetSingleton()->LoadKit(*selectedKitPtr); kitOpt.has_value()) {
		// 	const auto& kit = kitOpt.value();

		// 	if (kit.m_items.empty()) {
		// 		return;
		// 	}

		// 	for (const auto& kit_item : kit.m_items) {
		// 		for (const auto& table_item : tableList) {
		// 			if (table_item->GetEditorID() == kit_item->m_editorid) {
		// 				table_item->kitAmount = kit_item->m_amount;
		// 				table_item->kitEquipped = kit_item->m_equipped;
		// 			}
		// 		}
		// 	}
		// }
		
	}

	void UITable::ShowSort()
	{
		const float full_width = ImGui::GetContentRegionAvail().x;

		{
			const std::string icon = ICON_LC_SIGNATURE;
			const std::string tooltip = this->showEditorID ? Translate("EDITORID_DISABLE_TOOLTIP") : Translate("EDITORID_ENABLE_TOOLTIP");
			ImGui::PushStyleColor(ImGuiCol_Text, this->showEditorID ? ThemeConfig::GetColor("TEXT") : ThemeConfig::GetColor("TEXT_DISABLED"));
			if (UICustom::IconButton(icon.c_str(), tooltip.c_str(), this->showEditorID)) {
				UserData::User().Set<bool>(this->data_id + "::ShowEditorID", this->showEditorID);
			}
			ImGui::PopStyleColor();
		}

		if (this->HasFlag(ModexTableFlag_EnablePluginKitView)) {
			ImGui::SameLine();

			const std::string icon = ICON_LC_BOX;
			const std::string tooltip = this->showPluginKitView ? Translate("KITVIEW_DISABLE_TOOLTIP") : Translate("KITVIEW_ENABLE_TOOLTIP");
			bool pluginHasKits = !this->pluginKitList.empty();

			if (pluginHasKits && !this->showPluginKitView) {
				float pulse = PulseMinMax((float)ImGui::GetTime(), 5.0f, 0.5f, 0.0f, 1.0f);
				ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, pulse));
			} else if (pluginHasKits && this->showPluginKitView) {
				ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
			} else {
				ImGui::PushStyleColor(ImGuiCol_Text, ThemeConfig::GetColor("TEXT_DISABLED"));
			}

			if (UICustom::IconButton(icon.c_str(), tooltip.c_str(), this->showPluginKitView)) {
				if (!pluginHasKits) {
					this->showPluginKitView = false;
				} else {
					this->SortListBySpecs();
					this->UpdateImGuiTableIDs();
				}

				UserData::User().Set<bool>(this->data_id + "::ShowPluginKitView", this->showPluginKitView);
			}

			ImGui::PopStyleColor();
		}

		ImGui::SameLine();

		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
		DrawFormSearchBar(ImVec2(full_width / 3.0f, 0.0f));

		ImGui::SameLine();
		ImGui::AlignTextToFramePadding();
		ImGui::Text(" " ICON_LC_ARROW_RIGHT_TO_LINE);
		if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort | ImGuiHoveredFlags_NoSharedDelay)) {
			UICustom::FancyTooltip("TABLE_PLUGIN_TOOLTIP");
		}
		ImGui::SameLine();
		
		DrawPluginSearchBar(ImVec2(ImGui::GetContentRegionAvail().x - ImGui::GetStyle().WindowPadding.x, 0.0f));
		ImGui::PopStyleVar();

		ImGui::SameLine();
		ImGui::AlignTextToFramePadding();
		ImGui::Text(ICON_LC_SETTINGS);
		ImGui::SameLine();
		
		ImGui::SetNextItemAllowOverlap();
		const ImVec2 _size = ImVec2(ImGui::GetFrameHeight(), ImGui::GetFrameHeight());
		ImGui::SetCursorPosX(ImGui::GetCursorPosX() - _size.x - ImGui::GetStyle().WindowPadding.x);
		if (ImGui::InvisibleButton("##Table::Settings::Button", _size)) {
			ImGui::OpenPopup("TABLE_SETTINGS_POPUP");
		}

		// TODO: Table settings
		ImGui::SetNextWindowSize(ImVec2(ImGui::GetWindowSize().x / 4.0f, 0.0f));
		if (ImGui::BeginPopup("TABLE_SETTINGS_POPUP")) {
			DrawTableSettingsPopup();
			ImGui::EndPopup();
		}

	}

	// TODO: This is just a reminder, none line specific, that we need to setup persistent userdata for new stuff

	// This is called when we select a plugin from the plugin list to alter the view. This updates the
	// pluginKitList member to populate using a compare algorithm against all selected kits. So we also call it
	// when updating changes to our current kit.

	void UITable::LoadKitsFromSelectedPlugin()
	{
		if (this->HasFlag(ModexTableFlag_EnablePluginKitView)) {
			// pluginKitList.clear();

			// // We don't need to correlate kits to All Plugins since it's not a valid plugin.
			// if (selectedPlugin == Translate("SHOW_ALL")) {
			// 	return;
			// }

			// auto& equipment = EquipmentConfig::GetEquipmentList();

			// This shit is kind of expensive

			// auto& kits = EquipmentConfig::GetLoadedKits();

			// int table_id = 0;
			// for (auto& [name, kit] : kits) {
			// 	bool kitExists = false;
			// 	for (auto& existingKit : pluginKitList) {
			// 		if (existingKit->m_name == kit.m_name) {
			// 			kitExists = true;
			// 			break;
			// 		}
			// 	}
			// 	if (!kitExists) {
			// 		// We first need to check if any item in any given kit is from the selected plugin.
			// 		// We iterate over the items inside every kit to identify this. So far at 500+ kits this is not an issue.
			// 		// Once we detected that this kit *should* show inside the Plugin Kit View, we then create
			// 		// the meta data for the kit by iterating over its items again. The key point is that we
			// 		// early out of the first iteration on the first item found. So we don't actually iterate
			// 		// over the entire kit twice. We use one to match, then the other to build.

			// 		for (auto& item : kit.m_items) {
			// 			if (selectedPlugin == Translate("SHOW_ALL") || item->m_plugin == selectedPlugin) {
			// 				kit.m_tableID = table_id;

			// 				kit.m_weaponCount = 0;
			// 				kit.m_armorCount = 0;
			// 				kit.m_miscCount = 0;

			// 				for (auto& found : kit.m_items) {
			// 					const auto& form = RE::TESForm::LookupByEditorID(found->m_editorid);

			// 					if (form) {
			// 						if (form->IsWeapon()) {
			// 							kit.m_weaponCount++;
			// 						} else if (form->IsArmor()) {
			// 							kit.m_armorCount++;
			// 						} else {
			// 							kit.m_miscCount++;
			// 						}
			// 					}
			// 				}

			// 				this->pluginKitList.emplace_back(std::make_unique<Kit>(kit));

			// 				table_id++;
			// 				break;
			// 			}
			// 		}
			// 	}
			// }
		}
	}

	void UITable::DrawKit(const Kit& a_kit, const ImVec2& a_pos, const bool& a_selected)
	{
		(void)a_selected;  // If we want to handle selection visuals manually.

		const auto& DrawList = ImGui::GetWindowDrawList();
		const auto& config = UserConfig::Get();
		const float fontSize = config.globalFontSize;

		// Setup box and bounding box for positioning and drawing.
		const ImVec2 box_min(a_pos.x - 1, a_pos.y - 1);
		const ImVec2 box_max(box_min.x + LayoutItemSize.x + 2, box_min.y + LayoutItemSize.y + 2);  // Dubious
		ImRect bb(box_min, box_max);

		// Outline & Background
		const ImU32 bg_color = ThemeConfig::GetColorU32("TABLE_ROW_BG");
		const ImU32 bg_color_alt = ThemeConfig::GetColorU32("TABLE_ROW_BG_ALT");
		const ImU32 outline_color = ThemeConfig::GetColorU32("TABLE_ROW_BORDER");
		const ImU32 text_color = ThemeConfig::GetColorU32("TEXT");

		// Background
		if (a_kit.m_tableID % 2 == 0) {
			DrawList->AddRectFilled(bb.Min, bb.Max, bg_color);
		} else {
			DrawList->AddRectFilled(bb.Min, bb.Max, bg_color_alt);
		}
	

		// Outline
		DrawList->AddRect(bb.Min, bb.Max, outline_color, 0.0f, 0, 1.0f);

		const float spacing = LayoutItemSize.x / 3.0f;
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

	void UITable::PluginKitView()
	{
		if (!this->HasFlag(ModexTableFlag_EnablePluginKitView)) {
			return;
		}

		if (!pluginKitList.empty()) {
			UpdateLayout();

			const ImVec2 start_pos = ImGui::GetCursorScreenPos();
			ImGui::SetCursorScreenPos(start_pos);

			const int COLUMN_COUNT = 1;
			const int ITEMS_COUNT = static_cast<int>(std::ssize(pluginKitList));

			ImGuiListClipper clipper;
			clipper.Begin(ITEMS_COUNT, LayoutItemStep.y);

			while (clipper.Step()) {
				const int item_start = clipper.DisplayStart;
				const int item_end = clipper.DisplayEnd;

				for (int line_idx = item_start; line_idx < item_end; line_idx++) {
					const int item_min_idx_for_current_line = line_idx * COLUMN_COUNT;
					const int item_max_idx_for_current_line = (std::min)((line_idx + 1) * COLUMN_COUNT, ITEMS_COUNT);

					for (int kit_idx = item_min_idx_for_current_line; kit_idx < item_max_idx_for_current_line; ++kit_idx) {
						auto& item_data = pluginKitList[kit_idx];
						ImGui::PushID((int)item_data->m_tableID);

						ImVec2 pos = ImVec2(start_pos.x, start_pos.y + line_idx * LayoutItemStep.y);
						ImGui::SetCursorScreenPos(pos);

						bool is_selected = false;
						bool is_item_visible = ImGui::IsRectVisible(LayoutItemSize);
						const float button_area = (LayoutItemSize.x * 0.10f) + LayoutItemSize.y * 2.0f;


						ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.f, 0.f));
						ImGui::Selectable("", is_selected, 0, ImVec2(LayoutItemSize.x - (button_area), LayoutItemSize.y));
						ImGui::PopStyleVar();

						if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
							if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
								this->AddKitItemsToInventory(*item_data);
							}
						}

						if (is_item_visible) {
							DrawKit(*item_data, pos, false);

							const ImVec4 use_color = ThemeConfig::GetColor("KIT_USE");
							const ImVec4 del_color = ThemeConfig::GetColor("KIT_DELETE");
							const ImVec2 use_size = ImVec2(LayoutItemSize.x * 0.10f, LayoutItemSize.y);
							const ImVec2 del_size = ImVec2(LayoutItemSize.y * 2.0f, LayoutItemSize.y);
							const ImVec2 use_pos = ImVec2(pos.x + (LayoutItemSize.x - use_size.x - del_size.x), pos.y);
							const ImVec2 del_pos = ImVec2(use_pos.x + use_size.x, use_pos.y);

							ImGui::SetCursorScreenPos(use_pos);
							ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0.0f, 0.0f));
							ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f));

							ImGui::PushStyleColor(ImGuiCol_Button, use_color);
							if (ImGui::Button(ICON_LC_CHECK "Use", use_size)) {
								this->AddKitItemsToInventory(*item_data);
							}
							ImGui::PopStyleColor();

							ImGui::SameLine();

							bool _unused = false;
							// if (UICustom::IconButtonBg(ICON_LC_TRASH, Translate("KIT_DELETE_TOOLTIP"), _unused, del_size, del_color, 0.1f)) {
								// TODO: Implement delete kit popup.
							// }

							ImGui::PopStyleVar(2);
						}

						ImGui::PopID();
					}
				}
			}

			clipper.End();
		}
	}

	// TODO: Try adding that + or - or Trash icon background thing again.
	void UITable::HandleDragDropBehavior()
	{
		if (ImGui::BeginDragDropTarget()) {
			if (this->HasFlag(ModexTableFlag_Inventory)) { // Inventory Table Receiving Drag 'n Drop Event.
				if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("FROM_TABLE", ImGuiDragDropFlags_AcceptBeforeDelivery)) {
					if (payload->IsDelivery()) {
						const auto dragDropSourceTable = this->dragDropSourceList.at("FROM_TABLE");
						const int payload_count = (int)payload->DataSize / (int)sizeof(ImGuiID);

						std::vector<std::unique_ptr<BaseObject>> payload_items;
						for (int payload_idx = 0; payload_idx < payload_count; ++payload_idx) {
							const ImGuiID payload_item_id = ((ImGuiID*)payload->Data)[payload_idx];
							const auto& payload_item = (*dragDropSourceTable->GetTableListPtr())[payload_item_id];
							payload_items.emplace_back(std::make_unique<BaseObject>(*payload_item));
						}

						for (const auto& payload_item : payload_items) {
							this->AddPayloadItemToInventory(payload_item);
						}

						this->Refresh();
					}

					if (payload->IsPreview()) {
						this->DrawDragDropPayload(DragBehavior_Add);
					}
				}

				ImGui::EndDragDropTarget();
			} else if (this->HasFlag(ModexTableFlag_Kit)) { // Kit Table Receiving Drag 'n Drop Event.
				if (this->selectedKitPtr && !this->selectedKitPtr->empty()) {
					if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("FROM_TABLE", ImGuiDragDropFlags_AcceptBeforeDelivery)) {
						if (payload->IsDelivery()) {
							const auto dragDropSourceTable = this->dragDropSourceList.at("FROM_TABLE");
							const int payload_count = (int)payload->DataSize / (int)sizeof(ImGuiID);

							for (int payload_idx = 0; payload_idx < payload_count; ++payload_idx) {
								const ImGuiID payload_item_id = ((ImGuiID*)payload->Data)[payload_idx];
								const auto& payload_item = (*dragDropSourceTable->GetTableListPtr())[payload_item_id];

								this->AddPayloadItemToKit(payload_item);
							}

							this->SyncChangesToKit();
							dragDropSourceTable->LoadKitsFromSelectedPlugin();
							this->Refresh();

							// In-table kit view is disabled if there are no kits in the plugin.
							if (dragDropSourceTable->pluginKitList.empty()) {
								dragDropSourceTable->showPluginKitView = false;
							}
						}

						if (payload->IsPreview()) {
							this->DrawDragDropPayload(DragBehavior_Add);
						}
					}

					ImGui::EndDragDropTarget();
				} else { // TODO: Here is where we add behavior for kit creation on drag'n'drop.
					this->DrawDragDropPayload(DragBehavior_Invalid);
				}
			} else { // Item Table Receiving Drag 'n Drop Event
				const ImGuiPayload* kit_payload = ImGui::AcceptDragDropPayload("FROM_KIT", ImGuiDragDropFlags_AcceptBeforeDelivery);
				const ImGuiPayload* inventory_payload = ImGui::AcceptDragDropPayload("FROM_INVENTORY", ImGuiDragDropFlags_AcceptBeforeDelivery);
				if (kit_payload) {
					if (kit_payload->IsDelivery()) {
						const auto dragDropSourceTable = this->dragDropSourceList.at("FROM_KIT");
						const int payload_count = (int)kit_payload->DataSize / (int)sizeof(ImGuiID);

						std::vector<std::unique_ptr<BaseObject>> payload_items;
						for (int payload_idx = 0; payload_idx < payload_count; ++payload_idx) {
							const ImGuiID payload_item_id = ((ImGuiID*)kit_payload->Data)[payload_idx];
							const auto& payload_item = (*dragDropSourceTable->GetTableListPtr())[payload_item_id];
							payload_items.emplace_back(std::make_unique<BaseObject>(*payload_item));
						}

						for (const auto& payload_item : payload_items) {
							dragDropSourceTable->RemovePayloadItemFromKit(payload_item);
						}

						dragDropSourceTable->SyncChangesToKit();
						this->LoadKitsFromSelectedPlugin();
						dragDropSourceTable->Refresh();

						if (this->pluginKitList.empty()) {
							this->showPluginKitView = false;
						}
					}

					if (kit_payload->IsPreview()) {
						this->DrawDragDropPayload(DragBehavior_Remove);
					}
				} 
				else if (inventory_payload) {
					if (inventory_payload->IsDelivery()) {
						const auto dragDropSourceTable = this->dragDropSourceList.at("FROM_INVENTORY");
						const int payload_count = (int)inventory_payload->DataSize / (int)sizeof(ImGuiID);

						std::vector<std::unique_ptr<BaseObject>> payload_items;
						for (int payload_idx = 0; payload_idx < payload_count; ++payload_idx) {
							const ImGuiID payload_item_id = ((ImGuiID*)inventory_payload->Data)[payload_idx];
							const auto& payload_item = (*dragDropSourceTable->GetTableListPtr())[payload_item_id];
							payload_items.emplace_back(std::make_unique<BaseObject>(*payload_item));
						}

						for (const auto& payload_item : payload_items) {
							this->RemovePayloadItemFromInventory(payload_item);
						}

						// this->Refresh(); // Don't need to refresh Item table.
						dragDropSourceTable->Refresh();
					}

					if (inventory_payload->IsPreview()) {
						this->DrawDragDropPayload(DragBehavior_Add);
					}
				}

				ImGui::EndDragDropTarget();
			}

			selectionStorage.Clear();
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

			if (a_item->IsNPC()) {
				Commands::PlaceAtMe(a_item->GetEditorID(), 1);
			}

			this->AddItemToRecent(a_item);
		}
	}

	void UITable::HandleRightClickBehavior(const std::unique_ptr<BaseObject>& a_item)
	{

		static const int click_amount = 1; // TODO revisit this

		if (ImGui::IsItemClicked(ImGuiMouseButton_Right)) {
			ImGui::OpenPopup("TableViewContextMenu");
		}

		if (ImGui::BeginPopup("TableViewContextMenu")) {
			if (a_item->IsItem()) {
				if (HasFlag(ModexTableFlag_Inventory)) {
					if (ImGui::MenuItem(Translate("REMOVE_SELECTION"))) {
						if (GetSelectionCount() == 0) {
							Commands::RemoveItemFromInventory(tableTargetRef, a_item->GetEditorID(), click_amount);
							AddItemToRecent(a_item);
						} else {
							RemoveSelectedFromInventory();
						}

						this->Refresh();
						this->selectionStorage.Clear();
					}

					if (ImGui::MenuItem(Translate("DROP_SELECTION"))) {
						if (GetSelectionCount() == 0) {
							Commands::PlaceAtMe(a_item->GetEditorID(), click_amount);
							Commands::RemoveItemFromInventory(tableTargetRef, a_item->GetEditorID(), click_amount);
							AddItemToRecent(a_item);
						} else {
							PlaceSelectionOnGround(click_amount);
							RemoveSelectedFromInventory();
						}

						this->Refresh();
						this->selectionStorage.Clear();
					}

					ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal);
				}

				if (HasFlag(ModexTableFlag_Base)) {
					if (ImGui::MenuItem(Translate("ADD_SELECTION"))) {
						if (GetSelectionCount() == 0) {
							Commands::AddItemToRefInventory(tableTargetRef, a_item->GetEditorID(), click_amount);
							AddItemToRecent(a_item);
						} else {
							AddSelectionToInventory(click_amount);
						}
					}

					if (a_item->IsArmor() || a_item->IsWeapon()) {
						if (ImGui::MenuItem(Translate("EQUIP_SELECTION"))) {
							if (GetSelectionCount() == 0) {
								Commands::AddAndEquipItemToInventory(tableTargetRef, a_item->GetEditorID());
								AddItemToRecent(a_item);
							} else {
								EquipSelection();
							}
						}
					}

					if (ImGui::MenuItem(Translate("PLACE_SELECTION"))) {
						if (GetSelectionCount() == 0) {
							Commands::PlaceAtMe(a_item->GetEditorID(), click_amount);
							AddItemToRecent(a_item);
						} else {
							PlaceSelectionOnGround(click_amount);
						}
					}

					ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal);

					if (a_item->GetFormType() == RE::FormType::Book) {
						if (ImGui::MenuItem(Translate("GENERAL_READ_ME"))) {
							Commands::ReadBook(a_item->GetEditorID());
							UIManager::GetSingleton()->Close();
							AddItemToRecent(a_item);
						}

						ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal);
					}
				}

				if (HasFlag(ModexTableFlag_Kit)) { // Kit Table View Window
					if (ImGui::MenuItem(Translate("KIT_REMOVE"))) {
						if (GetSelectionCount() == 0) {
							this->RemovePayloadItemFromKit(a_item);
						} else {
							this->RemoveSelectedFromKit();
						}

						this->SyncChangesToKit();

						if (auto it = this->dragDropSourceList.find("FROM_KIT"); it != this->dragDropSourceList.end()) {
							const auto dragDropSourceTable = it->second;

							dragDropSourceTable->LoadKitsFromSelectedPlugin();
						}

						this->Refresh();
						this->selectionStorage.Clear();
					}

					ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal);
				}

				// We use dragDropSourceList to find paired Kit table, since selectedKit is stored in the kit table.
				if (HasFlag(ModexTableFlag_Base)) { // Main UITable Window
					if (auto it = this->dragDropSourceList.find("FROM_KIT"); it != this->dragDropSourceList.end()) {						
						const auto dragDropSourceTable = it->second;
						if (const auto selected_kit = dragDropSourceTable->selectedKitPtr; selected_kit && !selected_kit->empty()) {
							if (ImGui::MenuItem(Translate("KIT_ADD"))) {
								if (GetSelectionCount() == 0) {
									dragDropSourceTable->AddPayloadItemToKit(a_item);
									AddItemToRecent(a_item);
								} else {
									this->AddSelectedToKit();
									AddItemToRecent(a_item);
								}

								dragDropSourceTable->SyncChangesToKit();
								this->LoadKitsFromSelectedPlugin();

								dragDropSourceTable->Refresh();
								this->selectionStorage.Clear();
							}

							ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal);
						}
					}
				}
			}

			if (a_item->IsNPC()) { // ModexTableFlag_Base (?)
				if (ImGui::MenuItem(Translate("PLACE_SELECTION"))) {
					if (GetSelectionCount() == 0) {
						Commands::PlaceAtMe(a_item->GetEditorID(), click_amount);
						AddItemToRecent(a_item);
					} else {
						PlaceSelectionOnGround(click_amount);
					}
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

	void UITable::DrawKitItem(const std::unique_ptr<BaseObject>& a_item, const ImVec2& a_pos, const bool& a_selected)
	{
		(void)a_selected;
		
		if (!a_item || a_item.get() == nullptr) {
			return;
		}

		const auto& userdata = UserData::User();
		const auto& DrawList = ImGui::GetWindowDrawList();

		// Setup box and bounding box for positioning and drawing.
		const ImVec2 box_min(a_pos.x - 1, a_pos.y - 1);
		const ImVec2 box_max(box_min.x + LayoutItemSize.x + 2, box_min.y + LayoutItemSize.y + 2);  // Dubious
		ImRect bb(box_min, box_max);

		// Outline & Background
		const ImU32 bg_color = ThemeConfig::GetColorU32("TABLE_ROW_BG");
		const ImU32 bg_color_alt = ThemeConfig::GetColorU32("TABLE_ROW_BG_ALT");
		const ImU32 outline_color = ThemeConfig::GetColorU32("TABLE_ROW_BORDER");
		const ImU32 text_color = ThemeConfig::GetColorU32("TEXT");
		const ImU32 err_color = ThemeConfig::GetColorU32("ERROR");

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

		const float fontSize = UserConfig::Get().globalFontSize;
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
		float name_offset = 5.0f;  // initial offset

		if (showEditorID) {
			name_string = TRUNCATE(a_item->GetEditorID(), spacing * 1.5f);
		} else {
			name_string = TRUNCATE(name_string, spacing * 1.5f);
		}

		// TODO: Wtf was this doing
		// const std::string plugin_name = Utils::GetFormTypeIcon(a_item->GetFormType()) + TRUNCATE(a_item->GetPluginName(), ((spacing * 1.5f) - fontSize * 2.0f) - name_offset);
		// DrawList->AddText(center_left_align, text_color, plugin_name.c_str());
		const std::string plugin_icon = userdata.Get<bool>("Modex::Table::ShowPluginIcon", true) ? a_item->GetItemIcon() : "";
		const std::string plugin_name = TRUNCATE(plugin_icon + a_item->GetPluginName(), ((spacing * 1.5f) - fontSize * 2.0f) - name_offset);
		DrawList->AddText(center_left_align, text_color, plugin_name.c_str());

		const ImVec2 name_pos = ImVec2(bb.Min.x + (spacing * 1.5f) - name_offset, center_align);
		DrawList->AddText(name_pos, text_color, name_string.c_str());

		// I think this was used for the recent item list
		// if (overrideLayout == 1) {
		// 	const std::string item_id = std::to_string(a_item.m_tableID);
		// 	const ImVec2 id_pos = ImVec2(bb.Max.x - ((ImGui::CalcTextSize(item_id.c_str()).x + (LayoutOuterPadding * 2.0f))), center_align);
		// 	DrawList->AddText(id_pos, text_color, item_id.c_str());
		// }

		if (a_item->IsDummy()) {
			return;
		}
		
		ImVec2 equippable_pos = ImVec2(a_pos.x + (LayoutItemSize.x / 1.50f), a_pos.y);

		ImGui::SetCursorScreenPos(equippable_pos);

		const auto alpha = a_item->kitEquipped ? 1.0f : 0.25f;
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

	void UITable::DrawItem(const BaseObject& a_item, const ImVec2& a_pos, const bool& a_selected)
	{
		(void)a_selected;  // If we want to handle selection visuals manually.

		const auto& DrawList = ImGui::GetWindowDrawList();
		const auto& config = UserConfig::Get();
		const auto& userdata = UserData::User();
		const float fontSize = config.globalFontSize;

		auto test_tableRowBG = true;
		bool showPropertyColumn = userdata.Get<bool>("Modex::Table::ShowProperty", true);

		// Setup box and bounding box for positioning and drawing.
		const ImVec2 box_min(a_pos.x - 1, a_pos.y - 1);
		const float test_mod = userdata.Get<float>("Modex::Table::ItemWidth", 0.0f);
		const ImVec2 box_max(box_min.x + LayoutItemSize.x + 2 - test_mod, box_min.y + LayoutItemSize.y + 2);  // Dubious
		ImRect bb(box_min, box_max);

		// Outline & Background
		const ImU32 bg_color = ThemeConfig::GetColorU32("TABLE_ROW_BG");
		const ImU32 bg_color_alt = ThemeConfig::GetColorU32("TABLE_ROW_BG_ALT");
		const ImU32 outline_color = ThemeConfig::GetColorU32("TABLE_ROW_BORDER");
		const ImU32 text_color = ThemeConfig::GetColorU32("TEXT");
		const ImU32 err_color = ThemeConfig::GetColorU32("ERROR");
		bool is_enchanted = false;

		// Background
		if (test_tableRowBG) {
			if (a_item.m_tableID % 2 == 0) {
				DrawList->AddRectFilled(bb.Min, bb.Max, bg_color);
			} else {
				DrawList->AddRectFilled(bb.Min, bb.Max, bg_color_alt);
			}
		} else {
			DrawList->AddRectFilled(bb.Min, bb.Max, bg_color);
		}

		// Invalid / Missing plugin indicator
		if (a_item.IsDummy()) {
			DrawList->AddRectFilled(bb.Min, bb.Max, err_color);
		}

		// Outline
		DrawList->AddRect(bb.Min, bb.Max, outline_color, 0.0f, 0, 1.0f);

		// Type Color Pillar Identifier
		const float type_pillar_width = 5.0f;
		DrawList->AddRectFilled(
			ImVec2(bb.Min.x + LayoutOuterPadding, bb.Min.y + LayoutOuterPadding),
			ImVec2(bb.Min.x + LayoutOuterPadding + type_pillar_width, bb.Max.y - LayoutOuterPadding),
			GetFormTypeColor(a_item.GetFormType()));

		// Type Pillar tooltip
		if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows)) {
			if (IsMouseHoveringRectDelayed(
				ImVec2(bb.Min.x + LayoutOuterPadding, bb.Min.y + LayoutOuterPadding),
				ImVec2(bb.Min.x + LayoutOuterPadding + type_pillar_width, bb.Max.y - LayoutOuterPadding))) {
				ImGui::SetTooltip("%s", RE::FormTypeToString(a_item.GetFormType()).data());
			}
		}

		// We need to adjust the bounding box to account for the type pillar.
		bb.Min.x += type_pillar_width * 2.0f;

		float spacing = (LayoutItemSize.x / 6.0f) * 1.5f;
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

		std::string name_string = "";
		const std::string quantity_string = a_item.GetQuantity() > 1 ? std::format(" ({})", std::to_string(a_item.GetQuantity())) : "";

		// Initial name formatting based on valid name and width cutoff.
		if (!showEditorID) {
			if (a_item.GetName().empty()) {
				name_string = TRUNCATE(a_item.GetEditorID(), spacing) + quantity_string;
			} else {
				name_string = TRUNCATE(a_item.GetName(), spacing) + quantity_string;
			}
		} else {
			name_string = TRUNCATE(a_item.GetEditorID(), spacing) + quantity_string;
		}

		if (a_item.GetFormType() == RE::FormType::NPC) {
			if (const auto& npc = a_item.GetTESNPC(); npc.has_value()) {
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

		const ImVec2 property_text_pos = ImVec2(bb.Min.x + spacing + spacing, center_align);
		const float property_text_spacing = spacing * 0.75f;


		if (const auto& item = a_item; item.IsItem()) {
			const std::string value_string = std::to_string(item.GetGoldValue()) + " " + ICON_LC_COINS;
			const float value_width = ImGui::CalcTextSize(value_string.c_str()).x;
			const ImVec2 value_pos = ImVec2(center_right_align.x - value_width, center_right_align.y);
			DrawList->AddText(value_pos, text_color, value_string.c_str());

			if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows)) {
				if (IsMouseHoveringRectDelayed(value_pos, ImVec2(value_pos.x + value_width, value_pos.y + fontSize))) {
					UICustom::FancyTooltip(FilterProperty::GetIconTooltipKey(PropertyType::kGoldValue).c_str());
				}
			}

			if (const auto& armor = item.GetTESArmor(); armor.has_value()) {
				const auto& armorData = armor.value();

				if (armorData != nullptr) {
					const std::string rating_string = item.GetPropertyValueWithIcon(PropertyType::kArmorRating);
					const std::string type_string = TRUNCATE(item.GetPropertyValueWithIcon(PropertyType::kArmorType), property_text_spacing);

					if (armorData->formEnchanting != nullptr) {
						is_enchanted = true;
					}

					if (!showFormType && showPropertyColumn) {
						DrawList->AddText(property_text_pos, text_color, rating_string.c_str());
						if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows)) {
							if (IsMouseHoveringRectDelayed(property_text_pos, ImVec2(property_text_pos.x + fontSize, property_text_pos.y + fontSize))) {
								UICustom::FancyTooltip(FilterProperty::GetIconTooltipKey(PropertyType::kArmorRating).c_str());
							}
						}
					}
				}
			}

			if (item.GetFormType() == RE::FormType::Book) {
				if (const auto& book = item.GetTESForm()->As<RE::TESObjectBOOK>(); book != nullptr) {
					const auto teaches_skill = book->TeachesSkill();
					const auto teaches_spell = book->TeachesSpell();


					if (!showFormType && showPropertyColumn) {
						if (teaches_skill || book->GetSkill() != RE::ActorValue::kNone) {
							const std::string skill_string = TRUNCATE(item.GetPropertyValueWithIcon(PropertyType::kTomeSkill), property_text_spacing);
							DrawList->AddText(property_text_pos, text_color, skill_string.c_str());
						}
						
						if (teaches_spell || book->GetSpell() != nullptr) {
							const std::string spell_string = TRUNCATE(item.GetPropertyValueWithIcon(PropertyType::kTomeSpell), property_text_spacing);
							DrawList->AddText(property_text_pos, text_color, spell_string.c_str());
						}
						
						if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows)) {
							if (IsMouseHoveringRectDelayed(property_text_pos, ImVec2(property_text_pos.x + fontSize, property_text_pos.y + fontSize))) {
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

			if (const auto& weapon = item.GetTESWeapon(); weapon.has_value()) {
				const auto& weaponData = weapon.value();

				if (weaponData != nullptr) {
					const std::string damage_string = item.GetPropertyValueWithIcon(PropertyType::kWeaponDamage);
					const std::string skill_string = TRUNCATE(item.GetPropertyValueWithIcon(PropertyType::kWeaponSkill), property_text_spacing);
					// const std::string type_string = item.GetPropertyValueWithIcon(PropertyType::kWeaponType);

					if (weaponData->formEnchanting != nullptr) {
						is_enchanted = true;
					}
					
					if (!showFormType && showPropertyColumn) {
						DrawList->AddText(property_text_pos, text_color, damage_string.c_str());
						if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows)) {
							if (IsMouseHoveringRectDelayed(property_text_pos, ImVec2(property_text_pos.x + fontSize, property_text_pos.y + fontSize))) {
								UICustom::FancyTooltip(FilterProperty::GetIconTooltipKey(PropertyType::kWeaponDamage).c_str());
							}
						}
					}
				}
			}
		}

		// General Form Type display under Property column.
		if (showFormType && showPropertyColumn) {
			DrawList->AddText(property_text_pos, text_color, a_item.GetTypeName().data());
		}

		const std::string plugin_icon = userdata.Get<bool>("Modex::Table::ShowPluginIcon", true) ? a_item.GetItemIcon() : "";
		const std::string plugin_name = TRUNCATE(plugin_icon + a_item.GetPluginName(), spacing - ImGui::GetFontSize());
		DrawList->AddText(center_left_align, text_color, plugin_name.c_str());

		const PropertyType& sort_property = this->sortSystem->GetCurrentSortFilter().GetPropertyType();
		const std::string sort_text = TRUNCATE(a_item.GetPropertyValueWithIcon(sort_property), spacing * 0.75f);

		const ImVec2 name_pos = ImVec2(bb.Min.x + spacing, center_align);

		if (is_enchanted) {
			DrawList->AddText(name_pos, IM_COL32(180, 255, 255, (int)(ImGui::GetStyle().Alpha * 255)), name_string.c_str());
		} else {
			DrawList->AddText(name_pos, text_color, name_string.c_str());
		}

		// Conditionally draw sort-by so we don't overlap when sorted using header buttons.
		// This will need it's own function call soon if we expand it any more. A sort of "IsSortableProperty" check?
		if (sort_property != PropertyType::kPlugin and sort_property != PropertyType::kName and sort_property != PropertyType::kGoldValue) {
			const ImVec2 sort_pos = showPropertyColumn ? ImVec2(bb.Min.x + spacing * 2.75f, center_align) : property_text_pos;
			DrawList->AddText(sort_pos, text_color, sort_text.c_str());
			
			if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows)) {
				if (IsMouseHoveringRectDelayed(sort_pos, ImVec2(sort_pos.x + ImGui::CalcTextSize(sort_text.c_str()).x, sort_pos.y + fontSize))) {
					UICustom::FancyTooltip(Translate(FilterProperty::GetIconTooltipKey(sort_property).c_str()));
				}
			}
		}
	}

	// TODO: Make it fancy
	void UITable::DrawDragDropPayload(DragBehavior a_behavior)
	{
		ImGui::PushStyleColor(ImGuiCol_PopupBg, ImVec4(0.0f, 0.0f, 0.0f, 0.75f));
		if (ImGui::BeginTooltip()) {
			if (a_behavior == DragBehavior::DragBehavior_Add) {
				const static std::string hint = Translate("COPY");
				ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
				ImGui::Text(ICON_LC_PLUS " %s", hint.c_str());
			} else if (a_behavior == DragBehavior::DragBehavior_Invalid) {
				const static std::string hint = Translate("REQUIRES_KIT");
				ImGui::SetMouseCursor(ImGuiMouseCursor_NotAllowed);
				ImGui::Text(ICON_LC_X " %s", hint.c_str());
			} else if (a_behavior == DragBehavior::DragBehavior_Remove) {
				ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
				const static std::string hint = Translate("DELETE");
				ImGui::Text(ICON_LC_MINUS " %s", hint.c_str());
			} else {
				const static std::string hint = Translate("ITEM_SELECTED");
				ImGui::SetMouseCursor(ImGuiMouseCursor_NotAllowed);
				const auto payload_size = ImGui::GetDragDropPayload()->DataSize / (int)sizeof(ImGuiID);
				ImGui::Text("(%d) %s", payload_size, hint.c_str());
			}

			ImGui::EndTooltip();
		}

		ImGui::PopStyleColor();
	}

	void UITable::DrawTableSettingsPopup()
	{
		auto& user = UserData::User();
		UICustom::SubCategoryHeader(Translate("SETTINGS"));
		
		ImGui::PushStyleVar(ImGuiStyleVar_SeparatorTextAlign, ImVec2(0.5f, 0.5f));

		const float window_padding = ImGui::GetStyle().WindowPadding.x;
		const float button_size = ImGui::GetContentRegionAvail().x / 4.0f;
		const float _itemHeight = user.Get<float>("Modex::Table::ItemHeight", 0.0f);
		const float _itemWidth = user.Get<float>("Modex::Table::ItemWidth", 0.0f);
		const bool _showIcon = user.Get<bool>("Modex::Table::ShowPluginIcon");
		const bool _showProperty = user.Get<bool>("Modex::Table::ShowProperty", true);

		float item_height =  _itemHeight; 
		float item_width = _itemWidth;
		bool show_icon = _showIcon;
		bool show_property= _showProperty;

		ImGui::SeparatorText(Translate("TABLE_SETTING_HEIGHT"));
		ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
		if (ImGui::SliderFloat("##Table::Settings::ItemHeight", &item_height, -20.0f, 20.0f)) {
			user.Set<float>("Modex::Table::ItemHeight", item_height);
		}

		ImGui::SeparatorText(Translate("TABLE_SETTING_WIDTH"));
		ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
		if (ImGui::SliderFloat("##Table::Settings::ItemWidth", &item_width, -300.0f, 300.0f)) {
			user.Set<float>("Modex::Table::ItemWidth", item_width);
		}

		ImGui::SeparatorText(Translate("TABLE_SETTING_SHOW_PROPERTY"));
		ImGui::AlignTextToFramePadding();
		ImGui::Text("%s", Translate("TABLE_SETTING_SHOW_HIDE"));

		const float distance = (ImGui::GetContentRegionAvail().x - button_size + window_padding); 

		ImGui::SameLine(distance);
		// if (ImGui::ToggleButton("##Table::Settings::ShowProperty", &show_property, button_size)) {
			// user.Set<bool>("Modex::Table::ShowProperty", show_property);
		// }

		ImGui::SeparatorText(Translate("TABLE_SETTING_SHOW_ICON"));
		ImGui::AlignTextToFramePadding();
		ImGui::Text("%s", Translate("TABLE_SETTING_SHOW_HIDE"));

		ImGui::SameLine(distance);
		// if (ImGui::ToggleButton("##Table::Settings::PluginIcon", &show_icon, button_size)) {
			// user.Set<bool>("Modex::Table::ShowPluginIcon", show_icon);
		// }

		ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal);
		const bool reset = ImGui::Selectable("Reset");

		if (reset) {
			user.Set<float>("Modex::Table::ItemHeight", 0.0f);
			user.Set<float>("Modex::Table::ItemWidth", 0.0f);
			user.Set<bool>("Modex::Table::ShowProperty", true);
			user.Set<bool>("Modex::Table::ShowPluginIcon", true);
		}

		ImGui::PopStyleVar();
	}

	void UITable::DrawWarningBar()
	{
		static const std::string auxiliary_icon = HasFlag(ModexTableFlag_Kit) ? ICON_LC_BOX : ICON_LC_SEARCH;
		static constexpr std::string user_icon = ICON_LC_USER;
		static constexpr std::string warn_icon = ICON_LC_TRIANGLE_ALERT;

		ImVec4 user_color = ThemeConfig::GetColor("BUTTON"); // TODO: Unique Color Keys?

		std::string user_text;
		std::string auxiliary_text;
		std::string warn_text;

		if (HasFlag(ModexTableFlag_Kit)) {
			if (selectedKitPtr != nullptr && !selectedKitPtr->empty()) {
				auxiliary_text = auxiliary_icon + selectedKitPtr->GetNameTail();
			} else {
				warn_text = Translate("ERROR_NO_KIT_SELECTED");
			}
		} else {
			if (tableList.empty()) {
				warn_text = Translate("WARNING_NO_ITEMS");
			}

			auxiliary_text = auxiliary_icon + searchSystem->GetLastSearchBuffer();
		}

		if (tableTargetRef == nullptr) {
			warn_text = Translate("ERROR_MISSING_REFERENCE");
		} else {
			if (tableTargetRef->IsActor() == false) {
				warn_text = Translate("ERROR_INVALID_REFERENCE");
			} else {
				user_text = user_icon + tableTargetRef->GetName();

				if (tableTargetRef->IsPlayer()) {
					user_color = ThemeConfig::GetColor("BUTTON");
				} else {
					user_color = ThemeConfig::GetColor("CONTAINER_BUTTON");
				}
			}
		}

		bool _table_clicked_ 	= false;
		bool _user_clicked_ 	= false;
		bool _aux_clicked_ 		= false;
		bool _warn_clicked_ 	= false;

		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f));
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(ImGui::GetFontSize(), 3.0f));
		ImGui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign, ImVec2(0.0f, 0.5f));
		ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal, 2.0f);

		ImGui::PushStyleColor(ImGuiCol_Button, ImGuiCol_TableHeaderBg);
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImGuiCol_TableHeaderBg);
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImGuiCol_TableHeaderBg);
		_table_clicked_ = ImGui::Button((ICON_LC_TABLE + data_id).c_str());
		ImGui::PopStyleColor(3);

		if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort | ImGuiHoveredFlags_NoSharedDelay)) {
			UICustom::FancyTooltip(Translate("STATUS_BAR_TABLE_TOOLTIP"));
		}
		
		ImGui::SameLine();
		_user_clicked_ = ImGui::Button(user_text.c_str());

		if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort | ImGuiHoveredFlags_NoSharedDelay)) {
			UICustom::FancyTooltip(Translate("STATUS_BAR_TARGET_TOOLTIP"));
		}

		if (!auxiliary_text.empty() && warn_text.empty()) {
			ImGui::SameLine();
			ImGui::PushStyleColor(ImGuiCol_Button, ThemeConfig::GetColor("BUTTON_CONFIRM"));
			_aux_clicked_ = ImGui::Button(auxiliary_text.c_str(), ImVec2(ImGui::GetContentRegionAvail().x, 0.0f));
			ImGui::PopStyleColor();

			if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort | ImGuiHoveredFlags_NoSharedDelay)) {
				UICustom::FancyTooltip(Translate("STATUS_BAR_AUX_TOOLTIP"));
			}
		} else {
			ImGui::SameLine();
			ImGui::PushStyleColor(ImGuiCol_Button, ThemeConfig::GetColor("BUTTON_CANCEL_HOVER"));
			std::string _warn = TRUNCATE(warn_text, ImGui::GetContentRegionAvail().x * 0.75f);
			_warn_clicked_ = ImGui::Button((warn_icon + _warn).c_str(), ImVec2(ImGui::GetContentRegionAvail().x, 0.0f));
			ImGui::PopStyleColor();

			if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort | ImGuiHoveredFlags_NoSharedDelay)) {
				UICustom::FancyTooltip(Translate("STATUS_BAR_WARN_TOOLTIP"));
			}
		}
		ImGui::PopStyleVar(3);

		// This needs to be done outside of style push/pop stack:

		if (_table_clicked_) {
			UIManager::GetSingleton()->ShowInfoBox(
				Translate("STATUS_BAR_TABLE_TITLE"),
				Translate("STATUS_BAR_TABLE_DESC")
			);
		}

		if (_user_clicked_) {
			UIManager::GetSingleton()->ShowInfoBox(
				Translate("STATUS_BAR_TARGET_TITLE"),
				Translate("STATUS_BAR_TARGET_DESC")
			);
		}

		if (_aux_clicked_) {
			UIManager::GetSingleton()->ShowInfoBox(
				Translate("STATUS_BAR_AUX_TITLE"),
				Translate("STATUS_BAR_AUX_DESC")
			);
		}

		if (_warn_clicked_) {
			UIManager::GetSingleton()->ShowInfoBox(
				Translate("STATUS_BAR_WARN_TITLE"),
				warn_text
			);
		}

		ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal, 2.0f);
		ImGui::Spacing();
	}

	// TODO: Could these be single click exclusive except when holding CTRL?
	void UITable::DrawFormTypeTabs()
	{
		ImGui::PushID("##Modex::Table::CategoryTabs");
	
		if (const auto rootNode = this->filterSystem->FindNode("__root__")) {
			ASSERT_MSG(rootNode == nullptr, "Failed to find the \"FilterProperty\" root node in Table JSON configuration.");
			this->filterSystem->RenderNodeAndChildren(rootNode, ImGui::GetContentRegionAvail().x);
		}

		ImGui::PopID();
	}

	void UITable::DrawHeaderSortCombo(std::string a_header, float a_valueWidth, bool a_sorted)
	{
		const static ImVec4 text_col = ThemeConfig::GetColor("HEADER_TEXT");
		
		ImGui::TextColored(text_col, "%s", a_header.c_str());
		if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
			if (a_sorted) {
				this->sortSystem->ToggleAscending();
				this->SortListBySpecs();
				this->UpdateImGuiTableIDs();
			}
		}
		ImGui::SameLine();
		ImGui::SetCursorPosX(ImGui::GetCursorPosX() - ImGui::GetFontSize() / 2.0f); // Adjust for icon size

		constexpr auto combo_flags = ImGuiComboFlags_NoArrowButton | ImGuiComboFlags_HeightLarge;
		const ImVec4 prev_text_color = ThemeConfig::GetColor("TEXT");
		ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0, 0, 0, 0));
		ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0, 0, 0, 0));
		ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0, 0, 0, 0));
		ImGui::PushStyleColor(ImGuiCol_Text, text_col);
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2.0f, 1.0f));
		
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
		ImGui::PopStyleVar();
	}

	// TODO: Naturally clicking the header buttons implies sorting ascending/descending for that category
	// So we should implement some kind of ability to click those labels, and append a sort icon next to it
	// and also match it up to the sort system in the top right.
	void UITable::DrawHeader()
	{
		const auto& userdata = UserData::User();
		const static ImVec4 text_col = ThemeConfig::GetColor("HEADER_TEXT");
		const static ImVec4 button_col = ThemeConfig::GetColor("HEADER_SEPARATOR");

		ImGui::PushID("##Modex::Table::Header");
		ImGui::PushStyleColor(ImGuiCol_Separator, button_col);

		std::string header_plugin = Translate("PLUGIN");
		std::string header_name = this->showEditorID ? Translate("EDITORID") : Translate("NAME");
		std::string header_property = showFormType ? Translate("FORMTYPE") : Translate("PROPERTY");
		std::string header_custom = this->sortSystem->GetCurrentSortFilter().ToString();
		std::string header_value = Translate("VALUE");
		std::string sort_icon = sortSystem->GetSortAscending()  == true ? ICON_LC_ARROW_DOWN_A_Z : ICON_LC_ARROW_UP_A_Z;
		PropertyType current_sort = sortSystem->GetCurrentSortFilter().GetPropertyType();

		const bool is_plugin_sorted = current_sort == PropertyType::kPlugin;
		const bool is_name_sorted = current_sort == PropertyType::kName;
		const bool is_value_sorted = current_sort == PropertyType::kGoldValue;
		const bool is_custom_sorted = !is_plugin_sorted && !is_name_sorted && !is_value_sorted;

		header_plugin = is_plugin_sorted ? sort_icon + " " + header_plugin : header_plugin;
		header_name = is_name_sorted ? sort_icon + " " + header_name : header_name;
		header_value = is_value_sorted ? sort_icon + " " + header_value : header_value;
		header_custom = is_custom_sorted ? sort_icon : "";

		constexpr float name_offset = 4.0f;
		const float spacing = name_offset + (LayoutItemSize.x / 6.0f) * 1.5f;
		const float name_spacing = 4.0f + spacing * 1.0f;
		const float property_spacing = spacing * 2.0f;
		const float custom_spacing = spacing * 2.75f;
		const float value_width = ImGui::CalcTextSize(header_value.c_str()).x + ImGui::GetFontSize();


		if (HasFlag(ModexTableFlag_Kit)) {
			const std::string header_equip = Translate("EQUIP");
			const std::string header_amount = Translate("AMOUNT");

			ImGui::TextColored(text_col, "%s", header_plugin.c_str());
			ImGui::SameLine(4.0f + LayoutItemSize.x / 4.0f);
			ImGui::TextColored(text_col, "%s", header_name.c_str());
			ImGui::SameLine(LayoutItemSize.x / 1.50f);
			ImGui::TextColored(text_col, "%s", header_equip.c_str());
			ImGui::SameLine(LayoutItemSize.x / 1.50f + (LayoutItemSize.x / 7.0f) + 5.0f); // + equip_size
			ImGui::TextColored(text_col, "%s", header_amount.c_str());

		}
		
		if (!HasFlag(ModexTableFlag_Kit)) {
			if (showPluginKitView) {
				ImGui::TextColored(text_col, "%s", Translate("KIT"));
				ImGui::SameLine(spacing);
				ImGui::TextColored(text_col, "%s", Translate("PROPERTY"));
			} else {
				ImGui::TextColored(text_col, "%s", header_plugin.c_str());
				if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
					this->sortSystem->SetCurrentSortFilter(PropertyType::kPlugin);
					this->sortSystem->ToggleAscending();
					this->SortListBySpecs();
					this->UpdateImGuiTableIDs();
				}

				ImGui::SameLine(name_spacing);
				ImGui::TextColored(text_col, "%s", header_name.c_str());
				if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
					this->sortSystem->SetCurrentSortFilter(this->showEditorID ? PropertyType::kEditorID : PropertyType::kName);
					this->sortSystem->ToggleAscending();
					this->SortListBySpecs();
					this->UpdateImGuiTableIDs();
				}


				if (userdata.Get<bool>("Modex::Table::ShowProperty", true)) {
					ImGui::SameLine(property_spacing);
					ImGui::TextColored(text_col, "%s", header_property.c_str());
					if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
						showFormType = !showFormType;
					}

					ImGui::SameLine(custom_spacing);
				} else {
					ImGui::SameLine(property_spacing);
				}

				DrawHeaderSortCombo(header_custom, value_width, is_custom_sorted);

				ImGui::SameLine();
				ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x - value_width);
				ImGui::TextColored(text_col, "%s", header_value.c_str());
				if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
					this->sortSystem->SetCurrentSortFilter(PropertyType::kGoldValue);
					this->sortSystem->ToggleAscending();
					this->SortListBySpecs();
					this->UpdateImGuiTableIDs();
					
				}
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

		if (ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_A, ImGuiInputFlags_RouteFromRootWindow)) {
			if (selectionStorage.Size > 0) {
				selectionStorage.Clear();
			} else {
				for (auto& item : _tableList) {
					if (item) {
						selectionStorage.SetItemSelected(item->m_tableID, true);
					}
				}
			}
		}

		UpdateLayout();

		// DrawWarningBar();

		if (HasFlag(ModexTableFlag_EnableDebugToolkit)) {
			DrawDebugToolkit();
		}

		if (HasFlag(ModexTableFlag_EnableCategoryTabs)) {
			DrawFormTypeTabs();
		}

		if (HasFlag(ModexTableFlag_EnableHeader)) {
			DrawHeader();
		}


		if (ImGui::BeginChild("##UITable::Draw", ImVec2(0.0f, 0.0f), 0, ImGuiWindowFlags_NoMove)) {
			if (this->showPluginKitView) {
				PluginKitView();
				ImGui::EndChild();
				return;
			}

			if (std::ssize(this->pluginList) <= 1) {
				ImGui::TextColored(ImVec4(1.0f, 0.1f, 0.1f, 1.0f), "Error: Moduled failed to load. Try clicking the Sidebar Button for this module!");
				ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "If you don't see a button for this module in the sidebar, it's because it's disabled! You have to enable it in settings!");
				ImGui::EndChild();
				return;
			}

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
			const ImVec2 start_pos = ImGui::GetCursorScreenPos();
			ImGui::SetCursorScreenPos(start_pos);

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
						if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_AcceptBeforeDelivery | ImGuiDragDropFlags_SourceNoPreviewTooltip)) {
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
								ImGui::SetDragDropPayload(this->dragDropSourceID.c_str(), payload_items.Data, (size_t)payload_items.size_in_bytes());
							}

							ImGuiContext& g = *GImGui;
							const ImGuiPayload* payload = ImGui::GetDragDropPayload();
							const int payload_count = (int)payload->DataSize / (int)sizeof(ImGuiID);

							if (!g.DragDropAcceptIdPrev) {
								if (payload_count == 1) {
									this->DrawDragDropPayload(DragBehavior_None);
								} else {
									this->DrawDragDropPayload(DragBehavior_None);
								}
							}

							ImGui::EndDragDropSource();
						}

						// if the item is visible, offload drawing item data to separate function
						if (is_item_visible && item_data != nullptr) {
							if (this->HasFlag(ModexTableFlag_Kit)) {
								DrawKitItem(item_data, pos, is_item_selected);
							} else {
								DrawItem(*item_data, pos, is_item_selected);
							}
						}

						ImGui::PopID();
					}
				}
			}
			clipper.End();

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
