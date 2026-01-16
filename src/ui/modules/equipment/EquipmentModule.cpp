#include "EquipmentModule.h"
#include "config/UserData.h"
#include "config/EquipmentConfig.h"
#include "config/ThemeConfig.h"
#include "localization/Locale.h"
#include "ui/components/UIContainers.h"

namespace Modex
{
    void EquipmentModule::Draw(float a_offset)
    {
        ImVec2 window_padding = ImGui::GetStyle().WindowPadding;
        const float button_width = (ImGui::GetContentRegionAvail().x / static_cast<int>(Viewport::Count)) - window_padding.x / 2.0f;
        const float button_height = ImGui::GetFrameHeight();
        const float tab_bar_height = button_height + (ImGui::GetStyle().WindowPadding.y * 2.0f);
        const float table_width = ImGui::GetContentRegionAvail().x * 0.5f;
        const float table_height = ImGui::GetContentRegionAvail().y - (window_padding.y * 2.0f) - tab_bar_height;

        constexpr float action_button_count = 6.0f;
        const float panel_width = (table_width / 2.0f) - (window_padding.x * 2.0f);
        const float panel_height = (action_button_count * button_height) + ((action_button_count - 1.0f) * ImGui::GetStyle().ItemSpacing.y) + window_padding.x;

        ImGui::SameLine();
        ImGui::SetCursorPosX(window_padding.x + a_offset);
        ImGui::SetCursorPosY(window_padding.y);
        ImVec2 start_pos = ImGui::GetCursorPos();

        // Tab Button Area
        // TODO: Revisit loading logic now that we changed Equipment.cpp load. Do the same with AddItem
        if (ImGui::BeginChild("##Equipment::TabBar", ImVec2(0.0f, button_height), 0, ImGuiWindowFlags_NoFocusOnAppearing)) {
            ImGui::PushStyleVar(ImGuiStyleVar_SelectableTextAlign, ImVec2(0.5f, 0.5f));
            if (ImGui::Selectable("Equipment Setup", m_viewport == Viewport::EquipmentView, 0, ImVec2(button_width, button_height))) {
                m_viewport = Viewport::EquipmentView;
                m_tableView->RemoveDragDropTarget(m_inventoryView->GetDragDropID());
                m_tableView->AddDragDropTarget(m_kitTableView->GetDragDropID(), m_kitTableView.get());
                m_kitTableView->Load();
            }

            ImGui::SameLine();

            if (ImGui::Selectable("Inventory View", m_viewport == Viewport::InventoryView, 0, ImVec2(button_width, button_height))) {
                m_viewport = Viewport::InventoryView;
                m_tableView->RemoveDragDropTarget(m_kitTableView->GetDragDropID());
                m_tableView->AddDragDropTarget(m_inventoryView->GetDragDropID(), m_inventoryView.get());
                Data::GetSingleton()->GenerateInventoryList(); // TODO: Update callback system to auto-update inventory list.
                m_inventoryView->Load();
            }

            ImGui::SameLine();

            if (ImGui::Selectable("Follower View", m_viewport == Viewport::FollowerView, 0, ImVec2(button_width, button_height))) {
                m_viewport = Viewport::FollowerView;
                m_tableView->RemoveDragDropTarget(m_kitTableView->GetDragDropID());
                m_tableView->RemoveDragDropTarget(m_inventoryView->GetDragDropID());
            }

            ImGui::PopStyleVar();
        }

        ImGui::EndChild();
        ImGui::SameLine();
        ImGui::SetCursorPos(start_pos);

        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + button_height);
        ImGui::PushStyleColor(ImGuiCol_Separator, ThemeConfig::GetColor("FILTER_SEPARATOR"));
        ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal);
        ImGui::PopStyleColor();
        ImGui::SetCursorPos(start_pos);

        if (m_viewport == Viewport::EquipmentView) {
            const ImVec2 table_pos = ImGui::GetCursorPos() + ImVec2(0.0f, tab_bar_height);
            UIContainers::DrawAddItemTablePanel(table_pos, ImVec2(table_width, table_height), m_tableView);

            const ImVec2 search_pos = table_pos + ImVec2(table_width + window_padding.x, 0.0f);
            DrawKitSelectionPanel(search_pos, ImVec2(panel_width, panel_height));

            const ImVec2 action_pos = search_pos + ImVec2(panel_width + window_padding.x, 0.0f);
            UIContainers::DrawKitActionsPanel(action_pos, ImVec2(0.0f, panel_height), m_kitTableView, m_tableView, m_selectedKit);

            const ImVec2 kit_pos = search_pos + ImVec2(0.0f, panel_height + window_padding.y);
            UIContainers::DrawKitTablePanel(kit_pos, ImVec2(table_width - window_padding.x, table_height - panel_height - window_padding.y), m_kitTableView);
        }

        if (m_viewport == Viewport::InventoryView) {
            ImVec2 table_pos = ImGui::GetCursorPos();
            table_pos.y += tab_bar_height;

            UIContainers::DrawAddItemTablePanel(table_pos, ImVec2(table_width, table_height), m_tableView);

            ImVec2 inventory_pos = start_pos;
            inventory_pos.x += table_width + window_padding.x;
            inventory_pos.y += tab_bar_height; 

            UIContainers::DrawInventoryTablePanel(inventory_pos, ImVec2(table_width - window_padding.x, table_height), m_inventoryView);
        }
    }

    void EquipmentModule::Unload()
    {
        m_tableView->Unload();
        m_kitTableView->Unload();
        m_inventoryView->Unload();
    }

    void EquipmentModule::Load()
    {
        if (m_viewport == Viewport::EquipmentView) {
            m_tableView->SetDragDropTarget(m_kitTableView->GetDragDropID(), m_kitTableView.get());
            m_tableView->Load();

            m_kitTableView->SetDragDropTarget(m_tableView->GetDragDropID(), m_tableView.get());
            m_kitTableView->Load();
        } else if (m_viewport == Viewport::InventoryView) {
            m_tableView->SetDragDropTarget(m_inventoryView->GetDragDropID(), m_inventoryView.get());
            m_tableView->Load();
            
            m_inventoryView->SetDragDropTarget(m_tableView->GetDragDropID(), m_tableView.get());
            m_inventoryView->Load();
        }
    }

    EquipmentModule::EquipmentModule() 
    {
        m_viewport = Viewport::EquipmentView;
        m_selectedKit = Kit();

        m_searchSystem = std::make_unique<SearchSystem>(std::filesystem::path());
        m_searchSystem->Load(false);

        m_tableView = std::make_unique<UITable>();
        m_tableView->SetGenerator([]() { return Data::GetSingleton()->GetAddItemList(); });
        m_tableView->SetPluginType(Data::PLUGIN_TYPE::ITEM);
        m_tableView->SetDataID("Equipment");
        m_tableView->SetDragDropID("FROM_TABLE");
        m_tableView->AddFlag(UITable::ModexTableFlag_Base);
        m_tableView->AddFlag(UITable::ModexTableFlag_EnablePluginKitView);
        m_tableView->AddFlag(UITable::ModexTableFlag_EnableCategoryTabs);
        m_tableView->AddFlag(UITable::ModexTableFlag_EnableHeader);
	m_tableView->AddFlag(UITable::ModexTableFlag_EnableItemPreviewOnHover);
        m_tableView->Init();
        m_tableView->SetShowEditorID(UserData::User().Get<bool>("Equipment::ShowEditorID", false));
        m_tableView->SetShowPluginKitView(UserData::User().Get<bool>("Equipment::ShowPluginKitView", false));

        m_kitTableView = std::make_unique<UITable>();
        m_kitTableView->SetGenerator([this]() { return EquipmentConfig::GetItems(m_selectedKit); });
        m_kitTableView->SetKitPointer(&m_selectedKit);
        m_kitTableView->SetDataID("Equipment");
        m_kitTableView->SetDragDropID("FROM_KIT");
        m_kitTableView->AddFlag(UITable::ModexTableFlag_Kit);
        m_kitTableView->AddFlag(UITable::ModexTableFlag_EnableHeader);
        m_kitTableView->Init();

        // TODO: Revisit generator impl so that it registers tableref instead of just player
        m_inventoryView = std::make_unique<UITable>();
        m_inventoryView->SetGenerator([]() { return Data::GetSingleton()->GetInventoryList(); });
        m_inventoryView->SetDataID("Equipment");
        m_inventoryView->SetDragDropID("FROM_INVENTORY");
        m_inventoryView->AddFlag(UITable::ModexTableFlag_Inventory);
        m_inventoryView->AddFlag(UITable::ModexTableFlag_EnableHeader);
        m_inventoryView->Init();
    }
}
