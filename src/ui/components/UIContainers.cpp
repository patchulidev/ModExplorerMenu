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

	// Compact helper for Warning popup on large queries.
	bool UIContainers::QueryCheck(const bool a_condition)
	{
		if (a_condition) {
			return true;
		} else {
			UIManager::GetSingleton()->ShowWarning("Large Query", Translate("WARNING_LARGE_QUERY"));
			return false;
		}
	}


	// Used exclusively in the UITable renderer for the tree node filter buttons.
	bool UIContainers::TabButton(const char* a_label, const ImVec2& a_size, const bool a_condition, const ImVec4& a_color)
	{
		bool success = false;

		const ImVec4 active_color = ImVec4(a_color.x + 0.05f, a_color.y + 0.05f, a_color.z + 0.10f, a_color.w);
		const ImVec4 inactive_color = ImVec4(a_color.x, a_color.y, a_color.z, a_color.w * 0.35f);

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

	static inline uint32_t ADDITEM_MAX_QUERY = 100;
	void UIContainers::DrawAddItemActionPanel(const ImVec2 &a_pos, const ImVec2 &a_size, std::unique_ptr<UITable> &a_view)
	{
		const float button_height = ImGui::GetFontSize() * 1.5f;
		const ImVec4 button_color = ThemeConfig::GetColor("BUTTON");
		const ImVec4 cont_color = ThemeConfig::GetColor("CONTAINER_BUTTON");
		bool valid_npc = false;

		const int count = a_view->GetClickAmount() == nullptr ? 1 : *a_view->GetClickAmount();

		ImGui::SameLine();
        ImGui::SetCursorPos(a_pos);
		if (ImGui::BeginChild("Modex::AddItemWindow::Actions", a_size)) {
			const float max_width = ImGui::GetContentRegionAvail().x;
			const float button_half_width = (max_width / 2.0f) - (ImGui::GetStyle().WindowPadding.x / 2.0f);

			UICustom::SubCategoryHeader(Translate("HEADER_ACTIONS"));
			
			ImGui::SetNextItemWidth(a_size.x);
			ImGui::InputInt("##AddItem::Count", a_view->GetClickAmount(), 1, 100, ImGuiInputTextFlags_CharsDecimal);
			
			ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal);

			// TODO: Add QueryCheck implementation.
			if (UICustom::ActionButton("CONTAINER_VIEW", ImVec2(max_width, button_height), true, cont_color)) {
				if (a_view->GetSelectionCount() > 0) {
					PlayerChestSpawn::GetSingleton()->PopulateChestWithItems(a_view->GetSelection());
				} else {
					PlayerChestSpawn::GetSingleton()->PopulateChestWithItems(a_view->GetTableList());
				}
			}

			if (UICustom::ActionButton("ADD_SELECTION", ImVec2(max_width, button_height), a_view->GetSelectionCount() > 0, button_color)) {
				if (UIContainers::QueryCheck(a_view->GetSelectionCount() > 0)) {
					a_view->AddSelectionToTargetInventory(count);
				} else {
					a_view->AddSelectionToTargetInventory(count);
				}
			}

			if (UICustom::ActionButton("PLACE_SELECTION", ImVec2(max_width, button_height), a_view->GetSelectionCount() > 0, button_color)) {
				if (UIContainers::QueryCheck(a_view->GetSelectionCount() > 0)) {
					a_view->PlaceSelectionOnGround(count);
				} else {
					a_view->PlaceSelectionOnGround(count);
				}
			}

			if (UICustom::ActionButton("EQUIP_SELECTION", ImVec2(max_width, 0), a_view->GetSelectionCount() > 0, button_color)) {
				if (UIContainers::QueryCheck(a_view->GetSelectionCount() > 0)) {
					a_view->EquipSelectionToTarget();
				} else {
					a_view->EquipSelectionToTarget();
				}
			}

			if (UICustom::ActionButton("CLEAR_INVENTORY", ImVec2(max_width, button_height), true, button_color)) {
				UIManager::GetSingleton()->ShowWarning("Clear Inventory", Translate("GENERAL_CLEAR_INVENTORY_INSTRUCTION"), []() {
					if (auto player = RE::PlayerCharacter::GetSingleton()->AsReference(); player) {
						Commands::RemoveAllItemsFromInventory(player);
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
	// Used exclusively in the equipment module window for rendering Kit actionable buttons.
	void UIContainers::DrawKitActionsPanel(const ImVec2 &a_pos, const ImVec2 &a_size, std::unique_ptr<UITable> &a_kitView, std::unique_ptr<UITable> &a_mainView, Kit& a_kit)
	{
		const float button_height = ImGui::GetFrameHeight();
		const ImVec4 button_color = ThemeConfig::GetColor("BUTTON");
		const ImVec4 cont_color = ThemeConfig::GetColor("CONTAINER_BUTTON");
		
		const bool playerToggle = UserData::User().Get<bool>("Modex::TargetReference::IsPlayer", true);
		const std::string toggle_text = playerToggle ? Translate("TARGET_PLAYER") : Translate("TARGET_NPC");

		// TODO: Previously used for dynamic height adjustment. Smelly code, remove.
		const ImVec2 adjusted_size = ImVec2(a_size.x, !playerToggle ? a_size.y + (button_height) * 2.0f : a_size.y);
		
		ImGui::SameLine();
		ImGui::SetCursorPos(a_pos);
		if (ImGui::BeginChild("##Modex::KitActions", adjusted_size, false)) {
			const float max_width = ImGui::GetContentRegionAvail().x;
			const float half_width = (max_width - ImGui::GetStyle().ItemSpacing.x) / 2.0f;

			UICustom::SubCategoryHeader(Translate("HEADER_KIT_ACTIONS"));

			ImGui::SetNextItemWidth(max_width);
			ImGui::InputInt("##KitActions::Count", a_mainView->GetClickAmount(), 1, 100, ImGuiInputTextFlags_CharsDecimal);
			
			static auto targetRefr = a_kitView->GetTableTargetRef();
			static std::string target_name = targetRefr ? targetRefr->GetName() : Translate("NO_CONSOLE_SELECTION");
			const bool action_allowed = targetRefr && targetRefr->GetFormType() == RE::FormType::ActorCharacter && !a_kit.empty();

			if (UICustom::ActionButton("CONTAINER_VIEW_KIT", ImVec2(half_width, button_height), action_allowed, cont_color)) {
				PlayerChestSpawn::GetSingleton()->PopulateChestWithKit(a_kit);
			}

			ImGui::SameLine();

			const std::string action_text = playerToggle ? "ADD_KIT" : "ADD_KIT_NPC";
			if (UICustom::ActionButton(action_text.c_str(), ImVec2(half_width, button_height), action_allowed, button_color)) {
				a_kitView->AddKitToTargetInventory(a_kit);
			}
			

			if (UICustom::ActionButton("PLUGIN_DEPENDENCIES", ImVec2(max_width, button_height), action_allowed, button_color)) {
				std::unordered_set<std::string> dependencies;
				std::string message;

				for (auto& item : a_kit.m_items) {
					if (!dependencies.contains(item->m_plugin)) {
						dependencies.insert(item->m_plugin);
					}
				}

				auto pluginList = Data::GetSingleton()->GetModulePluginListSorted(Data::PLUGIN_TYPE::All, Data::SORT_TYPE::Load_Order_Ascending);
				for (auto& plugin : pluginList) {
					if (dependencies.contains(plugin->fileName)) {
						message += std::format("[{}] - {}\n", Translate("Found"), plugin->fileName);
						dependencies.erase(plugin->fileName);
					}
				}

				for (auto& dependency : dependencies) {
					message += std::format("[{}] - {}\n", Translate("Missing"), dependency);
				}

				UIManager::GetSingleton()->ShowInfoBox(Translate("Plugin Dependencies"), message);
			}

			if (UICustom::ActionButton("SET_START_EQUIP", ImVec2(max_width, button_height), action_allowed, button_color)) {
				// TODO: Implement starting equipment functionality
			}

			if (UICustom::ActionButton("CLEAR_INVENTORY", ImVec2(max_width, button_height), true, button_color)) {
				UIManager::GetSingleton()->ShowWarning("Clear Inventory", Translate("GENERAL_CLEAR_INVENTORY_INSTRUCTION"), []() {
					if (auto player = RE::PlayerCharacter::GetSingleton()->AsReference(); player) {
						Commands::RemoveAllItemsFromInventory(player);
					}
				});
			}
		}

		ImGui::EndChild();
	}

	// Can be used globally as a general purpose BaseObject table.
	void UIContainers::DrawBasicTablePanel(const ImVec2 &a_pos, const ImVec2 &a_size, std::unique_ptr<UITable> &a_view)
	{
		ImGui::SameLine();
		ImGui::SetCursorPos(a_pos);
		if (ImGui::BeginChild("##Modex::AddItemTable", a_size, false, ImGuiWindowFlags_NoBringToFrontOnFocus)) {
			a_view->Draw(a_view->GetTableList());
		}
		ImGui::EndChild();
	}

	void UIContainers::DrawInventoryTablePanel(const ImVec2 &a_pos, const ImVec2 &a_size, std::unique_ptr<UITable> &a_view)
	{
		ImGui::SameLine();
		ImGui::SetCursorPos(a_pos);
		if (ImGui::BeginChild("##Modex::InventoryTable", a_size, false)) {
			a_view->Draw(a_view->GetTableList());
		}
		ImGui::EndChild();
	}

	// BUG: Previously had a weird LayoutItemSize bug when a Separator was used between the
	// ShowSort() and Draw() functions below. 
	void UIContainers::DrawKitTablePanel(const ImVec2 &a_pos, const ImVec2 &a_size, std::unique_ptr<UITable> &a_view)
	{
		ImGui::SameLine();
		ImGui::SetCursorPos(a_pos);
		if (ImGui::BeginChild("##Modex::KitTable", a_size, false)) {
			a_view->Draw(a_view->GetTableList());
		}
		ImGui::EndChild();
	}
}
