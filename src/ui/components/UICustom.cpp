#include "UICustom.h"
#include "core/Graphic.h"
#include "external/icons/IconsLucide.h"
#include "imgui.h"
#include "ui/core/UIManager.h"
#include "config/UserConfig.h"
#include "config/ThemeConfig.h"
#include "localization/FontManager.h"
#include "localization/Locale.h"


namespace Modex::UICustom
{
	static inline float s_widgetWidth = 150.0f; // Fixed Settings right-align widget width.

	[[nodiscard]] float GetCenterTextPosX(const char* a_text)
	{
		return ImGui::GetCursorPosX() + (ImGui::GetContentRegionAvail().x / 2.0f) - (ImGui::CalcTextSize(a_text).x / 2.0f);
	}

	[[nodiscard]] float GetCenterTextPosX(const std::string& a_string)
	{
		return GetCenterTextPosX(a_string.c_str());
	}

	ImU32 GetFormTypeColor(const RE::FormType& a_type)
	{
		auto alpha = ImGui::GetStyle().Alpha;
		switch (a_type) {
		case RE::FormType::Armor:
			return ThemeConfig::GetColorU32("ARMO", alpha);
		case RE::FormType::AlchemyItem:
			return ThemeConfig::GetColorU32("ALCH", alpha);
		case RE::FormType::Ammo:
			return ThemeConfig::GetColorU32("AMMO", alpha);
		case RE::FormType::Book:
			return ThemeConfig::GetColorU32("BOOK", alpha);
		case RE::FormType::Ingredient:
			return ThemeConfig::GetColorU32("INGR", alpha);
		case RE::FormType::KeyMaster:
			return ThemeConfig::GetColorU32("KEYM", alpha);
		case RE::FormType::Misc:
			return ThemeConfig::GetColorU32("MISC", alpha);
		case RE::FormType::Scroll:
			return ThemeConfig::GetColorU32("SCRL", alpha);
		case RE::FormType::Weapon:
			return ThemeConfig::GetColorU32("WEAP", alpha);
		case RE::FormType::NPC:
			return ThemeConfig::GetColorU32("NPC_", alpha);
		case RE::FormType::Tree:
			return ThemeConfig::GetColorU32("TREE", alpha);
		case RE::FormType::Static:
			return ThemeConfig::GetColorU32("STAT", alpha);
		case RE::FormType::Container:
			return ThemeConfig::GetColorU32("CONT", alpha);
		case RE::FormType::Activator:
			return ThemeConfig::GetColorU32("ACTI", alpha);
		case RE::FormType::Light:
			return ThemeConfig::GetColorU32("LIGH", alpha);
		case RE::FormType::Door:
			return ThemeConfig::GetColorU32("DOOR", alpha);
		case RE::FormType::Furniture:
			return ThemeConfig::GetColorU32("FURN", alpha);
		default:
			return IM_COL32(169, 169, 169, 100 * alpha);  // Dark Gray
		}
	}

	// A helper function to dispatch custom input box for action amounts.
	void InputAmountHandler(bool a_condition, std::function<void (uint32_t)> a_onAmountEntered)
	{
		if (a_condition) {
			UIManager::GetSingleton()->ShowInputBox(
				Translate("AMOUNT"),
				Translate("ADD_AMOUNT_DESC"),
				"1",
				[a_onAmountEntered](const std::string& a_input) {
					uint32_t amount = 0;
					auto [ptr, ec] = std::from_chars(a_input.data(), a_input.data() + a_input.size(), amount);
					if (ec == std::errc() && amount > 0) {
						UIManager::GetSingleton()->ShowWarning(
							Translate("WARNING"),
							Translate("ERROR_MAX_QUERY"),
							(int)amount >= UserConfig::Get().maxQuery,
							[a_onAmountEntered, amount]() {
								a_onAmountEntered(amount);
							}
						); return;
					} else {
						UIManager::GetSingleton()->ShowWarning(
							Translate("INVALID"),
							Translate("INVALID_AMOUNT_DESC")
						);
					}
				}
			);
		} else {
			a_onAmountEntered(1);
		}
	}

	void SubCategoryHeader(const char* a_label)
	{
		ImGui::Spacing();
		ImGui::PushFontBold(ImGui::GetFontSize());
		const float center_x = UICustom::GetCenterTextPosX(a_label);
		ImGui::SetCursorPosX(center_x);
		ImGui::Text("%s", a_label);
		ImGui::PopFont();
		ImGui::Spacing();
	}

	bool ActionButton(const char* a_translate, const ImVec2& a_size, const bool a_condition)
	{
		bool success = false;
		if (a_condition) {
			success = ImGui::Button(Translate(a_translate), a_size);
		} else {
			ImGui::BeginDisabled();
			ImGui::Button(Translate(a_translate), a_size);
			ImGui::EndDisabled();
		}

		if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal | ImGuiHoveredFlags_NoSharedDelay | ImGuiHoveredFlags_AllowWhenDisabled)) {
			const auto tooltip_key = std::string(a_translate) + "_TOOLTIP";
			if (Locale::GetSingleton()->HasEntry(tooltip_key.c_str())) {
				UICustom::FancyTooltip(tooltip_key.c_str());
			}
		}

		return success;
	}

	bool FancyInputText(const char* a_id, const char *a_hint, const char* a_tooltip, char* a_buffer, float a_width, ImGuiInputTextFlags a_flags)
	{
		bool changed = false;
		auto pos = ImGui::GetCursorScreenPos();

		ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 12.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8.0f, 8.0f));

		ImGui::SetNextItemWidth(a_width);
		if (ImGui::InputTextWithHint(a_id, Translate(a_hint), a_buffer, MAX_PATH, a_flags)) {
			changed = true;
		}

		if (ImGui::IsItemActivated()) {
			ModexGUIMenu::FlushInputState();
		}

		if (Locale::GetSingleton()->HasEntry(a_tooltip)) {
			if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal | ImGuiHoveredFlags_NoSharedDelay)) {
				UICustom::FancyTooltip(a_tooltip);
			}
		}

		ImGui::SameLine();

		ImGui::PushFont(NULL, 18.0f);
		auto DrawList = ImGui::GetWindowDrawList();
		pos.x += a_width - ImGui::GetFrameHeightWithSpacing() + ImGui::GetStyle().FramePadding.x;
		pos.y += (ImGui::GetItemRectSize().y / 2.0f) - (ImGui::GetFontSize() / 2.0f);

		DrawList->AddText(pos, ThemeConfig::GetColorU32("TEXT", ImGui::GetStyle().Alpha), ICON_LC_SEARCH);
		ImGui::PopFont();
		
		ImGui::PopStyleVar(2);
		return changed;
	}

	bool FancyDropdown(const char* a_id, const char* a_tooltip, uint8_t& a_currentItem, const std::vector<std::string>& a_items, float a_width)
	{
		auto tempIndex = static_cast<int>(a_currentItem);
		return FancyDropdown(a_id, a_tooltip, tempIndex, a_items, a_width) && (a_currentItem = static_cast<uint8_t>(tempIndex), true);
	}

	// TEST: Need to re-verify on a blank locale file to see how this behaves.
	// OPTIMIZE: Go over this one more time for handling vector out-of-range exceptions.
	bool FancyDropdown(const char* a_id, const char* a_tooltip, int& a_currentItem, const std::vector<std::string>& a_items, float a_width)
	{
		bool changed = false;

		ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 12.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8.0f, 8.0f));
		auto pos = ImGui::GetCursorScreenPos();

		if (a_width == 0.0f) {
			a_width = ImGui::GetContentRegionAvail().x;
		}

		if (a_currentItem < 0 || a_currentItem >= static_cast<int>(a_items.size())) {
			a_currentItem = 0;
		}

		std::string current_item;
		if (!a_items.empty() && Locale::GetSingleton()->HasEntry(a_items[a_currentItem].c_str())) {
			current_item = Translate(a_items[a_currentItem].c_str());
		} else {
			current_item = a_items[a_currentItem];
		}

		current_item = TRUNCATE(current_item, a_width * 0.8f);

		ImGui::SetNextItemWidth(a_width);
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, ImGui::GetFontSize()));
		if (ImGui::BeginCombo(a_id, current_item.c_str(), ImGuiComboFlags_NoArrowButton)) {
			ImGui::Spacing();
			for (size_t i = 0; i < a_items.size(); i++) {
				bool isSelected = (a_currentItem == static_cast<int>(i));
				auto flags = isSelected ? ImGuiSelectableFlags_Highlight : 0;
				if (ImGui::Selectable(Translate(a_items[i].c_str()), isSelected, flags)) {
					a_currentItem = static_cast<int>(i);
					changed = true;
				}

				if (isSelected) {
					ImGui::SetItemDefaultFocus();
				}
			}
			ImGui::Spacing();
			ImGui::EndCombo();
		}
		ImGui::PopStyleVar();

		if (Locale::GetSingleton()->HasEntry(a_tooltip)) {
			if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal | ImGuiHoveredFlags_NoSharedDelay)) {
				UICustom::FancyTooltip(a_tooltip);
			}
		}

		ImGui::SameLine();

		ImGui::PushFont(NULL, 18.0f);
		auto DrawList = ImGui::GetWindowDrawList();
		pos.x += a_width - ImGui::GetFrameHeightWithSpacing() + ImGui::GetStyle().FramePadding.x;
		pos.y += (ImGui::GetItemRectSize().y / 2.0f) - (ImGui::GetFontSize() / 2.0f);

		DrawList->AddText(pos, ThemeConfig::GetColorU32("TEXT", ImGui::GetStyle().Alpha), ICON_LC_SQUARE_CHEVRON_DOWN);
		ImGui::PopFont();

		ImGui::PopStyleVar(2);
		return changed;
	}

	void FancyTooltip(const char* a_localeString)
	{
		const float width = ImGui::GetIO().DisplaySize.x * 0.20f;

		float window_pos_x = ImGui::GetMousePos().x - width;
		float window_pos_y = ImGui::GetMousePos().y + ImGui::GetFrameHeight();

		if (window_pos_x < 0.0f) window_pos_x = 0.0f;

		ImGui::SetNextWindowSize(ImVec2(width, 0.0f));
		ImGui::SetNextWindowPos(ImVec2(window_pos_x, window_pos_y));
		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 5.0f);
		if (ImGui::BeginTooltip()) {
			const auto& drawList = ImGui::GetWindowDrawList();
			const ImVec2 pos = ImGui::GetCursorScreenPos();
			const ImVec2 size = ImGui::GetWindowSize();
			
			ImGui::TextWrapped("%s", Translate(a_localeString));

			drawList->AddRectFilled(
				ImVec2(pos.x - ImGui::GetStyle().WindowPadding.x, pos.y + (ImGui::GetFontSize() * 1.5f)),
				ImVec2(pos.x + size.x, pos.y + (ImGui::GetFontSize() * 1.5f) + 1.0f),
				ThemeConfig::GetColorU32("PRIMARY"));

			ImGui::EndTooltip();
		}
		ImGui::PopStyleVar();
	}

	bool BeginTabBar(const char* a_id, float a_height, float a_offset, ImVec2& a_start)
	{
		const ImVec2 window_padding = ImGui::GetStyle().WindowPadding;
		const float tab_height = a_height + window_padding.y;

		ImGui::SameLine();
		ImGui::SetCursorPosX(window_padding.x + a_offset);
		ImGui::SetCursorPosY(window_padding.y);

		a_start.y += ImGui::GetFrameHeightWithSpacing() + window_padding.y;
		
		return ImGui::BeginChild(a_id, ImVec2(0.0f, tab_height), false, false);
	}

	bool IconButton(const char* a_icon, const char* a_tooltip, bool& a_toggle) 
	{
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0, 0, 0, 0));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0, 0, 0, 0));
		ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0, 0, 0, 0));
		ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0, 0, 0, 0));
		ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0, 0, 0, 0));
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));

		float size = ImGui::GetFrameHeightWithSpacing();
		bool pressed = ImGui::Button(a_icon, ImVec2(size, size));

		if (pressed) {
			a_toggle = !a_toggle;
		}

		ImGui::PopStyleColor(6);
		ImGui::PopStyleVar(1);

		if (a_tooltip[0] != '\0') {
			if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal | ImGuiHoveredFlags_NoSharedDelay)) {
				FancyTooltip(a_tooltip);
			}
		}

		return pressed;
	}


	// Exclusively used in the Frame Sidebar window.
	bool SidebarImageButton(const std::string& a_text, const std::string& a_icon, bool a_selected, ImVec2 a_buttonSize, float& a_textMod, bool a_expanded)
	{
		ImGui::PushID(a_text.c_str());
		const ImVec2 pos = ImGui::GetCursorPos();
		const ImVec2 glyph_offset = ImVec2(4.0f, 2.0f);
		const float spacing = ImGui::GetFrameHeight();

		ImGui::SetNextItemAllowOverlap();
		bool pressed = ImGui::Selectable("", a_selected, 0, a_buttonSize);
		bool hovered = ImGui::IsItemHovered();
		bool held = ImGui::IsItemActive();

		ImGui::SetCursorPos(pos);
		ImGui::SetCursorPosY(pos.y + (a_buttonSize.y / 2.0f) - (ImGui::GetFontSize() / 2.0f) - glyph_offset.y);

		if (!a_expanded)
		{
			ImGui::SetCursorPosX(UICustom::GetCenterTextPosX(a_icon) - glyph_offset.x);
		}

		ImGui::PushFont(NULL, ImGui::GetFontSize() + 8.0f);
		ImGui::Text("%s", a_icon.c_str());
		ImGui::PopFont();

		const float anim_step = 0.05f;
		if (hovered || held) {
			if (a_textMod < 1.0f) {
				a_textMod += anim_step;
			} else {
				a_textMod = 1.0f;
			}
		} else {
			if (a_textMod > 0.0f) {
				a_textMod -= anim_step;
			} else {
				a_textMod = 0.0f;
			}
		}

		if (!a_expanded)
		{
			ImGui::PopID();
			ImGui::Spacing();
			return pressed;
		}
		else {
			ImGui::SameLine();
			ImGui::SetCursorPosY(ImGui::GetCursorPosY() + glyph_offset.y);
			ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (spacing * a_textMod));

			if (a_selected) ImGui::PushFontBold();
			ImGui::Text("%s", a_text.c_str()); 
			if (a_selected) ImGui::PopFont();

			ImGui::PopID();
			ImGui::Spacing();
			return pressed;
		}
	};

	bool ToggleButton(const char* a_id, bool& a_toggle, float a_width)
	{
		ImGui::PushID(a_id);
		const std::string& button_color = a_toggle == true ? "CONFIRM" : "DECLINE";
		const std::string button_text = a_toggle == true ? Translate("ON") : Translate("OFF"); 

		ImGui::PushStyleColor(ImGuiCol_Button, ThemeConfig::GetColor(button_color));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ThemeConfig::GetColor(button_color));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ThemeConfig::GetColor(button_color));
		bool pressed = ImGui::Button(button_text.c_str(), ImVec2(a_width, 0));
		ImGui::PopStyleColor(3);

		if (pressed) {
			a_toggle = !a_toggle;
		}

		ImGui::PopID();
		return pressed;
	}

	bool Settings_ToggleButton(const char* a_localeString, bool& a_toggle)
	{
		ImGui::Indent();

		auto id = "##Settings::ToggleButton::" + std::string(a_localeString);
		
		ImGui::PushID(id.c_str());
		const std::string& button_color = a_toggle == true ? "CONFIRM" : "DECLINE";
		const std::string button_text = a_toggle == true ? Translate("ON") : Translate("OFF"); 

		ImGui::AlignTextToFramePadding();
		ImGui::Text("%s", Translate(a_localeString));
		ImGui::HelpMarker(a_localeString);

		ImGui::SameLine(ImGui::GetContentRegionAvail().x - s_widgetWidth);
		ImGui::PushStyleColor(ImGuiCol_Button, ThemeConfig::GetColor(button_color));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ThemeConfig::GetHover(button_color));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ThemeConfig::GetActive(button_color));
		bool pressed = ImGui::Button(button_text.c_str(), ImVec2(s_widgetWidth, 0.0f));
		ImGui::PopStyleColor(3);

		if (pressed) {
			a_toggle = !a_toggle;
		}

		ImGui::PopID();
		ImGui::Unindent();
		return pressed;
	}

	bool Settings_SliderInt(const char* a_localeString, int& a_value, int a_min, int a_max)
	{
		ImGui::Indent();

		auto id = "##Settings::SliderInt::" + std::string(a_localeString);

		ImGui::AlignTextToFramePadding();
		ImGui::Text("%s", Translate(a_localeString));
		ImGui::HelpMarker(a_localeString);

		ImGui::SameLine(ImGui::GetContentRegionAvail().x - s_widgetWidth);
		ImGui::SetNextItemWidth(s_widgetWidth);
		bool pressed = ImGui::SliderInt(id.c_str(), &a_value, a_min, a_max);

		ImGui::Unindent();
		return pressed;
	}

	bool Settings_SliderFloat(const char* a_localeString, float& a_valRef, float a_min, float a_max)
	{
		ImGui::Indent();

		auto id = "##Settings::SliderFloat" + std::string(a_localeString);

		bool changes = false;
		ImGui::AlignTextToFramePadding();
		ImGui::Text("%s", Translate(a_localeString));
		ImGui::HelpMarker(a_localeString);

		ImGui::SameLine(ImGui::GetContentRegionAvail().x - s_widgetWidth);
		ImGui::SetNextItemWidth(s_widgetWidth);
		if (ImGui::SliderFloat(id.c_str(), &a_valRef, a_min, a_max, "%.3f", ImGuiSliderFlags_ClampOnInput)) {
			changes = true;
		}
		ImGui::Unindent();

		return changes;
	}

	void Settings_Keybind(const char* a_title, const char* a_desc, uint32_t& a_keybind, uint32_t defaultKey, bool a_modifierOnly)
	{
		ImGui::Indent();

		auto id = "##Keybind" + std::string(a_title);

		ImGui::SetCursorPosY(ImGui::GetCursorPosY() + (ImGui::GetFrameHeightWithSpacing() / 2.0f)); 
		ImGui::AlignTextToFramePadding();
		ImGui::Text("%s", Translate(a_title));

		ImGui::HelpMarker(a_title);

		if (GraphicManager::imgui_library.empty()) {
			const float height = ImGui::GetFrameHeight() * 2.0f;
			const std::string label = a_keybind == 0 ? Translate("NONE") : KeyCode::SkyrimKeymap.at(a_keybind);

			ImGui::SameLine(ImGui::GetContentRegionAvail().x - s_widgetWidth);  // Right Align
			ImGui::SetCursorPosY(ImGui::GetCursorPosY() - (ImGui::GetFrameHeight() / 2.0f));

			if (ImGui::Button(label.c_str(), ImVec2(s_widgetWidth, height))) {
				UIManager::GetSingleton()->ShowHotkey(Translate(a_title), Translate(a_desc), &a_keybind, defaultKey, a_modifierOnly, [&]() {
					UserConfig::GetSingleton()->SaveSettings();
				});
			}
		} else {
			GraphicManager::Image img;

			const auto keyIt = KeyCode::SkyrimKeymap.find(a_keybind);
			if (a_keybind == 0 || keyIt == KeyCode::SkyrimKeymap.end()) {
				img = GraphicManager::imgui_library.at("UnknownKey");
			} else {
				const std::string& keyName = keyIt->second;
				img = GraphicManager::imgui_library.at(keyName);
			}

			// Scale the image to FrameHeight, while keeping aspect ratio maintained.
			const float imageRatio = static_cast<float>(img.width) / static_cast<float>(img.height);
			const float imageWidth = (ImGui::GetFrameHeight() * 2.0f) * imageRatio;
			const float imageHeight = ImGui::GetFrameHeight() * 2.0f;
			const ImVec2 size = ImVec2(imageWidth, imageHeight);

			ImGui::SameLine(ImGui::GetContentRegionAvail().x - imageWidth);

			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.f, 0.f, 0.f, 0.f));
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.f, 1.f, 1.f, 0.05f));
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.f, 0.f, 0.f, 0.f));
			ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.f, 0.f, 0.f, 0.f));
			ImGui::PushStyleColor(ImGuiCol_BorderShadow, ImVec4(0.f, 0.f, 0.f, 0.f));

			ImTextureID texture = (ImTextureID)(intptr_t)img.texture;
			ImGui::SetCursorPosY(ImGui::GetCursorPosY() - (ImGui::GetFrameHeight() / 2.0f));
			if (ImGui::ImageButton(id.c_str(), texture, size)) {
				UIManager::GetSingleton()->ShowHotkey(Translate(a_title), Translate(a_desc), &a_keybind, defaultKey, a_modifierOnly, []() {
						UserConfig::GetSingleton()->SaveSettings();
				});
			}

			if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNone)) {
				if (ImGui::IsKeyPressed(ImGuiKey_T, false)) {
					a_keybind = defaultKey;
					UserConfig::GetSingleton()->SaveSettings();
				}
			}

			ImGui::PopStyleColor(5);
		}
		ImGui::Unindent();
	}

	bool Settings_Dropdown(const char* a_localeString, uint32_t& a_value, const std::vector<std::string>& a_options, bool a_localizeList)
	{
		ImGui::Indent();

		auto id = "##Settings::Dropdown" + std::string(a_localeString);

		bool result = false;
		ImGui::AlignTextToFramePadding();
		ImGui::Text("%s", Translate(a_localeString));
		ImGui::HelpMarker(a_localeString);

		ImGui::SameLine(ImGui::GetContentRegionAvail().x - s_widgetWidth);
		ImGui::SetNextItemWidth(s_widgetWidth);
		if (ImGui::BeginCombo(id.c_str(), Translate(a_options[a_value].c_str()))) {
			for (uint8_t i = 0; i < a_options.size(); ++i) {
				const char* entry = a_localizeList ? Translate(a_options[i].c_str()) : a_options[i].c_str();
				if (ImGui::Selectable(entry)) {
					a_value = i;
					result = true;
				}
			}
			ImGui::EndCombo();
		}
		ImGui::Unindent();

		return result;
	}

	bool Settings_FontDropdown(const char* a_localeString, std::string* a_font)
	{
		ImGui::Indent();

		auto id = "##Settings::FontDropdown" + std::string(a_localeString);

		bool result = false;
		ImGui::AlignTextToFramePadding();
		ImGui::Text("%s", Translate(a_localeString));
		ImGui::HelpMarker(a_localeString);

		ImGui::SameLine(ImGui::GetContentRegionAvail().x - s_widgetWidth);
		
		ImGui::SetNextItemWidth(s_widgetWidth);
		const auto fontLibrary = FontManager::GetSingleton()->GetFontLibrary();
		if (ImGui::BeginCombo(id.c_str(), a_font->c_str())) {
			ImGui::PushID("##Settings::FontDropdown::Combo");
			for (const auto& font : fontLibrary) {
				if (ImGui::Selectable(font.name.c_str())) {
					*a_font = font.name;

					FontManager::GetSingleton()->SetFont(font.filepath);
					result = true;
				}
			}
			ImGui::PopID();
			ImGui::EndCombo();
		}
		ImGui::Unindent();

		return result;
	}

	bool Settings_LanguageDropdown(const char* a_localeString, std::string* a_config)
	{
		ImGui::Indent();

		auto id = "##Settings::LanguageDropdown" + std::string(a_localeString);

		bool result = false;
		ImGui::AlignTextToFramePadding();
		ImGui::Text("%s", Translate(a_localeString));
		ImGui::HelpMarker(a_localeString);

		ImGui::SameLine(ImGui::GetContentRegionAvail().x - s_widgetWidth);
		ImGui::SetNextItemWidth(s_widgetWidth);
		const auto languages = Locale::GetSingleton()->GetLanguages();
		if (ImGui::BeginCombo(id.c_str(), a_config->c_str())) {
			ImGui::PushID("##Settings::Language::Combo");
			for (const auto& language : languages) {
				if (ImGui::Selectable(language.c_str())) {
					const auto filepath = Locale::GetSingleton()->GetFilepath(language);
					if (!filepath.empty()) {
						Locale::GetSingleton()->SetFilePath(filepath);
						bool success = Locale::GetSingleton()->Load(false);
						
						if (success) {
							*a_config = language;
							result = true;
						}
					}
				}
			}
			ImGui::PopID();
			ImGui::EndCombo();
		}
		ImGui::Unindent();

		return result;
	}
	
	bool Settings_ThemeDropdown(const char* a_localeString, std::string* a_config)
	{
		ImGui::Indent();

		auto id = "##Settings::ThemeDropdown" + std::string(a_localeString);

		bool result = false;
		ImGui::AlignTextToFramePadding();
		ImGui::Text("%s", Translate(a_localeString));
		
		ImGui::SameLine(ImGui::GetContentRegionAvail().x - s_widgetWidth);
		ImGui::SetNextItemWidth(s_widgetWidth);

		std::vector<ModexTheme> themes = ThemeConfig::GetAvailableThemes();
		if (ImGui::BeginCombo("##ThemeSelection", a_config->c_str())) {
			for (size_t i = 0; i < themes.size(); ++i) {
				if (ImGui::Selectable(Translate(themes[i].m_name.c_str()))) {
					result = ThemeConfig::GetSingleton()->LoadTheme(themes[i]);

					if (result) {
						*a_config = themes[i].m_name;
					}
				}
			}
			ImGui::EndCombo();
		}
		ImGui::Unindent();

		return result;
	}

	void Settings_Header(const char* a_localeString)
	{
		ImGui::PushFontBold();
		ImGui::PushStyleColor(ImGuiCol_Text, ThemeConfig::GetColor("PRIMARY"));
		ImGui::SeparatorText(Translate(a_localeString));
		ImGui::PopStyleColor();
		ImGui::PopFont();
		ImGui::NewLine();
	}

	// Draws a header bar inline with a square X button to the right to close the menu
	bool Popup_MenuHeader(const char* label) 
	{
		ImGui::BeginGroup();

		auto DrawList = ImGui::GetWindowDrawList();
		float windowWidth = ImGui::GetWindowWidth();
		const ImVec2 padding = ImGui::GetStyle().WindowPadding;
		ImVec2 p = ImGui::GetCursorScreenPos();
		p.x -= padding.x;
		p.y -= padding.y;
		

		ImVec2 bb = ImVec2(windowWidth, ImGui::GetFrameHeight() + padding.y);
		ImU32 col_a = ImGui::GetColorU32(ImGuiCol_Header, 0.25f);
		ImU32 col_b = ImGui::GetColorU32(ImGuiCol_HeaderActive, 0.25f);
		DrawList->AddRectFilledMultiColor(ImVec2(p.x, p.y), ImVec2(p.x + bb.x, p.y + bb.y), col_a, col_a, col_b, col_b);
		DrawList->AddLine(ImVec2(p.x, p.y + bb.y), ImVec2(p.x + bb.x, p.y + bb.y), ImGui::GetColorU32(ImGuiCol_Border));
		
		ImGui::Text("%s", label);
		ImGui::SameLine(windowWidth - bb.y - padding.x);
		
		ImGui::SetCursorPosY(0);
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.0f, 1.0f, 1.0f, 0.25f));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(1.0f, 1.0f, 1.0f, 0.25f));
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
		
		bool pressed = ImGui::Button(" " ICON_LC_X, ImVec2(bb.y, bb.y));
		ImGui::Spacing();

		ImGui::PopStyleColor(3);
		ImGui::PopStyleVar();

		ImGui::EndGroup();


		return pressed;
	}

	bool Popup_ConfirmDeclineButtons(bool &a_confirm, bool &a_cancel, bool m_navAccept)
	{
		const float button_width = (ImGui::GetContentRegionAvail().x - ImGui::GetStyle().ItemSpacing.x) * 0.5f;

		ImGui::PushStyleColor(ImGuiCol_Button, m_navAccept ? ThemeConfig::GetHover("CONFIRM") : ThemeConfig::GetColor("CONFIRM"));
		bool confirm = ImGui::Button(Translate("CONFIRM"), ImVec2(button_width, ImGui::GetFrameHeightWithSpacing()));
		ImGui::PopStyleColor();
		
		ImGui::SameLine();

		ImGui::PushStyleColor(ImGuiCol_Button, m_navAccept ? ThemeConfig::GetColor("DECLINE") : ThemeConfig::GetHover("DECLINE"));
		bool decline = ImGui::Button(Translate("CANCEL"), ImVec2(button_width, ImGui::GetFrameHeightWithSpacing()));
		ImGui::PopStyleColor();

		if (confirm) {
			a_cancel = false;
			return a_confirm = true;
		}

		if (decline) {
			a_confirm = false;
			return a_cancel = true;
		}

		return false;
	}
}

namespace ImGui
{
	static void HelpMarker(const std::string& a_localeString)
	{
		const char* tooltip = Modex::Locale::GetSingleton()->GetTooltip(a_localeString.c_str());

		if (tooltip && tooltip[0] != '\0') {
			ImGui::SameLine();
			ImGui::TextDisabled(ICON_LC_MESSAGE_CIRCLE_QUESTION);

			if (ImGui::IsItemHovered(ImGuiHoveredFlags_NoSharedDelay | ImGuiHoveredFlags_DelayNone)) {
				Modex::UICustom::FancyTooltip(tooltip);
			}
		}
	}
}
