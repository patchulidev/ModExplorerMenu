#include "ui/components/UIModule.h"
#include "ui/components/UICustom.h"
#include "ui/Menu.h"
#include "config/ThemeConfig.h"
#include "localization/Locale.h"
#include "localization/FontManager.h"

namespace Modex
{
	void UIModule::SetActiveLayout(uint8_t a_layoutIndex)
	{
		for (size_t i = 0; i < m_layouts.size(); i++) {
			m_layouts[i].selected = (i == a_layoutIndex);
		}
	}
	
	uint8_t UIModule::GetActiveLayoutIndex() const
	{
		for (size_t i = 0; i < m_layouts.size(); i++) {
			if (m_layouts[i].selected) {
				return static_cast<uint8_t>(i);
			}
		}

		return 0;
	}

	void UIModule::UpdateTableTargets(RE::TESObjectREFR* a_ref)
	{
		for (auto& table : m_tables) {
			table->SetTargetByReference(a_ref);
		}
	}

	void UIModule::Load()
	{
		for (auto& table : m_tables) {
			table->Load();
		}

		m_show = true;
	}

	void UIModule::Unload()
	{
		for (auto& table : m_tables) {
			table->Unload();
		}

		m_show = false;
	}

	void UIModule::DrawTabMenu()
	{
		ImVec2 window_padding = ImGui::GetStyle().WindowPadding;
		const int layoutCount = std::ssize(m_layouts);
		const float button_width = (ImGui::GetContentRegionAvail().x / layoutCount);
		const float button_height = ImGui::GetFrameHeightWithSpacing();

		ImVec2 start_pos = ImGui::GetCursorPos();

        // Tab Button Area
		if (UICustom::BeginTabBar("#Modex::Layout::TabBar", button_height, m_offset, start_pos)) {
			ImGui::PushStyleColor(ImGuiCol_Header, ThemeConfig::GetColor("TAB_BUTTON"));
			ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(.0f, .0f));
			for (const auto& layout : m_layouts) {
				const auto selected = layout.selected;

				if (selected)
				{
					ImGui::PushFontBold();
				}

				if (ImGui::Selectable(Translate(layout.name.c_str()), layout.selected, ImGuiSelectableFlags_NoPadWithHalfSpacing, ImVec2(button_width, button_height))) {
					for (auto& l : m_layouts) {
						if (l.name == layout.name) {
							l.selected = true;
						} else {
							l.selected = false;
						}
					}
				}
				if (selected)
				{
					ImGui::PopFont();
				}

				ImGui::SameLine();
			}
			ImGui::PopStyleVar();
			ImGui::PopStyleColor();
		}

		ImGui::EndChild();
		ImGui::SetCursorPos(start_pos);

		// Tab Bar Separator
		ImGui::PushStyleColor(ImGuiCol_Separator, ThemeConfig::GetColor("TAB_BUTTON"));
		ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal, 2.0f);
		ImGui::PopStyleColor();

		start_pos.y += window_padding.y;
		ImGui::SetCursorPos(start_pos);

		for (const auto& layout : m_layouts) {
			if (layout.selected) {
				layout.DrawFunc(m_tables);
			}
		}
	}

	void UIModule::SetTargetReference(RE::TESObjectREFR* a_ref) {
		s_targetReference = a_ref;
	}

	std::optional<RE::TESObjectREFR*> UIModule::LookupReferenceByFormID(const RE::FormID& a_formID) {
		if (RE::TESForm* form = RE::TESForm::LookupByID(a_formID)) {
			if (const auto refr = form->As<RE::TESObjectREFR>(); refr) {
				return refr;
			}
		}

		return std::nullopt;
	}

	std::optional<RE::TESObjectREFR*> UIModule::LookupReferenceBySearch(const std::string& a_search) {
		if (a_search.length() > 8) {
			return std::nullopt;
		}

		for (const char& c : a_search) {
			if (std::isspace(c)) {
				return std::nullopt;
			}

			if (!std::isxdigit(c)) {
				return std::nullopt;
			}
		}

		RE::FormID formID = 0;
		auto [ptr, ec] = std::from_chars(a_search.c_str(), a_search.c_str() + a_search.size(), formID, 16);
		if (ec == std::errc()) {
			return LookupReferenceByFormID(formID);
		} else {
			return std::nullopt;
		}
	}
}
