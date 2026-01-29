#include "ObjectModule.h"
#include "config/UserData.h"
#include "localization/Locale.h"
#include "ui/components/UIContainers.h"

namespace Modex
{
	void ObjectModule::Draw()
	{
		DrawTabMenu();
	}

	static inline void DrawTableView(std::vector<std::unique_ptr<UITable>>& a_tables)
	{
		const ImVec2 window_padding = ImGui::GetStyle().WindowPadding;
		const float table_width = ImGui::GetContentRegionAvail().x * 0.75f + window_padding.x;
		const float table_height = ImGui::GetContentRegionAvail().y - (window_padding.y * 2.0f);

		ImVec2 table_pos = ImGui::GetCursorPos();
		UIContainers::DrawBasicTablePanel(table_pos, ImVec2(table_width, table_height), a_tables[0]);

		ImVec2 action_pos = table_pos;
		action_pos.x += table_width + window_padding.x;

		// TODO: Re-implement object action panel
	}

	ObjectModule::ObjectModule()
	{
		m_name = Translate("MODULE_OBJECTS");
		m_icon = ICON_LC_SHAPES;

		m_layouts.push_back({"Table View", true, DrawTableView}); // TODO: Locale

		auto table = std::make_unique<UITable>();
		table->SetGenerator([]() { return Data::GetSingleton()->GetObjectList(); });
		table->SetPluginType(Data::PLUGIN_TYPE::Object);
		table->AddFlag(UITable::ModexTableFlag_EnableCategoryTabs);
		table->AddFlag(UITable::ModexTableFlag_EnableHeader);
		table->SetUserDataID("Object");
		table->SetUseSharedTarget(true); // Do we really target anything?
		table->SetShowEditorID(UserData::User().Get<bool>("Object::ShowEditorID", false));
		table->Init();
		m_tables.push_back(std::move(table));
	}
}
