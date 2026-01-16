#include "ActorModule.h"

#include "ui/core/UIManager.h"
#include "ui/components/UICustom.h"
#include "localization/Locale.h"
#include "ui/components/ItemPreview.h"

namespace Modex
{
	void ActorModule::ShowActions()
	{
		const float button_height = ImGui::GetFontSize() * 1.5f;
		const float button_width = ImGui::GetContentRegionAvail().x;

		ImGui::SubCategoryHeader(Translate("GENERAL_DOUBLE_CLICK_BEHAVIOR"));

		bool _true_ = true;
		if (ImGui::GradientSelectableEX(TranslateIcon(ICON_LC_MAP_PIN_PLUS, "GENERAL_CLICK_TO_PLACE"), _true_, ImVec2(button_width, button_height))) {
			// Nothing
		}

		if (_true_) {
			ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal);

			// Add Count Input.
			ImGui::SetNextItemWidth(button_width);
			ImGui::InputInt("##NPCPlaceCount", &m_clickCount, 1, 100, ImGuiInputTextFlags_CharsDecimal);
		}

		ImGui::Spacing();
		ImGui::SubCategoryHeader(Translate("Actions"));

		if (ImGui::GradientButton(Translate("NPC_PLACE_SELECTED"), ImVec2(button_width, 0))) {
			m_tableView->PlaceSelectionOnGround(m_clickCount);
		}

		if (ImGui::GradientButton(Translate("NPC_UPDATE_REFERENCES"), ImVec2(button_width, 0))) {
			Data::GetSingleton()->CacheNPCRefIds();
			m_tableView->Refresh();
		}

		if (ImGui::IsItemHovered()) {
			ImGui::SetTooltip("%s", Translate("TOOLTIP_NPC_UPDATE"));
		}

		const auto& selectedNPC = GetTableView()->GetItemPreview();

		if (selectedNPC != nullptr) {
			// if (selectedNPC->refID != 0) {
			// 	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(a_style.button.x, a_style.button.y, a_style.button.z, a_style.button.w));
			// 	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(a_style.buttonHovered.x, a_style.buttonHovered.y, a_style.buttonHovered.z, a_style.buttonHovered.w));
			// } else {
			// 	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(a_style.button.x, a_style.button.y, a_style.button.z, a_style.button.w - 0.35f));
			// 	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(a_style.button.x, a_style.button.y, a_style.button.z, a_style.button.w - 0.35f));
			// }

			if (ImGui::GradientButton(Translate("NPC_GOTO_REFERENCE"), ImVec2(button_width, 0))) {
				if (selectedNPC->m_refID != 0) {
					if (auto playerREF = RE::PlayerCharacter::GetSingleton()->AsReference()) {
						if (auto ref = RE::TESForm::LookupByID<RE::TESObjectREFR>(selectedNPC->m_refID)) {
							playerREF->MoveTo(ref);
							UIManager::GetSingleton()->Close();
						}
					}
				}
			}

			if (ImGui::GradientButton(Translate("NPC_BRING_REFERENCE"), ImVec2(button_width, 0))) {
				if (selectedNPC->m_refID != 0) {
					if (auto playerREF = RE::PlayerCharacter::GetSingleton()->AsReference()) {
						if (auto ref = RE::TESForm::LookupByID<RE::TESObjectREFR>(selectedNPC->m_refID)) {
							ref->MoveTo(playerREF);
							UIManager::GetSingleton()->Close();
						}
					}
				}
			}

			// ImGui::PopStyleColor(2);
		}

		if (selectedNPC == nullptr) {
			return;
		}

		ImGui::Spacing();
		ImGui::SubCategoryHeader(Translate("HEADER_PREVIEW"));
		ShowItemPreview(m_tableView->GetItemPreview());
	}
}