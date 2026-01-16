#include "AddItemModule.h"
#include "ui/components/UIContainers.h"
#include "config/UserData.h"

namespace Modex
{
	// https://github.com/ocornut/imgui/issues/319
	void AddItemModule::Draw(float a_offset)
	{
       	const ImVec2 window_padding = ImGui::GetStyle().WindowPadding;
		const float full_width = ImGui::GetContentRegionAvail().x;
        const float button_width = ImGui::GetContentRegionAvail().x;
        const float button_height = ImGui::GetFontSize() * 1.5f;
        const float tab_bar_height = button_height + (ImGui::GetStyle().WindowPadding.y * 2.0f);
        // const float panel_width = ImGui::GetContentRegionAvail().x * 0.375f;
		// const float panel_height = (ImGui::GetFrameHeightWithSpacing() * 5.0f); // Magical T_T

        const float table_width = ImGui::GetContentRegionAvail().x * 0.75f + window_padding.x;
        const float table_height = ImGui::GetContentRegionAvail().y - (window_padding.y * 2.0f) - tab_bar_height;

        // Set cursor to top left of window plus sidebar offset.
        ImGui::SameLine();
        ImGui::SetCursorPosX(window_padding.x + a_offset);
        ImGui::SetCursorPosY(window_padding.y);
        ImVec2 start_pos = ImGui::GetCursorPos();

		// Tab Button Area
		if (ImGui::BeginChild("##AddItem::TabBar", ImVec2(0.0f, button_height), 0, ImGuiWindowFlags_NoFocusOnAppearing)) {
			ImGui::PushStyleVar(ImGuiStyleVar_SelectableTextAlign, ImVec2(0.5f, 0.5f));
			if (ImGui::Selectable("Table View", m_viewport == Viewport::TableView, 0, ImVec2(button_width, 0.0f))) {
				// activeViewport = Viewport::TableView;
				// m_tableView.Load();
				// m_tableView.BuildPluginList();
			}
			ImGui::PopStyleVar();
		}
		
        ImGui::EndChild();
        ImGui::SameLine();
        ImGui::SetCursorPos(start_pos);

        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + button_height);
        ImGui::PushStyleColor(ImGuiCol_Separator, ImGui::GetStyleColorVec4(ImGuiCol_Button));
        ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal);
        ImGui::PopStyleColor();
        ImGui::SetCursorPos(start_pos);
		
		// Table View Tab
		if (m_viewport == Viewport::TableView) {
			ImVec2 table_pos = ImGui::GetCursorPos();
			table_pos.y += tab_bar_height;

			UIContainers::DrawAddItemTablePanel(table_pos, ImVec2(table_width, table_height), m_tableView);

			ImVec2 action_pos = table_pos;
			action_pos.x += table_width + window_padding.x;

			DrawAddItemActionPanel(action_pos, ImVec2(full_width - table_width - window_padding.x, 0.0f), m_tableView);
		}
	}

	void AddItemModule::Unload()
	{
		m_tableView->Unload();
	}

	void AddItemModule::Load()
	{
		m_tableView->Load();
	}

	AddItemModule::AddItemModule() {
		
		m_clickCount = 1;
		m_viewport = Viewport::TableView;
		
		m_tableView = std::make_unique<UITable>();
		m_tableView->SetGenerator([]() { return Data::GetSingleton()->GetAddItemList(); });
		m_tableView->SetPluginType(Data::PLUGIN_TYPE::ITEM);
		m_tableView->SetDataID("AddItem");
		m_tableView->SetDragDropID("FROM_TABLE");
		// m_tableView->SetClickAmount(&m_clickCount);
		m_tableView->AddFlag(UITable::ModexTableFlag_Base);
		m_tableView->AddFlag(UITable::ModexTableFlag_EnablePluginKitView);
		m_tableView->AddFlag(UITable::ModexTableFlag_EnableCategoryTabs);
		m_tableView->AddFlag(UITable::ModexTableFlag_EnableHeader);
		m_tableView->Init();
		m_tableView->SetShowEditorID(UserData::User().Get<bool>("AddItem::ShowEditorID", false));
		m_tableView->SetShowPluginKitView(UserData::User().Get<bool>("AddItem::ShowPluginKitView", false));
	}
}