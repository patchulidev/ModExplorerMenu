#include "ActorModule.h"
#include "config/UserData.h"

namespace Modex
{
	void ActorModule::Draw(float a_offset)
	{
		const auto fontScale = ImGui::GetFontSize() * 6.0f;
		const float MIN_SEARCH_HEIGHT = 65.0f + fontScale;

		const ImGuiChildFlags flags = ImGuiChildFlags_Borders | ImGuiChildFlags_AlwaysUseWindowPadding;
		float search_height = UserData::User().Get<float>("NPC::SearchHeight", MIN_SEARCH_HEIGHT);
		float search_width = UserData::User().Get<float>("NPC::SearchWidth", ImGui::GetContentRegionAvail().x * 0.45f);
		float recent_width = UserData::User().Get<float>("NPC::RecentWidth", ImGui::GetContentRegionAvail().x * 0.35f);
		float window_padding = ImGui::GetStyle().WindowPadding.y;
		const float button_width = ImGui::GetContentRegionAvail().x / 2.0f;
		const float button_height = ImGui::GetFontSize() * 1.5f;
		const float tab_bar_height = button_height + (window_padding * 2);

		if (search_height < MIN_SEARCH_HEIGHT) {
			search_height = MIN_SEARCH_HEIGHT;  // Ensure height after font scaling
		}

		ImGui::SameLine();
		ImGui::SetCursorPosY(window_padding);
		ImGui::SetCursorPosX(window_padding + a_offset);
		ImVec2 backup_pos = ImGui::GetCursorPos();

		// Tab Button Area
		if (ImGui::BeginChild("##NPC::Blacklist", ImVec2(0.0f, button_height), 0, ImGuiWindowFlags_NoFocusOnAppearing)) {
			ImGui::PushStyleVar(ImGuiStyleVar_SelectableTextAlign, ImVec2(0.5f, 0.5f));
			if (ImGui::Selectable("Table View", m_viewport == Viewport::TableView, 0, ImVec2(button_width, 0.0f))) {
				m_viewport = Viewport::TableView;
				if (this->m_tableView->GetTableList().empty()) {
					this->m_tableView->Refresh();
				}

				this->m_tableView->BuildPluginList();
			}

			ImGui::PopStyleVar();
		}
		ImGui::EndChild();

		// Table View Table
		if (m_viewport == Viewport::TableView) {
			// Search Input Area
			ImGui::SameLine();
			ImGui::SetCursorPos(backup_pos);
			ImGui::SetCursorPosY(tab_bar_height - window_padding);
			backup_pos = ImGui::GetCursorPos();
			if (ImGui::BeginChild("##NPC::SearchArea", ImVec2(search_width - a_offset, search_height), flags, ImGuiWindowFlags_NoFocusOnAppearing)) {
				// this->m_tableView->ShowSearch();
			}
			ImGui::EndChild();

			// Vertical Search / Recent Splitter
			ImGui::SameLine();
			ImGui::SetCursorPosX(ImGui::GetCursorPosX() - window_padding);
			ImGui::SetCursorPosY(tab_bar_height - window_padding);

			// Recent Items Area
			ImGui::SameLine();
			ImGui::SetCursorPos(backup_pos);
			ImGui::SetCursorPosY(tab_bar_height - window_padding);
			ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ((search_width - a_offset)) + window_padding);
			if (ImGui::BeginChild("##NPC::Recent", ImVec2(recent_width - window_padding, search_height), flags)) {
				// this->m_tableView->ShowRecent();
			}
			ImGui::EndChild();

			// Horizontal Search / Table Splitter
			ImGui::SetCursorPosX(ImGui::GetCursorPosX() + a_offset);
			ImGui::SetCursorPosY(backup_pos.y + search_height);

			// Table Area
			ImGui::SetCursorPosX(ImGui::GetCursorPosX() + a_offset);
			ImGui::SetCursorPosY(ImGui::GetCursorPosY() - (window_padding / 2));
			if (ImGui::BeginChild("##NPC::TableArea", ImVec2(search_width + recent_width - a_offset, 0), flags, ImGuiWindowFlags_NoFocusOnAppearing)) {
				this->m_tableView->ShowSort();
				ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal);
				this->m_tableView->Draw(this->m_tableView->GetTableList());
			}
			ImGui::EndChild();

			// Vertical Search Table / Action Splitter
			ImGui::SameLine();
			ImGui::SetCursorPosX(ImGui::GetCursorPosX() - window_padding);
			ImGui::SetCursorPosY(tab_bar_height - window_padding);

			// Action Area
			ImGui::SameLine();
			ImGui::SetCursorPosY(tab_bar_height - window_padding);
			ImGui::SetCursorPosX(ImGui::GetCursorPosX() - window_padding);
			if (ImGui::BeginChild("##NPC::ActionArea",
					ImVec2(ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y), flags, ImGuiWindowFlags_NoFocusOnAppearing)) {
				this->ShowActions();
			}
			ImGui::EndChild();

			// Persist Search Area Width/Height
			UserData::User().Set<float>("NPC::SearchWidth", search_width);
			UserData::User().Set<float>("NPC::RecentWidth", recent_width);
			UserData::User().Set<float>("NPC::SearchHeight", search_height);
		}
	}

	void ActorModule::Unload()
	{
		m_tableView->Unload();
	}

	void ActorModule::Load()
	{
		m_tableView->Load();
	}

	ActorModule::ActorModule()
	{
		m_clickCount = 1;
		m_viewport = Viewport::TableView;

		m_tableView = std::make_unique<UITable>();
		m_tableView->SetGenerator([]() { return Data::GetSingleton()->GetNPCList(); });
		m_tableView->SetPluginType(Data::PLUGIN_TYPE::Actor);
		// m_tableView->SetClickAmount(&m_clickCount);
		m_tableView->SetUserDataID("Actor");
		m_tableView->Init();
		m_tableView->SetShowEditorID(UserData::User().Get<bool>("NPC::ShowEditorID", false));
	}
}
