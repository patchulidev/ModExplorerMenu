#pragma once
#include <include/D/Data.h>
#include <include/U/Util.h>

// TODO: Not sure why this is its own header.

namespace Modex
{
	template <class Object>
	void ShowItemPreview(const std::unique_ptr<Object>& a_object)
	{
		if (a_object == nullptr) {
			return;
		}

		ImVec2 barSize = ImVec2(100.0f, ImGui::GetFontSize());
		float maxWidth = ImGui::GetContentRegionAvail().x;

		constexpr auto ProgressColor = [](const double value, const float max_value) -> ImVec4 {
			const float dampen = 0.7f;
			const float ratio = (float)value / max_value;
			const float r = 1.0f - ratio * dampen;
			const float g = ratio * dampen;
			return ImVec4(r, g, 0.0f, 1.0f);
		};

		const auto InlineBar = [maxWidth, barSize, ProgressColor](const char* label, const float value, const float max_value) {
			ImGui::Text(label);
			ImGui::SameLine(maxWidth - barSize.x);
			ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ProgressColor(value, max_value));
			ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.0f);  // tight fit
			float curr = static_cast<float>(value);
			char buffer[256];
			sprintf(buffer, "%.2f", value);
			ImGui::ProgressBar(curr / max_value, barSize, buffer);
			ImGui::PopStyleColor(1);
			ImGui::PopStyleVar(1);
		};

		const auto InlineInt = [maxWidth, barSize](const char* label, const int value) {
			const auto defaultWidth = maxWidth - ImGui::CalcTextSize(std::to_string(value).c_str()).x;
			const auto width = (std::max)(defaultWidth, ImGui::GetContentRegionAvail().x - ImGui::CalcTextSize(std::to_string(value).c_str()).x);
			ImGui::Text(label);
			ImGui::SameLine(width);
			ImGui::Text("%d", value);
		};

		// TODO: Really need to abstract this out.
		const auto InlineText = [maxWidth](const char* label, const char* text) {
			const auto width = (std::max)(maxWidth - ImGui::CalcTextSize(text).x, ImGui::GetContentRegionAvail().x - ImGui::CalcTextSize(text).x);
			ImGui::Text(label);
			ImGui::SameLine(width);
			ImGui::Text(text);
		};

		const auto InlineTextMulti = [maxWidth](const char* label, std::vector<std::string> text) {
			const auto width = (std::max)(maxWidth - ImGui::CalcTextSize(text[0].c_str()).x, ImGui::GetContentRegionAvail().x - ImGui::CalcTextSize(text[0].c_str()).x);
			ImGui::Text(label);
			ImGui::SameLine(width);
			ImGui::Text(text[0].c_str());

			for (int i = 1; i < text.size(); i++) {
				ImGui::SetCursorPosX(maxWidth - ImGui::CalcTextSize(text[i].c_str()).x);
				ImGui::Text(text[i].c_str());
			}
		};

		const auto TruncateText = [](const char* text, const float width) -> std::string {
			if (ImGui::CalcTextSize(text).x > width) {
				std::string truncated = text;
				truncated.resize((size_t)(width / ImGui::CalcTextSize("A").x) - 3);
				truncated += "...";
				return truncated;
			}
			return std::string(text);
		};

		// TODO: This is a temporary solution during cmake -> xmake conversion.
		const auto GetSkillName = [](RE::ActorValue skill) -> std::string {
			switch (skill) {
				case RE::ActorValue::kOneHanded: return "One Handed";
				case RE::ActorValue::kTwoHanded: return "Two Handed";
				case RE::ActorValue::kArchery: return "Archery";
				case RE::ActorValue::kBlock: return "Block";
				case RE::ActorValue::kSmithing: return "Smithing";
				case RE::ActorValue::kHeavyArmor: return "Heavy Armor";
				case RE::ActorValue::kLightArmor: return "Light Armor";
				case RE::ActorValue::kSneak: return "Sneak";
				case RE::ActorValue::kLockpicking: return "Lockpicking";
				case RE::ActorValue::kPickpocket: return "Pickpocket";
				case RE::ActorValue::kAlchemy: return "Alchemy";
				case RE::ActorValue::kSpeech: return "Speech";
				case RE::ActorValue::kEnchanting: return "Enchanting";
				default: return "Unknown Skill";
			}
		};

		// Name Bar
		auto name = a_object->GetName();

		// TODO: Instead of manually assigning EditorID when missing a name, we should do this
		// in the constructor of ItemData automatically. That way we don't need to add these
		// conditions everywhere.

		if (name.empty()) {
			name = a_object->GetEditorID();
		}

		ImDrawList* draw_list = ImGui::GetWindowDrawList();
		const auto cursor = ImGui::GetCursorScreenPos();
		const auto size = ImGui::GetContentRegionAvail();
		const auto color = ImGui::GetStyleColorVec4(ImGuiCol_Border);
		draw_list->AddRectFilled(cursor, ImVec2(cursor.x + size.x, cursor.y + ImGui::GetFontSize() * 2.5f), ImGui::ColorConvertFloat4ToU32(ImVec4(0.15f, 0.15f, 0.15f, 0.5f)));
		draw_list->AddRect(cursor, ImVec2(cursor.x + size.x, cursor.y + ImGui::GetFontSize() * 2.5f), ImGui::ColorConvertFloat4ToU32(color));

		ImGui::NewLine();
		ImGui::SetCursorPosX(ImGui::GetCenterTextPosX(name.data()));
		ImGui::SetCursorPosY(ImGui::GetCursorPosY() - ImGui::GetFontSize() / 2);
		ImGui::Text(name.data());
		ImGui::NewLine();

		// NPC Specific Item Card details:
		if constexpr (std::is_same<Object, NPCData>::value) {
			auto* npc = a_object->GetForm()->As<RE::TESNPC>();

			if (npc == nullptr) {
				return;
			}

			InlineBar(Translate("Health") + ":", a_object->GetHealth(), 100);
			InlineBar(Translate("Magicka") + ":", a_object->GetMagicka(), 100);
			InlineBar(Translate("Stamina") + ":", a_object->GetStamina(), 100);
			InlineBar(Translate("Weight") + ":", a_object->GetCarryWeight(), 100);

			ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal);
		}

		// constexpr allows for compile-time evaluation of struct passed
		// allowing for member access to be determined by control paths.
		if constexpr (std::is_same<Object, ItemData>::value) {
			if (a_object->GetFormType() == RE::FormType::Armor) {
				auto* armor = a_object->GetForm()->As<RE::TESObjectARMO>();

				if (armor == nullptr) {
					return;
				}

				const auto armorType = Utils::GetArmorType(armor);
				const float armorRating = Utils::CalcBaseArmorRating(armor);
				const float armorRatingMax = Utils::CalcMaxArmorRating(armorRating, 50);
				const auto equipSlots = Utils::GetArmorSlots(armor);

				if (armorRating == 0) {
					// InlineText("Armor Rating:", "None");
					InlineText(TranslateIconFormat(ICON_LC_SHIELD, "Rating", ":"), Translate("None"));
				} else {
					// InlineBar("Armor Rating:", armorRating, armorRatingMax);
					InlineBar(TranslateIconFormat(ICON_LC_SHIELD, "Rating", ":"), armorRating, armorRatingMax);
				}

				InlineText(TranslateIconFormat(ICON_LC_PUZZLE, "Type", ":"), Translate(armorType));
				InlineTextMulti(TranslateIconFormat(ICON_LC_BETWEEN_HORIZONTAL_START, "Slot", ":"), equipSlots);
			}

			if (a_object->GetFormType() == RE::FormType::Weapon) {
				auto* weapon = a_object->GetForm()->As<RE::TESObjectWEAP>();

				if (weapon == nullptr) {
					return;
				}

				// TODO: Refactor along with Search.cpp reference.
				const char* weaponTypes[] = {
					"Hand to Hand",
					"One Handed Sword",
					"One Handed Dagger",
					"One Handed Axe",
					"One Handed Mace",
					"Two Handed Greatsword",
					"Two Handed Battleaxe",
					"Bow",
					"Staff",
					"Crossbow"
				};

				const float damage = Utils::CalcBaseDamage(weapon);
				const float max_damage = Utils::CalcMaxDamage(damage, 50);
				const float speed = weapon->weaponData.speed;
				const int dps = (int)(damage * speed);
				const uint16_t critDamage = weapon->GetCritDamage();
				const RE::ActorValue skill = weapon->weaponData.skill.get();
				const auto type = weaponTypes[static_cast<int>(weapon->GetWeaponType())];

				if (weapon->IsStaff()) {
					// InlineText("Base Damage:", "N/A");
					InlineText(ICON_LC_SWORD, "N/A");
				} else if (weapon->IsBow() || weapon->IsCrossbow()) {
					InlineBar(TranslateIconFormat(ICON_LC_SWORD, "DMG", ":"), damage, max_damage);
					InlineBar(TranslateIconFormat(ICON_LC_SWORD, "Speed", ":"), speed, 1.5f);
					InlineInt(TranslateIconFormat(ICON_LC_SWORD, "DPS", ":"), dps);
					ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal);
					InlineInt(TranslateIconFormat(ICON_LC_SWORD, "DMG", ":"), critDamage);
					InlineText(TranslateIconFormat(ICON_LC_SWORDS, "Skill", ":"), Translate(GetSkillName(skill).c_str()));
				} else {
					const float reach = (float)(weapon->weaponData.reach);
					const float stagger = weapon->weaponData.staggerValue;
					InlineBar(TranslateIconFormat(ICON_LC_SWORD, "DMG", ":"), damage, max_damage);
					InlineBar(TranslateIconFormat(ICON_LC_SWORD, "Speed", ":"), speed, 1.5f);
					InlineInt(TranslateIconFormat(ICON_LC_SWORD, "DPS", ":"), dps);
					ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal);
					InlineInt(TranslateIconFormat(ICON_LC_SWORD, "DMG", ":"), critDamage);
					InlineText(TranslateIconFormat(ICON_LC_BOOK_USER, "Skill", ":"), Translate(GetSkillName(skill).c_str()));
					InlineBar(TranslateIconFormat(ICON_LC_CHEVRONS_LEFT_RIGHT, "Reach", ":"), reach, 1.5f);
					InlineBar(TranslateIconFormat(ICON_LC_SCALE, "Stagger", ":"), stagger, 2.0f);
				}

				ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal);
				InlineText(TranslateIconFormat(ICON_LC_BOOK_USER, "Type", ":"), Translate(type));
			}

			InlineInt(ICON_LC_WEIGHT "WT:", (int)a_object->GetWeight());
			InlineInt(TranslateIconFormat(ICON_LC_COINS, "Value", ":"), a_object->GetValue());

			// Load Order Info Pane
			ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal);
			ImGui::SetCursorPosX(ImGui::GetCenterTextPosX(Translate("Load Order")));
			ImGui::Text(Translate("Load Order"));
			ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal);
			const auto sourceFiles = a_object->GetForm()->sourceFiles.array;

			if (sourceFiles) {
				for (uint32_t i = 0; i < sourceFiles->size(); i++) {
					if (const auto file = (*sourceFiles)[i]) {
						auto fileName = ValidateTESFileName(file);

						if (i == 0 && sourceFiles->size() > 1) {
							ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "%s", fileName.c_str());
						} else if (i == sourceFiles->size() - 1) {
							ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "%s", fileName.c_str());
						} else {
							ImGui::Text("%s", fileName.c_str());
						}
					}
				}
			}

			const std::string desc = Utils::GetItemDescription(a_object->GetForm());

			if (!desc.empty()) {
				ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal);
				ImGui::SetCursorPosX(ImGui::GetCenterTextPosX(Translate("Description")));
				ImGui::Text(Translate("Description"));
				ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal);
				if (a_object->GetFormType() == RE::FormType::Book) {
					ImGui::SetCursorPosX(ImGui::GetCenterTextPosX(Translate("Read Me!")));
					ImGui::Text(Translate("Read Me!"));
				} else {
					ImGui::PushTextWrapPos(maxWidth);
					ImGui::TextWrapped(desc.c_str());  // Archmage Robe Crash
					ImGui::PopTextWrapPos();
				}
			}
		}
	}
}