#pragma once

#include "UICustom.h"
#include "data/Data.h"
#include "core/Commands.h"
#include "imgui.h"
#include "imgui_internal.h"
#include "localization/Locale.h"
#include "config/ThemeConfig.h"
#include "ui/components/UINotification.h"
#include "external/framework/DescriptionFrameworkImpl.h"

namespace Modex
{
	// Forward declaration for use in drawOutfitItem.
	inline void ShowItemPreview(const std::unique_ptr<BaseObject>& a_item, bool a_tooltip = false);

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
		const ImVec2 bar_size = ImVec2(max_width / 3.0f, ImGui::GetFrameHeight());
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

	inline void inlineCheckbox(const std::unique_ptr<BaseObject>& a_item, PropertyType a_property)
	{
		const float max_width = ImGui::GetContentRegionAvail().x;
		const std::string& icon = a_item->GetPropertyTypeWithIcon(a_property);
		const std::string& tooltip = FilterProperty::GetPropertyTooltipKey(a_property);
		bool flag = a_item->GetPropertyByValue(a_property).find("true") == std::string::npos ? false : true;
		const float box_width = ImGui::GetFontSize();
		const float width = (std::max)(max_width - box_width, ImGui::GetContentRegionAvail().x - box_width);

		ImGui::Text("%s", icon.c_str());

		// Icon + Property Type
		if (ImGui::IsItemHovered()) {
			UINotification::ShowTooltip(tooltip.c_str());
		}

		// Right align flag checkbox
		ImGui::SameLine(width - 1.0f);
		ImGui::Checkbox("##NoLabel", &flag);

	}

	inline void inlineText(const std::unique_ptr<BaseObject>& a_item, PropertyType a_property)
	{
		const float max_width = ImGui::GetContentRegionAvail().x;
		const std::string& icon = a_item->GetPropertyTypeWithIcon(a_property);
		const std::string& tooltip = FilterProperty::GetPropertyTooltipKey(a_property);
		const std::string& text = TRUNCATE(a_item->GetPropertyByValue(a_property).c_str(), max_width * 0.75f);
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
		inlineText(a_object, PropertyType::kFormID);
		inlineText(a_object, PropertyType::kPlugin);
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

	inline void drawDebugInfo(const std::unique_ptr<BaseObject>& a_object)
	{
		if (!UserConfig::Get().developerMode) return;

		ImGui::Spacing();
		ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal);
		ImGui::Spacing();

		inlineTextEx("TableID", std::to_string(a_object->m_tableID).c_str(), "");
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
			} else {
				if (ImGui::IsItemHovered()) {
					UINotification::ShowTooltip(Translate(FilterProperty::GetPropertyTooltipKey(PropertyType::kKeywordList).c_str()));
				}
			}
		}

		// Dummy Object's don't contain valid form pointers, stop here.
		if (a_object->IsDummy()) return;

		drawDebugInfo(a_object);
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
			constexpr auto header_flags = ImGuiTreeNodeFlags_SpanAvailWidth;

			if (const auto skills = a_npc->GetSkills(); skills.has_value()) {
				ImGui::Spacing();
				if (ImGui::CollapsingHeader(Translate("SKILLS"), header_flags)) {
					ImGui::PushID("ItemPreview::Skills");
					for (size_t i = 0; i < RE::TESNPC::Skills::Skills::kTotal; i++) {
						uint32_t offset = static_cast<uint8_t>(RE::ActorValue::kOneHanded);

						uint8_t skillValue = skills.value().values[i];
						const std::string_view skillName = magic_enum::enum_name(static_cast<RE::ActorValue>(offset + i));

						inlineBarEx(skillName.data(), skillValue, 100.0f);

					}
					ImGui::PopID();
				} else {
					if (ImGui::IsItemHovered()) {
						UINotification::ShowTooltip(Translate("ACTOR_SKILLS"));
					}
				}
			}


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
				} else {
					if (ImGui::IsItemHovered()) {
						UINotification::ShowTooltip(Translate(FilterProperty::GetPropertyTooltipKey(PropertyType::kSpellList).c_str()));
					}
				}
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
							factionName = "";
						}

						inlineTextEx(icon.c_str(), factionName.c_str(), tooltip.c_str());
					}
					ImGui::PopID();
				} else {
					if (ImGui::IsItemHovered()) {
						UINotification::ShowTooltip(Translate(FilterProperty::GetPropertyTooltipKey(PropertyType::kFactionList).c_str()));
					}
				}
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

	// Render a single concrete item as a selectable row in the outfit preview.
	inline void drawOutfitItem(int a_index, RE::TESForm* a_form, uint16_t a_level)
	{
		const auto displayObject = std::make_unique<BaseObject>(a_form, Ownership::Outfit);
		const auto& draw_list = ImGui::GetWindowDrawList();
		const float pillar_width = 5.0f;

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
					Commands::AddItemToRefInventory(Ownership::Item, playerRef, displayObject->GetEditorID(), 1);
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
			const float pillar_width = 5.0f;
			const ImRect bb(ImGui::GetItemRectMin() - ImVec2(pillar_width, 0.0f), ImGui::GetItemRectMax()); 
			draw_list->AddRectFilled(
				ImVec2(bb.Min.x, bb.Min.y),
				ImVec2(bb.Min.x + pillar_width, bb.Max.y),
				UICustom::GetFormTypeColor(a_form->GetFormType())
			);
		};

		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 5.0f);
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
		ImGui::Spacing();
		ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal);
		ImGui::Spacing();

		inlineCheckbox(a_list, PropertyType::kLeveledAllLevelsFlag);
		inlineCheckbox(a_list, PropertyType::kLeveledEachFlag);
		inlineCheckbox(a_list, PropertyType::kLeveledUseAllFlag);
		inlineCheckbox(a_list, PropertyType::kLeveledSpecialFlag);

		ImGui::Spacing();
		ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal);
		ImGui::Spacing();

		inlineText(a_list, PropertyType::kLeveledChance);
	}

	inline void drawOutfitPreview(const std::unique_ptr<BaseObject>& a_item)
	{
		ImGui::Spacing();
		ImGui::Spacing();

		// Outfit Item List Header
		ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal);
		ImGui::SetCursorPosX(UICustom::GetCenterTextPosX(Translate("HEADER_OUTFIT_ITEMS")));
		ImGui::AlignTextToFramePadding();
		ImGui::Text("%s", Translate("HEADER_OUTFIT_ITEMS"));
		ImGui::SameLine();
		ImGui::AlignTextToFramePadding();
		ImGui::Text(ICON_LC_MESSAGE_CIRCLE_QUESTION);

		if (ImGui::IsItemHovered(ImGuiHoveredFlags_NoSharedDelay | ImGuiHoveredFlags_DelayShort)) {
			UICustom::FancyTooltip(Translate("HEADER_OUTFIT_TOOLTIP"));
		}

		ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal);
		ImGui::Spacing();

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
		ImGui::Spacing();
		ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal);
		ImGui::Spacing();

		inlineText(a_item, PropertyType::kName);
		inlineText(a_item, PropertyType::kEditorID);
	}

	inline float getDesiredWidth(const std::unique_ptr<BaseObject>& a_item, float a_min)
	{
		const auto& edid = a_item->GetEditorID();
		const auto& plugin = a_item->GetPluginName();

		const bool  use_plugin = plugin.length() > edid.length();
		const float desc = ImGui::CalcTextSize(FilterProperty::GetString(PropertyType::kEditorID).c_str()).x;
		const float padding = ImGui::GetFontSize() * 5.0f;
		const float text_width = use_plugin ? ImGui::CalcTextSize(plugin.c_str()).x : ImGui::CalcTextSize(edid.c_str()).x;

		return max(padding + desc + text_width, a_min);
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
	inline void ShowItemPreview(const std::unique_ptr<BaseObject>& a_item, bool a_tooltip)
	{
		if (a_item == nullptr) return;
		if (a_item->IsDummy()) return;

		const auto cursor = ImGui::GetCursorScreenPos();
		const float alpha = ImGui::GetStyle().Alpha;
		const float font_size = ImGui::GetFontSize();
		const float tooltip_width = getDesiredWidth(a_item, 200.0f);
		const float max_width = a_tooltip ? tooltip_width : ImGui::GetContentRegionAvail().x;
		const auto& draw_list = ImGui::GetWindowDrawList();

		{ // Name Bar
			auto name = TRUNCATE(a_item->GetName(), max_width * 0.80f);
			const auto color = ImGui::GetStyleColorVec4(ImGuiCol_Border);
			const auto text_color = a_item->IsEnchanted() ? ThemeConfig::GetColor("TEXT_ENCHANTED") : ThemeConfig::GetColor("TEXT");

			draw_list->AddRectFilled(cursor, ImVec2(cursor.x + max_width, cursor.y + font_size * 2.5f), ThemeConfig::GetColorU32("BG", alpha));
			draw_list->AddRect(cursor, ImVec2(cursor.x + max_width, cursor.y + font_size * 2.5f), ThemeConfig::GetColorU32("BORDER", alpha));

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
			ImGui::PushStyleColor(ImGuiCol_Separator, ThemeConfig::GetColorU32("PRIMARY"));
			drawBasePreview(a_item);

			if (a_item->GetTESNPC()) {
				drawActorPreview(a_item, a_tooltip);
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

			if (auto form = a_item->GetTESForm(); form) {
				if (auto leveled = form->As<RE::TESLeveledList>(); leveled) {
					drawLeveledListPreview(a_item);
				}
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
