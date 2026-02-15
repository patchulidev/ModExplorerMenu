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
		const float full_width = ImGui::GetContentRegionAvail().x;

		ImVec2 table_pos = ImGui::GetCursorPos();
		UIContainers::DrawBasicTablePanel("TABLE_ACTOR", table_pos, ImVec2(table_width, table_height), a_tables[0]);

		ImVec2 action_pos = table_pos;
		action_pos.x += table_width + window_padding.x;
		UIContainers::DrawActorActionPanel(action_pos, ImVec2(full_width - table_width - window_padding.x, 0.0f), a_tables[0]);

		//TODO: Re-implmement NPC Actions Panel. Apparently this is long gone?
	}

	ActorModule::~ActorModule()
	{
		// Destructor
	}

	ActorModule::ActorModule()
	{
		m_layouts.push_back({Translate("TAB_ACTOR"), true, DrawTableLayout});

		// Update NPC references when we navigate to this module :think:
		Data::GetSingleton()->CacheNPCRefIds();

		constexpr auto table_flags =
		UITable::ModexTableFlag_EnableFilterTree |
		UITable::ModexTableFlag_EnableSearch |
		UITable::ModexTableFlag_EnableHeader;

		auto table = std::make_unique<UITable>("Actor", true, 1, table_flags);

		m_tables.push_back(std::move(table));
	}
}
