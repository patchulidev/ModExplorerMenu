#pragma once

#include "UICustom.h"
#include "data/Data.h"
#include "core/Commands.h"
#include "imgui.h"
#include "imgui_internal.h"
#include "localization/FontManager.h"
#include "localization/Locale.h"
#include "config/ThemeConfig.h"
#include "config/UserConfig.h"
#include "ui/components/Item3DPreview.h"
#include "ui/components/UINotification.h"
#include "ui/style/LayoutMetrics.h"
#include "external/framework/DescriptionFrameworkImpl.h"

namespace Modex
{
	// Forward declaration for use in drawOutfitItem.
	inline void ShowItemPreview(const std::unique_ptr<BaseObject>& a_item, bool a_tooltip = false, const std::string& a_moduleId = "");

namespace
{
	inline ImU32 progressColor(const double value, const float max_value)
	{
		const float ratio = std::clamp((float)value / max_value, 0.0f, 1.0f);
		
		float r, g;
		if (ratio < 0.5f) {
			r = 1.0f;
			g = ratio * 2.0f;
		} else {
			r = 1.0f - (ratio - 0.5f) * 2.0f;
			g = 1.0f;
		}

		return ImGui::ColorConvertFloat4ToU32(ImVec4(r, g, 0.0f, 0.25f));
	}

	inline void inlineBarEx(const char* a_label, float value, float max_value)
	{
		const auto& draw_list = ImGui::GetWindowDrawList();
		const float max_width = ImGui::GetContentRegionAvail().x;
		const ImVec2 bar_size = ImVec2(max_width / ThemeConfig::GetWidgetStyle().itemPreview.inlineBarDivisor, ImGui::GetFrameHeight());
		const ImVec2 start = ImGui::GetCursorScreenPos();

		char buffer[256];
		ImFormatString(buffer, IM_ARRAYSIZE(buffer), "%d", static_cast<int>(value));

		// bar
		draw_list->AddRectFilled(
			ImVec2(start.x, start.y),
			ImVec2(start.x + (max_width * (value / max_value)), start.y + bar_size.y),
			progressColor(value, max_value)
		);

		// label
		draw_list->AddText(
			ImVec2(start.x + 4.0f, start.y + (ImGui::GetFrameHeight() - ImGui::GetFontSize())/2.0f),
			ImGui::GetColorU32(ImGuiCol_Text),
			a_label
		);

		// value right-hand side
		draw_list->AddText(
			ImVec2(start.x + max_width - ImGui::CalcTextSize(buffer).x - 4.0f, start.y + (ImGui::GetFrameHeight() - ImGui::GetFontSize())/2.0f),
			ImGui::GetColorU32(ImGuiCol_Text),
			buffer
		);

		ImGui::Dummy(ImVec2(max_width, bar_size.y)); // reserve space for the custom bar
	}

	inline void inlineBarExPair(const char* a_labelA, float a_valueA,
	                            const char* a_labelB, float a_valueB,
	                            float a_max)
	{
		const auto&  draw_list   = ImGui::GetWindowDrawList();
		const float  total_width = ImGui::GetContentRegionAvail().x;
		const float  gap         = ImGui::GetStyle().ItemSpacing.x;
		const float  col_width   = (total_width - gap) * 0.5f;
		const float  bar_h       = ImGui::GetFrameHeight();

		auto drawBar = [&](const char* a_label, float a_value) {
			const ImVec2 start = ImGui::GetCursorScreenPos();

			char buffer[256];
			ImFormatString(buffer, IM_ARRAYSIZE(buffer), "%d", static_cast<int>(a_value));

			draw_list->AddRectFilled(
				ImVec2(start.x, start.y),
				ImVec2(start.x + (col_width * (a_value / a_max)), start.y + bar_h),
				progressColor(a_value, a_max)
			);

			draw_list->AddText(
				ImVec2(start.x + 4.0f, start.y + (bar_h - ImGui::GetFontSize()) / 2.0f),
				ImGui::GetColorU32(ImGuiCol_Text),
				a_label
			);

			draw_list->AddText(
				ImVec2(start.x + col_width - ImGui::CalcTextSize(buffer).x - 4.0f,
				       start.y + (bar_h - ImGui::GetFontSize()) / 2.0f),
				ImGui::GetColorU32(ImGuiCol_Text),
				buffer
			);

			ImGui::Dummy(ImVec2(col_width, bar_h));
		};

		drawBar(a_labelA, a_valueA);
		ImGui::SameLine(0.0f, gap);
		drawBar(a_labelB, a_valueB);
	}

	inline void inlineBar(const std::unique_ptr<BaseObject>& a_item, PropertyType a_property, float max_value)
	{
		const float max_width = ImGui::GetContentRegionAvail().x;
		const std::string  icon    = FilterProperty::GetIcon(a_property);
		const std::string  tooltip = FilterProperty::GetPropertyTooltipKey(a_property);
		const ImVec2 bar_size = ImVec2(max_width / ThemeConfig::GetWidgetStyle().itemPreview.inlineBarMiniDivisor, ImGui::GetFontSize());

		float value = 0;
		char buffer[256];

		// Safely parse float from string.
		const auto target = a_item->GetPropertyByValue(a_property);
		const auto [ptr, ec] = std::from_chars(target.data(), target.data() + target.size(), value);

		if (ec != std::errc()) {
			value = -1;
		} else {
			ImFormatString(buffer, IM_ARRAYSIZE(buffer), "%.2f", value);
		}

		// Icon-only descriptor
		ImGui::Text("%s", icon.c_str());

		if (ImGui::IsItemHovered()) {
			UINotification::ShowTooltip(tooltip.c_str());
		}

		// Right-Align Bar Graphic
		ImGui::SameLine(max_width - bar_size.x - 1.0f);
		ImGui::PushStyleColor(ImGuiCol_PlotHistogram, progressColor(value, max_value));
		ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.0f);  // tight fit
		ImGui::ProgressBar(value / max_value, bar_size, buffer);
		ImGui::PopStyleColor(1);
		ImGui::PopStyleVar(1);
	}

	inline void inlineCheckbox(const std::unique_ptr<BaseObject>& a_item, PropertyType a_property, bool a_useLabel = false)
	{
		const float max_width = ImGui::GetContentRegionAvail().x;
		const std::string  icon    = FilterProperty::GetIcon(a_property);
		const std::string  tooltip = FilterProperty::GetPropertyTooltipKey(a_property);
		const std::string  descriptor = a_useLabel
			? icon + " " + FilterProperty::GetString(a_property)
			: icon;
		bool flag = a_item->GetPropertyByValue(a_property).find("true") == std::string::npos ? false : true;
		const float box_width = ImGui::GetFontSize();
		const float width = (std::max)(max_width - box_width, ImGui::GetContentRegionAvail().x - box_width);

		ImGui::Text("%s", descriptor.c_str());

		if (ImGui::IsItemHovered()) {
			UINotification::ShowTooltip(tooltip.c_str());
		}

		// Right align flag checkbox
		ImGui::SameLine(width - 1.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(ImGui::GetStyle().ItemSpacing.x, 0.f));
		ImGui::Checkbox("##NoLabel", &flag);
		ImGui::PopStyleVar();
	}

	inline void inlineText(const std::unique_ptr<BaseObject>& a_item, PropertyType a_property, bool a_useLabel = false)
	{
		const float max_width = ImGui::GetContentRegionAvail().x;
		const std::string  descriptor = a_useLabel
			? FilterProperty::GetIcon(a_property) + " " + FilterProperty::GetString(a_property)
			: FilterProperty::GetIcon(a_property);
		const std::string  tooltip = FilterProperty::GetPropertyTooltipKey(a_property);
		const std::string& text    = TRUNCATE(a_item->GetPropertyByValue(a_property).c_str(), max_width * 0.75f);
		const float text_width = ImGui::CalcTextSize(text.c_str()).x;
		const float width = (std::max)(max_width - text_width, ImGui::GetContentRegionAvail().x - text_width);

		ImGui::Text("%s", descriptor.c_str());

		if (ImGui::IsItemHovered()) {
			UINotification::ShowTooltip(tooltip.c_str());
		}

		// Right-Align Property's Value
		ImGui::SameLine(width - 1.0f);
		ImGui::Text("%s", text.c_str());
	}

	// Two inlineText cells on a single row, each occupying half the region
	inline void inlineTextPair(const std::unique_ptr<BaseObject>& a_item, PropertyType a_propA, PropertyType a_propB)
	{
		const float  total_width = ImGui::GetContentRegionAvail().x;
		const float  gap         = ImGui::GetStyle().ItemSpacing.x;
		const float  col_width   = (total_width - gap) * 0.5f;
		const ImVec2 start_pos   = ImGui::GetCursorPos();
		const float  col1_x      = start_pos.x;
		const float  col2_x      = col1_x + col_width + gap;

		auto drawCell = [&](PropertyType prop, float col_x) {
			const std::string  icon    = FilterProperty::GetIcon(prop);
			const std::string  tooltip = FilterProperty::GetPropertyTooltipKey(prop);
			const std::string& text    = TRUNCATE(a_item->GetPropertyByValue(prop).c_str(), col_width * 0.75f);
			const float        text_w  = ImGui::CalcTextSize(text.c_str()).x;

			ImGui::SetCursorPosX(col_x);
			// Icon-only descriptor
			ImGui::Text("%s", icon.c_str());

			if (ImGui::IsItemHovered()) {
				UINotification::ShowTooltip(tooltip.c_str());
			}

			// Right-align value within this column.
			ImGui::SameLine(col_x + col_width - text_w - 1.0f);
			ImGui::Text("%s", text.c_str());
		};

		drawCell(a_propA, col1_x);
		ImGui::SetCursorPos(ImVec2(col2_x, start_pos.y));
		drawCell(a_propB, col2_x);
	}

	// Render a list of strings as two columns, each cell = icon + value
	inline void inlineListPair(const std::vector<std::string>& a_items, const char* a_icon, const char* a_tooltip)
	{
		const float  total_width = ImGui::GetContentRegionAvail().x;
		const float  gap         = ImGui::GetStyle().ItemSpacing.x;
		const float  col_width   = (total_width - gap) * 0.5f;
		const float  col1_x      = ImGui::GetCursorPosX();
		const float  col2_x      = col1_x + col_width + gap;

		auto drawCell = [&](const std::string& value, float col_x) {
			const std::string& text   = TRUNCATE(value.c_str(), col_width * 0.75f);
			const float        text_w = ImGui::CalcTextSize(text.c_str()).x;

			ImGui::SetCursorPosX(col_x);
			ImGui::Text("%s", a_icon);

			if (ImGui::IsItemHovered()) {
				UINotification::ShowTooltip(a_tooltip);
			}

			ImGui::SameLine(col_x + col_width - text_w - 1.0f);
			ImGui::Text("%s", text.c_str());
		};

		std::vector<const std::string*> entries;
		entries.reserve(a_items.size());
		for (const auto& v : a_items) {
			if (!v.empty()) entries.push_back(&v);
		}

		for (size_t i = 0; i < entries.size(); i += 2) {
			const ImVec2 row_start = ImGui::GetCursorPos();
			drawCell(*entries[i], col1_x);

			if (i + 1 < entries.size()) {
				ImGui::SetCursorPos(ImVec2(col2_x, row_start.y));
				drawCell(*entries[i + 1], col2_x);
			}
		}
	}

	inline void inlineTextEx(const char* a_left, const char* a_right, const char* a_tooltip)
	{
		const float max_width = ImGui::GetContentRegionAvail().x;
		const float text_width = ImGui::CalcTextSize(a_right).x;
		const float width = (std::max)(max_width - text_width, ImGui::GetContentRegionAvail().x - text_width);

		// Left-hand Side
		ImGui::Text("%s", a_left);

		if (ImGui::IsItemHovered()) {
			UINotification::ShowTooltip(a_tooltip);
		}

		// Right-hand Side
		ImGui::SameLine(width - 1.0f);
		ImGui::Text("%s", a_right);
	}

	inline void drawBasePreview(const std::unique_ptr<BaseObject>& a_object)
	{
		ImGui::SeparatorText(Translate("INFO"));
		inlineText(a_object, PropertyType::kFormID,   true);
		inlineText(a_object, PropertyType::kPlugin,   true);
		inlineText(a_object, PropertyType::kEditorID, true);
	}

	inline void drawLoadOrder(const std::unique_ptr<BaseObject>& a_object)
	{
		ImGui::SeparatorText(Translate("ORDER"));

		// Populate list of plugins sorted by compileIdx
		if (const auto item_file = a_object->GetFile(UserConfig::GetCompileIndex()); item_file.has_value()) {
			for (auto master : item_file.value()->masters) {
				ImGui::TextDisabled("%s %s", TranslateFormat("MASTER", ":"), master);
			}
		}
		if (const auto source_files_opt = a_object->GetFileArray(); source_files_opt.has_value()) {
			if (const auto source_files = source_files_opt.value(); source_files != nullptr) {
				for (uint32_t i = 0; i < source_files->size(); i++) {
					if (const auto file = (*source_files)[i]) {
						const std::string fileName = file->GetFilename().data();

						ImGui::Text("%d: %s", i, fileName.c_str());
					}
				}
			}
		}
	}

	inline void drawDescriptionOverlay(const std::unique_ptr<BaseObject>& a_object, ImVec2 a_anchor, ImVec2 a_anchorSize)
	{
		const std::string desc = DescriptionFramework_Impl::GetItemDescription(a_object->GetTESForm());
		if (desc.empty() || a_object->GetFormType() == RE::FormType::Book) return;

		auto*       dl        = ImGui::GetWindowDrawList();
		auto*       font      = ImGui::GetFont();
		const float font_size = ImGui::GetFontSize();
		const float line_h    = ImGui::GetTextLineHeight();
		const ImVec2 pad      = ImGui::GetStyle().FramePadding;

		const float card_w = a_anchorSize.x * 0.85f;
		const float wrap_w = card_w - pad.x * 2.0f;

		// Word-wrap the description into [line_start, line_end) pairs,
		// honouring explicit \n. We render each line individually so we
		// can centre them, which ImGui::TextWrapped can't do natively.
		const char* text_end   = desc.c_str() + desc.size();
		const char* line_start = desc.c_str();
		std::vector<std::pair<const char*, const char*>> lines;
		while (line_start < text_end) {
			const char* line_end = font->CalcWordWrapPosition(font_size, line_start, text_end, wrap_w);
			if (line_end == line_start) line_end = line_start + 1;
			for (const char* p = line_start; p < line_end; ++p) {
				if (*p == '\n') { line_end = p; break; }
			}
			lines.emplace_back(line_start, line_end);
			line_start = line_end;
			while (line_start < text_end && (*line_start == ' ' || *line_start == '\n')) ++line_start;
		}

		const float text_h = line_h * static_cast<float>(lines.size());
		const float card_h = text_h + pad.y * 2.0f;

		// Anchor near the bottom of the supplied rect with a small margin.
		const float card_x = a_anchor.x + (a_anchorSize.x - card_w) * 0.5f;
		const float card_y = a_anchor.y + a_anchorSize.y - card_h - pad.y;

		const float alpha   = ImGui::GetStyle().Alpha;
		const ImU32 bg      = ThemeConfig::GetColorU32("BG",     alpha);
		const ImU32 border  = ThemeConfig::GetColorU32("BORDER", alpha);
		const ImU32 textCol = ImGui::GetColorU32(ImGuiCol_Text,  alpha);

		dl->AddRectFilled(ImVec2(card_x, card_y), ImVec2(card_x + card_w, card_y + card_h), bg);
		dl->AddRect      (ImVec2(card_x, card_y), ImVec2(card_x + card_w, card_y + card_h), border);

		float ty = card_y + pad.y;
		for (const auto& [s, e] : lines) {
			const ImVec2 line_sz = ImGui::CalcTextSize(s, e);
			const float  tx      = card_x + (card_w - line_sz.x) * 0.5f;
			dl->AddText(ImVec2(tx, ty), textCol, s, e);
			ty += line_h;
		}
	}

	inline void drawDebugInfo(const std::unique_ptr<BaseObject>& a_object)
	{
		if (!UserConfig::Get().developerMode) return;
		inlineTextEx("TableID", std::to_string(a_object->m_tableID).c_str(), "");
	}

	inline void drawFooter(const std::unique_ptr<BaseObject>& a_object, bool a_tooltip)
	{
		if (a_tooltip) return;

		// Item/Object Keywords — two-column layout to save vertical space.
		if (const auto keywords = a_object->GetKeywordList(); !keywords.empty()) {
			const auto tooltip = FilterProperty::GetPropertyTooltipKey(PropertyType::kKeyword);
			const auto icon    = FilterProperty::GetIcon(PropertyType::kKeyword);

			ImGui::SeparatorText(Translate("KEYWORDS"));
			ImGui::PushID("ItemPreview::Keywords");
			inlineListPair(keywords, icon.c_str(), tooltip.c_str());
			ImGui::PopID();
		}

		// Dummy Object's don't contain valid form pointers, stop here.
		if (a_object->IsDummy()) return;

		drawDebugInfo(a_object);
		drawLoadOrder(a_object);
	}

	inline void drawObjectPreview(const std::unique_ptr<BaseObject>& a_object)
	{
		inlineCheckbox(a_object, PropertyType::kPersistent);
		inlineCheckbox(a_object, PropertyType::kDeleted);
	}

	inline void drawActorPreview(const std::unique_ptr<BaseObject>& a_npc, bool a_tooltip)
	{
		// Vitals + level — pair related stats two per row to match the
		// weapon/armor preview density.
		inlineTextPair(a_npc, PropertyType::kHealth,  PropertyType::kMagicka);
		inlineTextPair(a_npc, PropertyType::kStamina, PropertyType::kLevel);

		// Identity
		inlineTextPair(a_npc, PropertyType::kRace,   PropertyType::kClass);
		inlineText    (a_npc, PropertyType::kGender);

		// Outfits
		inlineTextPair(a_npc, PropertyType::kDefaultOutfit, PropertyType::kSleepOutfit);

		if (a_tooltip) return;

		// Skills — two columns to halve the 18-row block.
		if (const auto skills = a_npc->GetSkills(); skills.has_value()) {
			ImGui::SeparatorText(Translate("SKILLS"));
			ImGui::PushID("ItemPreview::Skills");

			constexpr auto kOffset = static_cast<uint8_t>(RE::ActorValue::kOneHanded);
			constexpr auto kTotal  = static_cast<size_t>(RE::TESNPC::Skills::Skills::kTotal);
			const auto&    values  = skills.value().values;

			size_t i = 0;
			for (; i + 1 < kTotal; i += 2) {
				const auto nameA = magic_enum::enum_name(static_cast<RE::ActorValue>(kOffset + i));
				const auto nameB = magic_enum::enum_name(static_cast<RE::ActorValue>(kOffset + i + 1));
				inlineBarExPair(nameA.data(), values[i], nameB.data(), values[i + 1], 100.0f);
			}
			if (i < kTotal) {
				const auto name = magic_enum::enum_name(static_cast<RE::ActorValue>(kOffset + i));
				inlineBarEx(name.data(), values[i], 100.0f);
			}
			ImGui::PopID();
		}

		// Spells — two-column list to match KEYWORDS / SLOTS density.
		if (const auto spells = a_npc->GetSpellList(); !spells.empty()) {
			const auto tooltip = FilterProperty::GetPropertyTooltipKey(PropertyType::kSpell);
			const auto icon    = FilterProperty::GetIcon(PropertyType::kSpell);

			ImGui::SeparatorText(Translate("SPELLS"));
			ImGui::PushID("ItemPreview::Spells");
			inlineListPair(spells, icon.c_str(), tooltip.c_str());
			ImGui::PopID();
		}

		// Factions — two-column list.
		if (const auto factions = a_npc->GetFactionList(); !factions.empty()) {
			const auto tooltip = FilterProperty::GetPropertyTooltipKey(PropertyType::kFaction);
			const auto icon    = FilterProperty::GetIcon(PropertyType::kFaction);

			ImGui::SeparatorText(Translate("kFactionList"));
			ImGui::PushID("ItemPreview::Factions");
			inlineListPair(factions, icon.c_str(), tooltip.c_str());
			ImGui::PopID();
		}
	}

	inline void drawWeaponPreview(const std::unique_ptr<BaseObject>& a_weapon)
	{
		// Staves are governed by the spell socketed in them — speed, DPS,
		// crit, range, and stagger have no meaning, so we skip those rows.
		const bool isStaff = a_weapon->HasKeyword("WeapTypeStaff");

		if (isStaff) {
			inlineText(a_weapon, PropertyType::kWeaponDamage);
		} else {
			inlineTextPair(a_weapon, PropertyType::kWeaponDamage, PropertyType::kWeaponDamagePerSecond);
			inlineTextPair(a_weapon, PropertyType::kWeaponSpeed,  PropertyType::kWeaponCriticalDamage);
			inlineTextPair(a_weapon, PropertyType::kWeaponRange,  PropertyType::kWeaponStagger);
		}

		inlineTextPair(a_weapon, PropertyType::kWeaponType,    PropertyType::kWeaponSkill);
		inlineTextPair(a_weapon, PropertyType::kCarryWeight,   PropertyType::kGoldValue);
	}

	inline void drawArmorPreview(const std::unique_ptr<BaseObject>& a_armor, bool a_tooltip)
	{
		inlineTextPair(a_armor, PropertyType::kArmorRating, PropertyType::kArmorType);
		inlineTextPair(a_armor, PropertyType::kCarryWeight, PropertyType::kGoldValue);

		const auto equip_slots = a_armor->GetArmorSlots();

		if (!a_tooltip && !equip_slots.empty()) {
			const auto tooltip = FilterProperty::GetPropertyTooltipKey(PropertyType::kArmorSlot);
			const auto icon    = FilterProperty::GetIcon(PropertyType::kArmorSlot);

			ImGui::SeparatorText(Translate("SLOTS"));
			ImGui::PushID("ItemPreview::ArmorSlots");
			inlineListPair(equip_slots, icon.c_str(), tooltip.c_str());
			ImGui::PopID();
		}
	}

	// Render a single concrete item as a selectable row in the outfit preview.
	inline void drawOutfitItem(int a_index, RE::TESForm* a_form, uint16_t a_level)
	{
		const auto displayObject = std::make_unique<BaseObject>(a_form, Ownership::Outfit);
		const auto& draw_list = ImGui::GetWindowDrawList();
		const float pillar_width = Style::Metrics().pillarWidth;

		std::string icon = displayObject->GetItemIcon();
		std::string formid = std::format("{:08X}", a_form->GetFormID());
		std::string level = std::format("[Lv{}]", a_level); 
		std::string text = std::format("{} {}", icon.c_str(), displayObject->GetEditorID().c_str());

		ImGui::PushID(a_index);
		ImGui::PushStyleVar(ImGuiStyleVar_SelectableTextAlign, ImVec2(0.0f, 0.5f));
		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetStyle().WindowPadding.x + pillar_width);
		if (ImGui::Selectable(text.c_str(), false, ImGuiSelectableFlags_SpanAvailWidth)) {
			const auto player = RE::PlayerCharacter::GetSingleton();
			if (player) {
				if (auto playerRef = player->AsReference(); playerRef != nullptr) {
					Commands::AddItemToRefInventory(Ownership::Item, playerRef, displayObject->GetBaseFormID(), 1);
				}
			}
		}
		ImGui::PopStyleVar();

		// Type Pillar
		const ImRect bb(ImGui::GetItemRectMin() - ImVec2(pillar_width, 0.0f), ImGui::GetItemRectMax()); 
		draw_list->AddRectFilled(
			ImVec2(bb.Min.x, bb.Min.y),
			ImVec2(bb.Min.x + pillar_width, bb.Max.y),
			UICustom::GetFormTypeColor(a_form->GetFormType())
		);

		if (ImGui::IsItemHovered(ImGuiHoveredFlags_NoSharedDelay | ImGuiHoveredFlags_DelayShort)) {
			ImGui::BeginTooltip();
			ShowItemPreview(displayObject, true);
			ImGui::EndTooltip();
		}

		if (a_level != 0) {
			ImGui::SameLine();
			ImGui::SetNextItemAllowOverlap();
			ImGui::TextDisabled("[Lv%d]", a_level);
		}


		ImGui::SameLine();
		ImGui::SetNextItemAllowOverlap();
		ImGui::TextDisabled("[%s]", formid.c_str());
		ImGui::PopID();
	}

	// Recursively render a LeveledList as a collapsing tree in the outfit preview.
	inline void drawLeveledListTree(int a_index, RE::TESForm* a_form, RE::TESLeveledList* a_list)
	{
		ImGui::PushID(a_index);
		auto label = std::format("{} {}",
			ICON_LC_LIST, po3_GetEditorID(a_form->GetFormID()));

		auto showTreeNodePreview = [&a_form]() {
			if (ImGui::IsItemHovered(ImGuiHoveredFlags_NoSharedDelay | ImGuiHoveredFlags_DelayShort)) {
				const auto displayObject = std::make_unique<BaseObject>(a_form, Ownership::Outfit);

				ImGui::BeginTooltip();
				ShowItemPreview(displayObject, true);
				ImGui::EndTooltip();
			}
		};

		auto renderTypePillar = [&a_form]() {
			const auto& draw_list = ImGui::GetWindowDrawList();
			const float pillar_width = Style::Metrics().pillarWidth;
			const ImRect bb(ImGui::GetItemRectMin() - ImVec2(pillar_width, 0.0f), ImGui::GetItemRectMax()); 
			draw_list->AddRectFilled(
				ImVec2(bb.Min.x, bb.Min.y),
				ImVec2(bb.Min.x + pillar_width, bb.Max.y),
				UICustom::GetFormTypeColor(a_form->GetFormType())
			);
		};

		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + Style::Metrics().pillarWidth);
		if (ImGui::TreeNode(label.c_str())) {
			renderTypePillar();
			showTreeNodePreview();
			Commands::ForEachLeveledEntry(a_list,
				[](int a_index, RE::TESForm* a_item, [[maybe_unused]] uint16_t a_level, [[maybe_unused]] std::int32_t a_count) {
					drawOutfitItem(a_index, a_item, a_level);
				},
				[](int a_index, RE::TESForm* a_nestedForm, RE::TESLeveledList* a_nested) {
					drawLeveledListTree(a_index, a_nestedForm, a_nested);
				}
			);
			ImGui::TreePop();
		} else {
			renderTypePillar();
			showTreeNodePreview();
		}

		ImGui::SameLine();
		ImGui::TextDisabled("[%zu]", a_list->entries.size());

		ImGui::PopID();
	}

	inline void drawLeveledListPreview(const std::unique_ptr<BaseObject>& a_list)
	{
		// Shown via tooltip context where hovering the icon for its label
		// isn't viable — all four flag icons are identical, so render the
		// property name alongside each.
		inlineCheckbox(a_list, PropertyType::kLeveledAllLevelsFlag, true);
		inlineCheckbox(a_list, PropertyType::kLeveledEachFlag,      true);
		inlineCheckbox(a_list, PropertyType::kLeveledUseAllFlag,    true);
		inlineCheckbox(a_list, PropertyType::kLeveledSpecialFlag,   true);

		const auto chanceLabel = FilterProperty::GetIcon(PropertyType::kLeveledChance) + " " +
		                         FilterProperty::GetString(PropertyType::kLeveledChance);
		const auto chanceValue = a_list->GetPropertyByValue(PropertyType::kLeveledChance);
		const auto chanceTip   = FilterProperty::GetPropertyTooltipKey(PropertyType::kLeveledChance);
		inlineTextEx(chanceLabel.c_str(), chanceValue.c_str(), chanceTip.c_str());
	}

	inline void drawOutfitPreview(const std::unique_ptr<BaseObject>& a_item)
	{
		ImGui::SeparatorText(Translate("HEADER_OUTFIT_ITEMS"));

		if (ImGui::IsItemHovered(ImGuiHoveredFlags_NoSharedDelay | ImGuiHoveredFlags_DelayShort)) {
			UICustom::FancyTooltip(Translate("HEADER_OUTFIT_TOOLTIP"));
		}

		// Show the item list of the selected outfit
		if (a_item) {
			if (auto outfit = a_item->GetTESForm()->As<RE::BGSOutfit>()) {

				int index = 0;
				outfit->ForEachItem([&index](RE::TESForm* a_item) {
					index++;
					if (!a_item) return RE::BSContainer::ForEachResult::kContinue;

					if (a_item->GetFormType() == RE::FormType::LeveledItem) {
						if (auto leveledList = a_item->As<RE::TESLeveledList>()) {
							drawLeveledListTree(index, a_item, leveledList);
						}
					} else {
						drawOutfitItem(index, a_item, 0);
					}

					return RE::BSContainer::ForEachResult::kContinue;
				});
			}
		}
	}

	inline void drawCellPreview(const std::unique_ptr<BaseObject>& a_item)
	{
		inlineText(a_item, PropertyType::kName);
		inlineText(a_item, PropertyType::kEditorID);
	}

	inline void drawSpellPreview(const std::unique_ptr<BaseObject>& a_spell)
	{
		inlineText(a_spell, PropertyType::kSpellType,     true);
		inlineText(a_spell, PropertyType::kSpellSkill,    true);
		inlineText(a_spell, PropertyType::kSpellCastType, true);
		inlineText(a_spell, PropertyType::kSpellDelivery, true);
		inlineText(a_spell, PropertyType::kSpellCost,     true);
	}

	inline float getDesiredWidth(const std::unique_ptr<BaseObject>& a_item, float a_min)
	{
		const auto& edid = a_item->GetEditorID();
		const auto& plugin = a_item->GetPluginName();

		const bool  use_plugin = plugin.length() > edid.length();
		const float desc = ImGui::CalcTextSize(FilterProperty::GetString(PropertyType::kEditorID).c_str()).x;
		const float padding = ImGui::GetFontSize() * ThemeConfig::GetWidgetStyle().itemPreview.desiredWidthPadFont;
		const float text_width = use_plugin ? ImGui::CalcTextSize(plugin.c_str()).x : ImGui::CalcTextSize(edid.c_str()).x;

		return max(padding + desc + text_width, a_min);
	}

	// Empty moduleId routes to the shared theme defaults (e.g. tooltip-context previews).
	inline float getPreviewSetting(const std::string& a_moduleId, const std::string& a_key, float a_themeDefault)
	{
		if (a_moduleId.empty()) return a_themeDefault;
		return UserData::Get<float>(a_moduleId + "::ItemPreview::" + a_key, a_themeDefault);
	}

	inline void setPreviewSetting(const std::string& a_moduleId, const std::string& a_key, float a_value)
	{
		if (a_moduleId.empty()) return;
		UserData::Set<float>(a_moduleId + "::ItemPreview::" + a_key, a_value);
	}

	inline void drawModelPreview(const std::unique_ptr<BaseObject>& a_item, bool /*a_tooltip*/, const std::string& a_moduleId)
	{
		const bool atMainMenu     = RE::UI::GetSingleton()->IsMenuOpen(RE::MainMenu::MENU_NAME);
		const bool previewEnabled = UserConfig::Get().show3DPreview && UserConfig::Get().pauseGame;
		if (!previewEnabled) return;

		const auto& preview   = ThemeConfig::GetWidgetStyle().itemPreview;
		const float max_width = ImGui::GetContentRegionAvail().x;

		// Per-module overrides, falling back to theme defaults.
		const float boxScale    = getPreviewSetting(a_moduleId, "BoxScale",    preview.previewBoxScale);
		const float modelScale  = getPreviewSetting(a_moduleId, "ModelScale",  preview.previewModelScale);
		const float offsetX     = getPreviewSetting(a_moduleId, "OffsetX",     preview.previewOffsetX);
		const float offsetY     = getPreviewSetting(a_moduleId, "OffsetY",     preview.previewOffsetY);

		RE::TESBoundObject* previewObj    = nullptr;
		float               previewHeight = 0.0f;
		bool                showMenuHint  = false;

		// TODO: Determine why main menu positioning is offset so poorly.

		if (atMainMenu) {
			showMenuHint  = true;
			previewHeight = ImGui::GetTextLineHeightWithSpacing() * 2.0f;
		} else if (auto* form = a_item->GetTESForm()) {
			if (auto* boundObj = form->As<RE::TESBoundObject>()) {
				previewObj    = boundObj;
				previewHeight = max_width * boxScale;
			}
		}

		if (!previewObj && !showMenuHint) return;

		const ImVec2 size(max_width, previewHeight);

		// Visual divider between the name bar and the preview slot.
		ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal);

		// Helper: reserve a `size`-tall layout slot at the cursor and
		// draw a centred string inside it via the raw draw list.
		auto centredFallback = [&](const char* hint) {
			const ImVec2 cur = ImGui::GetCursorScreenPos();
			const ImVec2 ts  = ImGui::CalcTextSize(hint);
			const float  tx  = cur.x + (size.x - ts.x) * 0.5f;
			const float  ty  = cur.y + (size.y - ts.y) * 0.5f;
			ImGui::Dummy(size);
			ImGui::GetWindowDrawList()->AddText(ImVec2(tx, ty),
				ImGui::GetColorU32(ImGuiCol_TextDisabled), hint);
		};

		if (showMenuHint) {
			centredFallback(Translate("ITEM_PREVIEW_MAIN_MENU_HINT"));
			return;
		}

		// Invalid/No Model placeholder
		if (!Item3DPreview::HasValidModel(previewObj)) {
			centredFallback(Translate("ITEM_PREVIEW_NO_MODEL"));
			return;
		}

		auto* preview3D = Item3DPreview::GetSingleton();
		const ImVec2 image_pos = ImGui::GetCursorScreenPos();
		preview3D->Request(previewObj, image_pos, size, modelScale, offsetX, offsetY);

		void*        srv     = preview3D->GetSRV();
		const ImVec2 capSize = preview3D->GetCapturedSize();
		if (srv && capSize.x > 0.0f && capSize.y > 0.0f) {
			ImVec2 uv0, uv1;
			preview3D->GetDisplayUV(uv0, uv1);
			const float  a = ImGui::GetStyle().Alpha;
			const ImVec4 tint(1.0f, 1.0f, 1.0f, a);
			ImGui::ImageWithBg(static_cast<ImTextureID>(reinterpret_cast<intptr_t>(srv)),
				size, uv0, uv1,
				ImVec4(0.0f, 0.0f, 0.0f, 0.0f), tint);

			drawDescriptionOverlay(a_item, image_pos, size);
		} else {
			ImGui::Dummy(size);
		}

		// Settings cog overlay — top-left of the preview pane. Opens a popup
		// with the theme tokens that directly drive the preview's visual size.
		// Autosaves on slider release; no explicit Save button.
		{
			constexpr const char* kPopupId = "##PreviewSettingsPopup";

			const ImVec2 cursorAfter = ImGui::GetCursorScreenPos();
			const float  iconSize    = ImGui::GetFontSize() * 1.3f;
			const float  pad         = ImGui::GetStyle().FramePadding.x;

			ImGui::SetCursorScreenPos(ImVec2(image_pos.x + pad,
			                                 image_pos.y + pad));
			ImGui::PushID("##PreviewSettingsCog");

			// Frameless icon-only button: zero frame padding so the clickable
			// area matches the glyph; transparent normal/hover/active colors
			// so only the white cog shows. The hover/active overlays are kept
			// faint so the user still gets affordance feedback.
			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0.0f, 0.0f));
			ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.0f, 1.0f, 1.0f, 0.10f));
			ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(1.0f, 1.0f, 1.0f, 0.20f));
			ImGui::PushFontRegular(iconSize);
			const bool clicked = ImGui::Button(ICON_LC_SETTINGS);
			ImGui::PopFont();
			ImGui::PopStyleColor(3);
			ImGui::PopStyleVar();

			if (clicked) {
				ImGui::OpenPopup(kPopupId);
			}

			if (ImGui::BeginPopup(kPopupId)) {
				ImGui::TextDisabled("%s", Translate("THEME_EDITOR_SECT_ITEM_PREVIEW"));
				ImGui::Separator();

				constexpr float kSliderWidth = 180.0f;

				// UserData writes are in-memory; theme fallback flushes to disk on deactivate
				// to avoid spamming I/O.
				auto sliderRow = [&](const char* a_label, const char* a_key, float a_curValue,
				                     float a_min, float a_max, const char* a_fmt,
				                     float& a_themeField) {
					float v = a_curValue;
					ImGui::SetNextItemWidth(kSliderWidth);
					if (ImGui::SliderFloat(a_label, &v, a_min, a_max, a_fmt)) {
						if (a_moduleId.empty()) {
							a_themeField = v;
						} else {
							setPreviewSetting(a_moduleId, a_key, v);
						}
					}
					if (a_moduleId.empty() && ImGui::IsItemDeactivatedAfterEdit()) {
						ThemeConfig::GetSingleton()->SaveCurrentTheme();
					}
				};

				auto& w = ThemeConfig::GetWidgetStyleMutable();
				sliderRow(Translate("THEME_EDITOR_LBL_PREVIEW_BOX_SCALE"),   "BoxScale",   boxScale,    0.10f,    1.0f, "%.2f",     w.itemPreview.previewBoxScale);
				sliderRow(Translate("THEME_EDITOR_LBL_PREVIEW_MODEL_SCALE"), "ModelScale", modelScale,  0.25f,    1.0f, "%.2f",     w.itemPreview.previewModelScale);
				sliderRow(Translate("THEME_EDITOR_LBL_PREVIEW_OFFSET_X"),    "OffsetX",    offsetX,    -300.0f, 300.0f, "%.0f px",  w.itemPreview.previewOffsetX);
				sliderRow(Translate("THEME_EDITOR_LBL_PREVIEW_OFFSET_Y"),    "OffsetY",    offsetY,    -300.0f, 300.0f, "%.0f px",  w.itemPreview.previewOffsetY);

				ImGui::EndPopup();
			}
			ImGui::PopID();
			ImGui::SetCursorScreenPos(cursorAfter);
		}
	}
}

	inline void ShowFavoriteList()
	{
		ImGui::Spacing();
		UICustom::SubCategoryHeader(Translate("SHOWFAVORITE"));
		ImGui::Spacing();

		ImGui::PushStyleVar(ImGuiStyleVar_SelectableTextAlign, ImVec2(0.f, 0.5f));
		if (ImGui::BeginChild("##Modex::Favorite::List", ImVec2(0.f, 0.f), 0, 0)) {
			const auto& favorites = UserData::GetFavoritesAsVector();
			auto temp = std::vector<std::unique_ptr<BaseObject>>();

			// NOTE: Exterior cells that lack a FULL record likely aren't preloaded, and won't be
			// picked up by LookupByEditorID. So we create them with serialized favorited data.

			for (const auto& favoriteItem : favorites) {
				if (favoriteItem.owner == Ownership::Cell) {
					if (RE::TESForm* form = RE::TESForm::LookupByEditorID(favoriteItem.editorid); form != nullptr) {
						temp.emplace_back(std::make_unique<BaseObject>(form->GetName(), favoriteItem.editorid, favoriteItem.plugin, Ownership::Cell));
					} else {
						temp.emplace_back(std::make_unique<BaseObject>("", favoriteItem.editorid, favoriteItem.plugin, Ownership::Cell));
					}
				}
			}

			// OPTIMIZE: Should maybe store and maintain this list instead of reconstructing it
			// every frame. This is a semi-common pattern found in Modex that could be improved.

			for (const auto& favoriteItem : temp) {
				if (ImGui::Selectable(("##" + favoriteItem->GetEditorID()).c_str(), false, ImGuiSelectableFlags_SpanAvailWidth)) {
					Commands::CenterOnCell(Ownership::Cell, favoriteItem->GetEditorID());
				}	

				ImGui::SetNextItemAllowOverlap();
				ImGui::SameLine();

				ImGui::Text("%s", favoriteItem->GetEditorID().c_str());
				ImGui::SameLine();
				ImGui::TextDisabled("%s", favoriteItem->GetName().c_str());
			}
		}
		ImGui::EndChild();
		ImGui::PopStyleVar();
	}

	// @arg a_tooltip: the item preview is shown in a tooltip instead of a widget.
	// @arg a_moduleId: scopes preview scale/offset via UserData; empty falls back to theme.
	inline void ShowItemPreview(const std::unique_ptr<BaseObject>& a_item, bool a_tooltip, const std::string& a_moduleId)
	{
		if (a_item == nullptr) return;
		if (a_item->IsDummy()) return;

		const auto& preview = ThemeConfig::GetWidgetStyle().itemPreview;
		const float tooltip_width = getDesiredWidth(a_item, preview.minTooltipWidth);
		const float max_width = a_tooltip ? tooltip_width : ImGui::GetContentRegionAvail().x;
		const auto cursor = ImGui::GetCursorScreenPos();
		const float alpha = ImGui::GetStyle().Alpha;
		const float font_size = ImGui::GetFontSize();
		const auto& draw_list = ImGui::GetWindowDrawList();
		const float name_bar_h = font_size * preview.nameBarHeightScale;

		{ // Name Bar — text stays vertically centered regardless of bar height.
			auto name = TRUNCATE(a_item->GetName(), max_width * Style::Ratio::TruncateNameWide());
			const auto text_color = a_item->IsEnchanted() ? ThemeConfig::GetColor("TEXT_ENCHANTED") : ThemeConfig::GetColor("TEXT");

			draw_list->AddRectFilled(cursor, ImVec2(cursor.x + max_width, cursor.y + name_bar_h), ThemeConfig::GetColorU32("BG", alpha));
			draw_list->AddRect    (cursor, ImVec2(cursor.x + max_width, cursor.y + name_bar_h), ThemeConfig::GetColorU32("BORDER", alpha));

			// Capture the local cursor at the bar's top so we can both center
			// the text inside the bar and reserve the full bar height for the
			// content that follows.
			const float start_local_y = ImGui::GetCursorPosY();
			ImGui::SetCursorPosX(UICustom::GetCenterTextPosX(name.data()));
			ImGui::SetCursorPosY(start_local_y + (name_bar_h - font_size) * 0.5f);
			ImGui::TextColored(text_color, "%s", name.data());
			ImGui::SetCursorPosY(start_local_y + name_bar_h);
		}

		{ // Window Adjustment Hack
			ImGui::SetNextItemAllowOverlap();
			ImGui::Dummy(ImVec2(max_width, 0));
		}

		{
			ImGui::PushStyleColor(ImGuiCol_Separator, ThemeConfig::GetColorU32("PRIMARY"));

			if (a_item->GetTESForm() && (a_item->GetTESForm()->IsInventoryObject() || a_item->IsObject() || a_item->GetTESForm()->As<RE::SpellItem>())) {
				drawModelPreview(a_item, a_tooltip, a_moduleId);
			}

			const bool useScroll = !a_tooltip;
			if (useScroll) {
				ImGui::BeginChild("##ItemPreview::Info", ImVec2(0, 0), false,
				                  ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar);
			}
			
			drawBasePreview(a_item);

			if (a_item->GetTESNPC()) {
				drawActorPreview(a_item, a_tooltip);
			}

			if (a_item->IsObject()) {
				drawObjectPreview(a_item);
			}

			if (a_item->GetTESArmor()) {
				drawArmorPreview(a_item, a_tooltip);
			}

			if (a_item->GetTESWeapon()) {
				drawWeaponPreview(a_item);
			}

			if (a_item->GetTESOutfit()) {
				drawOutfitPreview(a_item);
			}

			if (a_item->GetFormType() == RE::FormType::Cell) {
				drawCellPreview(a_item);
			}

			if (a_item->GetTESForm() && a_item->GetTESForm()->As<RE::SpellItem>()) {
				drawSpellPreview(a_item);
			}

			if (auto form = a_item->GetTESForm(); form) {
				if (auto leveled = form->As<RE::TESLeveledList>(); leveled) {
					drawLeveledListPreview(a_item);
				}
			}

			drawFooter(a_item, a_tooltip);

			if (useScroll) {
				ImGui::EndChild();
			}

			ImGui::PopStyleColor();
		}

		// Draw FormType color gradient over Name container.
		const float height = ImGui::GetFrameHeight() * preview.gradientHeightScale;

		const ImVec2 start = a_tooltip ?
			ImGui::GetWindowPos() :
			cursor;	

		const ImVec2 end = a_tooltip ?
			ImVec2(start.x + ImGui::GetWindowWidth(), start.y + height) :
			ImVec2(cursor.x + max_width, cursor.y + height);

		ImGui::PushStyleVar(ImGuiStyleVar_Alpha, alpha * 0.2f);
		const ImU32 color = UICustom::GetFormTypeColor(a_item->GetFormType());
		const ImU32 empty = ImGui::GetColorU32(ImVec4(0, 0, 0, 0));
		ImGui::PopStyleVar();
		
		draw_list->AddRectFilledMultiColor(
				start,
				end,
				color,
				color,
				empty,
				empty
		);
	}

}
