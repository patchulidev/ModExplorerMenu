#include "ActorModule.h"
#include "ui/core/UIManager.h"
#include "ui/components/UICustom.h"
#include "localization/Locale.h"
#include "ui/components/ItemPreview.h"

namespace Modex
{
	// void ActorModule::ShowActions()
	// {
	// 	const float button_width = ImGui::GetContentRegionAvail().x;
	// 	UICustom::SubCategoryHeader(Translate("GENERAL_DOUBLE_CLICK_BEHAVIOR"));
	//
	// 	bool _true_ = true;
	// 	if (ImGui::Selectable(TranslateIcon(ICON_LC_MAP_PIN_PLUS, "GENERAL_CLICK_TO_PLACE"), _true_)) {
	// 		// Nothing
	// 	}
	//
	// 	if (_true_) {
	// 		ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal);
	//
	// 		// Add Count Input.
	// 		ImGui::SetNextItemWidth(button_width);
	// 		ImGui::InputInt("##NPCPlaceCount", &m_clickCount, 1, 100, ImGuiInputTextFlags_CharsDecimal);
	// 	}
	//
	// 	ImGui::Spacing();
	// 	UICustom::SubCategoryHeader(Translate("Actions"));
	//
	// 	if (ImGui::Button(Translate("NPC_PLACE_SELECTED"), ImVec2(button_width, 0))) {
	// 		m_tableView->PlaceSelectionOnGround(m_clickCount);
	// 	}
	//
	// 	if (ImGui::Button(Translate("NPC_UPDATE_REFERENCES"), ImVec2(button_width, 0))) {
	// 		Data::GetSingleton()->CacheNPCRefIds();
	// 		m_tableView->Refresh();
	// 	}
	//
	// 	if (ImGui::IsItemHovered()) {
	// 		ImGui::SetTooltip("%s", Translate("TOOLTIP_NPC_UPDATE"));
	// 	}
	//
	// 	const auto& selectedNPC = GetTableView()->GetItemPreview();
	//
	// 	if (selectedNPC != nullptr) {
	// 		if (ImGui::Button(Translate("NPC_GOTO_REFERENCE"), ImVec2(button_width, 0))) {
	// 			if (selectedNPC->m_refID != 0) {
	// 				if (auto playerREF = RE::PlayerCharacter::GetSingleton()->AsReference()) {
	// 					if (auto ref = RE::TESForm::LookupByID<RE::TESObjectREFR>(selectedNPC->m_refID)) {
	// 						playerREF->MoveTo(ref);
	// 						UIManager::GetSingleton()->Close();
	// 					}
	// 				}
	// 			}
	// 		}
	//
	// 		if (ImGui::Button(Translate("NPC_BRING_REFERENCE"), ImVec2(button_width, 0))) {
	// 			if (selectedNPC->m_refID != 0) {
	// 				if (auto playerREF = RE::PlayerCharacter::GetSingleton()->AsReference()) {
	// 					if (auto ref = RE::TESForm::LookupByID<RE::TESObjectREFR>(selectedNPC->m_refID)) {
	// 						ref->MoveTo(playerREF);
	// 						UIManager::GetSingleton()->Close();
	// 					}
	// 				}
	// 			}
	// 		}
	// 	}
	//
	// 	if (selectedNPC == nullptr) {
	// 		return;
	// 	}
	//
	// 	ImGui::Spacing();
	// 	UICustom::SubCategoryHeader(Translate("HEADER_PREVIEW"));
	// 	ShowItemPreview(m_tableView->GetItemPreview());
	// }
}
