#pragma once

#include "UICustom.h"
#include "data/Data.h"
#include "localization/Locale.h"
#include "config/ThemeConfig.h"
#include "ui/components/UINotification.h"
#include "external/framework/DescriptionFrameworkImpl.h"

namespace Modex
{

namespace 
{
	inline ImVec4 progressColor(const double value, const float max_value)
	{
		static const float dampen = 0.7f;
		const float ratio = (float)value / max_value;
		const float r = 1.0f - ratio * dampen;
		const float g = ratio * dampen;
		return ImVec4(r, g, 0.0f, 0.67f);
	}

	inline void inlineBar(const std::unique_ptr<BaseObject>& a_item, PropertyType a_property, float max_value)
	{
		const float max_width = ImGui::GetContentRegionAvail().x;
		const std::string icon = a_item->GetPropertyTypeWithIcon(a_property);
		const std::string tooltip = FilterProperty::GetPropertyTooltipKey(a_property);
		const ImVec2 bar_size = ImVec2(max_width / 4.0f, ImGui::GetFontSize());

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

		// Icon + Property Type
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

	inline void inlineText(const std::unique_ptr<BaseObject>& a_item, PropertyType a_property)
	{
		const float max_width = ImGui::GetContentRegionAvail().x;
		const std::string& icon = a_item->GetPropertyTypeWithIcon(a_property);
		const std::string& tooltip = FilterProperty::GetPropertyTooltipKey(a_property);
		const std::string& text = TRUNCATE(a_item->GetPropertyByValue(a_property).c_str(), max_width * 0.60f);
		const float text_width = ImGui::CalcTextSize(text.c_str()).x;
		const float width = (std::max)(max_width - text_width, ImGui::GetContentRegionAvail().x - text_width);

		// Icon + Property Type
		ImGui::Text("%s", icon.c_str());

		if (ImGui::IsItemHovered()) {
			UINotification::ShowTooltip(tooltip.c_str());
		}

		// Right-Align Property's Value
		ImGui::SameLine(width - 1.0f);
		ImGui::Text("%s", text.c_str());
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
		ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal);
		ImGui::Spacing();
		inlineText(a_object, PropertyType::kEditorID);
	}

	inline void drawLoadOrder(const std::unique_ptr<BaseObject>& a_object)
	{
		ImGui::Spacing();
		ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal);

		// Load Order Header with Tooltip
		ImGui::SetCursorPosX(UICustom::GetCenterTextPosX(Translate("ORDER")));
		ImGui::AlignTextToFramePadding();
		ImGui::Text("%s", Translate("ORDER"));
		ImGui::SameLine();
		ImGui::AlignTextToFramePadding();
		ImGui::Text(ICON_LC_MESSAGE_CIRCLE_QUESTION);

		if (ImGui::IsItemHovered(ImGuiHoveredFlags_NoSharedDelay | ImGuiHoveredFlags_DelayShort)) {
			UICustom::FancyTooltip(Translate("ORDER_TOOLTIP"));
		}

		ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal);
		ImGui::Spacing();

		// Populate list of plugins sorted by compileIdx
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

	inline void drawDescription(const std::unique_ptr<BaseObject>& a_object)
	{
		if (const std::string desc = DescriptionFramework_Impl::GetItemDescription(a_object->GetTESForm()); !desc.empty()) {
			if (a_object->GetFormType() != RE::FormType::Book) {
				ImGui::Spacing();
				ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal);
				ImGui::SetCursorPosX(UICustom::GetCenterTextPosX(Translate("DESCRIPTION")));
				ImGui::AlignTextToFramePadding();
				ImGui::Text("%s", Translate("DESCRIPTION"));
				ImGui::SameLine();
				ImGui::AlignTextToFramePadding();
				ImGui::Text(ICON_LC_MESSAGE_CIRCLE_QUESTION);

				if (ImGui::IsItemHovered(ImGuiHoveredFlags_NoSharedDelay | ImGuiHoveredFlags_DelayShort)) {
					UICustom::FancyTooltip(Translate("DESCRIPTION_TOOLTIP"));
				}

				ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal);
				ImGui::Spacing();

				ImGui::TextWrapped("%s", desc.c_str());  // Archmage Robe Crash
			}
		}
	}

	inline void drawFooter(const std::unique_ptr<BaseObject>& a_object, bool a_tooltip)
	{
		if (a_tooltip) return;

		constexpr auto header_flags = ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_DefaultOpen;

		// Item/Object Keyword Dropdown
		if (const auto keywords = a_object->GetKeywordList(); !keywords.empty()) {
			ImGui::Spacing();
			if (ImGui::CollapsingHeader(Translate("KEYWORDS"), header_flags)) {
				const auto tooltip = FilterProperty::GetPropertyTooltipKey(PropertyType::kKeyword);
				const auto icon = FilterProperty::GetIcon(PropertyType::kKeyword);

				ImGui::PushID("ItemPreview::Keywords");
				for (const auto& keyword : keywords) {
					if (keyword.empty())
						continue;

					inlineTextEx(icon.c_str(), keyword.c_str(), tooltip.c_str());
				}
				ImGui::PopID();
			}
		}

		// Dummy Object's don't contain valid form pointers, stop here.
		if (a_object->IsDummy()) return;

		drawLoadOrder(a_object);
		drawDescription(a_object);
	}

	inline void drawActorPreview(const std::unique_ptr<BaseObject>& a_npc, bool a_tooltip)
	{
		inlineText(a_npc, PropertyType::kHealth);
		inlineText(a_npc, PropertyType::kMagicka);
		inlineText(a_npc, PropertyType::kStamina);

		ImGui::Spacing();
		ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal);
		ImGui::Spacing();

		inlineText(a_npc, PropertyType::kRace);
		inlineText(a_npc, PropertyType::kClass);
		inlineText(a_npc, PropertyType::kGender);
		inlineText(a_npc, PropertyType::kLevel);

		ImGui::Spacing();
		ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal);
		ImGui::Spacing();

		inlineText(a_npc, PropertyType::kDefaultOutfit);
		inlineText(a_npc, PropertyType::kSleepOutfit);

		ImGui::Spacing();
		ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal);
		ImGui::Spacing();

		if (!a_tooltip) {
			constexpr auto header_flags = ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_DefaultOpen;

			// TODO: Draw Skill Tree here.

			if (const auto spells = a_npc->GetSpellList(); !spells.empty()) {
				ImGui::Spacing();
				if (ImGui::CollapsingHeader(Translate("SPELLS"), header_flags)) {
					const auto tooltip = FilterProperty::GetPropertyTooltipKey(PropertyType::kSpell);
					const auto icon = FilterProperty::GetIcon(PropertyType::kSpell);

					ImGui::PushID("ItemPreview::Spells");
					for (const auto& spell : spells) {
						if (spell.empty())
							continue;

						inlineTextEx(icon.c_str(), spell.c_str(), tooltip.c_str());
					}
					ImGui::PopID();
				}
			}

			if (ImGui::IsItemHovered()) {
				UINotification::ShowTooltip(Translate(FilterProperty::GetPropertyTooltipKey(PropertyType::kSpellList).c_str()));
			}

			if (const auto factions = a_npc->GetFactions(); factions.has_value()) {
				ImGui::Spacing();
				if (ImGui::CollapsingHeader(Translate("FACTION"), header_flags)) {
					const auto tooltip = FilterProperty::GetPropertyTooltipKey(PropertyType::kFaction);
					const auto icon = FilterProperty::GetIcon(PropertyType::kFaction);

					ImGui::PushID("ItemPreview::Factions");
					for (const auto& faction : factions.value()) {
						std::string factionName = faction.faction->GetName();

						if (factionName.empty()) {
							factionName = faction.faction->GetFullName();
						}

						if (factionName.empty()) {
							factionName = "Modex Error";
						}

						inlineTextEx(icon.c_str(), factionName.c_str(), tooltip.c_str());
					}
					ImGui::PopID();
				}
			}

			if (ImGui::IsItemHovered()) {
				UINotification::ShowTooltip(Translate(FilterProperty::GetPropertyTooltipKey(PropertyType::kFactionList).c_str()));
			}
		}
	}

	inline void drawWeaponPreview(const std::unique_ptr<BaseObject>& a_weapon)
	{
		inlineText(a_weapon, PropertyType::kWeaponDamage);

		// Exclude redundant properties for ineligible types.
		if (!a_weapon->HasKeyword("WeapTypeStaff")) {
			inlineText(a_weapon, PropertyType::kWeaponSpeed);
			inlineText(a_weapon, PropertyType::kWeaponDamagePerSecond);
			inlineText(a_weapon, PropertyType::kWeaponCriticalDamage);
		}

		ImGui::Spacing();
		ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal);
		ImGui::Spacing();

		inlineText(a_weapon, PropertyType::kWeaponType);
		inlineText(a_weapon, PropertyType::kWeaponSkill);

		// Exclude more redundancy for ineligible types.
		if (!a_weapon->HasKeyword("WeapTypeStaff")) {
			ImGui::Spacing();
			ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal);
			ImGui::Spacing();

			inlineBar(a_weapon, PropertyType::kWeaponRange, 1.5f);
			inlineBar(a_weapon, PropertyType::kWeaponStagger, 2.0f);
		}

		ImGui::Spacing();
		ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal);
		ImGui::Spacing();

		inlineText(a_weapon, PropertyType::kCarryWeight);
		inlineText(a_weapon, PropertyType::kGoldValue);

	}

	inline void drawArmorPreview(const std::unique_ptr<BaseObject>& a_armor, bool a_tooltip)
	{
		inlineText(a_armor, PropertyType::kArmorRating);
		inlineText(a_armor, PropertyType::kArmorType);

		ImGui::Spacing();
		ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal);
		ImGui::Spacing();

		inlineText(a_armor, PropertyType::kCarryWeight);
		inlineText(a_armor, PropertyType::kGoldValue);

		const auto equip_slots = a_armor->GetArmorSlots();

		if (!a_tooltip && !equip_slots.empty()) {
			ImGui::Spacing();
			ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal);
			ImGui::Spacing();

			constexpr auto header_flags = ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_DefaultOpen;

			ImGui::Spacing();
			if (ImGui::CollapsingHeader(Translate("SLOTS"), header_flags)) {
				const auto tooltip = FilterProperty::GetPropertyTooltipKey(PropertyType::kArmorSlot);
				const auto icon = FilterProperty::GetIcon(PropertyType::kArmorSlot);

				ImGui::PushID("ItemPreview::ArmorSlots");
				for (const auto& slot : equip_slots) {
					inlineTextEx(icon.c_str(), slot.c_str(), tooltip.c_str());
				}
				ImGui::PopID();
			}
		}
	}

	inline float getDesiredWidth(const std::unique_ptr<BaseObject>& a_item, float a_min)
	{
		const auto& name = a_item->GetName();
		const auto& edid = a_item->GetEditorID();

		// Represents approx. width of left-aligned text.
		const float desc = ImGui::CalcTextSize(FilterProperty::GetString(PropertyType::kEditorID).c_str()).x;
		const float padding = ImGui::GetFontSize() * 5.0f;
		const bool use_name = name.length() > edid.length();
		const float text_width = use_name ? ImGui::CalcTextSize(name.c_str()).x : ImGui::CalcTextSize(edid.c_str()).x;

		return max(padding + desc + text_width, a_min);
	}
}

	// @arg a_tooltip: the item preview is shown in a tooltip instead of a widget.
	inline void ShowItemPreview(const std::unique_ptr<BaseObject>& a_item, bool a_tooltip = false)
	{
		if (a_item == nullptr) return;

		const auto cursor = ImGui::GetCursorScreenPos();
		const float font_size = ImGui::GetFontSize();
		const float tooltip_width = getDesiredWidth(a_item, 200.0f);
		const float max_width = a_tooltip ? tooltip_width : ImGui::GetContentRegionAvail().x;
		const auto& draw_list = ImGui::GetWindowDrawList();

		{ // Name Bar
			auto name = TRUNCATE(a_item->GetName(), max_width * 0.80f);
			const auto color = ImGui::GetStyleColorVec4(ImGuiCol_Border);
			const auto text_color = a_item->IsEnchanted() ? ThemeConfig::GetColor("TEXT_ENCHANTED") : ThemeConfig::GetColor("TEXT");

			draw_list->AddRectFilled(cursor, ImVec2(cursor.x + max_width, cursor.y + font_size * 2.5f), ImGui::ColorConvertFloat4ToU32(ImVec4(0.15f, 0.15f, 0.15f, 0.25f)));
			draw_list->AddRect(cursor, ImVec2(cursor.x + max_width, cursor.y + ImGui::GetFontSize() * 2.5f), ImGui::ColorConvertFloat4ToU32(color));

			ImGui::NewLine();
			ImGui::SetCursorPosX(UICustom::GetCenterTextPosX(name.data()));
			ImGui::SetCursorPosY(ImGui::GetCursorPosY() - font_size / 2.0f);
			ImGui::TextColored(text_color, "%s", name.data());
			ImGui::NewLine();
		}

		{ // Window Adjustment Hack
			ImGui::SetNextItemAllowOverlap();
			ImGui::Dummy(ImVec2(max_width, 0));
		}
 
		// Tooltips don't play well with autosizing child windows.
		if (!a_tooltip) ImGui::BeginChild("##ItemPreview::ScrollArea", ImVec2(0, 0), false, false);
		{
			ImGui::PushStyleColor(ImGuiCol_Separator, ThemeConfig::GetColorU32("HEADER_SEPARATOR"));
			drawBasePreview(a_item);

			if (auto npc = a_item->GetTESNPC(); npc.has_value()) {
				drawActorPreview(a_item, a_tooltip);
			}

			if (auto armor = a_item->GetTESArmor(); armor.has_value()) {
				drawArmorPreview(a_item, a_tooltip);
			}

			if (auto weapon = a_item->GetTESWeapon(); weapon.has_value()) {
				drawWeaponPreview(a_item);
			}

			drawFooter(a_item, a_tooltip);

			ImGui::PopStyleColor();
		}

		if (!a_tooltip) ImGui::EndChild();

		// Draw FormType color gradient over Name container.
		const float height = ImGui::GetFrameHeight() * 2.0f;

		const ImVec2 start = a_tooltip ?
			ImGui::GetWindowPos() :
			cursor;	

		const ImVec2 end = a_tooltip ?
			ImVec2(start.x + ImGui::GetWindowWidth(), start.y + height) :
			ImVec2(cursor.x + max_width, cursor.y + height);

		ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.2f);
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

	inline void ShowMissingPlugin(const std::unique_ptr<BaseObject>& a_object)
	{
		const std::string warning = Translate("ERROR_MISSING_PLUGIN") + a_object->GetPluginName(); 

		ImGui::NewLine();
		ImGui::SetCursorPosX(UICustom::GetCenterTextPosX(warning.data()));
		ImGui::SetCursorPosY(ImGui::GetCursorPosY() - ImGui::GetFontSize() / 2);
		ImGui::Text("%s", warning.data());
		ImGui::NewLine();
	}
}
