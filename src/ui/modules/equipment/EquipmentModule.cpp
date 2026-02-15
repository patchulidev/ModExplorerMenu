#include "EquipmentModule.h"
#include "config/UserData.h"
#include "config/EquipmentConfig.h"
#include "config/ThemeConfig.h"
#include "imgui.h"
#include "localization/Locale.h"
#include "ui/components/UIContainers.h"
#include "core/Commands.h"
#include "ui/components/UITable.h"
#include "core/PlayerChestSpawn.h"

namespace Modex
{
	void EquipmentModule::Draw()
	{
		DrawTabMenu();
	}

	// Used exclusively in the equipment module window for rendering Kit actionable buttons.
	void EquipmentModule::DrawKitActionsPanel(const ImVec2 &a_pos, const ImVec2 &a_size)
	{
		
		ImGui::SameLine();
		ImGui::SetCursorPos(a_pos);
		if (ImGui::BeginChild("##Modex::KitActions", a_size, ImGuiChildFlags_AutoResizeY)) {
			const float max_width = ImGui::GetContentRegionAvail().x;
			const float half_width = (max_width - ImGui::GetStyle().ItemSpacing.x) / 2.0f;

			UICustom::SubCategoryHeader(Translate("HEADER_KIT_TABLE"));

			const float button_width = ImGui::GetContentRegionAvail().x;
			const float button_height = ImGui::GetFrameHeightWithSpacing();

			auto equipment_keys = EquipmentConfig::GetEquipmentListSortedKeys();
			std::string preview_string = m_selectedKit.GetNameTail();

			static bool hovered = false;
			ImGui::PushStyleColor(ImGuiCol_FrameBg, hovered ? ThemeConfig::GetHover("BG_LIGHT") : ThemeConfig::GetColor("BG_LIGHT"));
			if (m_searchSystem->InputTextComboBox("##KitActionBar::Search", m_searchBuffer, preview_string, 256, equipment_keys, button_width)) {
				m_selectedKit = EquipmentConfig::KitLookup(m_searchBuffer).value_or(Kit());

				m_searchBuffer[0] = '\0';
				m_tables[1]->Refresh();
			}
			ImGui::PopStyleColor();
			hovered = ImGui::IsItemHovered();

			if (UICustom::ActionButton("KIT_RENAME", ImVec2(half_width, button_height), !m_selectedKit.empty())) {
				UIManager::GetSingleton()->ShowInputBox(
					Translate("POPUP_KIT_RENAME_TITLE"),
					Translate("POPUP_KIT_RENAME_DESC"),
					m_selectedKit.GetName(),
					[&](std::string a_input) {
						if (const auto success = EquipmentConfig::RenameKit(m_selectedKit, a_input); success.has_value()) {
							m_selectedKit = std::move(success.value());
							m_tables[1]->Refresh();
						}
					}
				);
			}

			ImGui::SameLine();

			if (UICustom::ActionButton("KIT_BROWSE", ImVec2(half_width, button_height), true)) {
				UIManager::GetSingleton()->ShowBrowser(
					Translate("POPUP_KIT_BROWSE_TITLE"),
					equipment_keys,
					[&](const std::string& a_input) {
						if (const auto success = EquipmentConfig::KitLookup(a_input); success.has_value()) {
							m_selectedKit = std::move(success.value());
							ImFormatString(m_searchBuffer, 256, "");
							m_tables[1]->Refresh();
						}
					}
				);
			}

			if (UICustom::ActionButton("KIT_CREATE", ImVec2(button_width, button_height), true)) {
				UIManager::GetSingleton()->ShowInputBox(
					Translate("POPUP_KIT_CREATE_TITLE"),
					Translate("POPUP_KIT_CREATE_DESC"),
					"",
					[&](const std::string& a_input) {
						if (auto new_kit = EquipmentConfig::CreateKit(a_input); new_kit.has_value()) {
							ImFormatString(m_searchBuffer, 256, "");
							m_selectedKit = std::move(new_kit.value());
							m_tables[1]->Refresh();
						}
					}
				);
			}


			if (UICustom::ActionButton("KIT_COPY", ImVec2(button_width, button_height), !m_selectedKit.empty())) {
				if (auto new_kit = EquipmentConfig::CopyKit(m_selectedKit); new_kit.has_value()) {
					ImFormatString(m_searchBuffer, 256, "");
					m_selectedKit = std::move(new_kit.value());
					m_tables[1]->Refresh();
				}
			}

			if (UICustom::ActionButton("KIT_DELETE", ImVec2(button_width, button_height), !m_selectedKit.empty())) {
				UIManager::GetSingleton()->ShowWarning(
					std::string(Translate("POPUP_KIT_DELETE_TITLE")) + " - " + m_selectedKit.GetNameTail(),
					Translate("POPUP_KIT_DELETE_DESC"),
					true,
					[&]() {
						ImFormatString(m_searchBuffer, 256, "");
						EquipmentConfig::DeleteKit(m_selectedKit);
						m_selectedKit = Kit();
						m_tables[1]->Refresh();
					}
				);
			}

			if (m_selectedKit && !m_selectedKit.empty()) {
				UICustom::SubCategoryHeader(Translate("HEADER_KIT_ACTIONS"));
				
				static auto targetRefr = m_tables[1]->GetTableTargetRef();
				static std::string target_name = targetRefr ? targetRefr->GetName() : Translate("NO_CONSOLE_SELECTION");
				const bool action_allowed = !m_selectedKit.empty() && m_selectedKit.m_items.size() > 0;
				const bool shift_down = ImGui::GetIO().KeyShift;

				{ // Side-by-side buttons group.
					ImGui::PushStyleColor(ImGuiCol_Button, ThemeConfig::GetColor("SECONDARY"));
					ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ThemeConfig::GetHover("SECONDARY"));
					ImGui::PushStyleColor(ImGuiCol_ButtonActive, ThemeConfig::GetActive("SECONDARY"));
					if (UICustom::ActionButton("CONTAINER_VIEW_KIT", ImVec2(half_width, button_height), action_allowed)) {
						PlayerChestSpawn::GetSingleton()->PopulateChestWithKit(m_selectedKit);
					}
					ImGui::PopStyleColor(3);

					ImGui::SameLine();

					if (UICustom::ActionButton(Translate("ADD_KIT"), ImVec2(half_width, button_height), action_allowed)) {
						m_tables[1]->AddKitToTargetInventory(m_selectedKit);
					}
				}

				if (UICustom::ActionButton("PLUGIN_DEPENDENCIES", ImVec2(max_width, button_height), action_allowed)) {
					std::unordered_set<std::string> dependencies;
					std::string message;

					for (auto& item : m_selectedKit.m_items) {
						if (!dependencies.contains(item->m_plugin)) {
							dependencies.insert(item->m_plugin);
						}
					}

					auto pluginList = Data::GetSingleton()->GetModulePluginListSorted(Data::PluginType::All, Data::PluginSort::Load_Order_Ascending);
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

				const auto title = shift_down ? Translate("RESET_INVENTORY") : Translate("CLEAR_INVENTORY");
				const auto description = shift_down ? Translate("RESET_INVENTORY_DESC") : Translate("CLEAR_INVENTORY_DESC");

				if (UICustom::ActionButton(title, ImVec2(max_width, button_height), m_tables[1]->IsValidTargetReference())) {
					UIManager::GetSingleton()->ShowWarning(Translate("CLEAR_INVENTORY"), description, true, [this]() {
						if (auto target = this->m_tables[1]->GetTableTargetRef(); target) {
							if (ImGui::GetIO().KeyShift) {
								Commands::ResetTargetInventory(target);
							} else {
								Commands::RemoveAllItemsFromInventory(target);
							}
						}
					});
				}
			}
		}

		ImGui::EndChild();
	}

	void EquipmentModule::DrawEquipmentLayout(std::vector<std::unique_ptr<UITable>>& a_tables)
	{
		const ImVec2 window_padding = ImGui::GetStyle().WindowPadding;
		const float table_width = ImGui::GetContentRegionAvail().x * 0.60f;

		auto& table = a_tables[0];
		auto& kitTable = a_tables[1];

		const ImVec2 table_pos = ImGui::GetCursorPos();
		UIContainers::DrawBasicTablePanel("TABLE_ITEM", table_pos, ImVec2(table_width, 0.0f), table);

		const ImVec2 action_pos = table_pos + ImVec2(table_width + window_padding.x, 0.0f);
		DrawKitActionsPanel(action_pos, ImVec2(0.0f, 0.0f));

		const ImVec2 kit_pos = action_pos + ImVec2(0.0f, ImGui::GetItemRectSize().y + window_padding.y);
		UIContainers::DrawBasicTablePanel("TABLE_KIT", kit_pos, ImVec2(0.0f, 0.0f), kitTable);
	}

	EquipmentModule::~EquipmentModule()
	{
		UserData::Set<std::string>("Equipment::LastSelectedKit", m_selectedKit.m_key);
	}

	EquipmentModule::EquipmentModule() 
	{
		memset(m_searchBuffer, 0, sizeof(m_searchBuffer));

		// static
		m_searchSystem = std::make_unique<SearchSystem>(std::filesystem::path());
		m_searchSystem->Load(false);

		const auto& last_kit_key = UserData::Get<std::string>("Equipment::LastSelectedKit", "");
		m_selectedKit = EquipmentConfig::KitLookup(last_kit_key).value_or(Kit());
		// Setup available layouts for this module.
		// m_layouts.push_back({Translate("TAB_EQUIPMENT"), true, DrawEquipmentLayout});
		m_layouts.push_back({Translate("TAB_EQUIPMENT"), true,
				[this](std::vector<std::unique_ptr<UITable>>& a_tables) {
					DrawEquipmentLayout(a_tables);
				}
			});

		constexpr auto table_flags =
		UITable::ModexTableFlag_Base |
		UITable::ModexTableFlag_EnableFilterTree |
		UITable::ModexTableFlag_EnableHeader |
		UITable::ModexTableFlag_EnableSearch |
		UITable::ModexTableFlag_EnableItemPreviewOnHover;

		auto table = std::make_unique<UITable>("AddItem", true, 0, table_flags);
		table->SetDragDropHandle(UITable::DragDropHandle::Table);

		constexpr auto kit_flags = 
		UITable::ModexTableFlag_Kit |
		UITable::ModexTableFlag_EnableItemPreviewOnHover |
		UITable::ModexTableFlag_EnableHeader;

		auto kit = std::make_unique<UITable>("Equipment", true, 0, kit_flags);
		kit->SetKitPointer(&m_selectedKit);
		kit->SetDragDropHandle(UITable::DragDropHandle::Kit);
		kit->Refresh();

		// Setup drag and drop target linkage.
		table->AddDragDropTarget(kit->GetDragDropHandle(), kit.get());
		kit->AddDragDropTarget(table->GetDragDropHandle(), table.get());

		m_tables.push_back(std::move(table));
		m_tables.push_back(std::move(kit));
	}
}
