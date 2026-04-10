#include "KitSelectorModule.h"
#include "config/EquipmentConfig.h"
#include "config/ThemeConfig.h"
#include "data/BaseObject.h"
#include "imgui.h"
#include "localization/Locale.h"
#include "ui/components/UIContainers.h"
#include "ui/components/UICustom.h"

namespace Modex
{
	void KitSelectorModule::Draw()
	{
		DrawTabMenu();
	}

	void KitSelectorModule::DrawTabMenu()
	{
		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetStyle().WindowPadding.x);

		const ImVec2 window_padding = ImGui::GetStyle().WindowPadding;
		const float avail_width = ImGui::GetContentRegionAvail().x;
		const float avail_height = ImGui::GetContentRegionAvail().y;
		const float action_pane_width = avail_width / 3.0f;
		const float table_width = avail_width - action_pane_width - window_padding.x;

		ImVec2 table_pos = ImGui::GetCursorPos();
		UIContainers::DrawBasicTablePanel("TABLE_KITSELECTOR", table_pos, ImVec2(table_width, avail_height), m_tables[0]);

		ImGui::SameLine();
		DrawActionPane(ImVec2(action_pane_width, avail_height));
	}

	int KitSelectorModule::GetKitGoldValue(const std::string& a_kitKey)
	{
		auto* kit = EquipmentConfig::KitLookup(a_kitKey);
		if (!kit) return 0;

		int value = 0;
		for (const auto& item : kit->m_items) {
			if (auto* form = RE::TESForm::LookupByEditorID(item->m_editorid); form) {
				value += form->GetGoldValue();
			}
		}
		return value;
	}

	int KitSelectorModule::GetTotalCost() const
	{
		int value = 0;
		for (const auto& key : m_selectedKeys) {
			value += static_cast<int>(GetKitGoldValue(key) * m_options.costMultiplier);
		}
		return value;
	}

	bool KitSelectorModule::CanConfirm() const
	{
		if (m_selectedKeys.empty()) {
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

	void KitSelectorModule::DrawActionPane(const ImVec2& a_size)
	{
		ImGui::BeginChild("##KitSelector::ActionPane", a_size, false);

		const float padding = ImGui::GetStyle().WindowPadding.y;

		// Title
		if (!m_options.title.empty()) {
			ImGui::Text("%s  %s", ICON_LC_PACKAGE, m_options.title.c_str());
		} else {
			ImGui::Text("%s  %s (%d)", ICON_LC_PACKAGE, Translate("SELECTION"), static_cast<int>(m_selectedKeys.size()));
		}

		// Max count indicator
		if (m_options.maxCount > 0) {
			ImGui::SameLine();
			ImGui::TextDisabled("[%d / %d]", static_cast<int>(m_selectedKeys.size()), m_options.maxCount);
		}

		ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal);

		const float lower_padding = m_options.showTotalCost || m_options.requireTotalCost ? 3.0f : 2.0f;
		const float list_height = a_size.y - padding - ImGui::GetFrameHeightWithSpacing() * lower_padding;

		if (ImGui::BeginChild("##KitSelector::SelectionList", ImVec2(0, list_height), false, false)) {
			int remove_index = -1;
			for (int i = 0; i < static_cast<int>(m_selectedKeys.size()); i++) {
				ImGui::PushID(i);

				auto* kit = EquipmentConfig::KitLookup(m_selectedKeys[i]);
				const auto name = kit ? kit->GetNameTail() : m_selectedKeys[i];
				const auto value = kit ? GetKitGoldValue(m_selectedKeys[i]) : 0;

				ImGui::Text("%s %s", ICON_LC_PACKAGE, TRUNCATE(name, ImGui::GetContentRegionAvail().x / 2.25f).c_str());

				if (m_options.requireTotalCost) {
					ImGui::SameLine();
					ImGui::TextDisabled("%s %d", ICON_LC_COINS, value);
				}

				ImGui::SameLine(ImGui::GetContentRegionAvail().x - ImGui::GetFrameHeightWithSpacing());
				if (ImGui::Button(ICON_LC_MINUS, ImVec2(ImGui::GetFrameHeight() + 1.0f, ImGui::GetFrameHeight()))) {
					remove_index = i;
				}

				// Expand kit items
				if (kit && !kit->m_items.empty()) {
					ImGui::Indent(4.0f);
					const float max_width = ImGui::GetContentRegionAvail().x;

					for (const auto& item : kit->m_items) {
						auto* form = RE::TESForm::LookupByEditorID(item->m_editorid);
						if (form) {
							const auto object = BaseObject(form, Ownership::None);
							const auto icon = object.GetItemIcon();
							const auto edid = object.GetEditorID();
							const auto item_value = object.GetGoldValue();
							ImGui::Text("%s %s", icon.c_str(), TRUNCATE(edid, max_width * 0.65f).c_str());
							ImGui::SameLine();
							ImGui::TextDisabled("%s %d", ICON_LC_COINS, item_value);
						} else {
							ImGui::Text("  %s", TRUNCATE(item->m_editorid, max_width * 0.65f).c_str());
						}
					}

					ImGui::Unindent(4.0f);
				}

				ImGui::Spacing();
				ImGui::PopID();
			}

			if (remove_index >= 0) {
				m_selectedKeys.erase(m_selectedKeys.begin() + remove_index);
			}

			if (m_selectedKeys.empty()) {
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

		// Cost display
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

	void KitSelectorModule::AddSelection(const std::string& a_kitKey)
	{
		// Single select mode: replace existing selection
		if (m_options.singleSelect) {
			m_selectedKeys.clear();
			m_selectedKeys.push_back(a_kitKey);
			return;
		}

		// Max count enforcement
		if (m_options.maxCount > 0 && static_cast<int>(m_selectedKeys.size()) >= m_options.maxCount) {
			return;
		}

		m_selectedKeys.push_back(a_kitKey);
	}

	void KitSelectorModule::RemoveSelection(int a_index)
	{
		if (a_index >= 0 && a_index < static_cast<int>(m_selectedKeys.size())) {
			m_selectedKeys.erase(m_selectedKeys.begin() + a_index);
		}
	}

	void KitSelectorModule::ClearSelection()
	{
		m_selectedKeys.clear();
	}

	void KitSelectorModule::ConfirmSelection()
	{
		if (m_callback) {
			m_callback(m_selectedKeys);
		}

		UIManager::GetSingleton()->Close();
	}

	KitSelectorModule::~KitSelectorModule()
	{
	}

	KitSelectorModule::KitSelectorModule(const FormSelectorOptions& a_options, SelectionCallback a_callback)
		: m_options(a_options)
		, m_callback(std::move(a_callback))
	{
		m_layouts.push_back({ Translate("TAB_FORMSELECTOR"), true, nullptr });

		constexpr auto table_flags =
			UITable::ModexTableFlag_Base |
			UITable::ModexTableFlag_APIMode |
			UITable::ModexTableFlag_EnableSearch |
			UITable::ModexTableFlag_EnableHeader;

		auto table = std::make_unique<UITable>("Kit", true, Ownership::Kit, table_flags);
		table->SetSelectionChangedStringCallback([this](const std::vector<std::string>& keys) {
			for (const auto& key : keys) {
				AddSelection(key);
			}
		});

		m_tables.push_back(std::move(table));
	}
}
