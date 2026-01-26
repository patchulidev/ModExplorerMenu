#include "EquipmentModule.h"
#include "config/EquipmentConfig.h"
#include "config/ThemeConfig.h"
#include "localization/Locale.h"
#include "ui/components/UIContainers.h"

namespace Modex 
{
	void EquipmentModule::DrawKitSelectionPanel(const ImVec2& a_pos, const ImVec2& a_size, std::unique_ptr<UITable>& a_kitTable)
	{
		(void)a_size; // TODO: Deprecated Argument

		ImGui::SetCursorPos(a_pos);
		if (ImGui::BeginChild("##Modex::KitSelection", a_size, false)) {
			UICustom::SubCategoryHeader(Translate("HEADER_KIT_TABLE"));

			auto& searchSystem = GetSearchSystem();
			auto& selectedKit = GetSelectedKit();

			const float button_width = ImGui::GetContentRegionAvail().x;
			const float rename_width = button_width / 3.0f;
			const float button_height = ImGui::GetFrameHeight();

			auto equipment_keys = EquipmentConfig::GetEquipmentListSortedKeys();
			std::string preview_string = selectedKit.GetNameTail();
			
			const float width = ImGui::GetContentRegionAvail().x - rename_width - ImGui::GetStyle().ItemSpacing.x;
			if (searchSystem->InputTextComboBox("##KitBar::Search", m_searchBuffer, preview_string, IM_ARRAYSIZE(m_searchBuffer), equipment_keys, width)) {
				selectedKit = EquipmentConfig::KitLookup(m_searchBuffer).value_or(Kit());

				m_searchBuffer[0] = '\0';
				a_kitTable->Refresh();
			}

			ImGui::SameLine();

			if (UIContainers::ActionButton("KIT_RENAME", ImVec2(rename_width, 0), !selectedKit.empty(), ThemeConfig::GetColor("BUTTON"))) {
				UIManager::GetSingleton()->ShowInputBox(
					Translate("POPUP_KIT_RENAME_TITLE"),
					Translate("POPUP_KIT_RENAME_DESC"),
					selectedKit.GetName(),
					[&selectedKit, &a_kitTable](std::string a_input) {
						if (const auto success = EquipmentConfig::RenameKit(selectedKit, a_input); success.has_value()) {
							selectedKit = std::move(success.value());
							a_kitTable->Refresh();
						}
					}
				);
			}

			if (UIContainers::ActionButton("KIT_COPY", ImVec2(button_width, button_height), !selectedKit.empty(), ThemeConfig::GetColor("BUTTON"))) {
				if (auto new_kit = EquipmentConfig::CopyKit(selectedKit); new_kit.has_value()) {
					ImFormatString(m_searchBuffer, IM_ARRAYSIZE(m_searchBuffer), "");
					selectedKit = std::move(new_kit.value());
					a_kitTable->Refresh();
				}
			}

			if (UIContainers::ActionButton("KIT_DELETE", ImVec2(button_width, button_height), !selectedKit.empty(), ThemeConfig::GetColor("BUTTON"))) {
				UIManager::GetSingleton()->ShowWarning(
					std::string(Translate("POPUP_KIT_DELETE_TITLE")) + " - " + selectedKit.GetNameTail(),
					Translate("POPUP_KIT_DELETE_DESC"),
					[&selectedKit, &a_kitTable]() {
						ImFormatString(m_searchBuffer, IM_ARRAYSIZE(m_searchBuffer), "");
						EquipmentConfig::DeleteKit(selectedKit);
						selectedKit = Kit();
						a_kitTable->Refresh();
					}
				);
			}

			if (UIContainers::ActionButton("KIT_CREATE", ImVec2(button_width, button_height), true, ThemeConfig::GetColor("BUTTON"))) {
				UIManager::GetSingleton()->ShowInputBox(
					Translate("POPUP_KIT_CREATE_TITLE"),
					Translate("POPUP_KIT_CREATE_DESC"),
					"",
					[&selectedKit, &a_kitTable](const std::string& a_input) {
						if (auto new_kit = EquipmentConfig::CreateKit(a_input); new_kit.has_value()) {
							ImFormatString(m_searchBuffer, IM_ARRAYSIZE(m_searchBuffer), "");
							selectedKit = std::move(new_kit.value());
							a_kitTable->Refresh();
						}
					}
				);
			}

			if (UIContainers::ActionButton("KIT_BROWSE", ImVec2(button_width, button_height), true, ThemeConfig::GetColor("BUTTON"))) {
				UIManager::GetSingleton()->ShowBrowser(
					Translate("POPUP_KIT_BROWSER_TITLE"),
					equipment_keys,
					[&selectedKit, &a_kitTable](const std::string& a_selection) {
						if (const auto success = EquipmentConfig::KitLookup(a_selection); success.has_value()) {
							selectedKit = std::move(success.value());
							ImFormatString(m_searchBuffer, IM_ARRAYSIZE(m_searchBuffer), "");
							a_kitTable->Refresh();
						}
					}
				);
			}
		}

		ImGui::EndChild();
	}
}
