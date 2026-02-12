#include "UIContainers.h"
#include "core/Commands.h"
#include "config/UserData.h"
#include "core/PlayerChestSpawn.h"
#include "config/EquipmentConfig.h"
#include "config/ThemeConfig.h"
#include "localization/Locale.h"
#include "ui/components/ItemPreview.h"

namespace Modex
{
	// Used exclusively in the UITable renderer for the tree node filter buttons.
	bool UIContainers::TabButton(const char* a_label, const ImVec2& a_size, const bool a_condition, const ImVec4& a_color)
	{
		bool success = false;

		const ImVec4 active_color = ImVec4(a_color.x, a_color.y, a_color.z, a_color.w);
		const ImVec4 inactive_color = ImVec4(a_color.x - 0.20f, a_color.y - 0.20f, a_color.z - 0.20f, a_color.w * 0.5f);

		ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 2.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
		ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(.0f, .0f, .0f, 1.0f));

		if (a_condition) {
			ImGui::PushStyleColor(ImGuiCol_Button, active_color);
			success = ImGui::Button(a_label, a_size);
			ImGui::PopStyleColor();
		} else {
			ImGui::PushStyleColor(ImGuiCol_Button, inactive_color);
			success = ImGui::Button(a_label, a_size);
			ImGui::PopStyleColor();
		}

		ImGui::PopStyleVar(2);
		ImGui::PopStyleColor();

		return success;
	}

	void UIContainers::DrawAddItemActionPanel(const ImVec2 &a_pos, const ImVec2 &a_size, std::unique_ptr<UITable> &a_view)
	{
		const float button_height = ImGui::GetFontSize() * 1.5f;

		ImGui::SameLine();
        ImGui::SetCursorPos(a_pos);
		if (ImGui::BeginChild("Modex::AddItemWindow::Actions", a_size)) {
			const float max_width = ImGui::GetContentRegionAvail().x;
			const auto shift_down = ImGui::GetIO().KeyShift;
			const bool action_allowed = a_view->IsActionAllowed();

			UICustom::SubCategoryHeader(Translate("HEADER_ACTIONS"));
			
			ImGui::PushStyleColor(ImGuiCol_Button, ThemeConfig::GetColor("SECONDARY"));
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ThemeConfig::GetHover("SECONDARY"));
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, ThemeConfig::GetActive("SECONDARY"));
			if (UICustom::ActionButton("CONTAINER_VIEW", ImVec2(max_width, button_height), action_allowed)) {
				if (a_view->GetSelectionCount() > 0) {
					PlayerChestSpawn::GetSingleton()->PopulateChestWithItems(a_view->GetSelection());
				} else {
					PlayerChestSpawn::GetSingleton()->PopulateChestWithItems(a_view->GetTableList());
				}
			}
			ImGui::PopStyleColor(3);

			if (UICustom::ActionButton("ADD_SELECTION", ImVec2(max_width, button_height), action_allowed)) {
				UICustom::InputAmountHandler(shift_down, [&a_view](uint32_t amount) {
					a_view->AddSelectionToTargetInventory(amount);
				});
			}

			if (UICustom::ActionButton("PLACE_SELECTION", ImVec2(max_width, button_height), action_allowed)) {
				UICustom::InputAmountHandler(shift_down, [&a_view](uint32_t amount) {
					a_view->PlaceSelectionOnGround(amount);
				});
			}

			if (UICustom::ActionButton("EQUIP_SELECTION", ImVec2(max_width, button_height), action_allowed)) {
				a_view->EquipSelectionToTarget();
			}

			const auto title = shift_down ? Translate("RESET_INVENTORY") : Translate("CLEAR_INVENTORY");
			const auto description = shift_down ? Translate("RESET_INVENTORY_DESC") : Translate("CLEAR_INVENTORY_DESC");

			if (UICustom::ActionButton(title, ImVec2(max_width, button_height), a_view->IsValidTargetReference())) {
				UIManager::GetSingleton()->ShowWarning(Translate("CLEAR_INVENTORY"), description, [&a_view, shift_down]() {
					if (auto target = a_view->GetTableTargetRef(); target) {
						if (shift_down) {
							Commands::ResetTargetInventory(target);
						} else {
							Commands::RemoveAllItemsFromInventory(target);
						}
					}
				});
			}

			ImGui::Spacing();
			ImGui::Spacing();

			UICustom::SubCategoryHeader(Translate("HEADER_PREVIEW"));

			ShowItemPreview(a_view->GetItemPreview());
		}

		ImGui::EndChild();
	}

	// Can be used globally as a general purpose BaseObject table.
	void UIContainers::DrawBasicTablePanel(const char* a_localeText, const ImVec2 &a_pos, const ImVec2 &a_size, std::unique_ptr<UITable> &a_view)
	{
		ImGui::SameLine();
		ImGui::SetCursorPos(a_pos);
		if (ImGui::BeginChild(a_localeText, a_size, false, ImGuiWindowFlags_NoBringToFrontOnFocus)) {
			UICustom::SubCategoryHeader(Translate(a_localeText));
			ImGui::Spacing();
			a_view->Draw(a_view->GetTableList());
		}
		ImGui::EndChild();
	}
}
