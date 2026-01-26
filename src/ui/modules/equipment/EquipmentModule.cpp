#include "EquipmentModule.h"
#include "config/UserData.h"
#include "config/EquipmentConfig.h"
#include "config/ThemeConfig.h"
#include "localization/Locale.h"
#include "ui/components/UIContainers.h"

namespace Modex
{
	void EquipmentModule::Draw()
	{
		DrawTabMenu();
	}

	void EquipmentModule::Load()
	{
		UIModule::Load();

		auto& table = m_tables[0];
		auto& kitTable = m_tables[1];
		auto& invTable = m_tables[2];

		// TEST: Can we setup targets without layout conditions now?
		table->AddDragDropTarget(kitTable->GetDragDropHandle(), kitTable.get());
		kitTable->AddDragDropTarget(table->GetDragDropHandle(), table.get());

		table->AddDragDropTarget(invTable->GetDragDropHandle(), invTable.get());
		invTable->AddDragDropTarget(table->GetDragDropHandle(), table.get());

		auto last_kit = UserData::User().Get<std::string>("lastSelectedKit", "");
		m_selectedKit = EquipmentConfig::KitLookup(last_kit).value_or(Kit());
		kitTable->Refresh();
	}

	void EquipmentModule::Unload()
	{
		UIModule::Unload();
		UserData::User().Set<std::string>("lastSelectedKit", m_selectedKit.m_key);
	}

	static inline void DrawEquipmentLayout(std::vector<std::unique_ptr<UITable>>& a_tables)
	{
		const ImVec2 window_padding = ImGui::GetStyle().WindowPadding;
		const float table_width = ImGui::GetContentRegionAvail().x * 0.5f;
		const float table_height = ImGui::GetContentRegionAvail().y - (window_padding.y * 2.0f);

		const float panel_width = (table_width / 2.0f) - (window_padding.x * 2.0f);
		const float panel_height = table_height / 2.5f; // FIX: Broken;

		auto& table = a_tables[0];
		auto& kitTable = a_tables[1];

		const ImVec2 table_pos = ImGui::GetCursorPos();
		UIContainers::DrawBasicTablePanel(table_pos, ImVec2(table_width, table_height), table);

		const ImVec2 search_pos = table_pos + ImVec2(table_width + window_padding.x, 0.0f);
		EquipmentModule::DrawKitSelectionPanel(search_pos, ImVec2(panel_width, panel_height), kitTable);

		const ImVec2 action_pos = search_pos + ImVec2(panel_width + window_padding.x, 0.0f);
		UIContainers::DrawKitActionsPanel(action_pos, ImVec2(0.0f, panel_height), kitTable, table, EquipmentModule::m_selectedKit);

		const ImVec2 kit_pos = search_pos + ImVec2(0.0f, panel_height + window_padding.y);
		UIContainers::DrawKitTablePanel(kit_pos, ImVec2(table_width - window_padding.x, table_height - panel_height - window_padding.y), kitTable);
	}

	static inline void DrawInventoryLayout(std::vector<std::unique_ptr<UITable>>& a_tables)
	{
		const ImVec2 window_padding = ImGui::GetStyle().WindowPadding;
		const float table_width = ImGui::GetContentRegionAvail().x * 0.5f;
		const float table_height = ImGui::GetContentRegionAvail().y - (window_padding.y * 2.0f);

		auto& table = a_tables[0];
		auto& invTable = a_tables[2];

		ImVec2 table_pos = ImGui::GetCursorPos();
		UIContainers::DrawBasicTablePanel(table_pos, ImVec2(table_width, table_height), table);

		ImVec2 inventory_pos = table_pos;
		inventory_pos.x += table_width + window_padding.x;
		UIContainers::DrawInventoryTablePanel(inventory_pos, ImVec2(table_width - window_padding.x, table_height), invTable);
	}

	EquipmentModule::EquipmentModule() 
	{
		m_name = Translate("MODULE_EQUIPMENT");
		m_icon = ICON_LC_PACKAGE;

		m_selectedKit = Kit();
		m_searchSystem = std::make_unique<SearchSystem>(std::filesystem::path());
		m_searchSystem->Load(false);

		m_layouts.push_back({"Equipment Layout", true, DrawEquipmentLayout});
		m_layouts.push_back({"Inventory Layout", false, DrawInventoryLayout});

		auto table = std::make_unique<UITable>();
		table->SetGenerator([]() { return Data::GetSingleton()->GetAddItemList(); });
		table->SetPluginType(Data::PLUGIN_TYPE::Item);
		table->SetUserDataID("Equipment");
		table->SetDragDropHandle(UITable::DragDropHandle::Table);
		table->AddFlag(UITable::ModexTableFlag_Base);
		table->AddFlag(UITable::ModexTableFlag_EnableCategoryTabs);
		table->AddFlag(UITable::ModexTableFlag_EnableHeader);
		table->AddFlag(UITable::ModexTableFlag_EnableItemPreviewOnHover);
		table->SetShowEditorID(UserData::User().Get<bool>("Equipment::ShowEditorID", false));
		table->Init();
		m_tables.push_back(std::move(table));

		auto kit = std::make_unique<UITable>();
		kit->SetGenerator([]() { return EquipmentConfig::GetItems(m_selectedKit); });
		kit->SetKitPointer(&m_selectedKit);
		kit->SetUserDataID("Equipment");
		kit->SetDragDropHandle(UITable::DragDropHandle::Kit);
		kit->AddFlag(UITable::ModexTableFlag_Kit);
		kit->AddFlag(UITable::ModexTableFlag_EnableHeader);
		kit->Init();
		m_tables.push_back(std::move(kit));

		// TODO: Revisit generator impl so that it registers tableref instead of just player
		auto inventory = std::make_unique<UITable>();
		inventory->SetGenerator([]() { return Data::GetSingleton()->GetInventoryList(); });
		inventory->SetUserDataID("Equipment");
		inventory->SetDragDropHandle(UITable::DragDropHandle::Inventory);
		inventory->AddFlag(UITable::ModexTableFlag_Inventory);
		inventory->AddFlag(UITable::ModexTableFlag_EnableHeader);
		inventory->Init();
		m_tables.push_back(std::move(inventory));
	}
}
