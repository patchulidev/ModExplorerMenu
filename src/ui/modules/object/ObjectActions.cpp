#include "ObjectModule.h"
#include "ui/components/UICustom.h"
#include "localization/Locale.h"

namespace Modex
{
	void ObjectModule::ShowActions()
	{
		const float button_width = ImGui::GetContentRegionAvail().x;

		UICustom::SubCategoryHeader(Translate("GENERAL_DOUBLE_CLICK_BEHAVIOR"));

		// Click To Place Toggle
		bool _true_ = true;
		if (ImGui::Selectable(TranslateIcon(ICON_LC_MAP_PIN_PLUS, "GENERAL_CLICK_TO_PLACE"), _true_)) {
			// nothing
		}

		if (_true_) {
			ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal);

			// Add Count Input.
			ImGui::SetNextItemWidth(button_width);
			ImGui::InputInt("##ObjectPlaceCount", &m_clickCount, 1, 100, ImGuiInputTextFlags_CharsDecimal);
		}

		ImGui::Spacing();
		UICustom::SubCategoryHeader(Translate("Actions"));
		// ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(a_style.secondaryButton.x, a_style.secondaryButton.y, a_style.secondaryButton.z, a_style.secondaryButton.w));
		// ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(a_style.secondaryButtonActive.x, a_style.secondaryButtonActive.y, a_style.secondaryButtonActive.z, a_style.secondaryButtonActive.w));
		// ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(a_style.secondaryButtonHovered.x, a_style.secondaryButtonHovered.y, a_style.secondaryButtonHovered.z, a_style.secondaryButtonHovered.w));

		if (ImGui::Button(Translate("OBJECT_PLACE_SELECTED"), ImVec2(button_width, 0))) {
			m_tableView->PlaceSelectionOnGround(m_clickCount);
		}

		// ImGui::PopStyleColor(3);  // End of Green Buttons

		const auto& selectedObject = GetTableView()->GetItemPreview();

		if (selectedObject == nullptr) {
			return;
		}

		ImGui::Spacing();
		UICustom::SubCategoryHeader(Translate("Info"));

		BaseObject* obj = selectedObject.get();

		if (obj == nullptr) {
			return;
		}

		// Name Bar

		ImDrawList* draw_list = ImGui::GetWindowDrawList();
		const auto cursor = ImGui::GetCursorScreenPos();
		const auto size = ImGui::GetContentRegionAvail();
		const auto color = ImGui::GetStyleColorVec4(ImGuiCol_Border);
		draw_list->AddRectFilled(cursor, ImVec2(cursor.x + size.x, cursor.y + ImGui::GetFontSize() * 2.5f), ImGui::ColorConvertFloat4ToU32(ImVec4(0.15f, 0.15f, 0.15f, 0.5f)));
		draw_list->AddRect(cursor, ImVec2(cursor.x + size.x, cursor.y + ImGui::GetFontSize() * 2.5f), ImGui::ColorConvertFloat4ToU32(color));

		auto name = obj->GetName();

		if (name.empty()) {
			name = obj->GetEditorID();
		}

		ImGui::NewLine();
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f));
		ImGui::SetCursorPosX(UICustom::GetCenterTextPosX(name.c_str()));
		ImGui::SetCursorPosY(ImGui::GetCursorPosY() - ImGui::GetFontSize() / 2);
		ImGui::Text("%s", name.c_str());
		ImGui::NewLine();
		ImGui::PopStyleVar();


		// Load Order Info Pane
		// See ItemPreview.h for other implementation.
		ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal);
		ImGui::SetCursorPosX(UICustom::GetCenterTextPosX(Translate("Load Order")));
		ImGui::Text("%s",Translate("Load Order"));
		ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal);
		

		if (const auto& sourceFiles = obj->GetFileArray(); sourceFiles.has_value()) {
			for (uint32_t i = 0; i < sourceFiles.value()->size(); i++) {
				if (const auto file = (*sourceFiles.value())[i]) {
					const std::string fileName = file->GetFilename().data();

					if (i == 0 && sourceFiles.value()->size() > 1) {
						ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "%s", fileName.c_str());
					} else if (i == sourceFiles.value()->size() - 1) {
						ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "%s", fileName.c_str());
					} else {
						ImGui::Text("%s", fileName.c_str());
					}
				}
			}
		}

		ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal);

		// Model Info
		auto SplitString = [](std::string a_string, std::string a_delimiter) -> std::vector<std::string> {
			std::vector<std::string> strings;
			std::stringstream ss(a_string);
			std::string string;

			while (std::getline(ss, string, a_delimiter[0])) {
				strings.push_back(string);
			}

			return strings;
		};

		if (obj->GetEditorID().empty()) {
			return;
		}

		ImGui::NewLine();

		RE::TESModel* model = obj->GetTESForm()->As<RE::TESModel>();

		if (model != nullptr) {
			std::string modelFullPath = model->GetModel();

			if (modelFullPath.empty()) {
				return;
			}

			std::vector<std::string> modelPath = SplitString(modelFullPath, "\\");

			if (ImGui::TreeNodeEx(TranslateFormat("Model Path", ":"), ImGuiTreeNodeFlags_DefaultOpen)) {
				for (size_t i = 0; i < modelPath.size(); ++i) {
					if (i < modelPath.size() - 1) {
						ImGui::Text("%s/", modelPath[i].c_str());
						ImGui::Indent(5.0f);
					} else {
						ImGui::Text("%s", modelPath[i].c_str());
					}
				}

				for (size_t i = 0; i < modelPath.size() - 1; ++i) {
					ImGui::Unindent(5.0f);
				}

				ImGui::TreePop();
			}
		}

		ImGui::NewLine();
	}
}
