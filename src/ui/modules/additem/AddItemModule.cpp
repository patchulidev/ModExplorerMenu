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
		const float table_height = ImGui::GetContentRegionAvail().y - (window_padding.y * 2.0f);
		const float full_width = ImGui::GetContentRegionAvail().x;

		ImVec2 table_pos = ImGui::GetCursorPos();
		UIContainers::DrawBasicTablePanel(table_pos, ImVec2(table_width, table_height), a_tables[0]);

		ImVec2 action_pos = table_pos;
		action_pos.x += table_width + window_padding.x;
		UIContainers::DrawAddItemActionPanel(action_pos, ImVec2(full_width - table_width - window_padding.x, 0.0f), a_tables[0]);
	}

	AddItemModule::AddItemModule()
	{
		m_name = Translate("MODULE_ADD_ITEM");
		m_icon = ICON_LC_PLUS;

		m_layouts.push_back({"Table View", true, DrawTableView}); // TODO: Locale
		
		auto table = std::make_unique<UITable>();
		table->SetGenerator([]() { return Data::GetSingleton()->GetAddItemList(); });
		table->SetPluginType(Data::PLUGIN_TYPE::Item);
		table->SetUserDataID("AddItem");
		table->SetUseSharedTarget(true);
		table->SetDragDropHandle(UITable::DragDropHandle::Table);
		table->AddFlag(UITable::ModexTableFlag_Base);
		table->AddFlag(UITable::ModexTableFlag_EnableFilterTree);
		table->AddFlag(UITable::ModexTableFlag_EnableSearch);
		table->AddFlag(UITable::ModexTableFlag_EnableHeader);
		table->SetShowEditorID(UserData::User().Get<bool>("AddItem::ShowEditorID", false));
		table->Init();
		m_tables.push_back(std::move(table));
	}
}
