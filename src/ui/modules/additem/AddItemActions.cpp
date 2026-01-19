#include "AddItemModule.h"
#include "core/Commands.h"
#include "config/UserData.h"
#include "config/ThemeConfig.h"
#include "core/PlayerChestSpawn.h"
#include "localization/Locale.h"
#include "ui/components/ItemPreview.h"
#include "ui/components/UIContainers.h"

namespace Modex
{
	static inline uint32_t ADDITEM_MAX_QUERY = 100;

	// TODO: Why is this here and not in UIContainers?
	void AddItemModule::DrawAddItemActionPanel(const ImVec2 &a_pos, const ImVec2 &a_size, std::unique_ptr<UITable> &a_view)
	{
		const float button_height = ImGui::GetFontSize() * 1.5f;
		const ImVec4 button_color = ThemeConfig::GetColor("BUTTON");
		const ImVec4 cont_color = ThemeConfig::GetColor("CONTAINER_BUTTON");
		bool valid_npc = false;

		const int count = a_view->GetClickAmount() == nullptr ? 1 : *a_view->GetClickAmount();
		const bool playerToggle = UserData::User().Get<bool>("Modex::TargetReference::IsPlayer", true);
		const std::string toggle_text = playerToggle ? Translate("TARGET_PLAYER") : Translate("TARGET_NPC");

		ImGui::SameLine();
        ImGui::SetCursorPos(a_pos);
		if (ImGui::BeginChild("Modex::AddItemWindow::Actions", a_size)) {
			const float max_width = ImGui::GetContentRegionAvail().x;
			const float button_half_width = (max_width / 2.0f) - (ImGui::GetStyle().WindowPadding.x / 2.0f);

			UICustom::SubCategoryHeader(Translate("HEADER_ACTIONS"));
			
			ImGui::SetNextItemWidth(a_size.x);
			ImGui::InputInt("##AddItem::Count", a_view->GetClickAmount(), 1, 100, ImGuiInputTextFlags_CharsDecimal);
			
			ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal);

			if (!playerToggle) {
				std::string npc_name = Translate("NO_CONSOLE_SELECTION");

				if (a_view->GetTableTargetRef() != nullptr) {
					npc_name = a_view->GetTableTargetRef()->GetName();
					valid_npc = a_view->GetTableTargetRef()->GetFormType() == RE::FormType::ActorCharacter;
				}

				if (ImGui::BeginChild("Modex::AddItem::NPCHeader", ImVec2(max_width, button_height * 2.0f), true)) {
					const float center_x = (max_width - ImGui::CalcTextSize(npc_name.c_str()).x) / 2.0f;
					const float center_y = (button_height * 2.0f - ImGui::CalcTextSize(npc_name.c_str()).y) / 2.0f;
					ImGui::SetCursorPos(ImVec2(center_x, center_y));
					ImGui::Text("%s", npc_name.c_str());
				}

				ImGui::EndChild();
			}

			// TODO: Add QueryCheck implementation.
			if (UIContainers::ActionButton("CONTAINER_VIEW", ImVec2(max_width, button_height), true, cont_color)) {
				if (a_view->GetSelectionCount() > 0) {
					PlayerChestSpawn::GetSingleton()->PopulateChestWithItems(a_view->GetSelection());
				} else {
					PlayerChestSpawn::GetSingleton()->PopulateChestWithItems(a_view->GetTableList());
				}
			}

			if (playerToggle) {
				if (UIContainers::ActionButton("ADD_SELECTION", ImVec2(max_width, button_height), a_view->GetSelectionCount() > 0, button_color)) {
					if (UIContainers::QueryCheck(a_view->GetSelectionCount() > 0)) {
						a_view->AddSelectionToInventory(count);
					} else {
						a_view->AddSelectionToInventory(count);
					}
				}

				if (UIContainers::ActionButton("PLACE_SELECTION", ImVec2(max_width, button_height), a_view->GetSelectionCount() > 0, button_color)) {
					if (UIContainers::QueryCheck(a_view->GetSelectionCount() > 0)) {
						a_view->PlaceSelectionOnGround(count);
					} else {
						a_view->PlaceSelectionOnGround(count);
					}
				}

				if (UIContainers::ActionButton("EQUIP_SELECTION", ImVec2(max_width, 0), a_view->GetSelectionCount() > 0, button_color)) {
					if (UIContainers::QueryCheck(a_view->GetSelectionCount() > 0)) {
						a_view->EquipSelection();
					} else {
						a_view->EquipSelection();
					}
				}

				if (UIContainers::ActionButton("CLEAR_INVENTORY", ImVec2(max_width, button_height), true, button_color)) {
					UIManager::GetSingleton()->ShowWarning("Clear Inventory", Translate("GENERAL_CLEAR_INVENTORY_INSTRUCTION"), []() {
						if (auto player = RE::PlayerCharacter::GetSingleton()->AsReference(); player) {
							Commands::RemoveAllItemsFromInventory(player);
						}
					});
				}
			} else {
				const bool is_enabled = valid_npc && a_view->GetSelectionCount() > 0;

				if (UIContainers::ActionButton("ADD_SELECTION_NPC", ImVec2(max_width, button_height), is_enabled, button_color)) {
					if (a_view->GetSelectionCount() >= ADDITEM_MAX_QUERY) {
						UIManager::GetSingleton()->ShowWarning("Large Query", Translate("WARNING_LARGE_QUERY"), [&a_view, count]() {
							a_view->AddSelectionToInventory(count);
						});
					} else {
						a_view->AddSelectionToInventory(count);
					}
				}

				if (UIContainers::ActionButton("EQUIP_SELECTION_NPC", ImVec2(max_width, 0), is_enabled, button_color)) {
					if (a_view->GetSelectionCount() >= ADDITEM_MAX_QUERY) {
						UIManager::GetSingleton()->ShowWarning("Large Query",Translate("WARNING_LARGE_QUERY"), [&a_view]() {
							a_view->EquipSelection();
						});
					} else {
						a_view->EquipSelection();
					}
				}

				if (UIContainers::ActionButton("CLEAR_INVENTORY_NPC", ImVec2(max_width, button_height), valid_npc, button_color)) {
					UIManager::GetSingleton()->ShowWarning("Clear Inventory", Translate("GENERAL_CLEAR_INVENTORY_INSTRUCTION"), [&a_view]() {
						if (const auto ref = a_view->GetTableTargetRef(); ref != nullptr) {
							Commands::RemoveAllItemsFromInventory(ref);
						}
					});
				}

				if (UIContainers::ActionButton("RESET_INVENTORY_NPC", ImVec2(max_width, button_height), valid_npc, button_color)) {
					UIManager::GetSingleton()->ShowWarning("Reset Inventory", Translate("GENERAL_RESET_INVENTORY_INSTRUCTION"), [&a_view]() {
						if (const auto ref = a_view->GetTableTargetRef(); ref != nullptr) {
							Commands::ResetTargetInventory(ref);
						}
					});
				}

			}

			ImGui::Spacing();
			ImGui::Spacing();
			ImGui::Spacing();

			UICustom::SubCategoryHeader(Translate("HEADER_PREVIEW"));

			ShowItemPreview(a_view->GetItemPreview());
		}

		ImGui::EndChild();
	}
}
