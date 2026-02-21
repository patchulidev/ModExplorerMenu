#include "UIPopup.h"
#include "config/ThemeConfig.h"
#include "ui/components/UICustom.h"
#include "ui/components/UIModule.h"

namespace Modex
{
	void UIPopupReferenceLookup::FilterList(const std::string& a_filter)
	{
		const auto& cache = Data::GetSingleton()->GetNPCList();

		s_cache.objects.clear();

		for (const auto& obj : cache) {
			if (obj.GetRefID() == 0) {
				continue;
			}

			if (!a_filter.empty()) {
				bool matches = false;
				std::string name = obj.GetName();

				if (name.empty()) {
					name = obj.GetEditorID();
				}

				std::string compare = a_filter;
				std::transform(compare.begin(), compare.end(), compare.begin(), ::tolower);

				std::transform(name.begin(), name.end(), name.begin(), ::tolower);
				if (name.find(compare) != std::string::npos) {
					matches = true;
				}

				std::string formIDStr = std::format("{:08X}", obj.GetRefID());
				std::transform(formIDStr.begin(), formIDStr.end(), formIDStr.begin(), ::tolower);
				if (formIDStr.find(compare) != std::string::npos) {
					matches = true;
				}

				if (!matches) {
					continue;
				}
			}
			
			s_cache.objects.push_back(&obj);
		}
	}

	void UIPopupReferenceLookup::SortTable()
	{
		if (ImGuiTableSortSpecs* sortSpecs = ImGui::TableGetSortSpecs()) {
			if (sortSpecs->SpecsDirty || 
				s_cache.sortColumn != sortSpecs->Specs[0].ColumnUserID ||
				s_cache.sortDirection != sortSpecs->Specs[0].SortDirection) {
				
				const ImGuiTableColumnSortSpecs* spec = &sortSpecs->Specs[0];
				
				std::sort(s_cache.objects.begin(), s_cache.objects.end(), 
					[spec](const BaseObject* a, const BaseObject* b) {
						int result = 0;
						
						switch (spec->ColumnUserID) {
							case 0: { // Name
								std::string nameA = a->GetName();
								if (nameA.empty()) nameA = a->GetEditorID();
								
								std::string nameB = b->GetName();
								if (nameB.empty()) nameB = b->GetEditorID();
								
								result = nameA.compare(nameB);
								break;
							}
							case 1: // Reference ID
								result = (a->GetRefID() < b->GetRefID()) ? -1 : 
										 (a->GetRefID() > b->GetRefID()) ? 1 : 0;
								break;
						}
						
						return (spec->SortDirection == ImGuiSortDirection_Ascending) ? 
							   (result < 0) : (result > 0);
					});
				
				s_cache.sortColumn = spec->ColumnUserID;
				s_cache.sortDirection = spec->SortDirection;
				sortSpecs->SpecsDirty = false;
			}
		}
	}

	void UIPopupReferenceLookup::DrawTable()
	{
		ImGui::NewLine();
		ImGui::Spacing();

		if (ImGui::BeginChild("##Modex::ReferenceLookup::List", ImVec2(0, 200), false)) {
			static ImGuiTableFlags flags = ImGuiTableFlags_Sortable | ImGuiTableFlags_ScrollY;
			const float reference_width = ImGui::GetContentRegionAvail().x / 3.0f;
			
			if (ImGui::BeginTable("##Modex::ReferenceLookup::Table", 2, flags)) {
				ImGui::TableSetupColumn(Translate("kName"), ImGuiTableColumnFlags_DefaultSort | ImGuiTableColumnFlags_WidthStretch, 0.0f, 0);
				ImGui::TableSetupColumn(Translate("kReferenceID"), ImGuiTableColumnFlags_WidthFixed, reference_width, 1);
				ImGui::TableSetupScrollFreeze(0, 1);
				ImGui::TableHeadersRow();
				
				SortTable();
				
				for (auto& npc : s_cache.objects) {
					std::string displayName = npc->GetName();
					if (displayName.empty()) {
						displayName = npc->GetEditorID();
					}
					
					ImGui::TableNextRow();
					ImGui::TableNextColumn();
					
					ImGui::PushID(npc->GetRefID());
					ImGui::PushStyleVar(ImGuiStyleVar_SelectableTextAlign, ImVec2(0.0f, 0.5f));
					if (ImGui::Selectable(displayName.c_str(), m_currentSelection == npc->m_refID, ImGuiSelectableFlags_SpanAllColumns)) {
						m_currentSelection = npc->m_refID;
						AcceptEntry();
					}
					ImGui::PopStyleVar();
					
					ImGui::TableNextColumn();
					ImGui::TextColored(ThemeConfig::GetColor("TEXT"), "%08X", npc->GetRefID());
					ImGui::PopID();
				}
				
				ImGui::EndTable();
			}
		}
		ImGui::EndChild();
	}

	void UIPopupReferenceLookup::Draw()
	{
		static float height;
		auto width = ImGui::GetMainViewport()->Size.x * 0.25f;
		const float center_x = ImGui::GetMainViewport()->Size.x * 0.5f;
		const float center_y = ImGui::GetMainViewport()->Size.y * 0.5f;
		const float pos_x = center_x - (width * 0.5f);
		const float pos_y = center_y - (height * 0.5f);

		DrawPopupBackground(m_alpha);

		ImGui::SetNextWindowSize(ImVec2(width, 0));
		ImGui::SetNextWindowPos(ImVec2(pos_x, pos_y));
		ImGui::SetNextWindowFocus();

		ImGui::PushStyleVar(ImGuiStyleVar_Alpha, m_alpha);
		if (ImGui::Begin("##Modex::ReferenceLookup", NULL, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoFocusOnAppearing)) {
			if (ImGui::IsWindowAppearing()) {
				ImGui::GetIO().ClearInputKeys();
			}

			if (UICustom::Popup_MenuHeader(m_pendingFuzzyListTitle.c_str())) {
				DeclineEntry();
			}

			if (ImGui::IsKeyPressed(ImGuiKey_LeftArrow) || ImGui::IsKeyPressed(ImGuiKey_RightArrow)) {
				if (!ImGui::GetIO().WantTextInput) {
					m_navAccept = !m_navAccept;
				}
			}

			ImGui::TextWrapped("%s", m_pendingFuzzyDesc.c_str());
			ImGui::NewLine();

			static char currentSearch[256] = "";
			ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
			ImGui::PushStyleColor(ImGuiCol_FrameBg, ThemeConfig::GetColor("BG_LIGHT", m_alpha));
			ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ThemeConfig::GetHover("BG_LIGHT", m_alpha));
			ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ThemeConfig::GetActive("BG_LIGHT", m_alpha));
			if (UICustom::FancyInputText("##Modex::ReferenceLookup::InputText", "", "", currentSearch, ImGui::GetContentRegionAvail().x, ImGuiInputTextFlags_EnterReturnsTrue)) {
				RE::FormID refID = 0;
				if (TryParseFormID(currentSearch, refID)) {
					m_currentSelection = refID;
					AcceptEntry();
				}
			}
			ImGui::PopStyleColor(3);

			if (ImGui::IsItemEdited()) {
				FilterList(std::string(currentSearch));
			}

			if (ImGui::IsWindowAppearing()) {
				ImGui::SetKeyboardFocusHere(-1);
			}

			DrawTable();

			bool _confirm, _cancel;
			if (UICustom::Popup_ConfirmDeclineButtons(_confirm, _cancel, m_navAccept)) {
				if (_confirm) {
					if (currentSearch[0] != '\0') {
						RE::FormID refID = 0;

						if (TryParseFormID(currentSearch, refID) && refID != 0) {
							m_currentSelection = refID;
						}
					}

					AcceptEntry();
				} else if (_cancel) {
					DeclineEntry();
				}
			}

			height = ImGui::GetWindowSize().y;
		}

		ImGui::End();
		ImGui::PopStyleVar();
	}

	void UIPopupReferenceLookup::PopupReferenceLookup(const std::string& a_title, const std::string& a_message, std::function<void(RE::FormID)> a_onSelectCallback)
	{
		m_pendingFuzzyListTitle = a_title;
		m_pendingFuzzyDesc = a_message;
		m_currentSelection = 0;
		m_onSelectCallback = a_onSelectCallback;
		m_captureInput = true;
		m_navAccept = true;

		FilterList("");
	}

	void UIPopupReferenceLookup::AcceptEntry()
	{
		if (m_onSelectCallback && m_currentSelection != 0) {
			m_onSelectCallback(m_currentSelection);
		}

		CloseWindow();
	}

	void UIPopupReferenceLookup::DeclineEntry()
	{
		CloseWindow();
	}
}
