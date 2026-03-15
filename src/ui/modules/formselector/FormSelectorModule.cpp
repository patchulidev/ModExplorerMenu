#include "FormSelectorModule.h"
#include "core/Commands.h"
#include "data/BaseObject.h"
#include "imgui.h"
#include "localization/Locale.h"
#include "ui/components/UIContainers.h"
#include "config/ThemeConfig.h"
#include "ui/components/UICustom.h"

namespace Modex
{
	void FormSelectorModule::Draw()
	{
		DrawTabMenu();
	}

	void FormSelectorModule::DrawTabMenu()
	{
		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetStyle().WindowPadding.x);

		const ImVec2 window_padding = ImGui::GetStyle().WindowPadding;
		const float avail_width = ImGui::GetContentRegionAvail().x;
		const float avail_height = ImGui::GetContentRegionAvail().y;
		const float action_pane_width = avail_width / 3.0f;
		const float table_width = avail_width - action_pane_width - window_padding.x;

		ImVec2 table_pos = ImGui::GetCursorPos();
		UIContainers::DrawBasicTablePanel("TABLE_FORMSELECTOR", table_pos, ImVec2(table_width, avail_height), m_tables[0]);

		ImGui::SameLine();
		DrawActionPane(ImVec2(action_pane_width, avail_height));
	}

	int FormSelectorModule::GetTotalCost() const
	{
		int value = 0;

		for (auto formID : m_selected) {
			if (auto* form = RE::TESForm::LookupByID(formID); form) {
				if (form->GetFormType() == RE::FormType::Outfit) {
					value += Commands::GetOutfitValue(form->As<RE::BGSOutfit>());
				} else {
					value += static_cast<int>(form->GetGoldValue() * m_options.costMultiplier);
				}
			}
		}

		return value;
	}

	bool FormSelectorModule::CanConfirm() const
	{
		if (m_selected.empty()) {
			return false;
		}

		if (m_options.requireTotalCost) {
			auto player_gold = RE::PlayerCharacter::GetSingleton()->GetGoldAmount();
			if (GetTotalCost() > player_gold) {
				return false;
			}
		}

		if (m_options.maxCost > 0) {
			if (GetTotalCost() > m_options.maxCost) {
				return false;
			}
		}

		return true;
	}

	void FormSelectorModule::DrawActionPane(const ImVec2& a_size)
	{
		ImGui::BeginChild("##FormSelector::ActionPane", a_size, false);

		const float padding = ImGui::GetStyle().WindowPadding.y;

		// Title
		if (!m_options.title.empty()) {
			ImGui::Text("%s  %s", ICON_LC_LIST, m_options.title.c_str());
		} else {
			ImGui::Text("%s  %s (%d)", ICON_LC_LIST, Translate("SELECTION"), static_cast<int>(m_selected.size()));
		}

		// Max count indicator
		if (m_options.maxCount > 0) {
			ImGui::SameLine();
			ImGui::TextDisabled("[%d / %d]", static_cast<int>(m_selected.size()), m_options.maxCount);
		}

		ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal);

		const float lower_padding = m_options.showTotalCost || m_options.requireTotalCost ? 3.0f : 2.0f; 
		const float list_height = a_size.y - padding - ImGui::GetFrameHeightWithSpacing() * lower_padding;

		if (ImGui::BeginChild("##FormSelector::SelectionList", ImVec2(0, list_height), false, false)) {
			int remove_index = -1;
			for (int i = 0; i < static_cast<int>(m_selected.size()); i++) {
				ImGui::PushID(i);

				const auto* form = RE::TESForm::LookupByID(m_selected[i]);
				const auto name = form->GetName()[0] != '\0' ? form->GetName() : po3_GetEditorID(m_selected[i]);
				const auto value = m_ownership == Ownership::Item ? form->GetGoldValue() : Commands::GetOutfitValue(form->As<RE::BGSOutfit>()); 

				ImGui::Text("%s", TRUNCATE(name, ImGui::GetContentRegionAvail().x / 2.25f).c_str());

				if (m_options.requireTotalCost) {
					ImGui::SameLine();
					ImGui::TextDisabled("%s %d", ICON_LC_COINS, value);
				}

				ImGui::SameLine(ImGui::GetContentRegionAvail().x - ImGui::GetFrameHeightWithSpacing());
				if (ImGui::Button(ICON_LC_MINUS, ImVec2(ImGui::GetFrameHeight() + 1.0f, ImGui::GetFrameHeight()))) {
					remove_index = i;
				}

				if (m_ownership == Ownership::Outfit) {
					if (const auto outfit = form->As<RE::BGSOutfit>(); outfit) {
						ImGui::Indent(4.0f);
						const float max_width = ImGui::GetContentRegionAvail().x;

						for (auto entry : outfit->outfitItems) {
							if (entry->GetFormType() == RE::FormType::LeveledItem) {
								const auto icon = FilterProperty::GetIcon(PropertyType::kLeveledItem);
								const auto edid = po3_GetEditorID(entry->GetFormID());
								const auto value = Commands::GetProjectedLeveledListValue(entry->As<RE::TESLeveledList>());
								ImGui::Text("%s %s", icon.c_str(), TRUNCATE(edid, max_width * 0.65f).c_str());
								ImGui::SameLine();
								ImGui::TextDisabled("%s %d", ICON_LC_COINS, value);
							} else {
								const auto object = BaseObject(entry, Ownership::None);
								const auto icon = object.GetItemIcon();
								const auto edid = object.GetEditorID();
								const auto value = object.GetGoldValue();
								ImGui::Text("%s %s", icon.c_str(), TRUNCATE(edid, max_width * 0.65f).c_str());
								ImGui::SameLine();
								ImGui::TextDisabled("%s %d", ICON_LC_COINS, value);
							}
						}

						ImGui::Unindent(4.0f);
					}

					ImGui::Spacing();
				}

				ImGui::PopID();
			}

			if (remove_index >= 0) {
				m_selected.erase(m_selected.begin() + remove_index);
			}

			if (m_selected.empty()) {
				ImGui::SetCursorPosY(ImGui::GetContentRegionAvail().y / 2.0f);

				const auto hint_1_pos = UICustom::GetCenterTextPosX(Translate("API_SELECTION_HINT_1"));
				ImGui::SetCursorPosX(hint_1_pos);
				ImGui::TextDisabled("%s", Translate("API_SELECTION_HINT_1"));

				const auto hint_2_pos = UICustom::GetCenterTextPosX(Translate("API_SELECTION_HINT_2"));
				ImGui::SetCursorPosX(hint_2_pos);
				ImGui::TextDisabled("%s", Translate("API_SELECTION_HINT_2"));
			}
		}
		ImGui::EndChild();

		const float button_width = ImGui::GetContentRegionAvail().x;

		// Cost display (only when showTotalCost or requireTotalCost is set)
		if (m_options.showTotalCost || m_options.requireTotalCost) {
			auto cost = GetTotalCost();
			auto player_gold = RE::PlayerCharacter::GetSingleton()->GetGoldAmount();
			bool can_afford = player_gold >= cost;

			if (!can_afford && m_options.requireTotalCost) ImGui::PushStyleColor(ImGuiCol_TextDisabled, ThemeConfig::GetColor("ERROR"));
			{
				ImGui::TextDisabled("%s", TranslateFormat("COST", ":"));
				ImGui::SameLine();

				if (m_options.requireTotalCost) {
					ImGui::TextDisabled("%s", std::format("{} / {}", cost, player_gold).c_str());
				} else {
					ImGui::TextDisabled("%d", cost);
				}
			}
			if (!can_afford && m_options.requireTotalCost) ImGui::PopStyleColor();
		}

		// Max cost indicator
		if (m_options.maxCost > 0) {
			auto cost = GetTotalCost();
			bool over_budget = cost > m_options.maxCost;

			if (over_budget) ImGui::PushStyleColor(ImGuiCol_TextDisabled, ThemeConfig::GetColor("ERROR"));
			ImGui::TextDisabled("%s %d / %d", Translate("MAX_COST"), cost, m_options.maxCost);
			if (over_budget) ImGui::PopStyleColor();
		}

		// Confirm button
		bool can_confirm = CanConfirm();

		ImGui::PushStyleColor(ImGuiCol_Button, ThemeConfig::GetColor("CONFIRM"));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ThemeConfig::GetHover("CONFIRM"));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ThemeConfig::GetActive("CONFIRM"));

		if (!can_confirm) {
			ImGui::BeginDisabled();
		}

		if (ImGui::Button(Translate("CONFIRM"), ImVec2(button_width * 0.5f - 2.0f, 0))) {
			ConfirmSelection();
		}

		if (!can_confirm) {
			ImGui::EndDisabled();
		}

		ImGui::PopStyleColor(3);

		ImGui::SameLine();

		ImGui::PushStyleColor(ImGuiCol_Button, ThemeConfig::GetColor("DECLINE"));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ThemeConfig::GetHover("DECLINE"));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ThemeConfig::GetActive("DECLINE"));
		if (ImGui::Button(Translate("CLEAR"), ImVec2(ImGui::GetContentRegionAvail().x, 0))) {
			ClearSelection();
		}
		ImGui::PopStyleColor(3);

		ImGui::EndChild();
	}

	void FormSelectorModule::AddSelection(RE::FormID a_formID)
	{
		// Single select mode: replace existing selection
		if (m_options.singleSelect) {
			m_selected.clear();
			m_selected.push_back(a_formID);
			return;
		}

		// Max count enforcement
		if (m_options.maxCount > 0 && static_cast<int>(m_selected.size()) >= m_options.maxCount) {
			return;
		}

		m_selected.push_back(a_formID);
	}

	void FormSelectorModule::RemoveSelection(RE::FormID a_formID)
	{
		m_selected.erase(std::remove(m_selected.begin(), m_selected.end(), a_formID), m_selected.end());
	}

	void FormSelectorModule::ClearSelection()
	{
		m_selected.clear();
	}

	void FormSelectorModule::ConfirmSelection()
	{
		if (m_callback) {
			m_callback(m_selected);
		}

		UIManager::GetSingleton()->Close();
	}

	FormSelectorModule::~FormSelectorModule()
	{
	}

	const char* GetOwnershipType(Ownership a_ownership)
	{
		switch (a_ownership) {
			case Ownership::Outfit:
				return "Outfit";
			default:
				return "AddItem";
		}
	}

	FormSelectorModule::FormSelectorModule(Ownership a_ownership, const FormSelectorOptions& a_options, SelectionCallback a_callback)
		: m_ownership(a_ownership)
		, m_options(a_options)
		, m_callback(std::move(a_callback))
	{
		m_layouts.push_back({ Translate("TAB_FORMSELECTOR"), true, nullptr });

		constexpr auto table_flags =
			UITable::ModexTableFlag_Base |
			UITable::ModexTableFlag_APIMode |
			UITable::ModexTableFlag_EnableItemPreviewOnHover |
			UITable::ModexTableFlag_EnableFilterTree |
			UITable::ModexTableFlag_EnableSearch |
			UITable::ModexTableFlag_EnableHeader;

		auto table = std::make_unique<UITable>(GetOwnershipType(a_ownership), true, m_ownership, table_flags);
		table->SetSelectionChangedCallback([this](const std::vector<RE::FormID>& ids) {
			for (const auto& id : ids) {
				AddSelection(id);
			}
		});
		m_tables.push_back(std::move(table));
	}
}
