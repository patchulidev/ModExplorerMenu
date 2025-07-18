#include "include/U/UserSettings.h"
#include "include/A/AddItem.h"
#include "include/D/Data.h"
#include "include/G/Graphic.h"
#include "include/K/Keycode.h"
#include "include/M/Menu.h"
#include "include/N/NPC.h"
#include "include/O/Object.h"
#include "include/P/Persistent.h"
#include "include/T/Teleport.h"
#include "include/U/UIManager.h"
#include "include/U/Util.h"

// TODO: God this is a mess. It started out strong, but as I've added more and more settings without
// much consideration into scale, it's bad. I need to re-do this entireley ASAP.

namespace Modex
{
	//
	// # Begin Helpers
	//

	static constexpr float p_fixedWidth = 150.0f;
	static constexpr float p_padding = 0.0f;  // placeholder

	void AddColorPicker(const char* a_text, ImVec4& a_colRef)
	{
		constexpr ImGuiColorEditFlags flags = ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel | ImGuiColorEditFlags_AlphaBar;
		auto id = "##ColorPicker" + std::string(_T(a_text));
		auto popup = id + "-Popup";
		ImGui::Spacing();
		ImGui::Text(_T(a_text));
		ImGui::SameLine(ImGui::GetContentRegionMax().x - p_fixedWidth - p_padding - ImGui::GetStyle().IndentSpacing);

		if (ImGui::ColorButton(id.c_str(), a_colRef, flags, ImVec2(p_fixedWidth, 0))) {
			ImGui::OpenPopup(popup.c_str());
		}

		if (ImGui::BeginPopup(popup.c_str())) {
			if (ImGui::ColorPicker4(id.c_str(), (float*)&a_colRef, flags)) {
				SettingsWindow::changes.store(true);
				SettingsWindow::file_changes.store(true);
			}
			ImGui::EndPopup();
		}

		ImGui::Spacing();

		// ImGui::PushItemWidth(50.0f);
		// ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
		// if (ImGui::ColorEdit4(id.c_str(), (float*)&a_colRef, flags)) {
		// 	SettingsWindow::changes.store(true);
		// 	SettingsWindow::file_changes.store(true);
		// }
	}

	void AddSliderPicker(const char* a_text, float& a_valRef, float a_min, float a_max)
	{
		auto id = "##SliderPicker" + std::string(a_text);
		ImGui::Spacing();
		ImGui::Text(_T(a_text));
		ImGui::SameLine(ImGui::GetContentRegionMax().x - p_fixedWidth - p_padding - ImGui::GetStyle().IndentSpacing);
		ImGui::SetNextItemWidth(p_fixedWidth);
		if (ImGui::SliderFloat(id.c_str(), &a_valRef, a_min, a_max)) {
			SettingsWindow::changes.store(true);
			SettingsWindow::file_changes.store(true);
		}
		ImGui::Spacing();
	}

	void AddDualSlider(const char* a_text, float& a_valRef_a, float& a_valRef_b, float a_min, float a_max)
	{
		auto id = "##DualSlider" + std::string(a_text);
		float new_size[] = { a_valRef_a, a_valRef_b };
		ImGui::Spacing();
		ImGui::Text(_T(a_text));
		ImGui::SameLine(ImGui::GetContentRegionMax().x - p_fixedWidth - p_padding - ImGui::GetStyle().IndentSpacing);
		ImGui::SetNextItemWidth(p_fixedWidth);
		if (ImGui::SliderFloat2(id.c_str(), new_size, a_min, a_max, "%.1f")) {
			a_valRef_a = new_size[0];
			a_valRef_b = new_size[1];
			SettingsWindow::changes.store(true);
			SettingsWindow::file_changes.store(true);
		}
		ImGui::Spacing();
	}

	void AddKeybind(const char* a_text, uint32_t& a_keybind, uint32_t defaultKey, bool a_modifierOnly, ImVec4& a_hover)
	{
		Settings::Config& config = Settings::GetSingleton()->GetConfig();

		auto id = "##Keybind" + std::string(a_text);
		ImGui::Spacing();
		ImGui::SetCursorPosY(ImGui::GetCursorPosY() + ImGui::GetStyle().ItemSpacing.y / 2 + ImGui::GetFontSize() / 2);
		ImGui::Text(_T(a_text));

		if (GraphicManager::imgui_library.empty()) {
			const float height = ((config.uiScaleVertical / 100) * 20.0f) + 10.0f;

			ImGui::SameLine(ImGui::GetContentRegionMax().x - p_fixedWidth - p_padding - ImGui::GetStyle().IndentSpacing);  // Right Align
			ImGui::SetCursorPosY(ImGui::GetCursorPosY() - (height / 2) + 5.0f);                                            // Center Align

			ImGui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign, ImVec2(0.5f, 0.5f));
			if (ImGui::Button(ImGui::SkyrimKeymap.at(a_keybind), ImVec2(p_fixedWidth, height + 5.0f))) {
				UIManager::GetSingleton()->ShowHotkey(&a_keybind, defaultKey, a_modifierOnly, [&]() {
					Settings::GetSingleton()->SaveSettings();
				});
			}
			ImGui::PopStyleVar();
		} else {
			const ImVec2& uv0 = ImVec2(0, 0);
			const ImVec2& uv1 = ImVec2(1, 1);
			const ImVec4& bg_col = ImVec4(0, 0, 0, 0);
			const float alpha = 1.0f;

			GraphicManager::Image img = GraphicManager::imgui_library[ImGui::ImGuiKeymap.at(a_keybind)];

			float scale = config.uiScaleVertical / 100.0f;
			const float imageWidth = ((float)img.width * 0.5f) * scale;
			const float imageHeight = ((float)img.height * 0.5f) * scale;
			const ImVec2& size = ImVec2(imageWidth, imageHeight);

			// ImGui::SameLine(ImGui::GetContentRegionMax().x - (p_fixedWidth - p_padding) + imageWidth / scale);  // Right Align
			ImGui::SameLine(ImGui::GetContentRegionMax().x - ImGui::GetStyle().IndentSpacing - p_padding - imageWidth);
			ImGui::SetCursorPosY(ImGui::GetCursorPosY() - (imageHeight / 2) + 5.0f);  // Center Align

			ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * alpha);
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.f, 0.f, 0.f, 0.f));
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.f, 0.f, 0.f, 0.f));
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.f, 0.f, 0.f, 0.f));
			ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.f, 0.f, 0.f, 0.f));
			ImGui::PushStyleColor(ImGuiCol_BorderShadow, ImVec4(0.f, 0.f, 0.f, 0.f));

			ImTextureID texture = (ImTextureID)(intptr_t)img.texture;
			if (ImGui::ImageButton(id.c_str(), texture, size, uv0, uv1, bg_col, a_hover)) {
				UIManager::GetSingleton()->ShowHotkey(&a_keybind, defaultKey, a_modifierOnly, [&]() {
					Settings::GetSingleton()->SaveSettings();
				});
			}

			if (ImGui::IsItemHovered()) {
				a_hover = ImVec4(1.f, 1.f, 1.f, 1.f);
			} else {
				a_hover = ImVec4(0.9f, 0.9f, 0.9f, 0.9f);
			}

			ImGui::PopStyleVar();
			ImGui::PopStyleColor(5);
		}

		ImGui::Spacing();
	}

	bool AddToggleButton(const char* a_text, bool& a_boolRef)
	{
		// const float width = ImGui::GetFrameHeight() * 3.55f;
		bool result = false;

		ImGui::Spacing();
		ImGui::Text(_T(a_text));
		ImGui::SameLine(ImGui::GetContentRegionMax().x - p_fixedWidth - p_padding - ImGui::GetStyle().IndentSpacing);
		if (ImGui::ToggleButton(a_text, &a_boolRef, p_fixedWidth)) {
			result = true;
		}
		ImGui::Spacing();

		return result;
	}

	bool AddSelectionDropdown(const char* a_text, int& a_selection, const std::vector<std::string>& a_items)
	{
		bool result = false;
		auto id = "##SelectionDropdown" + std::string(a_text);
		ImGui::Spacing();
		ImGui::Text(_T(a_text));
		ImGui::SameLine(ImGui::GetContentRegionMax().x - p_fixedWidth - p_padding - ImGui::GetStyle().IndentSpacing);
		ImGui::PushItemWidth(p_fixedWidth);
		if (ImGui::BeginCombo(id.c_str(), _T(a_items[a_selection]))) {
			for (int i = 0; i < a_items.size(); ++i) {
				if (ImGui::Selectable(_T(a_items[i]))) {
					a_selection = i;
					// SettingsWindow::changes.store(true);
					// SettingsWindow::file_changes.store(true);
					result = true;
				}
			}
			ImGui::EndCombo();
		}
		ImGui::Spacing();
		ImGui::PopItemWidth();

		return result;
	}

	bool AddImageDropdown(const char* a_text, GraphicManager::Image* a_imageRef)
	{
		auto id = "##ImageDropdown" + std::string(a_text);
		auto imageName = GraphicManager::GetImageName(*a_imageRef);
		bool result = false;
		constexpr auto flags = ImGuiComboFlags_None;

		ImGui::Spacing();
		ImGui::Text(_T(a_text));
		ImGui::SameLine(ImGui::GetContentRegionMax().x - p_fixedWidth - p_padding - ImGui::GetStyle().IndentSpacing);
		ImGui::PushItemWidth(p_fixedWidth);
		if (ImGui::BeginCombo(id.c_str(), imageName.c_str(), flags)) {
			auto images = GraphicManager::GetListOfImages();
			ImGui::PushID("##ImageSelectionPopup");
			for (const auto& image : images) {
				if (ImGui::Selectable(image.first.c_str())) {
					*a_imageRef = image.second;
					result = true;
					SettingsWindow::changes.store(true);
					SettingsWindow::file_changes.store(true);
				}
			}
			ImGui::PopID();
			ImGui::EndCombo();
		}
		ImGui::Spacing();
		ImGui::PopItemWidth();

		return result;
	}

	bool AddFontDropdown(const char* a_text, std::string* a_font)
	{
		auto id = "##FontDropdown" + std::string(a_text);
		bool result = false;

		ImGui::Spacing();
		ImGui::Text(_T(a_text));
		ImGui::SameLine(ImGui::GetContentRegionMax().x - p_fixedWidth - p_padding - ImGui::GetStyle().IndentSpacing);
		ImGui::PushItemWidth(p_fixedWidth);
		if (ImGui::BeginCombo(id.c_str(), a_font->c_str())) {
			auto fontLibrary = FontManager::GetFontLibrary();
			ImGui::PushID("##FontSelectionPopup");
			for (const auto& font : fontLibrary) {
				if (ImGui::Selectable(font.c_str())) {
					*a_font = font;

					// SettingsWindow::changes.store(true);
					// SettingsWindow::file_changes.store(true);

					Menu::GetSingleton()->RefreshFont();
					result = true;
				}
			}
			ImGui::PopID();
			ImGui::EndCombo();
		}
		ImGui::Spacing();
		ImGui::PopItemWidth();

		return result;
	}

	void AddSubCategoryHeader(const char* a_text)
	{
		ImGui::Unindent();
		ImGui::SubCategoryHeader(_T(a_text));
		ImGui::Indent();
		ImGui::Spacing();
	}

	//
	// # End Helpers
	//

	static inline ImVec4 keyHoverTintColor = ImVec4(0.9f, 0.9f, 0.9f, 0.9f);
	static inline ImVec4 modifierHoverTint = ImVec4(0.9f, 0.9f, 0.9f, 0.9f);
	void SettingsWindow::DrawGeneralSettings()
	{
		// Settings::Style& style = Settings::GetSingleton()->GetStyle();
		Settings::Config& config = Settings::GetSingleton()->GetConfig();

		if (changes.load()) {
			changes.store(false);
		}

		AddSubCategoryHeader(_T("SETTING_GENERAL"));

		AddKeybind("SETTING_MENU_KEYBIND", config.showMenuKey, 211, false, keyHoverTintColor);

		ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal);

		AddKeybind("SETTING_MENU_MODIFIER", config.showMenuModifier, 0, true, modifierHoverTint);

		ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal);

		std::vector<std::string> sorts = { "SETTING_SORT_ALPHA", "SETTING_SORT_LOAD_ASC", "SETTING_SORT_LOAD_DESC" };
		if (AddSelectionDropdown("SETTING_SORT", config.modListSort, sorts)) {
			Settings::GetSingleton()->SaveSettings();
		}

		ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal);

		// UI Scale Setting
		ImGui::Spacing();
		ImGui::Text(_T("SETTINGS_UI_SCALE_VERTICAL"));
		ImGui::SameLine(ImGui::GetContentRegionMax().x - p_fixedWidth - p_padding - ImGui::GetStyle().IndentSpacing);
		ImGui::PushItemWidth(p_fixedWidth);
		ImGui::SliderInt("##UIVerticalScaleSelection", &_uiScaleVertical, 50, 150, "%d%%");

		if (ImGui::IsItemDeactivatedAfterEdit()) {
			config.uiScaleVertical = _uiScaleVertical;

			// SettingsWindow::changes.store(true);
			// SettingsWindow::file_changes.store(true);
			Settings::GetSingleton()->SaveSettings();
		}
		ImGui::Spacing();
		ImGui::PopItemWidth();

		ImGui::Spacing();
		ImGui::Text(_T("SETTINGS_UI_SCALE_HORIZONTAL"));
		ImGui::SameLine(ImGui::GetContentRegionMax().x - p_fixedWidth - p_padding - ImGui::GetStyle().IndentSpacing);
		ImGui::PushItemWidth(p_fixedWidth);
		ImGui::SliderInt("##UIHorizontalScaleSelection", &_uiScaleHorizontal, 50, 150, "%d%%");

		if (ImGui::IsItemDeactivatedAfterEdit()) {
			config.uiScaleHorizontal = _uiScaleHorizontal;

			// SettingsWindow::changes.store(true);
			// SettingsWindow::file_changes.store(true);
			Settings::GetSingleton()->SaveSettings();
		}
		ImGui::Spacing();
		ImGui::PopItemWidth();
		// End UI Scale Setting

		ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal);

		if (AddToggleButton("SETTING_FULLSCREEN", config.fullscreen)) {
			Settings::GetSingleton()->SaveSettings();
		}

		ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal);

		if (AddToggleButton("SETTINGS_PAUSE_GAME", config.pauseGame)) {
			Settings::GetSingleton()->SaveSettings();
		}

		ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal);

		if (AddToggleButton("SETTINGS_DISABLE_IN_MENU", config.disableInMenu)) {
			Settings::GetSingleton()->SaveSettings();
		}

		ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal);

		std::vector<std::string> levels = { "trace", "debug", "info", "warn", "error", "critical" };

		ImGui::Spacing();
		ImGui::Text(_T("SETTING_LOG_LEVEL"));
		ImGui::SameLine(ImGui::GetContentRegionMax().x - p_fixedWidth - p_padding - ImGui::GetStyle().IndentSpacing);
		ImGui::PushItemWidth(p_fixedWidth);
		if (ImGui::BeginCombo("##LogLevelSelection", _T(levels[config.logLevel]))) {
			for (int i = 0; i < levels.size(); ++i) {
				if (ImGui::Selectable(_T(levels[i]))) {
					config.logLevel = static_cast<spdlog::level::level_enum>(i);
					Settings::GetSingleton()->SaveSettings();
				}
			}
			ImGui::EndCombo();
		}
		ImGui::Spacing();
		ImGui::PopItemWidth();

		AddSubCategoryHeader(_T("SETTING_FONT_AND_LANGUAGE"));

		// Language Dropdown
		ImGui::Spacing();
		ImGui::Text(_T("Language"));
		ImGui::SameLine(ImGui::GetContentRegionMax().x - p_fixedWidth - p_padding - ImGui::GetStyle().IndentSpacing);
		ImGui::PushItemWidth(p_fixedWidth);

		auto languages = Language::GetLanguages();

		if (ImGui::BeginCombo("##LanguageSelection", config.language.c_str())) {
			for (auto& language : languages) {
				if (ImGui::Selectable(language.c_str())) {
					config.language = language;

					// SettingsWindow::changes.store(true);
					// SettingsWindow::file_changes.store(true);

					Translate::GetSingleton()->RefreshLanguage(config.language);
					Settings::GetSingleton()->SaveSettings();
				}
			}
			ImGui::EndCombo();
		}
		ImGui::Spacing();
		ImGui::PopItemWidth();

		ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal);

		// Glpyh Dropdown
		ImGui::Spacing();
		ImGui::Text(_T("SETTING_GLYPH_RANGE"));
		ImGui::SameLine(ImGui::GetContentRegionMax().x - p_fixedWidth - p_padding - ImGui::GetStyle().IndentSpacing);
		ImGui::PushItemWidth(p_fixedWidth);

		auto glyphs = Language::GetListOfGlyphNames();
		auto currentGlyph = Language::GetGlyphName(config.glyphRange);

		if (ImGui::BeginCombo("##GlpyhSelection", currentGlyph.data())) {
			for (auto& glyph : glyphs) {
				if (ImGui::Selectable(glyph.data())) {
					config.glyphRange = Language::GetGlyphRange(glyph);

					// SettingsWindow::changes.store(true);
					// SettingsWindow::file_changes.store(true);

					Menu::GetSingleton()->RefreshFont();
					Settings::GetSingleton()->SaveSettings();
				}
			}
			ImGui::EndCombo();
		}
		ImGui::Spacing();
		ImGui::PopItemWidth();

		ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal);

		if (AddFontDropdown("SETTING_FONT", &config.globalFont)) {
			Settings::GetSingleton()->SaveSettings();
		}

		ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal);

		// Font Size Setting
		ImGui::Spacing();
		ImGui::Text(_T("SETTING_FONT_SIZE"));
		ImGui::SameLine(ImGui::GetContentRegionMax().x - p_fixedWidth - p_padding - ImGui::GetStyle().IndentSpacing);
		ImGui::PushItemWidth(p_fixedWidth);

		ImGui::SliderInt("##FontSizeSelection", &_fontSize, 8, 28, "%d");

		if (ImGui::IsItemDeactivatedAfterEdit()) {
			config.globalFontSize = static_cast<float>(_fontSize);

			// SettingsWindow::changes.store(true);
			// SettingsWindow::file_changes.store(true);

			Menu::GetSingleton()->RefreshFont();
			Settings::GetSingleton()->SaveSettings();
		}
		ImGui::Spacing();
		ImGui::PopItemWidth();

		// End of Text & Font Settings
		// Begin of Module settings

		AddSubCategoryHeader(_T("SETTING_MODULE"));

		if (AddSelectionDropdown("SETTING_DEFAULT_SHOW", config.defaultShow, { "Home", "Add Item", "Object", "NPC", "Teleport", "Settings" })) {
			Settings::GetSingleton()->SaveSettings();
		}

		ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal);

		if (AddToggleButton("SETTING_SHOW_HOME", config.showHomeMenu)) {
			Settings::GetSingleton()->SaveSettings();
		}

		ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal);

		if (AddToggleButton("SETTING_SHOW_ADDITEM", config.showAddItemMenu)) {
			Settings::GetSingleton()->SaveSettings();

			if (config.showAddItemMenu) {
				Data::GetSingleton()->GenerateItemList();
				AddItemWindow::GetSingleton()->Load();
			} else {
				AddItemWindow::GetSingleton()->Unload();
			}
		}

		ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal);

		if (AddToggleButton("SETTING_SHOW_OBJECT", config.showObjectMenu)) {
			Settings::GetSingleton()->SaveSettings();

			if (config.showObjectMenu) {
				Data::GetSingleton()->GenerateObjectList();
				ObjectWindow::GetSingleton()->Load();
			} else {
				ObjectWindow::GetSingleton()->Unload();
			}
		}

		ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal);

		if (AddToggleButton("SETTING_SHOW_NPC", config.showNPCMenu)) {
			Settings::GetSingleton()->SaveSettings();

			if (config.showNPCMenu) {
				Data::GetSingleton()->GenerateNPCList();
				Data::GetSingleton()->GenerateNPCClassList();
				Data::GetSingleton()->GenerateNPCFactionList();
				Data::GetSingleton()->GenerateNPCRaceList();
				NPCWindow::GetSingleton()->Load();
			} else {
				NPCWindow::GetSingleton()->Unload();
			}
		}

		ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal);

		if (AddToggleButton("SETTING_SHOW_TELEPORT", config.showTeleportMenu)) {
			Settings::GetSingleton()->SaveSettings();

			if (config.showTeleportMenu) {
				TeleportWindow::GetSingleton()->Load();
			} else {
				TeleportWindow::GetSingleton()->Unload();
			}
		}
	}

	void SettingsWindow::Draw()
	{
		const float button_width = ImGui::GetContentRegionAvail().x / static_cast<int>(Viewport::Count);
		const float button_height = ImGui::GetFontSize() * 1.5f;
		const float window_padding = ImGui::GetStyle().WindowPadding.y;
		const float tab_bar_height = button_height + (window_padding * 2.0f);

		ImGui::SameLine();
		ImGui::SetCursorPosY(window_padding);

		const ImVec2 backup_pos = ImGui::GetCursorPos();
		if (ImGui::BeginChild("##Settings::TabBar", ImVec2(0.0f, button_height), 0, ImGuiWindowFlags_NoFocusOnAppearing)) {
			ImGui::PushStyleVar(ImGuiStyleVar_SelectableTextAlign, ImVec2(0.5f, 0.5f));
			if (ImGui::Selectable(_T("SETTING_USER"), activeViewport == Viewport::UserSettings, 0, ImVec2(button_width, 0.0f))) {
				activeViewport = Viewport::UserSettings;
			}

			ImGui::PopStyleVar();
		}
		ImGui::EndChild();

		ImGui::SameLine();
		ImGui::SetCursorPos(backup_pos);
		ImGui::SetCursorPosY(tab_bar_height - window_padding);

		if (ImGui::BeginChild("##Settings::View", ImGui::GetContentRegionAvail(), ImGuiChildFlags_Borders)) {
			ImGui::Indent();
			if (activeViewport == Viewport::UserSettings) {
				DrawGeneralSettings();
			}
			ImGui::Unindent();
		}
		ImGui::EndChild();
	}

	// Called from XSEPlugin -> Frame::Install() -> Modules::Init()
	// This is important, because settings *must* be loaded before.
	void SettingsWindow::Init()
	{
		_uiScaleVertical = Settings::GetSingleton()->GetConfig().uiScaleVertical;
		_uiScaleHorizontal = Settings::GetSingleton()->GetConfig().uiScaleHorizontal;
		_fontSize = (int)Settings::GetSingleton()->GetConfig().globalFontSize;
	}
}
