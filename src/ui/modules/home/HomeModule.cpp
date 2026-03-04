#include "HomeModule.h"
#include "config/EquipmentConfig.h"
#include "config/ThemeConfig.h"
#include "data/Data.h"
#include "imgui_internal.h"
#include "localization/FontManager.h"
#include "localization/Locale.h"
#include "ui/components/UICustom.h"
#include "ui/core/UIManager.h"

// A large part of the ImGui design/layout code here was written by Claude AI. <3

namespace Modex
{
	void HomeModule::Draw()
	{
		DrawTabMenu();
	}

	// -- Feature Card (Welcome Tab) --
	// Renders: [colored left border] [icon] [bold title] / [description] / [arrow on hover]
	void HomeModule::DrawFeatureCard(const char* a_icon, const char* a_titleKey, const char* a_descKey, uint8_t a_moduleIndex, float a_cardWidth)
	{
		const float card_height = ImGui::GetFontSize() * 4.0f;
		const float border_width = 4.0f;
		const float icon_size = ImGui::GetFontSize() * 1.5f;
		const float padding = ImGui::GetStyle().FramePadding.x;
		const float alpha = ImGui::GetStyle().Alpha;

		ImVec2 cursor = ImGui::GetCursorScreenPos();

		// Invisible button for click detection.
		ImGui::PushID(a_titleKey);
		bool clicked = ImGui::InvisibleButton("##card", ImVec2(a_cardWidth, card_height));
		bool hovered = ImGui::IsItemHovered();
		ImGui::PopID();

		ImDrawList* draw_list = ImGui::GetWindowDrawList();

		// Card background.
		ImU32 bg_color = hovered
			? ThemeConfig::GetColorU32("BG", 0.9f * alpha)
			: ThemeConfig::GetColorU32("BG", 0.5f * alpha);
		draw_list->AddRectFilled(cursor, ImVec2(cursor.x + a_cardWidth, cursor.y + card_height), bg_color, 4.0f);

		// Left border accent.
		ImU32 primary_color = ThemeConfig::GetColorU32("PRIMARY", alpha);
		draw_list->AddRectFilled(cursor, ImVec2(cursor.x + border_width, cursor.y + card_height), primary_color, 4.0f, ImDrawFlags_RoundCornersLeft);

		// Icon.
		float text_x = cursor.x + border_width + padding * 2;
		float text_y = cursor.y + padding;
		ImGui::PushFont(NULL, icon_size);
		draw_list->AddText(ImVec2(text_x, text_y), ThemeConfig::GetColorU32("PRIMARY", alpha), a_icon);
		ImGui::PopFont();

		// Title (bold).
		float title_x = text_x + icon_size + padding;
		ImGui::PushFontBold();
		draw_list->AddText(ImVec2(title_x, text_y), ThemeConfig::GetColorU32("TEXT", alpha), Translate(a_titleKey));
		ImGui::PopFont();

		// Description.
		float desc_y = text_y + ImGui::GetFontSize() * 1.6f;
		draw_list->AddText(ImVec2(title_x, desc_y), ThemeConfig::GetColorU32("TEXT_DISABLED", alpha), Translate(a_descKey));

		// Hover arrow.
		if (hovered) {
			float arrow_x = cursor.x + a_cardWidth - ImGui::GetFontSize() * 1.5f;
			float arrow_y = cursor.y + (card_height - ImGui::GetFontSize()) * 0.5f;
			draw_list->AddText(ImVec2(arrow_x, arrow_y), ThemeConfig::GetColorU32("PRIMARY", alpha), ICON_LC_CHEVRON_RIGHT);
		}

		if (clicked) {
			UIManager::GetSingleton()->NavigateToModule(a_moduleIndex);
		}
	}

	// -- Shortcut Entry (Shortcuts Tab) --
	// Renders: [key visual rounded rect] + [description text]
	void HomeModule::DrawShortcutEntry(const char* a_keyLabel, const char* a_descKey)
	{
		ImDrawList* draw_list = ImGui::GetWindowDrawList();
		const float font_size = ImGui::GetFontSize();
		const float padding = ImGui::GetStyle().FramePadding.x;
		const float alpha = ImGui::GetStyle().Alpha;

		// Measure key label with bold font so the box fits correctly.
		ImGui::PushFontBold();
		ImVec2 key_text_size = ImGui::CalcTextSize(a_keyLabel);
		float key_width = key_text_size.x + padding * 4;
		float key_height = key_text_size.y + padding * 2;

		ImVec2 cursor = ImGui::GetCursorScreenPos();

		// Key background rectangle.
		ImU32 key_bg = ThemeConfig::GetColorU32("BG", 0.8f * alpha);
		ImU32 key_border = ThemeConfig::GetColorU32("BORDER", alpha);
		draw_list->AddRectFilled(cursor, ImVec2(cursor.x + key_width, cursor.y + key_height), key_bg, 3.0f);
		draw_list->AddRect(cursor, ImVec2(cursor.x + key_width, cursor.y + key_height), key_border, 3.0f);

		// Key text (centered in rect).
		float key_text_x = cursor.x + (key_width - key_text_size.x) * 0.5f;
		float key_text_y = cursor.y + (key_height - key_text_size.y) * 0.5f;
		draw_list->AddText(ImGui::GetFont(), ImGui::GetFontSize(), ImVec2(key_text_x, key_text_y), ThemeConfig::GetColorU32("TEXT", alpha), a_keyLabel);
		ImGui::PopFont();

		// Description text.
		float desc_x = cursor.x + key_width + padding * 2;
		float desc_y = cursor.y + (key_height - font_size) * 0.5f;
		draw_list->AddText(ImVec2(desc_x, desc_y), ThemeConfig::GetColorU32("TEXT", alpha), Translate(a_descKey));

		// Advance cursor.
		ImGui::Dummy(ImVec2(0, key_height + ImGui::GetStyle().ItemSpacing.y));
	}

	// -- Feature Section (Features Tab) --
	void HomeModule::DrawFeatureSection(const char* a_titleKey, const char* a_descKey)
	{
		ImGui::PushFontBold();
		ImGui::PushStyleColor(ImGuiCol_Text, ThemeConfig::GetColor("PRIMARY", ImGui::GetStyle().Alpha));
		ImGui::SeparatorText(Translate(a_titleKey));
		ImGui::PopStyleColor();
		ImGui::PopFont();

		ImGui::Indent(8.0f);
		ImGui::PushTextWrapPos(ImGui::GetContentRegionAvail().x + ImGui::GetCursorPosX() - 16.0f);
		ImGui::TextWrapped("%s", Translate(a_descKey));
		ImGui::PopTextWrapPos();
		ImGui::Unindent(8.0f);
		ImGui::NewLine();
	}

	// -- Stats Row --
	// Renders centered inline stat badges: [icon] [count bold] [label dim] ...
	void HomeModule::DrawStatsRow()
	{
		auto* data = Data::GetSingleton();
		const float alpha = ImGui::GetStyle().Alpha;
		const float inner_pad = 4.0f;
		const float badge_gap = 20.0f;

		struct Stat {
			const char* icon;
			const char* labelKey;
			size_t      count;
		};

		Stat stats[] = {
			{ ICON_LC_DATABASE, "HOME_STAT_ITEMS",   data->GetAddItemList().size() },
			{ ICON_LC_USER,     "HOME_STAT_NPCS",    data->GetNPCList().size() },
			{ ICON_LC_MAP_PIN,  "HOME_STAT_CELLS",   data->GetTeleportList().size() },
			{ ICON_LC_SHIRT,    "HOME_STAT_OUTFITS", data->GetOutfitList().size() },
			{ ICON_LC_SAVE,     "HOME_STAT_KITS",    EquipmentConfig::GetEquipmentList().size() },
		};

		constexpr int num_stats = 5;

		// Pre-format count strings.
		char count_strs[num_stats][16];
		for (int i = 0; i < num_stats; i++) {
			snprintf(count_strs[i], sizeof(count_strs[i]), "%zu", stats[i].count);
		}

		// Measure total width for centering.
		float total_width = 0;
		for (int i = 0; i < num_stats; i++) {
			total_width += ImGui::CalcTextSize(stats[i].icon).x + inner_pad;
			ImGui::PushFontBold();
			total_width += ImGui::CalcTextSize(count_strs[i]).x + inner_pad;
			ImGui::PopFont();
			total_width += ImGui::CalcTextSize(Translate(stats[i].labelKey)).x;
			if (i < num_stats - 1) {
				total_width += badge_gap;
			}
		}

		ImDrawList* dl = ImGui::GetWindowDrawList();
		float start_x = ImGui::GetCursorScreenPos().x + (ImGui::GetContentRegionAvail().x - total_width) * 0.5f;
		float y = ImGui::GetCursorScreenPos().y;
		float x = start_x;

		for (int i = 0; i < num_stats; i++) {
			// Icon.
			dl->AddText(ImVec2(x, y), ThemeConfig::GetColorU32("PRIMARY", alpha), stats[i].icon);
			x += ImGui::CalcTextSize(stats[i].icon).x + inner_pad;

			// Count (bold).
			ImGui::PushFontBold();
			dl->AddText(ImGui::GetFont(), ImGui::GetFontSize(), ImVec2(x, y), ThemeConfig::GetColorU32("TEXT", alpha), count_strs[i]);
			x += ImGui::CalcTextSize(count_strs[i]).x + inner_pad;
			ImGui::PopFont();

			// Label.
			dl->AddText(ImVec2(x, y), ThemeConfig::GetColorU32("TEXT_DISABLED", alpha), Translate(stats[i].labelKey));
			x += ImGui::CalcTextSize(Translate(stats[i].labelKey)).x + badge_gap;
		}

		ImGui::Dummy(ImVec2(0, ImGui::GetFontSize() + ImGui::GetStyle().ItemSpacing.y));
	}

	// -- Target Reference Status Panel --
	// Shows current target info or a "no target" prompt. Clickable to open reference lookup.
	void HomeModule::DrawTargetStatus(float a_width, float a_height)
	{
		const float border_width = 4.0f;
		const float padding = ImGui::GetStyle().FramePadding.x;
		const float alpha = ImGui::GetStyle().Alpha;

		ImVec2 cursor = ImGui::GetCursorScreenPos();

		// Click detection.
		ImGui::PushID("##home_target");
		bool clicked = ImGui::InvisibleButton("##target", ImVec2(a_width, a_height));
		bool hovered = ImGui::IsItemHovered();
		ImGui::PopID();

		ImDrawList* dl = ImGui::GetWindowDrawList();

		// Background.
		ImU32 bg = hovered
			? ThemeConfig::GetColorU32("BG", 0.7f * alpha)
			: ThemeConfig::GetColorU32("BG", 0.4f * alpha);
		dl->AddRectFilled(cursor, ImVec2(cursor.x + a_width, cursor.y + a_height), bg, 4.0f);

		auto* target = UIModule::GetTargetReference();
		bool has_target = target != nullptr;

		// Left border color based on target state.
		ImU32 accent = has_target
			? ThemeConfig::GetColorU32("PRIMARY", alpha)
			: ThemeConfig::GetColorU32("WARN", alpha);
		dl->AddRectFilled(cursor, ImVec2(cursor.x + border_width, cursor.y + a_height), accent, 4.0f, ImDrawFlags_RoundCornersLeft);

		float content_x = cursor.x + border_width + padding * 2;

		// Header line: icon + "Target Reference" or "No Target Set"
		const char* icon = has_target ? ICON_LC_CROSSHAIR : ICON_LC_CIRCLE_ALERT;
		const char* header_key = has_target ? "HOME_TARGET_HEADER" : "HOME_TARGET_NONE";

		float line_y = cursor.y + padding;
		dl->AddText(ImVec2(content_x, line_y), accent, icon);
		float icon_w = ImGui::CalcTextSize(icon).x;

		ImGui::PushFontBold();
		dl->AddText(ImGui::GetFont(), ImGui::GetFontSize(), ImVec2(content_x + icon_w + padding, line_y),
			ThemeConfig::GetColorU32("TEXT", alpha), Translate(header_key));
		ImGui::PopFont();

		// Detail line: name + formID, or hint text.
		float detail_y = line_y + ImGui::GetFontSize() * 1.5f;
		if (has_target) {
			const char* name = target->GetName();
			if (!name || name[0] == '\0') {
				name = "(unnamed)";
			}
			char formid_str[16];
			snprintf(formid_str, sizeof(formid_str), "  [%08X]", target->formID);

			dl->AddText(ImVec2(content_x + padding, detail_y), ThemeConfig::GetColorU32("TEXT", alpha), name);
			float name_w = ImGui::CalcTextSize(name).x;
			dl->AddText(ImVec2(content_x + padding + name_w, detail_y), ThemeConfig::GetColorU32("TEXT_DISABLED", alpha), formid_str);
		} else {
			dl->AddText(ImVec2(content_x + padding, detail_y), ThemeConfig::GetColorU32("TEXT_DISABLED", alpha), Translate("HOME_TARGET_HINT"));
		}

		// Hover hint.
		if (hovered) {
			float hint_y = detail_y + ImGui::GetFontSize() * 1.4f;
			dl->AddText(ImVec2(content_x + padding, hint_y), ThemeConfig::GetColorU32("PRIMARY", 0.8f * alpha), Translate("HOME_TARGET_CLICK"));
		}

		if (clicked) {
			UIManager::GetSingleton()->ShowReferenceLookup(
				Translate("STATUS_BAR_TITLE"),
				Translate("STATUS_BAR_DESC"),
				[](RE::FormID a_formID) {
					if (auto* ref = UIModule::LookupReferenceByFormID(a_formID)) {
						UIModule::SetTargetReference(ref);
					}
				}
			);
		}
	}

	// -- Quick Actions Panel --
	// Small action buttons: Set Target (Player), Settings, Browse Kits
	void HomeModule::DrawQuickActions(float a_width, float a_height)
	{
		const float border_width = 4.0f;
		const float padding = ImGui::GetStyle().FramePadding.x;
		const float alpha = ImGui::GetStyle().Alpha;

		ImVec2 cursor = ImGui::GetCursorScreenPos();
		ImDrawList* dl = ImGui::GetWindowDrawList();

		// Panel background.
		dl->AddRectFilled(cursor, ImVec2(cursor.x + a_width, cursor.y + a_height),
			ThemeConfig::GetColorU32("BG", 0.4f * alpha), 4.0f);

		// Left border accent.
		dl->AddRectFilled(cursor, ImVec2(cursor.x + border_width, cursor.y + a_height),
			ThemeConfig::GetColorU32("SECONDARY", alpha), 4.0f, ImDrawFlags_RoundCornersLeft);

		float content_x = cursor.x + border_width + padding * 2;

		// Header.
		ImGui::PushFontBold();
		dl->AddText(ImGui::GetFont(), ImGui::GetFontSize(), ImVec2(content_x, cursor.y + padding),
			ThemeConfig::GetColorU32("TEXT", alpha), Translate("HOME_ACTIONS_HEADER"));
		ImGui::PopFont();

		// Action buttons (stacked vertically).
		struct ActionInfo {
			const char* icon;
			const char* labelKey;
			uint8_t     moduleIndex;  // 0xFF = special action
		};

		static const ActionInfo actions[] = {
			{ ICON_LC_CROSSHAIR, "HOME_ACTION_SET_PLAYER", 0xFF },
			{ ICON_LC_SETTINGS,  "HOME_ACTION_SETTINGS",   7 },
			{ ICON_LC_PACKAGE,   "HOME_ACTION_KITS",       2 },
		};

		float btn_y = cursor.y + padding + ImGui::GetFontSize() * 1.6f;
		const float btn_height = ImGui::GetFontSize() * 1.4f;
		const float btn_width = a_width - border_width - padding * 4;

		for (int i = 0; i < 3; i++) {
			ImVec2 btn_pos = ImVec2(content_x, btn_y);
			ImVec2 btn_end = ImVec2(content_x + btn_width, btn_y + btn_height);

			// Hit test.
			ImGui::SetCursorScreenPos(btn_pos);
			ImGui::PushID(actions[i].labelKey);
			bool btn_clicked = ImGui::InvisibleButton("##action", ImVec2(btn_width, btn_height));
			bool btn_hovered = ImGui::IsItemHovered();
			ImGui::PopID();

			// Button background on hover.
			if (btn_hovered) {
				dl->AddRectFilled(btn_pos, btn_end,
					ThemeConfig::GetColorU32("BG", 0.6f * alpha), 3.0f);
			}

			// Icon + label.
			float text_y = btn_y + (btn_height - ImGui::GetFontSize()) * 0.5f;
			dl->AddText(ImVec2(content_x + padding, text_y),
				btn_hovered ? ThemeConfig::GetColorU32("PRIMARY", alpha) : ThemeConfig::GetColorU32("TEXT_DISABLED", alpha),
				actions[i].icon);
			float icon_w = ImGui::CalcTextSize(actions[i].icon).x;
			dl->AddText(ImVec2(content_x + padding + icon_w + padding, text_y),
				btn_hovered ? ThemeConfig::GetColorU32("TEXT", alpha) : ThemeConfig::GetColorU32("TEXT_DISABLED", alpha),
				Translate(actions[i].labelKey));

			if (btn_clicked) {
				if (actions[i].moduleIndex == 0xFF) {
					// Set target to player reference.
					if (auto* player = RE::PlayerCharacter::GetSingleton()) {
						UIModule::SetTargetReference(player);
					}
				} else {
					UIManager::GetSingleton()->NavigateToModule(actions[i].moduleIndex);
				}
			}

			btn_y += btn_height;
		}

		// Reset cursor past the panel.
		ImGui::SetCursorScreenPos(ImVec2(cursor.x, cursor.y));
		ImGui::Dummy(ImVec2(a_width, a_height));
	}

	// -- Tip Card --
	// Rotating "Did you know?" tip that cycles through helpful hints.
	void HomeModule::DrawTipCard(float a_width, float a_height)
	{
		const float border_width = 4.0f;
		const float padding = ImGui::GetStyle().FramePadding.x;
		const float alpha = ImGui::GetStyle().Alpha;

		ImVec2 cursor = ImGui::GetCursorScreenPos();
		ImDrawList* dl = ImGui::GetWindowDrawList();

		// Background.
		dl->AddRectFilled(cursor, ImVec2(cursor.x + a_width, cursor.y + a_height),
			ThemeConfig::GetColorU32("BG", 0.4f * alpha), 4.0f);

		// Left border.
		dl->AddRectFilled(cursor, ImVec2(cursor.x + border_width, cursor.y + a_height),
			ThemeConfig::GetColorU32("TEXT_DISABLED", 0.6f * alpha), 4.0f, ImDrawFlags_RoundCornersLeft);

		float content_x = cursor.x + border_width + padding * 2;

		// Header: lightbulb icon + "Did You Know?"
		dl->AddText(ImVec2(content_x, cursor.y + padding),
			ThemeConfig::GetColorU32("TEXT_DISABLED", alpha), ICON_LC_LIGHTBULB);
		float icon_w = ImGui::CalcTextSize(ICON_LC_LIGHTBULB).x;

		ImGui::PushFontBold();
		dl->AddText(ImGui::GetFont(), ImGui::GetFontSize(), ImVec2(content_x + icon_w + padding, cursor.y + padding),
			ThemeConfig::GetColorU32("TEXT", alpha), Translate("HOME_TIP_HEADER"));
		ImGui::PopFont();

		// Rotating tip text.
		static const char* tip_keys[] = {
			"HOME_TIP_1",
			"HOME_TIP_2",
			"HOME_TIP_3",
			"HOME_TIP_4",
			"HOME_TIP_5",
			"HOME_TIP_6",
			"HOME_TIP_7",
			"HOME_TIP_8",
		};
		constexpr int num_tips = 8;

		int tip_index = static_cast<int>(ImGui::GetTime() / 5.0) % num_tips;

		// Tip body (wrapped).
		float text_x = content_x + padding;
		float text_y = cursor.y + padding + ImGui::GetFontSize() * 1.5f;
		float wrap_width = a_width - border_width - padding * 5;

		dl->AddText(ImGui::GetFont(), ImGui::GetFontSize(), ImVec2(text_x, text_y),
			ThemeConfig::GetColorU32("TEXT_DISABLED", alpha), Translate(tip_keys[tip_index]),
			nullptr, wrap_width);

		// Advance cursor.
		ImGui::Dummy(ImVec2(a_width, a_height));
	}

	// -- Welcome Tab Layout --
	void HomeModule::DrawWelcomeLayout(std::vector<std::unique_ptr<UITable>>& a_tables)
	{
		(void)a_tables;

		if (ImGui::BeginChild("##HomeModule::Welcome", ImVec2(0, 0), false)) {
			const float spacing = ImGui::GetStyle().ItemSpacing.x;
			const float avail_w = ImGui::GetContentRegionAvail().x;

			ImGui::NewLine();

			// Title.
			ImGui::PushFontBold(ImGui::GetFontSize() * 1.5f);
			ImGui::SetCursorPosX(UICustom::GetCenterTextPosX(Translate("HOME_WELCOME_TITLE")));
			ImGui::Text("%s", Translate("HOME_WELCOME_TITLE"));
			ImGui::PopFont();

			// Subtitle.
			ImGui::SetCursorPosX(UICustom::GetCenterTextPosX(Translate("HOME_WELCOME_DESC")));
			ImGui::TextDisabled("%s", Translate("HOME_WELCOME_DESC"));

			ImGui::NewLine();

			// Stats row.
			DrawStatsRow();

			ImGui::NewLine();

			// Module card grid (2 columns, 3 rows).
			const float card_width = (avail_w - spacing) * 0.5f;

			struct CardInfo {
				const char* icon;
				const char* titleKey;
				const char* descKey;
				uint8_t     moduleIndex;
			};

			// m_moduleInfo vector indices (Object commented out):
			// Home=0, AddItem=1, Equipment=2, Inventory=3, Actor=4, Teleport=5, Outfit=6, Settings=7
			static const CardInfo cards[] = {
				{ ICON_LC_PLUS,    "HOME_CARD_ADDITEM",    "HOME_CARD_ADDITEM_DESC",    1 },
				{ ICON_LC_PACKAGE, "HOME_CARD_EQUIPMENT",  "HOME_CARD_EQUIPMENT_DESC",  2 },
				{ ICON_LC_PACKAGE, "HOME_CARD_INVENTORY",  "HOME_CARD_INVENTORY_DESC",  3 },
				{ ICON_LC_USER,    "HOME_CARD_ACTOR",      "HOME_CARD_ACTOR_DESC",      4 },
				{ ICON_LC_MAP_PIN, "HOME_CARD_TELEPORT",   "HOME_CARD_TELEPORT_DESC",   5 },
				{ ICON_LC_SHIRT,   "HOME_CARD_OUTFIT",     "HOME_CARD_OUTFIT_DESC",     6 },
			};

			for (int i = 0; i < 6; i++) {
				if (i % 2 != 0) {
					ImGui::SameLine(0, spacing);
				}

				DrawFeatureCard(cards[i].icon, cards[i].titleKey, cards[i].descKey, cards[i].moduleIndex, card_width);
			}

			ImGui::NewLine();

			const float panel_height = ImGui::GetFontSize() * 6.0f;
			const float panel_width = (avail_w - spacing * 2) / 3.0f;

			DrawTargetStatus(panel_width, panel_height);
			ImGui::SameLine(0, spacing);
			DrawQuickActions(panel_width, panel_height);
			ImGui::SameLine(0, spacing);
			DrawTipCard(panel_width, panel_height);
		}
		ImGui::EndChild();
	}

	// -- Shortcuts Tab Layout --
	void HomeModule::DrawShortcutsLayout(std::vector<std::unique_ptr<UITable>>& a_tables)
	{
		(void)a_tables;

		if (ImGui::BeginChild("##HomeModule::Shortcuts", ImVec2(0, 0), false)) {
			ImGui::NewLine();

			// Navigation section.
			UICustom::Settings_Header("HOME_SHORTCUTS_NAV");

			DrawShortcutEntry("Space",             "HOME_SHORTCUT_SPACE");
			DrawShortcutEntry("Escape",            "HOME_SHORTCUT_ESCAPE");
			DrawShortcutEntry("Up / Down",         "HOME_SHORTCUT_UPDOWN");
			DrawShortcutEntry("Shift + Up / Down", "HOME_SHORTCUT_SHIFT_UPDOWN");

			ImGui::NewLine();

			// Table Actions section.
			UICustom::Settings_Header("HOME_SHORTCUTS_TABLE");

			DrawShortcutEntry("Double-click",           "HOME_SHORTCUT_DOUBLECLICK");
			DrawShortcutEntry("Shift + Double-click",   "HOME_SHORTCUT_SHIFT_DOUBLECLICK");
			DrawShortcutEntry("Right-click",            "HOME_SHORTCUT_RIGHTCLICK");
			DrawShortcutEntry("Ctrl + Left Arrow",      "HOME_SHORTCUT_CTRL_LEFT");
			DrawShortcutEntry("F",                      "HOME_SHORTCUT_F");

			ImGui::NewLine();

			// Target Reference section.
			UICustom::Settings_Header("HOME_SHORTCUTS_TARGET");

			DrawShortcutEntry("T",             "HOME_SHORTCUT_T");
			DrawShortcutEntry("C",             "HOME_SHORTCUT_C");
			DrawShortcutEntry("Shift + Click", "HOME_SHORTCUT_SHIFT_CLICK");
			DrawShortcutEntry("Ctrl + Click",  "HOME_SHORTCUT_CTRL_CLICK");
		}
		ImGui::EndChild();
	}

	// -- Features Tab Layout --
	void HomeModule::DrawFeaturesLayout(std::vector<std::unique_ptr<UITable>>& a_tables)
	{
		(void)a_tables;

		if (ImGui::BeginChild("##HomeModule::Features", ImVec2(0, 0), false)) {
			ImGui::NewLine();

			DrawFeatureSection("HOME_FEATURE_TARGETS",   "HOME_FEATURE_TARGETS_DESC");
			DrawFeatureSection("HOME_FEATURE_DRAGDROP",  "HOME_FEATURE_DRAGDROP_DESC");
			DrawFeatureSection("HOME_FEATURE_SEARCH",    "HOME_FEATURE_SEARCH_DESC");
			DrawFeatureSection("HOME_FEATURE_FAVORITES", "HOME_FEATURE_FAVORITES_DESC");
			DrawFeatureSection("HOME_FEATURE_KITS",      "HOME_FEATURE_KITS_DESC");
		}
		ImGui::EndChild();
	}

	HomeModule::HomeModule()
	{
		m_layouts.push_back({"HOME_TAB_WELCOME",   true,  DrawWelcomeLayout});
		m_layouts.push_back({"HOME_TAB_SHORTCUTS", false, DrawShortcutsLayout});
		m_layouts.push_back({"HOME_TAB_FEATURES",  false, DrawFeaturesLayout});
	}
}
