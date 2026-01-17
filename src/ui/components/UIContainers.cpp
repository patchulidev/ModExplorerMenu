#include "UIContainers.h"

#include "core/Commands.h"
#include "config/UserData.h"
#include "core/PlayerChestSpawn.h"
#include "config/EquipmentConfig.h"
#include "config/ThemeConfig.h"
#include "localization/Locale.h"

namespace Modex
{

    bool UIContainers::QueryCheck(const bool a_condition)
    {
        if (a_condition) {
            return true;
        } else {
            UIManager::GetSingleton()->ShowWarning("Large Query", Translate("WARNING_LARGE_QUERY"));
            return false;
        }
    }

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

    // Buttons used for actions pane inside module windows (right-hand side).
    bool UIContainers::ActionButton(const char* a_translate, const ImVec2& a_size, const bool a_condition, const ImVec4& a_color)
    {
        bool success = false;
        ImGui::PushStyleColor(ImGuiCol_Button, ImGui::ColorConvertFloat4ToU32(a_color));
        if (a_condition) {
            success = ImGui::Button(Translate(a_translate), a_size);
        } else {
            ImGui::BeginDisabled();
            ImGui::Button(Translate(a_translate), a_size);
            ImGui::EndDisabled();
        }
        ImGui::PopStyleColor();

        if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal | ImGuiHoveredFlags_NoSharedDelay | ImGuiHoveredFlags_AllowWhenDisabled)) {
            UICustom::FancyTooltip(Translate((std::string(a_translate) + "_TOOLTIP").c_str()));
        }

        return success;
    }

    void UIContainers::DrawKitActionsPanel(const ImVec2 &a_pos, const ImVec2 &a_size, std::unique_ptr<UITable> &a_kitView, std::unique_ptr<UITable> &a_mainView, Kit& a_kit)
    {
        const float button_height = ImGui::GetFrameHeight();
        const ImVec4 button_color = ThemeConfig::GetColor("BUTTON");
        const ImVec4 cont_color = ThemeConfig::GetColor("CONTAINER_BUTTON");
        
        // const int count = a_mainView->GetClickAmount() == nullptr ? 1 : *a_mainView->GetClickAmount();
        const bool playerToggle = UserData::User().Get<bool>("Modex::TargetReference::IsPlayer", true);
        const std::string toggle_text = playerToggle ? Translate("TARGET_PLAYER") : Translate("TARGET_NPC");

        // TODO: Remove
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

            if (ActionButton("CONTAINER_VIEW_KIT", ImVec2(half_width, button_height), action_allowed, cont_color)) {
                PlayerChestSpawn::GetSingleton()->PopulateChestWithKit(a_kit);
            }

            ImGui::SameLine();

            const std::string action_text = playerToggle ? "ADD_KIT" : "ADD_KIT_NPC";
            if (ActionButton(action_text.c_str(), ImVec2(half_width, button_height), action_allowed, button_color)) {
                a_kitView->AddKitItemsToInventory(a_kit);
            }
            

            if (ActionButton("PLUGIN_DEPENDENCIES", ImVec2(max_width, button_height), action_allowed, button_color)) {
                std::unordered_set<std::string> dependencies;
                std::string message;

                for (auto& item : a_kit.m_items) {
                    if (!dependencies.contains(item->m_plugin)) {
                        dependencies.insert(item->m_plugin);
                    }
                }

                auto pluginList = Data::GetSingleton()->GetModulePluginListSorted(Data::PLUGIN_TYPE::ALL, Data::SORT_TYPE::COMPILEINDEX_ASC);
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

            if (ActionButton("SET_START_EQUIP", ImVec2(max_width, button_height), action_allowed, button_color)) {
                // TODO: Implement starting equipment functionality
            }

            if (ActionButton("CLEAR_INVENTORY", ImVec2(max_width, button_height), true, button_color)) {
                UIManager::GetSingleton()->ShowWarning("Clear Inventory", Translate("GENERAL_CLEAR_INVENTORY_INSTRUCTION"), []() {
                    if (auto player = RE::PlayerCharacter::GetSingleton()->AsReference(); player) {
                        Commands::RemoveAllItemsFromInventory(player);
                    }
                });
            }
        }

        ImGui::EndChild();
    }

    void UIContainers::DrawAddItemTablePanel(const ImVec2 &a_pos, const ImVec2 &a_size, std::unique_ptr<UITable> &a_view)
    {
        ImGui::SameLine();
        ImGui::SetCursorPos(a_pos);
        if (ImGui::BeginChild("##Modex::AddItemTable", a_size, false)) {
            // ImGui::SubCategoryHeader(Translate("HEADER_ITEM_TABLE"));
	    a_view->DrawWarningBar();
            a_view->ShowSort();
            ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal);
            a_view->Draw(a_view->GetTableList());
        }
        ImGui::EndChild();
    }

    void UIContainers::DrawInventoryTablePanel(const ImVec2 &a_pos, const ImVec2 &a_size, std::unique_ptr<UITable> &a_view)
    {
        ImGui::SameLine();
        ImGui::SetCursorPos(a_pos);
        if (ImGui::BeginChild("##Modex::InventoryTable", a_size, false)) {
            UICustom::SubCategoryHeader(Translate("HEADER_INVENTORY_TABLE"));
            // a_view->ShowSort();
            ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal);
            a_view->Draw(a_view->GetTableList());
        }
        ImGui::EndChild();
    }

    void UIContainers::DrawKitTablePanel(const ImVec2 &a_pos, const ImVec2 &a_size, std::unique_ptr<UITable> &a_view)
    {
        // FIXME: Removed Separator between Sort and Draw to potentially fix LayoutItemSize issues.
        ImGui::SameLine();
        ImGui::SetCursorPos(a_pos);
        if (ImGui::BeginChild("##Modex::KitTable", a_size, false)) {
            a_view->DrawWarningBar();
            a_view->ShowSort();
            a_view->Draw(a_view->GetTableList());
        }
        ImGui::EndChild();
    }
}
