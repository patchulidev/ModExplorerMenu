#include "EquipmentModule.h"
#include "config/UserData.h"
#include "config/EquipmentConfig.h"
#include "config/ThemeConfig.h"
#include "localization/Locale.h"
#include "ui/components/UIContainers.h"
#include "core/Commands.h"
#include "ui/components/UITable.h"

namespace Modex
{
	void EquipmentModule::Draw()
	{
		DrawTabMenu();
	}

	static inline void DrawEquipmentLayout(std::vector<std::unique_ptr<UITable>>& a_tables)
	{
		const ImVec2 window_padding = ImGui::GetStyle().WindowPadding;
		const float table_width = ImGui::GetContentRegionAvail().x * 0.5f;
		const float table_height = ImGui::GetContentRegionAvail().y - window_padding.y;

		const float panel_width = (table_width / 2.0f) - (window_padding.x * 2.0f);
		const float panel_height = table_height / 2.5f; // FIX: Broken;

		auto& table = a_tables[0];
		auto& kitTable = a_tables[1];

		const ImVec2 table_pos = ImGui::GetCursorPos();
		UIContainers::DrawBasicTablePanel(table_pos, ImVec2(table_width, table_height), table);

		const ImVec2 search_pos = table_pos + ImVec2(table_width + window_padding.x, 0.0f);
		EquipmentModule::DrawKitSelectionPanel(search_pos, ImVec2(panel_width, panel_height), kitTable);

		const ImVec2 action_pos = search_pos + ImVec2(panel_width + window_padding.x, 0.0f);
		UIContainers::DrawKitActionsPanel(action_pos, ImVec2(0.0f, panel_height), kitTable, table, EquipmentModule::m_selectedKit);

		const ImVec2 kit_pos = search_pos + ImVec2(0.0f, panel_height + window_padding.y);
		UIContainers::DrawKitTablePanel(kit_pos, ImVec2(table_width - window_padding.x, table_height - panel_height - window_padding.y), kitTable);
	}

	static inline void DrawInventoryLayout(std::vector<std::unique_ptr<UITable>>& a_tables)
	{
		const ImVec2 window_padding = ImGui::GetStyle().WindowPadding;
		const float table_width = ImGui::GetContentRegionAvail().x * 0.5f;
		const float table_height = ImGui::GetContentRegionAvail().y - (window_padding.y * 2.0f);

		auto& table = a_tables[0];
		auto& invTable = a_tables[2];

		ImVec2 table_pos = ImGui::GetCursorPos();
		UIContainers::DrawBasicTablePanel(table_pos, ImVec2(table_width, table_height), table);

		ImVec2 inventory_pos = table_pos;
		inventory_pos.x += table_width + window_padding.x;
		UIContainers::DrawInventoryTablePanel(inventory_pos, ImVec2(table_width - window_padding.x, table_height), invTable);
	}

	EquipmentModule::~EquipmentModule()
	{
		UserData::Set<std::string>("lastSelectedKit", m_selectedKit.m_key);
		m_searchSystem->SaveState("EquipmentModule::KitSelection");
	}

	EquipmentModule::EquipmentModule() 
	{
		// overrides
		m_name = Translate("MODULE_EQUIPMENT");
		m_icon = ICON_LC_PACKAGE;

		// static
		m_selectedKit = Kit();
		m_searchSystem = std::make_unique<SearchSystem>(std::filesystem::path());
		m_searchSystem->Load(false);
		m_searchSystem->LoadState("EquipmentModule::KitSelection");

		// Setup available layouts for this module.
		m_layouts.push_back({"Equipment Layout", true, DrawEquipmentLayout}); // TODO: Locale
		m_layouts.push_back({"Inventory Layout", false, DrawInventoryLayout}); // TODO Locale

		constexpr auto table_flags =
		UITable::ModexTableFlag_Base |
		UITable::ModexTableFlag_EnableFilterTree |
		UITable::ModexTableFlag_EnableHeader |
		UITable::ModexTableFlag_EnableSearch |
		UITable::ModexTableFlag_EnableItemPreviewOnHover;

		auto table = std::make_unique<UITable>("AddItem", true, 0, table_flags);
		table->SetDragDropHandle(UITable::DragDropHandle::Table);

		constexpr auto kit_flags = 
		UITable::ModexTableFlag_Kit |
		UITable::ModexTableFlag_EnableHeader;

		auto last_kit = UserData::Get<std::string>("lastSelectedKit", "");
		m_selectedKit = EquipmentConfig::KitLookup(last_kit).value_or(Kit());

		auto kit = std::make_unique<UITable>("Equipment", true, 0, kit_flags);
		kit->SetKitPointer(&m_selectedKit);
		kit->SetDragDropHandle(UITable::DragDropHandle::Kit);
		kit->Refresh();

		constexpr auto inventory_flags = 
		UITable::ModexTableFlag_EnableHeader | 
		UITable::ModexTableFlag_EnableItemPreviewOnHover |
		UITable::ModexTableFlag_Inventory;

		auto inventory = std::make_unique<UITable>("Inventory", false, 0, inventory_flags);
		inventory->SetDragDropHandle(UITable::DragDropHandle::Inventory);

		// Setup drag and drop target linkage.
		table->AddDragDropTarget(kit->GetDragDropHandle(), kit.get());
		kit->AddDragDropTarget(table->GetDragDropHandle(), table.get());
		table->AddDragDropTarget(inventory->GetDragDropHandle(), inventory.get());
		inventory->AddDragDropTarget(table->GetDragDropHandle(), table.get());

		m_tables.push_back(std::move(table));
		m_tables.push_back(std::move(kit));
		m_tables.push_back(std::move(inventory));
	}
}
