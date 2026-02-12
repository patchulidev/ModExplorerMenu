#include "AddItemModule.h"
#include "config/UserData.h"
#include "localization/Locale.h"
#include "ui/components/UIContainers.h"

namespace Modex
{
	void AddItemModule::Draw()
	{
		DrawTabMenu();
	}

	static inline void DrawTableView(std::vector<std::unique_ptr<UITable>>& a_tables)
	{
		const ImVec2 window_padding = ImGui::GetStyle().WindowPadding;
		const float table_width = ImGui::GetContentRegionAvail().x * 0.75f + window_padding.x;
		const float table_height = ImGui::GetContentRegionAvail().y - window_padding.y;
		const float full_width = ImGui::GetContentRegionAvail().x;

		ImVec2 table_pos = ImGui::GetCursorPos();
		UIContainers::DrawBasicTablePanel("TABLE_ITEM", table_pos, ImVec2(table_width, table_height), a_tables[0]);

		ImVec2 action_pos = table_pos;
		action_pos.x += table_width + window_padding.x;
		UIContainers::DrawAddItemActionPanel(action_pos, ImVec2(full_width - table_width - window_padding.x, 0.0f), a_tables[0]);
	}

	AddItemModule::~AddItemModule()
	{
		// Destructor
	}

	AddItemModule::AddItemModule()
	{
		m_layouts.push_back({Translate("TAB_ADDITEM"), true, DrawTableView}); // TODO: Locale
		
		constexpr auto table_flags =
		UITable::ModexTableFlag_Base |
		UITable::ModexTableFlag_EnableFilterTree |
		UITable::ModexTableFlag_EnableSearch |
		UITable::ModexTableFlag_EnableHeader;

		auto table = std::make_unique<UITable>("AddItem", true, 0, table_flags);
		table->SetDragDropHandle(UITable::DragDropHandle::Table);

		m_tables.push_back(std::move(table));
	}
}
