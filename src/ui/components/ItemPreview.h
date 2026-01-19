#pragma once

#include "UICustom.h"
#include "data/Data.h"
#include "localization/Locale.h"
#include "external/framework/DescriptionFrameworkImpl.h"

// TODO: Change this style toward a more modern approach wiht a gradient background?

namespace Modex
{
	template <class Object>
	void ShowMissingPlugin(const std::unique_ptr<Object>& a_object)
	{
		const std::string warning = "Missing Plugin: " + a_object->GetPluginName(); 
		ImGui::NewLine();
		ImGui::SetCursorPosX(UICustom::GetCenterTextPosX(warning.data()));
		ImGui::SetCursorPosY(ImGui::GetCursorPosY() - ImGui::GetFontSize() / 2);
		ImGui::Text(warning.data());
		ImGui::NewLine();
	}


	inline void ShowItemPreview(const std::unique_ptr<BaseObject>& a_item)
	{
		if (a_item == nullptr) {
			return;
		}

		if (!a_item.get()) {
			return;
		}

		ImVec2 barSize = ImVec2(100.0f, ImGui::GetFontSize());
		float maxWidth = ImGui::GetContentRegionAvail().x;

		// TODO: Move these lambda functions to UICustom class.

		constexpr auto ProgressColor = [](const double value, const float max_value) -> ImVec4 {
			static const float dampen = 0.7f;
			const float ratio = (float)value / max_value;
			const float r = 1.0f - ratio * dampen;
			const float g = ratio * dampen;
			return ImVec4(r, g, 0.0f, 0.67f);
		};

		const auto InlineBar = [maxWidth, barSize, ProgressColor](const std::unique_ptr<BaseObject>& a_item, PropertyType a_property, const float value, const float max_value) {
			const std::string icon = a_item->GetPropertyTypeWithIcon(a_property);
			const std::string tooltip = FilterProperty::GetIconTooltipKey(a_property);
			ImGui::Text("%s", icon.c_str());

			if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal | ImGuiHoveredFlags_NoSharedDelay)) {
				UICustom::FancyTooltip(tooltip.c_str());
			}

			ImGui::SameLine(maxWidth - barSize.x - 1.0f);
			ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ProgressColor(value, max_value));
			ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.0f);  // tight fit
			float curr = static_cast<float>(value);
			char buffer[256];
			ImFormatString(buffer, IM_ARRAYSIZE(buffer), "%.2f", value);
			ImGui::ProgressBar(curr / max_value, barSize, buffer);
			ImGui::PopStyleColor(1);
			ImGui::PopStyleVar(1);
		};

		const auto InlineInt = [maxWidth](const std::unique_ptr<BaseObject>& a_item, PropertyType a_property, const int value) {
			const std::string icon = a_item->GetPropertyTypeWithIcon(a_property);
			const std::string tooltip = FilterProperty::GetIconTooltipKey(a_property);
			const auto width = (std::max)(maxWidth - ImGui::CalcTextSize(std::to_string(value).c_str()).x, ImGui::GetContentRegionAvail().x - ImGui::CalcTextSize(std::to_string(value).c_str()).x);
			ImGui::Text("%s",icon.c_str());

			if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal | ImGuiHoveredFlags_NoSharedDelay)) {
				UICustom::FancyTooltip(tooltip.c_str());
			}

			ImGui::SameLine(width - 1.0f);
			ImGui::Text("%d", value);
		};

		const auto InlineText = [maxWidth](const std::unique_ptr<BaseObject>& a_item, PropertyType a_property, const char* text) {
			const std::string icon = a_item->GetPropertyTypeWithIcon(a_property);
			const std::string tooltip = FilterProperty::GetIconTooltipKey(a_property);
			const auto width = (std::max)(maxWidth - ImGui::CalcTextSize(text).x, ImGui::GetContentRegionAvail().x - ImGui::CalcTextSize(text).x);
			ImGui::Text("%s", icon.c_str());

			if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal | ImGuiHoveredFlags_NoSharedDelay)) {
				UICustom::FancyTooltip(tooltip.c_str());
			}

			ImGui::SameLine(width - 1.0f);
			ImGui::Text("%s", text);
		};

		const auto InlineTextEx = [maxWidth](const char* a_left, const char* a_right) {
			const auto width = (std::max)(maxWidth - ImGui::CalcTextSize(a_right).x, ImGui::GetContentRegionAvail().x - ImGui::CalcTextSize(a_right).x);
			ImGui::Text("%s", a_left);
			ImGui::SameLine(width - 1.0f);
			ImGui::Text("%s", a_right);
		};

		// Name Bar
		auto name = a_item->GetName();

		if (name.empty()) {
			name = a_item->GetEditorID();
		}

		ImDrawList* draw_list = ImGui::GetWindowDrawList();
		const auto cursor = ImGui::GetCursorScreenPos();
		const auto size = ImGui::GetContentRegionAvail();
		const auto color = ImGui::GetStyleColorVec4(ImGuiCol_Border);
		draw_list->AddRectFilled(cursor, ImVec2(cursor.x + size.x, cursor.y + ImGui::GetFontSize() * 2.5f), ImGui::ColorConvertFloat4ToU32(ImVec4(0.15f, 0.15f, 0.15f, 0.25f)));
		draw_list->AddRect(cursor, ImVec2(cursor.x + size.x, cursor.y + ImGui::GetFontSize() * 2.5f), ImGui::ColorConvertFloat4ToU32(color));

		ImGui::NewLine();
		ImGui::SetCursorPosX(UICustom::GetCenterTextPosX(name.data()));
		ImGui::SetCursorPosY(ImGui::GetCursorPosY() - ImGui::GetFontSize() / 2);
		ImGui::Text("%s",name.data());
		ImGui::NewLine();

		static auto constexpr flags = 	ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoScrollbar |
		                       			ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing;


		if (ImGui::BeginChild("##ItemPreview::ScrollArea", ImVec2(ImGui::GetContentRegionAvail().x, 0), false, flags)) {
			// NPC Specific Item Card details:
			if (auto npc = a_item->GetTESNPC(); npc.has_value()) {
				InlineInt(a_item, PropertyType::kHealth, static_cast<int>(a_item->GetActorValue(RE::ActorValue::kHealth)));
				InlineInt(a_item, PropertyType::kMagicka, static_cast<int>(a_item->GetActorValue(RE::ActorValue::kMagicka)));
				InlineInt(a_item, PropertyType::kStamina, static_cast<int>(a_item->GetActorValue(RE::ActorValue::kStamina)));

				ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal);

				InlineText(a_item, PropertyType::kRace, a_item->GetRace().data());
				InlineText(a_item, PropertyType::kClass, a_item->GetClass().data());
				InlineText(a_item, PropertyType::kGender, a_item->GetGender().data());
				InlineInt(a_item, PropertyType::kLevel, a_item->GetLevel());

				ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal);

				// TODO: REFACTOR we removed skill names from Utils
				// if (ImGui::CollapsingHeader(Translate("SKILLS"), ImGuiTreeNodeFlags_SpanAvailWidth)) {
				// 	ImGui::PushID("ItemPreview::Skills");
				// 	if (const auto skills = a_item->GetSkills(); skills.has_value()) {
				// 		const auto skillNames = Utils::GetSkillNames();

				// 		for (int i = 0; i < 18; i++) {
				// 			const auto skillName = skillNames[i];
				// 			const auto skillLevel = skills.value().values[i];
							
				// 			InlineSkillBar(skillName.c_str(), skillLevel);
				// 		}
				// 	}
				// 	ImGui::PopID();
				// }

				// Saved for later
				//				auto castType = Utils::GetCastingType(spell->data.castingType);
				// 				auto spellType = Utils::GetSpellType(spell->data.spellType);
				// 				auto delType = Utils::GetDeliveryType(spell->data.delivery);
				// 				auto cost = spell->CalculateMagickaCost(npc->GetForm()->As<RE::Actor>());

				if (ImGui::CollapsingHeader(Translate("SPELLS"), ImGuiTreeNodeFlags_SpanAvailWidth)) {
					if (const auto spellList = a_item->GetSpellList(); spellList.has_value()) {
						const auto spellData = spellList.value();

						ImGui::PushID("ItemPreview::Spells");
						if (spellData != nullptr && spellData->numSpells > 0) {
							for (uint32_t i = 0; i < spellData->numSpells; i++) {
								if (spellData->spells[i] == nullptr)
									continue;

								const auto* spell = spellData->spells[i];

								if (spell == nullptr) {
									continue;
								}

								static const std::string icon = FilterProperty::GetIcon(PropertyType::kSpellList).c_str();
								const std::string spellName = spell->GetName() ? spell->GetName() : (spell->GetFullName() ? spell->GetFullName() : "Unnamed Spell");

								InlineTextEx(icon.c_str(), spellName.c_str());
							}
						} else {
							InlineTextEx(FilterProperty::GetIconWithText(PropertyType::kSpellList).c_str(), Translate("None"));
						}
						ImGui::PopID();
					}
				}

				if (ImGui::CollapsingHeader(Translate("FACTION"), ImGuiTreeNodeFlags_SpanAvailWidth)) {
					const auto factions = a_item->GetFactions();
					if (factions.has_value()) {
						ImGui::PushID("ItemPreview::Factions");
						for (const auto& faction : factions.value()) {
							std::string factionName = faction.faction->GetName();

							if (factionName.empty()) {
								factionName = faction.faction->GetFullName();
							}

							if (factionName.empty()) {
								factionName = "Unnamed Faction";
							}

							// TODO: Sometimes faction value can be -1?

							InlineTextEx(FilterProperty::GetIcon(PropertyType::kFactionList).c_str(), factionName.c_str());
						}
						ImGui::PopID();
					} else {
						InlineTextEx(FilterProperty::GetIconWithText(PropertyType::kFactionList).c_str(), Translate("NONE"));
					}
				}

				if (ImGui::CollapsingHeader(Translate("OTHER"), ImGuiTreeNodeFlags_SpanAvailWidth)) {
					if (const auto defaultOutfit = a_item->GetDefaultOutfit(); !defaultOutfit.empty()) {
						InlineText(a_item, PropertyType::kDefaultOutfit, defaultOutfit.c_str());
					} else {
						InlineTextEx(FilterProperty::GetIconWithText(PropertyType::kDefaultOutfit).c_str(), Translate("NONE"));
					}

					if (const auto sleepOutfit = a_item->GetSleepOutfit(); !sleepOutfit.empty()) {
						InlineText(a_item, PropertyType::kSleepOutfit, sleepOutfit.c_str());
					} else {
						InlineTextEx(FilterProperty::GetIconWithText(PropertyType::kSleepOutfit).c_str(), Translate("NONE"));
					}
				}
			}
			else if (a_item->IsArmor()) {
				if (auto armor = a_item->GetTESArmor(); armor.has_value()) {
					const auto armorType = a_item->GetArmorType();
					const float armorRating = static_cast<float>(a_item->GetArmorRating());
					const auto equipSlots = a_item->GetArmorSlots();
					
					InlineInt(a_item, PropertyType::kArmorRating, static_cast<int>(armorRating)); // Rounded, hehe
					InlineText(a_item, PropertyType::kArmorType, armorType.data());
					ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal);
					InlineInt(a_item, PropertyType::kCarryWeight, a_item->GetWeight());
					InlineInt(a_item, PropertyType::kGoldValue, a_item->GetGoldValue());
					// InlineTextMulti(a_item, PropertyType::kArmorSlot, equipSlots);

					ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal);

					if (ImGui::CollapsingHeader(Translate("SLOTS"), ImGuiTreeNodeFlags_SpanAvailWidth)) {
						if (!equipSlots.empty()) {
							ImGui::PushID("ItemPreview::ArmorSlots");
							for (const auto& slot : equipSlots) {
								InlineTextEx(FilterProperty::GetIcon(PropertyType::kArmorSlot).c_str(), slot.c_str());
							}
							ImGui::PopID();
						} else {
							InlineTextEx(FilterProperty::GetIconWithText(PropertyType::kArmorSlot).c_str(), Translate("NONE"));
						}
					}
				}
			}
			else if (a_item->IsWeapon()) {
				if (auto weapon = a_item->GetTESWeapon(); weapon.has_value()) {
					const float speed = a_item->GetWeaponSpeed();
					const int damage = static_cast<int>(a_item->GetWeaponDamage());
					const int dps = static_cast<int>(damage * speed);
					const int critDamage = static_cast<int>(a_item->GetWeaponCritical());
					const std::string skill = a_item->GetWeaponSkill();
					const std::string type = a_item->GetWeaponType();

					if (weapon.value()->IsStaff()) {
						InlineText(a_item, PropertyType::kWeaponDamage, "N/A");
					} else if (weapon.value()->IsBow() || weapon.value()->IsCrossbow()) {
						InlineInt(a_item, PropertyType::kWeaponDamage, static_cast<int>(damage));
						InlineBar(a_item, PropertyType::kWeaponSpeed, speed, 1.5f);
						InlineInt(a_item, PropertyType::kWeaponDamagePerSecond, dps);
						ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal);
						InlineInt(a_item, PropertyType::kWeaponCriticalDamage, critDamage);
						InlineText(a_item, PropertyType::kWeaponSkill, skill.c_str());
					} else {
						const float range = (float)(weapon.value()->weaponData.reach);
						const float stagger = weapon.value()->weaponData.staggerValue;
						InlineInt(a_item, PropertyType::kWeaponDamage, static_cast<int>(damage));
						InlineBar(a_item, PropertyType::kWeaponSpeed, speed, 1.5f);
						InlineInt(a_item, PropertyType::kWeaponDamagePerSecond, dps);
						ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal);
						InlineInt(a_item, PropertyType::kWeaponCriticalDamage, critDamage);
						InlineText(a_item, PropertyType::kWeaponSkill, skill.c_str());
						InlineBar(a_item, PropertyType::kWeaponRange, range, 1.5f);
						InlineBar(a_item, PropertyType::kWeaponStagger, stagger, 2.0f);
					}

					ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal);
					InlineText(a_item, PropertyType::kWeaponType, type.c_str());
					InlineInt(a_item, PropertyType::kCarryWeight, a_item->GetWeight());
					InlineInt(a_item, PropertyType::kGoldValue, a_item->GetGoldValue());
					ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal);
				}
			}
			else {
				InlineInt(a_item, PropertyType::kCarryWeight, a_item->GetWeight());
				InlineInt(a_item, PropertyType::kGoldValue, a_item->GetGoldValue());

				ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal);
			}
			
			// Applied to ALL items.
			if (ImGui::CollapsingHeader(Translate("KEYWORDS"), ImGuiTreeNodeFlags_SpanAvailWidth)) {
				if (const auto keywords = a_item->GetKeywordList(); !keywords.empty()) {
					ImGui::PushID("ItemPreview::Keywords");
					for (const auto& keyword : keywords) {
						if (keyword.empty())
							continue;

						InlineTextEx(FilterProperty::GetIcon(PropertyType::kKeywordList).c_str(), keyword.c_str());
					}
					ImGui::PopID();
				} else {
					InlineTextEx(FilterProperty::GetIconWithText(PropertyType::kKeywordList).c_str(), Translate("NONE"));
				}
			}

			// Load Order Info Pane
			ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal);
			ImGui::SetCursorPosX(UICustom::GetCenterTextPosX(Translate("ORDER")));
			ImGui::Text("%s", Translate("ORDER"));
			ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal);

			if (a_item->IsDummy()) {
				ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "%s", Translate("Item is a dummy object!"));
				return;
			} else {
				if (const auto source_files_opt = a_item->GetFileArray(); source_files_opt.has_value()) {
					if (const auto source_files = source_files_opt.value(); source_files != nullptr) {
						for (uint32_t i = 0; i < source_files->size(); i++) {
							if (const auto file = (*source_files)[i]) {
								auto fileName = ValidateTESFileName(file);
								
								if (i == 0 && source_files->size() > 1) {
									ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "%s", fileName.c_str());
								} else if (i == source_files->size() - 1) {
									ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "%s", fileName.c_str());
								} else {
									ImGui::Text("%s", fileName.c_str());
								}
							}
						}
					}
				}
				
				if (const std::string desc = DescriptionFramework_Impl::GetItemDescription(a_item->GetTESForm()); !desc.empty()) {
					if (a_item->GetFormType() != RE::FormType::Book) {
						ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal);
						ImGui::SetCursorPosX(UICustom::GetCenterTextPosX(Translate("DESCRIPTION")));
						ImGui::Text("%s", Translate("DESCRIPTION"));
						ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal);
						ImGui::PushTextWrapPos(maxWidth);
						ImGui::TextWrapped("%s", desc.c_str());  // Archmage Robe Crash
						ImGui::PopTextWrapPos();
					}
				}
			}
		}

		ImGui::EndChild();
	}
}
