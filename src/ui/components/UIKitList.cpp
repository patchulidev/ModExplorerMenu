#include "UIKitList.h"
#include "config/EquipmentConfig.h"
#include "config/ThemeConfig.h"
#include "external/icons/IconsLucide.h"
#include "imgui.h"
#include "localization/Locale.h"
#include "ui/components/UICustom.h"
#include "ui/core/UIManager.h"
#include "ui/style/LayoutMetrics.h"

namespace Modex
{
	namespace
	{
		enum KitColumnID : int
		{
			KitColumn_Name = 0,
			KitColumn_Collection,
			KitColumn_Items,
			KitColumn_Weapons,
			KitColumn_Armor,
			KitColumn_Value,
			KitColumn_Actions,
			KitColumn_Count_,
		};

		struct ColumnDef
		{
			int                     id;
			const char*             label;
			const char*             tooltipKey;
			ImGuiTableColumnFlags   flags;
			float                   weight;
			bool                    center; // center-align header label + cell content
		};

		// UIKitList-local view of the layout metrics. Shared scaffolding
		// (pad/gap/radius) comes from Style::Metrics(); per-widget tunables
		// (column widths, search button width, etc.) come from the
		// WidgetStyle::KitList block in ThemeConfig.
		struct Tokens
		{
			float font;
			float u;
			float pad_x;
			float pad_y;
			float row_h;
			float gap_sm;
			float radius_sm;
			float col_icon_w;
			float col_value_w;
		};

		Tokens GetTokens()
		{
			const auto  m  = Style::Metrics();
			const auto& kl = ThemeConfig::GetWidgetStyle().kitList;
			Tokens t{};
			t.font        = m.font;
			t.u           = m.u;
			t.pad_x       = m.padX;
			t.pad_y       = m.padY;
			t.row_h       = m.font + t.pad_y * 2.0f;
			t.gap_sm      = m.gapSm;
			t.radius_sm   = m.radiusSm;
			t.col_icon_w  = m.font + m.u * kl.colIconWidthU;
			t.col_value_w = m.font + m.u * kl.colValueWidthU;
			return t;
		}

		int ComputeKitValue(const Kit& a_kit)
		{
			int value = 0;
			for (const auto& item : a_kit.m_items) {
				if (auto* form = RE::TESForm::LookupByEditorID(item->m_editorid); form) {
					value += form->GetGoldValue() * (std::max)(1, item->m_amount);
				}
			}
			return value;
		}

		void ComputeKitBreakdown(const Kit& a_kit, int& a_weapons, int& a_armor, std::vector<std::string>& a_missing)
		{
			a_weapons = a_armor = 0;
			a_missing.clear();
			for (const auto& item : a_kit.m_items) {
				const auto* form = RE::TESForm::LookupByEditorID(item->m_editorid);
				if (!form) {
					a_missing.push_back(item->m_editorid);
					continue;
				}
				switch (form->GetFormType()) {
				case RE::FormType::Weapon: a_weapons++; break;
				case RE::FormType::Armor:  a_armor++;   break;
				default: break;
				}
			}
		}

		bool ContainsICase(std::string_view a_haystack, std::string_view a_needle)
		{
			if (a_needle.empty()) return true;
			auto it = std::search(
				a_haystack.begin(), a_haystack.end(),
				a_needle.begin(), a_needle.end(),
				[](char a, char b) { return std::tolower(a) == std::tolower(b); });
			return it != a_haystack.end();
		}

		std::string FormatGrouped(int a_value)
		{
			std::string s = std::to_string(a_value);
			const int n = static_cast<int>(s.size());
			for (int i = n - 3; i > 0; i -= 3) {
				s.insert(s.begin() + i, ',');
			}
			return s;
		}
	}

	UIKitList::UIKitList(const std::string& a_dataID, SelectionMode a_mode) :
		m_data_id(a_dataID),
		m_mode(a_mode)
	{
		BuildRows();
	}

	void UIKitList::Refresh()
	{
		BuildRows();
		ApplyFilter();
	}

	void UIKitList::SetSelectionMode(SelectionMode a_mode)
	{
		if (m_mode == a_mode) return;
		m_mode = a_mode;

		// Trim down to a single selection if we just dropped into Single mode.
		if (m_mode == SelectionMode::Single && m_selected.size() > 1) {
			m_selected.resize(1);
			EmitSelectionChanged();
		}
	}

	void UIKitList::SetSelectedKey(const std::string& a_key)
	{
		m_selected.clear();
		if (!a_key.empty()) {
			m_selected.push_back(a_key);
		}
		EmitSelectionChanged();
	}

	void UIKitList::SetSelectedKeys(const std::vector<std::string>& a_keys)
	{
		m_selected = a_keys;
		if (m_mode == SelectionMode::Single && m_selected.size() > 1) {
			m_selected.resize(1);
		}
		EmitSelectionChanged();
	}

	void UIKitList::ClearSelection()
	{
		if (m_selected.empty()) return;
		m_selected.clear();
		EmitSelectionChanged();
	}

	bool UIKitList::IsSelected(const std::string& a_key) const
	{
		return std::find(m_selected.begin(), m_selected.end(), a_key) != m_selected.end();
	}

	void UIKitList::BuildRows()
	{
		m_rows.clear();
		m_visible.clear();

		auto& cache = EquipmentConfig::GetEquipmentList();
		m_rows.reserve(cache.size());

		for (const auto& [key, kit] : cache) {
			Row row;
			row.key        = key;
			row.name       = kit.GetNameTail();
			row.collection = kit.m_collection;
			row.tags       = kit.GetTags();
			row.spellCount = static_cast<int>(kit.m_spells.size());
			row.totalCount = static_cast<int>(kit.m_items.size() + kit.m_spells.size());
			ComputeKitBreakdown(kit, row.weaponCount, row.armorCount, row.missingItems);
			row.totalValue = ComputeKitValue(kit);
			m_rows.push_back(std::move(row));
		}

		ApplyFilter();
	}

	void UIKitList::ApplyFilter()
	{
		m_visible.clear();
		m_visible.reserve(m_rows.size());

		std::string_view needle{ m_searchBuffer };
		for (const auto& row : m_rows) {
			if (!needle.empty() &&
				!ContainsICase(row.name, needle) &&
				!ContainsICase(row.collection, needle)) {
				continue;
			}

			// Tag filter is AND across selected tags: kit must carry every
			// active filter tag to survive.
			if (!m_tagFilter.empty()) {
				bool has_all = true;
				for (const auto& required : m_tagFilter) {
					if (std::find(row.tags.begin(), row.tags.end(), required) == row.tags.end()) {
						has_all = false;
						break;
					}
				}
				if (!has_all) continue;
			}

			m_visible.push_back(&row);
		}

		SortVisible(m_sortColumn, m_sortDirection);
	}

	void UIKitList::SortVisible(int a_columnUserID, ImGuiSortDirection a_dir)
	{
		m_sortColumn    = a_columnUserID;
		m_sortDirection = a_dir;

		const bool asc = (a_dir == ImGuiSortDirection_Ascending);
		auto cmp = [a_columnUserID, asc](const Row* a, const Row* b) {
			auto ord = [asc](int r) { return asc ? (r < 0) : (r > 0); };
			auto icmp = [](int x, int y) { return (x < y) ? -1 : (x > y) ? 1 : 0; };
			switch (a_columnUserID) {
			case KitColumn_Collection: return ord(a->collection.compare(b->collection));
			case KitColumn_Items:      return ord(icmp(a->totalCount, b->totalCount));
			case KitColumn_Weapons:    return ord(icmp(a->weaponCount, b->weaponCount));
			case KitColumn_Armor:      return ord(icmp(a->armorCount, b->armorCount));
			case KitColumn_Value:      return ord(icmp(a->totalValue, b->totalValue));
			case KitColumn_Name:
			default:                   return ord(a->name.compare(b->name));
			}
		};

		std::sort(m_visible.begin(), m_visible.end(), cmp);
	}

	void UIKitList::DrawSearchBar(float a_width)
	{
		const float font   = ImGui::GetFontSize();
		const auto& kl     = ThemeConfig::GetWidgetStyle().kitList;
		const float btn_w  = font * kl.searchBtnWidthFont;
		const float gap    = ImGui::GetStyle().ItemSpacing.x;
		const float input_w = (std::max)(a_width - btn_w - gap, font * kl.searchMinInputFont);

		static bool hovered = false;
		ImGui::PushStyleColor(ImGuiCol_FrameBg,
			hovered ? ThemeConfig::GetHover("BG_LIGHT") : ThemeConfig::GetColor("BG_LIGHT"));

		ImGui::NewLine();
		const auto cursor_pos = ImGui::GetCursorScreenPos();

		if (UICustom::FancyInputText("##UIKitList::Search", "TABLE_SEARCH_HINT", "", m_searchBuffer, input_w)) {
			ApplyFilter();
		}

		if (ImGui::Shortcut(ImGuiKey_Space, ImGuiInputFlags_RouteGlobal)) {
			ImGui::SetKeyboardFocusHere(-1);
		}
		hovered = ImGui::IsItemHovered();

		ImGui::PopStyleColor();

		{ // Dropdown descriptor — centered over the search input width.
			const auto draw_list = ImGui::GetWindowDrawList();
			const auto text = Translate("SEARCH_PHRASE");
			const auto text_pos_x = (cursor_pos.x + (input_w / 2.0f)) - (ImGui::CalcTextSize(text).x / 2.0f);
			const auto text_pos_y = cursor_pos.y - ImGui::GetFrameHeight();
			const auto alpha = ImGui::GetStyle().Alpha;
			draw_list->AddText(ImVec2(text_pos_x, text_pos_y), ThemeConfig::GetColorU32("TEXT_DISABLED", alpha), text);
		}

		ImGui::SameLine();
		const int active = static_cast<int>(m_tagFilter.size());
		if (UICustom::FancyDropdownButton("UIKitList::TagFilterBtn", "FILTER_BY_TAGS", ICON_LC_TAG, "", btn_w, active)) {
			ImGui::OpenPopup("##UIKitList::TagFilter");
		}
		DrawTagFilterPopup();

		ImGui::Spacing();
	}

	void UIKitList::DrawTagFilterPopup()
	{
		if (!ImGui::BeginPopup("##UIKitList::TagFilter")) return;

		const float font  = ImGui::GetFontSize();
		const auto  known = EquipmentConfig::GetKnownTags();

		// "Clear filters" affordance only shows when something is active.
		if (!m_tagFilter.empty()) {
			if (ImGui::SmallButton(Translate("TAG_FILTER_CLEAR"))) {
				m_tagFilter.clear();
				ApplyFilter();
			}
			ImGui::Separator();
		}

		if (known.empty()) {
			ImGui::TextDisabled("%s", Translate("KIT_TAGS_NONE_KNOWN"));
		} else {
			const auto& kl      = ThemeConfig::GetWidgetStyle().kitList;
			const float row_h   = ImGui::GetFrameHeightWithSpacing();
			const float desired = static_cast<float>(known.size()) * row_h;
			const float max_h   = font * kl.tagFilterListMaxFont;
			const float list_h  = (std::min)(desired, max_h);
			const float list_w  = font * kl.tagFilterListWidthFont;

			if (ImGui::BeginChild("##UIKitList::TagFilter::List", ImVec2(list_w, list_h), false)) {
				for (const auto& tag : known) {
					bool checked = m_tagFilter.contains(tag);
					if (ImGui::Checkbox(tag.c_str(), &checked)) {
						if (checked) m_tagFilter.insert(tag);
						else         m_tagFilter.erase(tag);
						ApplyFilter();
					}
				}
			}
			ImGui::EndChild();
		}

		ImGui::EndPopup();
	}

	void UIKitList::DrawTable(const ImVec2& a_size)
	{
		const auto t = GetTokens();

		std::vector<ColumnDef> columns = {
			{ KitColumn_Name,       Translate("kName"),      "",                ImGuiTableColumnFlags_WidthStretch | ImGuiTableColumnFlags_DefaultSort | ImGuiTableColumnFlags_NoHide, 4.0f,            false },
			{ KitColumn_Collection, Translate("COLLECTION"), "",                ImGuiTableColumnFlags_WidthStretch, 2.0f,                                                                false },
			{ KitColumn_Items,      ICON_LC_BOX,             "ITEM_COUNT",      ImGuiTableColumnFlags_WidthFixed,   t.col_icon_w,                                                        true  },
			{ KitColumn_Weapons,    ICON_LC_SWORD,           "WEAPON_COUNT",    ImGuiTableColumnFlags_WidthFixed,   t.col_icon_w,                                                        true  },
			{ KitColumn_Armor,      ICON_LC_SHIELD,          "ARMOR_COUNT",     ImGuiTableColumnFlags_WidthFixed,   t.col_icon_w,                                                        true  },
			{ KitColumn_Value,      ICON_LC_COINS,           "kGoldValue",      ImGuiTableColumnFlags_WidthFixed,   t.col_value_w,                                                       true  },
		};

		if (m_showDeleteAction) {
			columns.push_back({
				KitColumn_Actions, "", "",
				ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoSort |
				ImGuiTableColumnFlags_NoResize | ImGuiTableColumnFlags_NoReorder |
				ImGuiTableColumnFlags_NoHeaderLabel,
				t.col_value_w,
				false
			});
		}

		const int column_count = static_cast<int>(columns.size());

		constexpr ImGuiTableFlags table_flags =
			ImGuiTableFlags_Sortable                |
			ImGuiTableFlags_RowBg                   |
			ImGuiTableFlags_ScrollY                 |
			ImGuiTableFlags_SizingStretchProp       |
			ImGuiTableFlags_PadOuterX               |
			ImGuiTableFlags_NoBordersInBodyUntilResize;

		ImGui::PushStyleVar(ImGuiStyleVar_CellPadding,         ImVec2(t.pad_x, 0.0f));
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,        ImVec2(t.pad_x, t.pad_y));
		ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding,       t.radius_sm);
		ImGui::PushStyleVar(ImGuiStyleVar_SelectableTextAlign, ImVec2(0.0f, 0.5f));
		ImGui::PushStyleColor(ImGuiCol_TableHeaderBg, ThemeConfig::GetColor("BG_LIGHT"));
		ImGui::PushStyleColor(ImGuiCol_TableRowBg,    ThemeConfig::GetColor("TABLE_BG_ALT"));
		ImGui::PushStyleColor(ImGuiCol_TableRowBgAlt, ThemeConfig::GetColor("TABLE_BG"));

		if (!ImGui::BeginTable("##UIKitList::Table", column_count, table_flags, a_size)) {
			ImGui::PopStyleColor(3);
			ImGui::PopStyleVar(4);
			return;
		}

		ImGui::TableSetupScrollFreeze(0, 1);
		for (const auto& c : columns) {
			ImGui::TableSetupColumn(c.label, c.flags, c.weight, c.id);
		}

		ImGui::TableNextRow(ImGuiTableRowFlags_Headers, t.row_h);
		for (int col = 0; col < column_count; col++) {
			if (!ImGui::TableSetColumnIndex(col)) continue;
			ImGui::PushID(col);

			ImGui::SetCursorPosY(ImGui::GetCursorPosY() + t.pad_y);

			// Optional horizontal centering for icon-only columns.
			if (columns[col].center) {
				const float label_w = ImGui::CalcTextSize(columns[col].label).x;
				const float avail   = ImGui::GetContentRegionAvail().x;
				if (avail > label_w) {
					ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (avail - label_w) * 0.5f);
				}
			}

			ImGui::TableHeader(ImGui::TableGetColumnName(col));
			if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort) && columns[col].tooltipKey[0] != '\0') {
				UICustom::FancyTooltip(columns[col].tooltipKey);
			}
			ImGui::PopID();
		}

		if (ImGuiTableSortSpecs* specs = ImGui::TableGetSortSpecs()) {
			if (specs->SpecsDirty && specs->SpecsCount > 0) {
				SortVisible(specs->Specs[0].ColumnUserID, specs->Specs[0].SortDirection);
				specs->SpecsDirty = false;
			}
		}

		const ImU32 disabled_col = ImGui::GetColorU32(ImGuiCol_TextDisabled);

		auto drawIntCellCenter = [&](int a_value, bool a_grouped = false) {
			const std::string text = (a_value <= 0)
				? std::string("-")
				: (a_grouped ? FormatGrouped(a_value) : std::to_string(a_value));
			const float text_w = ImGui::CalcTextSize(text.c_str()).x;
			const float avail  = ImGui::GetContentRegionAvail().x;
			if (avail > text_w) {
				ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (avail - text_w) * 0.5f);
			}
			ImGui::AlignTextToFramePadding();
			if (a_value <= 0) {
				ImGui::PushStyleColor(ImGuiCol_Text, disabled_col);
				ImGui::TextUnformatted(text.c_str());
				ImGui::PopStyleColor();
			} else {
				ImGui::TextUnformatted(text.c_str());
			}
		};

		for (const Row* row : m_visible) {
			ImGui::TableNextRow(0, t.row_h);
			ImGui::PushID(row->key.c_str());

			ImGui::TableSetColumnIndex(KitColumn_Name);
			const ImVec2 name_origin = ImGui::GetCursorScreenPos();
			const float  name_cell_w = ImGui::GetContentRegionAvail().x;
			const bool   selected    = IsSelected(row->key);
			ImGuiSelectableFlags row_flags =
				ImGuiSelectableFlags_SpanAllColumns |
				ImGuiSelectableFlags_AllowDoubleClick |
				ImGuiSelectableFlags_NoPadWithHalfSpacing;
			if (m_showDeleteAction || !row->missingItems.empty()) {
				row_flags |= ImGuiSelectableFlags_AllowOverlap;
			}
			if (ImGui::Selectable(row->name.c_str(), selected, row_flags, ImVec2(0.0f, t.row_h))) {
				HandleRowClick(*row);
			}

			if (!row->missingItems.empty()) {
				const ImVec2 icon_sz = ImGui::CalcTextSize(ICON_LC_TRIANGLE_ALERT);
				const ImVec2 icon_pos(
					name_origin.x + name_cell_w - icon_sz.x,
					name_origin.y + (t.row_h - icon_sz.y) * 0.5f);
				const ImU32 icon_col = ImGui::GetColorU32(ThemeConfig::GetColor("WARN"));
				ImGui::GetWindowDrawList()->AddText(icon_pos, icon_col, ICON_LC_TRIANGLE_ALERT);

				if (ImGui::IsMouseHoveringRect(icon_pos, icon_pos + icon_sz)) {
					ImGui::BeginTooltip();
					ImGui::Text("%s %d", ICON_LC_TRIANGLE_ALERT,
						static_cast<int>(row->missingItems.size()));
					ImGui::Separator();
					constexpr size_t kMaxShown = 8;
					const size_t shown = (std::min)(row->missingItems.size(), kMaxShown);
					for (size_t i = 0; i < shown; i++) {
						ImGui::TextUnformatted(row->missingItems[i].c_str());
					}
					if (row->missingItems.size() > kMaxShown) {
						ImGui::TextDisabled("... +%d", static_cast<int>(row->missingItems.size() - kMaxShown));
					}
					ImGui::EndTooltip();
				}
			}

			if (m_showDeleteAction) {
				DrawRowContextMenu(*row);
			}

			ImGui::TableSetColumnIndex(KitColumn_Collection);
			ImGui::AlignTextToFramePadding();
			if (row->collection.empty()) {
				ImGui::PushStyleColor(ImGuiCol_Text, disabled_col);
				ImGui::TextUnformatted("-");
				ImGui::PopStyleColor();
			} else {
				ImGui::TextUnformatted(row->collection.c_str());
			}

			ImGui::TableSetColumnIndex(KitColumn_Items);   drawIntCellCenter(row->totalCount);
			ImGui::TableSetColumnIndex(KitColumn_Weapons); drawIntCellCenter(row->weaponCount);
			ImGui::TableSetColumnIndex(KitColumn_Armor);   drawIntCellCenter(row->armorCount);
			ImGui::TableSetColumnIndex(KitColumn_Value);   drawIntCellCenter(row->totalValue, /*grouped=*/true);

			if (m_showDeleteAction && ImGui::TableSetColumnIndex(KitColumn_Actions)) {
				const ImVec2 origin = ImGui::GetCursorScreenPos();
				const ImVec2 btn_sz(t.row_h, t.row_h);
				const bool   clicked = ImGui::InvisibleButton("##del", btn_sz);
				const bool   hovered = ImGui::IsItemHovered();
				const bool   active  = ImGui::IsItemActive();

				ImDrawList* dl = ImGui::GetWindowDrawList();
				if (hovered || active) {
					const ImU32 bg = ImGui::GetColorU32(active
						? ThemeConfig::GetActive("DECLINE")
						: ThemeConfig::GetHover("DECLINE"));
					dl->AddRectFilled(origin, origin + btn_sz, bg, t.radius_sm);
				}

				const ImVec2 icon_sz  = ImGui::CalcTextSize(ICON_LC_TRASH_2);
				const ImVec2 icon_pos = origin + (btn_sz - icon_sz) * 0.5f;
				const ImU32  icon_col = hovered
					? IM_COL32_WHITE
					: ImGui::GetColorU32(ImGuiCol_TextDisabled);
				dl->AddText(icon_pos, icon_col, ICON_LC_TRASH_2);

				if (clicked) {
					RequestDeleteWithConfirm(row->key, row->name);
				}
			}

			ImGui::PopID();
		}

		ImGui::EndTable();
		ImGui::PopStyleColor(3);
		ImGui::PopStyleVar(4);
	}

	void UIKitList::DrawRowContextMenu(const Row& a_row)
	{
		if (!ImGui::BeginPopupContextItem("##UIKitList::RowCtx")) {
			return;
		}

		if (ImGui::MenuItem(Translate("KIT_COPY"))) {
			if (auto* kit = EquipmentConfig::KitLookup(a_row.key)) {
				EquipmentConfig::CopyKit(*kit);
				Refresh();
			}
		}

		if (ImGui::MenuItem(Translate("KIT_TAGS"))) {
			const std::string key = a_row.key;
			UIManager::GetSingleton()->ShowKitTagsEditor(key, [this]() { Refresh(); });
		}

		if (ImGui::MenuItem(Translate("KIT_RENAME"))) {
			const std::string key     = a_row.key;
			const std::string current = a_row.name;
			UIManager::GetSingleton()->ShowInputBox(
				Translate("POPUP_KIT_RENAME_TITLE"),
				Translate("POPUP_KIT_RENAME_DESC"),
				current,
				[this, key](std::string a_input) {
					if (auto* kit = EquipmentConfig::KitLookup(key)) {
						if (auto renamed = EquipmentConfig::RenameKit(*kit, a_input); renamed) {
							auto it = std::find(m_selected.begin(), m_selected.end(), key);
							if (it != m_selected.end()) {
								*it = renamed.m_key;
								EmitSelectionChanged();
							}
							Refresh();
						}
					}
				}
			);
		}

		ImGui::Separator();

		if (ImGui::MenuItem(Translate("KIT_DELETE"))) {
			RequestDeleteWithConfirm(a_row.key, a_row.name);
		}

		ImGui::EndPopup();
	}

	void UIKitList::RequestDeleteWithConfirm(const std::string& a_key, const std::string& a_label)
	{
		UIManager::GetSingleton()->ShowWarning(
			std::string(Translate("POPUP_KIT_DELETE_TITLE")) + " - " + a_label,
			Translate("POPUP_KIT_DELETE_DESC"),
			true,
			[this, key = a_key]() {
				if (auto* kit = EquipmentConfig::KitLookup(key)) {
					EquipmentConfig::DeleteKit(*kit);
				}
				auto it = std::find(m_selected.begin(), m_selected.end(), key);
				if (it != m_selected.end()) {
					m_selected.erase(it);
					EmitSelectionChanged();
				}
				Refresh();
			}
		);
	}

	void UIKitList::HandleRowClick(const Row& a_row)
	{
		const bool ctrl  = ImGui::GetIO().KeyCtrl;
		const bool dbl   = ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left);

		if (m_mode == SelectionMode::Multi && ctrl) {
			auto it = std::find(m_selected.begin(), m_selected.end(), a_row.key);
			if (it != m_selected.end()) {
				m_selected.erase(it);
			} else {
				m_selected.push_back(a_row.key);
			}
		} else if (m_mode == SelectionMode::Multi) {
			m_selected.assign(1, a_row.key);
		} else {
			m_selected.assign(1, a_row.key);
		}

		EmitSelectionChanged();

		if (dbl && m_onKitActivated) {
			m_onKitActivated(a_row.key);
		}
	}

	void UIKitList::EmitSelectionChanged()
	{
		if (m_onSelectionChanged) {
			m_onSelectionChanged(m_selected);
		}
	}

	void UIKitList::Draw(const ImVec2& a_size)
	{
		const auto t = GetTokens();
		const float avail_w = a_size.x > 0.0f ? a_size.x : ImGui::GetContentRegionAvail().x;
		const float avail_h = a_size.y > 0.0f ? a_size.y : ImGui::GetContentRegionAvail().y;
		const float start_y = ImGui::GetCursorPosY();

		DrawSearchBar(avail_w);
		ImGui::Dummy(ImVec2(0.0f, t.gap_sm));

		// Reserve space for the footer below the table.
		const float footer_h = t.font + t.gap_sm;
		const float consumed = ImGui::GetCursorPosY() - start_y;
		const float table_h  = avail_h > 0.0f ? (std::max)(0.0f, avail_h - consumed - footer_h) : 0.0f;

		if (m_rows.empty()) {
			DrawEmptyState(ImVec2(avail_w, table_h), Translate("KIT_LIST_EMPTY"), /*showCreateCTA=*/true);
		} else if (m_visible.empty()) {
			DrawEmptyState(ImVec2(avail_w, table_h), Translate("KIT_LIST_NO_MATCH"), /*showCreateCTA=*/false);
		} else {
			DrawTable(ImVec2(avail_w, table_h));
		}

		DrawFooter(avail_w);
	}

	void UIKitList::DrawEmptyState(const ImVec2& a_size, const char* a_message, bool a_showCreateCTA)
	{
		const auto t = GetTokens();
		const bool cta_visible = a_showCreateCTA && static_cast<bool>(m_onCreateRequested);

		ImGui::PushStyleColor(ImGuiCol_ChildBg, ThemeConfig::GetColor("BG_LIGHT"));
		ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, t.radius_sm);
		if (ImGui::BeginChild("##UIKitList::EmptyState", a_size, true, ImGuiWindowFlags_NoScrollbar)) {
			const float avail   = ImGui::GetContentRegionAvail().x;
			const float text_w  = ImGui::CalcTextSize(a_message).x;
			const float btn_w   = t.font * ThemeConfig::GetWidgetStyle().kitList.emptyStateBtnFont;
			const float btn_h   = t.row_h;

			// Vertically center the message + optional CTA as a single block.
			const float block_h = cta_visible ? (t.font + t.gap_sm + btn_h) : t.font;
			const float top_pad = (ImGui::GetContentRegionAvail().y - block_h) * 0.5f;
			if (top_pad > 0.0f) ImGui::SetCursorPosY(ImGui::GetCursorPosY() + top_pad);

			// Message
			const float msg_x = (avail - text_w) * 0.5f;
			if (msg_x > 0.0f) ImGui::SetCursorPosX(ImGui::GetCursorPosX() + msg_x);
			ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetColorU32(ImGuiCol_TextDisabled));
			ImGui::TextUnformatted(a_message);
			ImGui::PopStyleColor();

			// CTA
			if (cta_visible) {
				ImGui::Dummy(ImVec2(0.0f, t.gap_sm));
				const float btn_x = (avail - btn_w) * 0.5f;
				if (btn_x > 0.0f) ImGui::SetCursorPosX(ImGui::GetCursorPosX() + btn_x);
				ImGui::PushStyleColor(ImGuiCol_Button,        ThemeConfig::GetColor("PRIMARY"));
				ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ThemeConfig::GetHover("PRIMARY"));
				ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ThemeConfig::GetActive("PRIMARY"));
				const std::string label = std::string(ICON_LC_PLUS) + "  " + Translate("KIT_CREATE");
				if (ImGui::Button(label.c_str(), ImVec2(btn_w, btn_h))) {
					m_onCreateRequested();
				}
				ImGui::PopStyleColor(3);
			}
		}
		ImGui::EndChild();
		ImGui::PopStyleVar();
		ImGui::PopStyleColor();
	}

	void UIKitList::DrawFooter(float a_width)
	{
		const int total    = static_cast<int>(m_rows.size());
		const int shown    = static_cast<int>(m_visible.size());
		const int selected = static_cast<int>(m_selected.size());
		const bool filtered = m_searchBuffer[0] != '\0';

		const std::string left = filtered ? std::format("{} / {}", shown, total) : std::format("{}", total);
		ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetColorU32(ImGuiCol_TextDisabled));
		ImGui::TextUnformatted(left.c_str());

		if (selected > 0) {
			const std::string right = std::format("{} {}", selected, Translate("SELECTED"));
			const float right_w = ImGui::CalcTextSize(right.c_str()).x;
			ImGui::SameLine(a_width - right_w);
			ImGui::TextUnformatted(right.c_str());
		}
		ImGui::PopStyleColor();
	}
}
