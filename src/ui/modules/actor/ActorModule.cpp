#include "ActorModule.h"
#include "config/UserData.h"
#include "localization/Locale.h"
#include "ui/components/UIContainers.h"

namespace Modex
{
	void ActorModule::Draw()
	{
		DrawTabMenu();
	}

	void DrawTableLayout(std::vector<std::unique_ptr<UITable>>& a_tables)
	{
		const ImVec2 window_padding = ImGui::GetStyle().WindowPadding;
		const float table_width = ImGui::GetContentRegionAvail().x * 0.75f + window_padding.x;
		const float table_height = ImGui::GetContentRegionAvail().y - (window_padding.y * 2.0f);

		ImVec2 table_pos = ImGui::GetCursorPos();
		UIContainers::DrawBasicTablePanel(table_pos, ImVec2(table_width, table_height), a_tables[0]);

		ImVec2 action_pos = table_pos;
		action_pos.x += table_width + window_padding.x;
		ImGui::SetCursorPos(action_pos);

		//TODO: Re-implmement NPC Actions Panel. Apparently this is long gone?
	}

	ActorModule::ActorModule()
	{
		m_name = Translate("MODULE_ACTOR");
		m_icon = ICON_LC_USER_PLUS;

		m_layouts.push_back({"Table View", true, DrawTableLayout}); // TODO: Locale

		auto table = std::make_unique<UITable>();
		table->SetGenerator([]() { return Data::GetSingleton()->GetNPCList(); });
		table->SetPluginType(Data::PLUGIN_TYPE::Actor);
		table->SetUserDataID("Actor");
		table->AddFlag(UITable::ModexTableFlag_EnableCategoryTabs);
		table->AddFlag(UITable::ModexTableFlag_EnableHeader);
		table->SetShowEditorID(UserData::User().Get<bool>("NPC::ShowEditorID", false));
		table->Init();
		m_tables.push_back(std::move(table));
	}
}
