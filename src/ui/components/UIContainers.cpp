#include "UIContainers.h"
#include "RE/B/BSContainer.h"
#include "core/Commands.h"
#include "config/UserData.h"
#include "core/PlayerChestSpawn.h"
#include "config/EquipmentConfig.h"
#include "config/ThemeConfig.h"
#include "data/BaseObject.h"
#include "external/icons/IconsLucide.h"
#include "imgui_internal.h"
#include "localization/Locale.h"
#include "ui/components/ItemPreview.h"
#include "ui/style/LayoutMetrics.h"

namespace Modex
{
	// Used exclusively in the UITable renderer for the tree node filter buttons.
	bool UIContainers::TabButton(const char* a_label, const ImVec2& a_size, const bool a_condition, const ImVec4& a_color)
	{
		bool success = false;

		const auto& tb = ThemeConfig::GetWidgetStyle().tabButton;
		const ImVec4 active_color = ImVec4(
			a_color.x + tb.activeValueDelta,
			a_color.y + tb.activeValueDelta,
			a_color.z + tb.activeValueDelta,
			a_color.w * tb.activeAlphaScale);
		const ImVec4 inactive_color = ImVec4(
			a_color.x + tb.inactiveValueDelta,
			a_color.y + tb.inactiveValueDelta,
			a_color.z + tb.inactiveValueDelta,
			a_color.w * tb.inactiveAlphaScale);

		ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, Style::Metrics().radiusSm);
		ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, tb.borderSize);
		ImGui::PushStyleColor(ImGuiCol_Border, tb.borderColor);

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
		const float button_height = ImGui::GetFontSize() * ThemeConfig::GetWidgetStyle().actionButton.heightScale;

		ImGui::SameLine();
        ImGui::SetCursorPos(a_pos);
		if (ImGui::BeginChild("Modex::AddItemWindow::Actions", a_size)) {
			const float max_width = ImGui::GetContentRegionAvail().x;
			const auto shift_down = ImGui::GetIO().KeyShift;
			const bool action_allowed = a_view->IsActionAllowed();
			const bool is_player_target = a_view->GetTableTargetRef() ? a_view->GetTableTargetRef()->IsPlayerRef() : false;
			const auto action_header = a_view->IsValidTargetReference() ? a_view->GetTableTargetRef()->GetName() : Translate("HEADER_ACTIONS");

			UICustom::SubCategoryHeader(action_header);
			
			ImGui::PushStyleColor(ImGuiCol_Button, ThemeConfig::GetColor("SECONDARY"));
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ThemeConfig::GetHover("SECONDARY"));
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, ThemeConfig::GetActive("SECONDARY"));
			if (UICustom::ActionButton("CONTAINER_VIEW_TARGET", ImVec2(max_width, button_height), !is_player_target && !Commands::IsGameMenuOpen() && a_view->IsValidTargetReference())) {
				Commands::OpenActorInventory(a_view->GetTableTargetRef());
			}

			{
				const float padding_x = ImGui::GetStyle().FramePadding.x;
				const float padding_y = (button_height - ImGui::GetFontSize()) * 0.5f;
				ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(padding_x, padding_y));

				const float checkbox_w = ImGui::GetFrameHeight();
				const float container_view_w = max_width - checkbox_w;

				bool hideChest = UserData::Get<bool>("ShowResults.HideChest", true);

				ImGui::PushStyleColor(ImGuiCol_FrameBg, ThemeConfig::GetColor("BG_LIGHT"));
				ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ThemeConfig::GetHover("BG_LIGHT"));
				ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ThemeConfig::GetActive("BG_LIGHT"));
				ImGui::PushStyleColor(ImGuiCol_CheckMark, ThemeConfig::GetColor("PRIMARY"));
				if (ImGui::Checkbox("##CONTAINER_VIEW_HIDDEN", &hideChest)) {
					UserData::Set<bool>("ShowResults.HideChest", hideChest);
				}
				ImGui::PopStyleColor(4);
				ImGui::PopStyleVar();

				if (ImGui::IsItemHovered()) {
					UICustom::FancyTooltip("CONTAINER_VIEW_HIDDEN_TOOLTIP");
				}

				ImGui::SameLine(0.0f, 0.0f);

				const ImVec2 text_size = ImGui::CalcTextSize(Translate("CONTAINER_VIEW"));
				const float free_space = container_view_w - text_size.x;
				const float align_x = free_space > 0.0f
					? ((max_width - text_size.x) * 0.5f - checkbox_w) / free_space
					: 0.5f;

				ImGui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign, ImVec2(align_x, 0.5f));
				if (UICustom::ActionButton("CONTAINER_VIEW", ImVec2(container_view_w, button_height), !Commands::IsGameMenuOpen())) {
					bool max_query = std::ssize(a_view->GetTableList()) >= UserConfig::Get().maxQuery;
					UIManager::GetSingleton()->ShowWarning(Translate("WARNING"), Translate("ERROR_MAX_QUERY"), max_query, [&a_view, hideChest]() {
						const auto placement = hideChest
							? PlayerChestSpawn::Placement::HiddenBelowPlayer
							: PlayerChestSpawn::Placement::InFrontOfPlayer;
						PlayerChestSpawn::GetSingleton()->PopulateChestWithItems(a_view->GetTableList(), placement);
					});
				}
				ImGui::PopStyleVar();
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

			ImGui::PushStyleColor(ImGuiCol_Button, ThemeConfig::GetColor("DECLINE"));
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ThemeConfig::GetHover("DECLINE"));
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, ThemeConfig::GetActive("DECLINE"));
			if (UICustom::ActionButton(title, ImVec2(max_width, button_height), !Commands::IsGameMenuOpen() && a_view->IsValidTargetReference())) {
				UIManager::GetSingleton()->ShowWarning(Translate("CLEAR_INVENTORY"), description, true, [&a_view, shift_down]() {
					if (auto target = a_view->GetTableTargetRef(); target) {
						if (shift_down) {
							Commands::ResetTargetInventory(a_view->GetOwnership(), target);
						} else {
							Commands::RemoveAllItemsFromInventory(a_view->GetOwnership(), target);
						}
					}
				});
			}
			ImGui::PopStyleColor(3);

			ImGui::Spacing();
			ImGui::Spacing();

			ShowItemPreview(a_view->GetItemPreview(), false, a_view->GetDataID());
		}

		ImGui::EndChild();
	}

	void UIContainers::DrawActorActionPanel(const ImVec2 &a_pos, const ImVec2 &a_size, std::unique_ptr<UITable> &a_view)
	{
		const float button_height = ImGui::GetFontSize() * ThemeConfig::GetWidgetStyle().actionButton.heightScale;

		ImGui::SameLine();
		ImGui::SetCursorPos(a_pos);
		if (ImGui::BeginChild("Modex::ActorWindow::Actions", a_size)) {
			const float max_width = ImGui::GetContentRegionAvail().x;
			const bool shift_down = ImGui::GetIO().KeyShift;

			const bool is_multiple_selected = a_view->GetSelectionCount() > 1;
			const bool valid_table_target = a_view->IsValidTargetReference() && !Commands::IsGameMenuOpen() && !a_view->GetTableTargetRef()->IsPlayerRef();
			const bool valid_selection_target = a_view->IsValidSelectionReference() && !a_view->GetSelectedReference()->IsPlayerRef() && !Commands::IsGameMenuOpen() && a_view->GetSelectionCount() == 1;
			const bool valid_multi_target = a_view->GetSelectionCount() >= 1 && !Commands::IsGameMenuOpen();

			UICustom::SubCategoryHeader(Translate("HEADER_ACTIONS"));

			ImGui::PushStyleColor(ImGuiCol_Button, ThemeConfig::GetColor("SECONDARY"));
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ThemeConfig::GetHover("SECONDARY"));
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, ThemeConfig::GetActive("SECONDARY"));
			if (UICustom::ActionButton("CONTAINER_VIEW_TARGET", ImVec2(max_width, button_height), valid_table_target)) {
				PlayerChestSpawn::GetSingleton()->PopulateChestWithActorInventory(a_view->GetTableTargetRef());
			}
			if (UICustom::ActionButton("CONTAINER_VIEW_SELECTION", ImVec2(max_width, button_height), valid_selection_target)) {
				PlayerChestSpawn::GetSingleton()->PopulateChestWithActorInventory(a_view->GetSelectedReference());
			}
			ImGui::PopStyleColor(3);

			if (a_view->GetSelectedReference()) {
				if (UICustom::ActionButton("TABLE_SET_TARGET", ImVec2(max_width, button_height), !is_multiple_selected)) {
					a_view->SetTargetByReference(a_view->GetSelectedReference());
				}
			}

			if (UICustom::ActionButton("PLACE_SELECTION", ImVec2(max_width, button_height), !Commands::IsGameMenuOpen() && valid_multi_target)) {
				UICustom::InputAmountHandler(shift_down, [&a_view](uint32_t amount) {
					a_view->ExecuteCommandOnSelection([&a_view, amount](const std::unique_ptr<BaseObject>& a_actor) {
						Commands::PlaceAtMe(a_view->GetOwnership(), a_actor->GetBaseFormID(), amount);
					});
				});
			}

			// Allow from main menu.
			if (UICustom::ActionButton("GOTO_SELECTION", ImVec2(max_width, button_height), a_view->IsValidSelectionReference())) {
				Commands::TeleportPlayerToREFR(a_view->GetOwnership(), a_view->GetSelectedReference());
			}

			// These actions are pointless within main menu, steer players to use in-game.
			if (!Commands::IsGameMenuOpen() && a_view->IsValidSelectionReference()) {
				if (UICustom::ActionButton("BRING_SELECTION", ImVec2(max_width, button_height), valid_multi_target)) {
					a_view->ExecuteCommandOnSelection([&a_view](const std::unique_ptr<BaseObject>& a_actor) {
						if (auto reference = RE::TESForm::LookupByID<RE::TESObjectREFR>(a_actor->GetRefID()); reference) {
							Commands::TeleportREFRToPlayer(a_view->GetOwnership(), reference);
						}
					});
				}

				if (a_view->GetSelectedReference()->IsDead()) {
					if (UICustom::ActionButton("RESURRECT_ACTOR", ImVec2(max_width, button_height), valid_selection_target)) {
						Commands::ResurrectRefr(a_view->GetOwnership(), a_view->GetSelectedReference());
					}
				} else {
					if (UICustom::ActionButton("KILL_ACTOR", ImVec2(max_width, button_height), valid_selection_target)) {
						Commands::KillRefr(a_view->GetOwnership(), a_view->GetSelectedReference());
					}
				}

				if (a_view->GetSelectedReference()->IsDisabled()) {
					if (UICustom::ActionButton("ENABLE_REFERENCE", ImVec2(max_width, button_height), valid_selection_target)) {
						Commands::EnableRefr(a_view->GetOwnership(), a_view->GetSelectedReference(), false);
					}
				} else {
					if (UICustom::ActionButton("DISABLE_REFERENCE", ImVec2(max_width, button_height), valid_selection_target)) {
						Commands::DisableRefr(a_view->GetOwnership(), a_view->GetSelectedReference());
					}
				}
			}

			ImGui::Spacing();
			ImGui::Spacing();

			ShowItemPreview(a_view->GetItemPreview(), false, a_view->GetDataID());
		}
		ImGui::EndChild();

	}

	static inline int16_t s_outfitLevel = 0;
	void UIContainers::DrawOutfitActionPanel(const ImVec2 &a_pos, const ImVec2 &a_size, std::unique_ptr<UITable> &a_view)
	{
		const float button_height = ImGui::GetFontSize() * ThemeConfig::GetWidgetStyle().actionButton.heightScale;

		ImGui::SameLine();
		ImGui::SetCursorPos(a_pos);
		if (ImGui::BeginChild("Modex::OutfitWindow::Actions", a_size)) {
			const float max_width = ImGui::GetContentRegionAvail().x;
			const bool action_allowed = a_view->IsActionAllowed();
			const auto shift_down = ImGui::GetIO().KeyShift;
			const auto& selection = a_view->GetSelection();

			UICustom::SubCategoryHeader(Translate("HEADER_ACTIONS"));

			ImGui::PushStyleColor(ImGuiCol_Button, ThemeConfig::GetColor("SECONDARY"));
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ThemeConfig::GetHover("SECONDARY"));
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, ThemeConfig::GetActive("SECONDARY"));
			if (UICustom::ActionButton("CONTAINER_VIEW", ImVec2(max_width, button_height), action_allowed && a_view->GetSelectionCount() == 1)) {
				if (auto form = a_view->GetSelection()[0]->GetTESForm(); form) {
					if (auto outfit = form->As<RE::BGSOutfit>(); outfit) {
						PlayerChestSpawn::GetSingleton()->PopulateChestWithOutfit(outfit, s_outfitLevel);
					}
				}
			}

			static std::string new_kit_name = "";
			if (UICustom::ActionButton("KIT_FROM_OUTFIT", ImVec2(max_width, button_height), a_view->GetSelectionCount() == 1)) {
				UIManager::GetSingleton()->ShowInputBox(
					Translate("POPUP_KIT_CREATE_TITLE"),
					Translate("POPUP_KIT_CREATE_DESC"),
					"",
					[](const std::string& a_output) {
						new_kit_name = a_output;
					}
				);
			}

			// OPTIMIZE: Handled next frame for pointer lifetime. Can be put inside the previous
			// ShowInputBox lambda if we copy the data we need instead of using references which may
			// fall out of scope when we popup a window.

			if (!new_kit_name.empty() && a_view->GetSelectionCount() == 1) {
				if (auto form = a_view->GetSelection()[0]->GetTESForm(); form) {
					if (auto outfit = form->As<RE::BGSOutfit>(); outfit) {
						EquipmentConfig::CreateKitFromOutfit(new_kit_name, outfit, s_outfitLevel);
					}
				}

				new_kit_name.clear();
			}

			ImGui::PopStyleColor(3);

			if (UICustom::ActionButton("ADD_OUTFIT_ITEMS", ImVec2(max_width, button_height), action_allowed)) {
				for (const auto& item : selection) {
					if (auto outfit = item->GetTESForm()->As<RE::BGSOutfit>()) {
						Commands::AddOutfitItemsToInventory(a_view->GetOwnership(), a_view->GetTableTargetRef(), outfit, s_outfitLevel);
					}
				}
			}

			if (UICustom::ActionButton("EQUIP_OUTFIT_ITEMS", ImVec2(max_width, button_height), action_allowed && a_view->GetSelectionCount() == 1)) {
				if (!selection.empty()) {	
					if (auto outfit = selection[0]->GetTESForm()->As<RE::BGSOutfit>()) {
						Commands::EquipOutfit(a_view->GetOwnership(), a_view->GetTableTargetRef(), outfit, s_outfitLevel);
					}
				}
			}

			if (action_allowed && a_view->GetSelectionCount() == 1 && !a_view->GetTableTargetRef()->IsPlayerRef()) {
				ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal);

				if (UICustom::ActionButton("SET_DEFAULT_OUTFIT", ImVec2(max_width, button_height), action_allowed && a_view->GetSelectionCount() == 1)) {
					if (!selection.empty()) {
						if (auto outfit = selection[0]->GetTESForm()->As<RE::BGSOutfit>()) {
							Commands::SetDefaultOutfitOnActor(a_view->GetOwnership(), a_view->GetTableTargetRef(), outfit);
						}
					}
				}

				if (UICustom::ActionButton("SET_SLEEP_OUTFIT", ImVec2(max_width, button_height), action_allowed && a_view->GetSelectionCount() == 1)) {
					if (!selection.empty()) {
						if (auto outfit = selection[0]->GetTESForm()->As<RE::BGSOutfit>()) {
							Commands::SetSleepOutfitOnActor(a_view->GetOwnership(), a_view->GetTableTargetRef(), outfit);
						}
					}
				}

				ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal);
			}

			const auto title = shift_down ? Translate("RESET_INVENTORY") : Translate("CLEAR_INVENTORY");
			const auto description = shift_down ? Translate("RESET_INVENTORY_DESC") : Translate("CLEAR_INVENTORY_DESC");

			ImGui::PushStyleColor(ImGuiCol_Button, ThemeConfig::GetColor("DECLINE"));
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ThemeConfig::GetHover("DECLINE"));
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, ThemeConfig::GetActive("DECLINE"));

			if (UICustom::ActionButton(title, ImVec2(max_width, button_height), !Commands::IsGameMenuOpen() && a_view->IsValidTargetReference())) {
				UIManager::GetSingleton()->ShowWarning(Translate("CLEAR_INVENTORY"), description, true, [&a_view, shift_down]() {
					if (auto target = a_view->GetTableTargetRef(); target) {
						if (shift_down) {
							Commands::ResetTargetInventory(Ownership::Actor, target);
						} else {
							Commands::RemoveAllItemsFromInventory(Ownership::Actor, target);
						}
					}
				});
			}
			ImGui::PopStyleColor(3);

			ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal);
			ImGui::Text("%s", Translate("DISTRIBUTE_AT_LEVEL"));
			ImGui::SameLine();
			ImGui::Text(ICON_LC_MESSAGE_CIRCLE_QUESTION);

			if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNone)) {
				UICustom::FancyTooltip("DISTRIBUTE_AT_LEVEL_TOOLTIP");
			}

			int _level = static_cast<int>(s_outfitLevel); 
			const float input_width = ImGui::GetContentRegionAvail().x / 3.0f;
			ImGui::SameLine(ImGui::GetContentRegionAvail().x - input_width);
			ImGui::SetNextItemWidth(input_width);
			if (ImGui::InputInt("##NoLabel", &_level, 1, 10)) {
				s_outfitLevel = max(0, static_cast<int16_t>(_level));
			}

			ImGui::Spacing();
			ImGui::Spacing();

			ShowItemPreview(a_view->GetItemPreview(), false, a_view->GetDataID());
		}
		ImGui::EndChild();
	}

	void UIContainers::DrawTeleportActionPanel(const ImVec2 &a_pos, const ImVec2 &a_size, std::unique_ptr<UITable> &a_view)
	{
		const float button_height = ImGui::GetFontSize() * ThemeConfig::GetWidgetStyle().actionButton.heightScale;

		ImGui::SameLine();
		ImGui::SetCursorPos(a_pos);
		if (ImGui::BeginChild("Modex::TeleportWindow::Actions", a_size)) {
			const float max_width = ImGui::GetContentRegionAvail().x;
			const bool action_allowed = a_view->GetSelectionCount() > 0;

			UICustom::SubCategoryHeader(Translate("HEADER_ACTIONS"));

			if (UICustom::ActionButton("CENTER_ON_CELL", ImVec2(max_width, button_height), action_allowed && a_view->GetSelectionCount() == 1)) {
				const auto& selection = a_view->GetSelection();
				if (!selection.empty()) {
					const auto& selected = selection[0];
					Commands::CenterOnCell(a_view->GetOwnership(), selected->GetEditorID());
					UserData::SendEvent(ModexActionType::CenterOnCell, selected);
				}
			}

			ShowFavoriteList();
		}
		ImGui::EndChild();
	}

	void UIContainers::DrawObjectActionPanel(const ImVec2 &a_pos, const ImVec2 &a_size, std::unique_ptr<UITable> &a_view)
	{
		const float button_height = ImGui::GetFontSize() * ThemeConfig::GetWidgetStyle().actionButton.heightScale;

		ImGui::SameLine();
		ImGui::SetCursorPos(a_pos);
		if (ImGui::BeginChild("Modex::ObjectWindow::Actions", a_size)) {
			const float max_width = ImGui::GetContentRegionAvail().x;
			const bool shift_down = ImGui::GetIO().KeyShift;
			const bool action_allowed = a_view->IsActionAllowed();

			UICustom::SubCategoryHeader(Translate("HEADER_ACTIONS"));

			if (UICustom::ActionButton("PLACE_SELECTION", ImVec2(max_width, button_height), action_allowed)) {
				UICustom::InputAmountHandler(shift_down, [&a_view](uint32_t amount) {
					a_view->PlaceSelectionOnGround(amount);
				});
			}

			ImGui::Spacing();
			ImGui::Spacing();

			ShowItemPreview(a_view->GetItemPreview(), false, a_view->GetDataID());
		}
		ImGui::EndChild();
	}

	void UIContainers::DrawSpellActionPanel(const ImVec2 &a_pos, const ImVec2 &a_size, std::unique_ptr<UITable> &a_view)
	{
		const float button_height = ImGui::GetFontSize() * ThemeConfig::GetWidgetStyle().actionButton.heightScale;

		ImGui::SameLine();
		ImGui::SetCursorPos(a_pos);
		if (ImGui::BeginChild("Modex::SpellWindow::Actions", a_size)) {
			const float max_width = ImGui::GetContentRegionAvail().x;
			const bool action_allowed = a_view->IsActionAllowed();
			const auto action_header = a_view->IsValidTargetReference() ? a_view->GetTableTargetRef()->GetName() : Translate("HEADER_ACTIONS");

			UICustom::SubCategoryHeader(action_header);

			ImGui::PushStyleColor(ImGuiCol_Button, ThemeConfig::GetColor("SECONDARY"));
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ThemeConfig::GetHover("SECONDARY"));
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, ThemeConfig::GetActive("SECONDARY"));
			if (UICustom::ActionButton("ADD_SPELL_TO_TARGET", ImVec2(max_width, button_height), action_allowed)) {
				a_view->ExecuteCommandOnSelection([&a_view](const std::unique_ptr<BaseObject>& a_spell) {
					Commands::AddSpellToActor(a_view->GetOwnership(), a_view->GetTableTargetRef(), a_spell->GetBaseFormID());
				});
			}
			ImGui::PopStyleColor(3);

			ImGui::PushStyleColor(ImGuiCol_Button, ThemeConfig::GetColor("DECLINE"));
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ThemeConfig::GetHover("DECLINE"));
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, ThemeConfig::GetActive("DECLINE"));
			if (UICustom::ActionButton("REMOVE_SPELL_FROM_TARGET", ImVec2(max_width, button_height), action_allowed)) {
				a_view->ExecuteCommandOnSelection([&a_view](const std::unique_ptr<BaseObject>& a_spell) {
					Commands::RemoveSpellFromActor(a_view->GetOwnership(), a_view->GetTableTargetRef(), a_spell->GetBaseFormID());
				});
			}
			ImGui::PopStyleColor(3);

			ImGui::Spacing();
			ImGui::Spacing();

			ShowItemPreview(a_view->GetItemPreview(), false, a_view->GetDataID());
		}

		ImGui::EndChild();
	}

	// Can be used globally as a general purpose BaseObject table.
	void UIContainers::DrawBasicTablePanel(const char* a_localeText, const ImVec2 &a_pos, const ImVec2 &a_size, std::unique_ptr<UITable> &a_view)
	{
		ImGui::SameLine();
		ImGui::SetCursorPos(a_pos);
		if (ImGui::BeginChild(a_localeText, a_size, false, ImGuiWindowFlags_NoBringToFrontOnFocus)) {
			// std::string count = std::format("[{}]", a_view->GetTableList().size());
			// UICustom::SubCategoryHeader(Translate(a_localeText), count.c_str());
			// ImGui::Spacing();
			a_view->Draw(a_view->GetTableList());
		}
		ImGui::EndChild();
	}
}
