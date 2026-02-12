#include "InventoryModule.h"

#include "ui/components/UIContainers.h"

namespace Modex
{
	void InventoryModule::Draw()
	{
		DrawTabMenu();
	}

	static inline void DrawInventoryLayout(std::vector<std::unique_ptr<UITable>>& a_tables)
	{
		const ImVec2 window_padding = ImGui::GetStyle().WindowPadding;
		const float table_width = ImGui::GetContentRegionAvail().x * 0.5f;
		const float table_height = ImGui::GetContentRegionAvail().y - (window_padding.y * 2.0f);

		auto& table = a_tables[0];
		auto& invTable = a_tables[1];

		ImVec2 table_pos = ImGui::GetCursorPos();
		UIContainers::DrawBasicTablePanel("TABLE_ITEM", table_pos, ImVec2(table_width, table_height), table);

		ImVec2 inventory_pos = table_pos;
		inventory_pos.x += table_width + window_padding.x;
		UIContainers::DrawBasicTablePanel("TABLE_INVENTORY", inventory_pos, ImVec2(table_width - window_padding.x, table_height), invTable);
	}

	static inline void DrawDoubleInventoryLayout(std::vector<std::unique_ptr<UITable>>& a_tables)
	{
		const ImVec2 window_padding = ImGui::GetStyle().WindowPadding;
		const float table_width = ImGui::GetContentRegionAvail().x * 0.5f;
		const float table_height = ImGui::GetContentRegionAvail().y - (window_padding.y * 2.0f);

		auto& invTableRHS = a_tables[1];
		auto& invTableLHS = a_tables[2];

		const ImVec2 inventory_lhs_pos = ImGui::GetCursorPos();
		ImGui::PushID("##LeftHandInventory");
		UIContainers::DrawBasicTablePanel("TABLE_INVENTORY", inventory_lhs_pos, ImVec2(table_width, table_height), invTableLHS);
		ImGui::PopID();

		ImVec2 inventory_rhs_pos = inventory_lhs_pos;
		inventory_rhs_pos.x += table_width + window_padding.x;

		ImGui::PushID("##RightHandInventory");
		UIContainers::DrawBasicTablePanel("TABLE_INVENTORY", inventory_rhs_pos, ImVec2(table_width - window_padding.x, table_height), invTableRHS);
		ImGui::PopID();
	}

	InventoryModule::~InventoryModule()
	{

	}

	InventoryModule::InventoryModule()
	{
		m_layouts.push_back({Translate("TAB_INVENTORY"), true, DrawInventoryLayout});
		m_layouts.push_back({Translate("TAB_MULTI_INVENTORY"), false, DrawDoubleInventoryLayout});

		constexpr auto table_flags =
		UITable::ModexTableFlag_Base |
		UITable::ModexTableFlag_EnableFilterTree |
		UITable::ModexTableFlag_EnableHeader |
		UITable::ModexTableFlag_EnableSearch |
		UITable::ModexTableFlag_EnableItemPreviewOnHover;

		auto table = std::make_unique<UITable>("AddItem", true, 0, table_flags);
		table->SetDragDropHandle(UITable::DragDropHandle::Table);

		constexpr auto inventory_flags = 
		UITable::ModexTableFlag_EnableHeader | 
		UITable::ModexTableFlag_EnableItemPreviewOnHover |
		UITable::ModexTableFlag_Inventory;

		auto inventory_rhs = std::make_unique<UITable>("Inventory", true, 0, inventory_flags);
		inventory_rhs->SetDragDropHandle(UITable::DragDropHandle::Inventory);

		auto inventory_lhs = std::make_unique<UITable>("Inventory", false, 0, inventory_flags);
		inventory_lhs->SetDragDropHandle(UITable::DragDropHandle::Inventory);

		// Pair table + inventory table for inventory insertion / deletion.
		table->AddDragDropTarget(inventory_rhs->GetDragDropHandle(), inventory_rhs.get());
		inventory_rhs->AddDragDropTarget(table->GetDragDropHandle(), table.get());

		// Pair inventory + inventory tables for trading layout.
		inventory_rhs->AddDragDropTarget(inventory_lhs->GetDragDropHandle(), inventory_lhs.get());
		inventory_lhs->AddDragDropTarget(inventory_rhs->GetDragDropHandle(), inventory_rhs.get());

		m_tables.push_back(std::move(table));
		m_tables.push_back(std::move(inventory_rhs));
		m_tables.push_back(std::move(inventory_lhs));
	}
}
