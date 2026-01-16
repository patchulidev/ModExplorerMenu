#include "UICustom.h"

#include "core/Graphic.h"
#include "external/icons/IconsLucide.h"
#include "ui/core/UIManager.h"
#include "config/UserConfig.h"
#include "config/ThemeConfig.h"
#include "localization/Language.h"
#include "localization/Locale.h"


namespace Modex::UICustom
{
	static inline float p_fixedWidth = 150.0f; // TODO: FUCK YOU

	void AddFancyTooltip(const char* a_text)
	{
		const float width = ImGui::GetIO().DisplaySize.x * 0.20f;

		ImGui::SetNextWindowSize(ImVec2(width, 0.0f));
		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 5.0f);
		if (ImGui::BeginTooltip()) {
			const auto& drawList = ImGui::GetWindowDrawList();
			const ImVec2 pos = ImGui::GetCursorScreenPos();
			
			
			ImGui::TextWrapped("%s", Translate(a_text));
			
			const ImVec2 size = ImGui::GetWindowSize();

			// Draw a horizontal rule to separate the first line from the rest:
			drawList->AddRectFilled(
				ImVec2(pos.x - ImGui::GetStyle().WindowPadding.x, pos.y + (ImGui::GetFontSize() * 1.5f)),
				ImVec2(pos.x + size.x, pos.y + (ImGui::GetFontSize() * 1.5f) + 1.0f),
				ThemeConfig::GetColorU32("TOOLTIP_SEPARATOR"));

			ImGui::EndTooltip();
		}
		ImGui::PopStyleVar();
	}

	bool AddIconButton(const char* a_icon, const char* a_tooltip, bool& a_toggle) {
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0, 0, 0, 0));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0, 0, 0, 0));
		ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0, 0, 0, 0));
		ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0, 0, 0, 0));
		ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0, 0, 0, 0));
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));

		bool pressed = false;

		if (ImGui::Button(a_icon, ImVec2(ImGui::GetFrameHeightWithSpacing(), ImGui::GetFrameHeightWithSpacing()))) {
			a_toggle = !a_toggle;
			pressed = true;
		}

		ImGui::PopStyleColor(6);
		ImGui::PopStyleVar(1);

		if (a_tooltip[0] != '\0') {
			if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal | ImGuiHoveredFlags_NoSharedDelay)) {
				UICustom::AddFancyTooltip(a_tooltip);
			}
		}

		return pressed;
	}

	bool AddIconButtonBg(const char* a_icon, const char* a_tooltip, bool& a_toggle, const ImVec2& a_size, const ImVec4& a_bgColor, float a_offset = 0.0f)
	{
		(void)a_offset;

		ImGui::PushStyleColor(ImGuiCol_Button, a_bgColor);
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, a_bgColor + ImVec4(0.1f, 0.1f, 0.1f, 0.0f));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, a_bgColor + ImVec4(0.1f, 0.1f, 0.1f, 0.0f));
		ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0, 0, 0, 0));
		ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0, 0, 0, 0));
		ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0, 0, 0, 0));
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));

		bool pressed = false;

		if (ImGui::Button(a_icon, a_size)) {
			a_toggle = !a_toggle;
			pressed = true;
		}

		ImGui::PopStyleColor(6);
		ImGui::PopStyleVar(1);

		if (a_tooltip[0] != '\0') {
			if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal | ImGuiHoveredFlags_NoSharedDelay)) {
				UICustom::AddFancyTooltip(a_tooltip);
			}
		}

		return pressed;
	}

    bool AddColorPicker(const char* a_text, ImVec4& a_colRef)
	{
		constexpr ImGuiColorEditFlags flags = ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel | ImGuiColorEditFlags_AlphaBar;
		auto id = "##ColorPicker" + std::string(Translate(a_text));
		auto popup = id + "-Popup";
		bool change = false;

		ImGui::Spacing();
		ImGui::Text(Translate(a_text));
		ImGui::SameLine(ImGui::GetContentRegionMax().x - p_fixedWidth - ImGui::GetStyle().IndentSpacing);

		if (ImGui::ColorButton(id.c_str(), a_colRef, flags, ImVec2(p_fixedWidth, 0))) {
			ImGui::OpenPopup(popup.c_str());
		}

		if (ImGui::BeginPopup(popup.c_str())) {
			if (ImGui::ColorPicker4(id.c_str(), (float*)&a_colRef, flags)) {
				change = true;
			}
			ImGui::EndPopup();
		}
		
		ImGui::Spacing();
		
		return change;
	}

	bool AddSliderPicker(const char* a_text, float& a_valRef, float a_min, float a_max)
	{
		auto id = "##SliderPicker" + std::string(a_text);
		bool changes = false;

		ImGui::Spacing();
		ImGui::Text(Translate(a_text));
		ImGui::SameLine(ImGui::GetContentRegionMax().x - p_fixedWidth - ImGui::GetStyle().IndentSpacing);
		ImGui::SetNextItemWidth(p_fixedWidth);
		if (ImGui::SliderFloat(id.c_str(), &a_valRef, a_min, a_max)) {
			changes = true;
		}
		ImGui::Spacing();

		return changes;
	}

	bool AddDualSlider(const char* a_text, float& a_valRef_a, float& a_valRef_b, float a_min, float a_max)
	{
		auto id = "##DualSlider" + std::string(a_text);
		float new_size[] = { a_valRef_a, a_valRef_b };
		bool changes = false;
		
		ImGui::Spacing();
		ImGui::Text(Translate(a_text));
		ImGui::SameLine(ImGui::GetContentRegionMax().x - p_fixedWidth - ImGui::GetStyle().IndentSpacing);
		ImGui::SetNextItemWidth(p_fixedWidth);
		if (ImGui::SliderFloat2(id.c_str(), new_size, a_min, a_max, "%.1f")) {
			a_valRef_a = new_size[0];
			a_valRef_b = new_size[1];
			changes = true;
		}
		ImGui::Spacing();

		return changes;
	}

	bool AddKeybind(const char* a_text, uint32_t& a_keybind, uint32_t defaultKey, ImVec4& a_hover)
	{
		auto& config = UserConfig::Get();

		auto id = "##Keybind" + std::string(a_text);
		bool changes = false;

		ImGui::Spacing();
		ImGui::SetCursorPosY(ImGui::GetCursorPosY() + ImGui::GetStyle().ItemSpacing.y / 2 + ImGui::GetFontSize() / 2);
		ImGui::Text(Translate(a_text));

		if (GraphicManager::imgui_library.empty()) {
			const float height = ((config.uiScaleVertical / 100) * 20.0f) + 10.0f;

			ImGui::SameLine(ImGui::GetContentRegionMax().x - p_fixedWidth - ImGui::GetStyle().IndentSpacing);  // Right Align
			ImGui::SetCursorPosY(ImGui::GetCursorPosY() - (height / 2) + 5.0f);                                            // Center Align

			if (ImGui::Button(ImGui::SkyrimKeymap.at(a_keybind), ImVec2(p_fixedWidth, height + 5.0f))) {
				UIManager::GetSingleton()->ShowHotkey(&a_keybind, defaultKey, [&]() {
					changes = true;
				});
			}
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
			ImGui::SameLine(ImGui::GetContentRegionMax().x - ImGui::GetStyle().IndentSpacing - imageWidth);
			ImGui::SetCursorPosY(ImGui::GetCursorPosY() - (imageHeight / 2) + 5.0f);  // Center Align

			ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * alpha);
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.f, 0.f, 0.f, 0.f));
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.f, 0.f, 0.f, 0.f));
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.f, 0.f, 0.f, 0.f));
			ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.f, 0.f, 0.f, 0.f));
			ImGui::PushStyleColor(ImGuiCol_BorderShadow, ImVec4(0.f, 0.f, 0.f, 0.f));

			ImTextureID texture = (ImTextureID)(intptr_t)img.texture;
			if (ImGui::ImageButton(id.c_str(), texture, size, uv0, uv1, bg_col, a_hover)) {
				UIManager::GetSingleton()->ShowHotkey(&a_keybind, defaultKey, [&]() {
					changes = true;
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

		return changes;
	}

	bool AddToggleButton(const char* a_text, bool& a_boolRef)
	{
		// const float width = ImGui::GetFrameHeight() * 3.55f;
		bool result = false;

		ImGui::Spacing();
		ImGui::Text(Translate(a_text));
		ImGui::SameLine(ImGui::GetContentRegionMax().x - p_fixedWidth - ImGui::GetStyle().IndentSpacing);
		if (ImGui::ToggleButton(a_text, &a_boolRef, p_fixedWidth)) {
			result = true;
		}
		ImGui::Spacing();

		return result;
	}

	bool AddSelectionDropdown(const char* a_text, int& a_value, const std::vector<std::string>& a_options)
	{
		bool result = false;
		auto id = "##SelectionDropdown" + std::string(a_text);

		ImGui::Spacing();
		ImGui::Text(Translate(a_text));
		ImGui::SameLine(ImGui::GetContentRegionMax().x - p_fixedWidth - ImGui::GetStyle().IndentSpacing);
		ImGui::PushItemWidth(p_fixedWidth);
		if (ImGui::BeginCombo(id.c_str(), Translate(a_options[a_value].c_str()))) {
			for (int i = 0; i < a_options.size(); ++i) {
				if (ImGui::Selectable(Translate(a_options[i].c_str()))) {
					a_value = i;
					result = true;
				}
			}
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
		ImGui::Text(Translate(a_text));
		ImGui::SameLine(ImGui::GetContentRegionMax().x - p_fixedWidth - ImGui::GetStyle().IndentSpacing);
		ImGui::PushItemWidth(p_fixedWidth);
		if (ImGui::BeginCombo(id.c_str(), a_font->c_str())) {
			auto fontLibrary = FontManager::GetFontLibrary();
			ImGui::PushID("##FontSelectionPopup");
			for (const auto& font : fontLibrary) {
				if (ImGui::Selectable(font.c_str())) {
					*a_font = font;

					UIManager::GetSingleton()->RefreshFont();
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

	// Exclusively used in the Frame Sidebar window.
	bool SidebarButton(const char* a_text, bool a_selected, ImTextureID a_texture, ImVec2 a_imageSize, ImVec2 a_buttonSize, float& a_textMod)
	{
		ImGuiWindow* window = ImGui::GetCurrentWindow();
		if (window->SkipItems)
			return false;

		ImGuiContext& g = *GImGui;
		const ImGuiStyle& style = g.Style;
		const ImGuiID id = window->GetID(a_text);
		const ImVec2 label_size = ImGui::CalcTextSize(a_text, NULL, true);
		ImU32 image_color = a_selected ? ThemeConfig::GetColorU32("TEXT") : ThemeConfig::GetColorU32("TEXT", 0.25f);
		auto bg_color_1 = a_selected ? ThemeConfig::GetColorU32("SIDEBAR_A") : ThemeConfig::GetColorU32("SIDEBAR_A", 0.6f);
		auto bg_color_2 = ThemeConfig::GetColorU32("SIDEBAR_B", 0.6f);

		// Calculate size and bounding box of button element.
		ImVec2 pos = window->DC.CursorPos;
		ImVec2 size =
			ImGui::CalcItemSize(ImVec2(a_buttonSize.x, a_buttonSize.y), label_size.x + style.FramePadding.x * 2.0f,
				label_size.y + style.FramePadding.y * 2.0f);

		const ImRect bb(pos, ImVec2(pos.x + size.x, pos.y + size.y));
		ImGui::ItemSize(size, style.FramePadding.y);
		if (!ImGui::ItemAdd(bb, id))
			return false;

		// Emulate button behavior since we're not using ImGui::Button.
		ImGuiButtonFlags flags = ImGuiButtonFlags_None;

		bool hovered, held;
		bool expanded = (a_buttonSize.x) > a_imageSize.x * 4.0f;
		bool pressed = ImGui::ButtonBehavior(bb, id, &hovered, &held, flags);

		const float anim_step = 0.05f;

		if (hovered || held) {
			if (a_textMod < 0.5f) {
				a_textMod += anim_step;
			} else {
				a_textMod = 0.5f;
			}
		} else {
			if (a_textMod > 0.0f) {
				a_textMod -= anim_step;
			} else {
				a_textMod = 0.0f;
			}
		}

		// This code handles the gradient coloring.
		// https://github.com/ocornut/imgui/issues/4722
		const bool is_gradient = bg_color_1 != bg_color_2;
		if (held || hovered) {
			// Modify colors (ultimately this can be prebaked in the style)
			float h_increase = (held || hovered) ? 0.02f : 0.02f;
			float v_increase = (held || hovered) ? 10.0f : 0.07f;

			image_color = ImGui::GetColorU32(ImGuiCol_Text, 1.0f); // white

			ImVec4 bg1f = ImGui::ColorConvertU32ToFloat4(bg_color_1);
			ImGui::ColorConvertRGBtoHSV(bg1f.x, bg1f.y, bg1f.z, bg1f.x, bg1f.y, bg1f.z);
			bg1f.x = ImMin(bg1f.x + h_increase, 1.0f);
			bg1f.z = ImMin(bg1f.z + v_increase, 1.0f);
			ImGui::ColorConvertHSVtoRGB(bg1f.x, bg1f.y, bg1f.z, bg1f.x, bg1f.y, bg1f.z);
			bg_color_1 = ImGui::GetColorU32(bg1f);
			if (is_gradient) {
				ImVec4 bg2f = ImGui::ColorConvertU32ToFloat4(bg_color_2);
				ImGui::ColorConvertRGBtoHSV(bg2f.x, bg2f.y, bg2f.z, bg2f.x, bg2f.y, bg2f.z);
				bg2f.z = ImMin(bg2f.z + h_increase, 1.0f);
				bg2f.z = ImMin(bg2f.z + v_increase, 1.0f);
				ImGui::ColorConvertHSVtoRGB(bg2f.x, bg2f.y, bg2f.z, bg2f.x, bg2f.y, bg2f.z);
				bg_color_2 = ImGui::GetColorU32(bg2f);
			} else {
				bg_color_2 = bg_color_1;
			}
		}
		ImGui::RenderNavHighlight(bb, id);

		// Calculate image bounding box, relative to the underlying button.
		const ImVec2 image_size = ImVec2((float)a_imageSize.x * 0.80f, (float)a_imageSize.y * 0.80f);
		const float image_pos_height = bb.Min.y + (bb.GetHeight() - image_size.y) * 0.5f;  // always centered.
		const float image_pos_width_min = bb.Min.x + (image_size.x * 0.5f) + 6.0f;         // 6.0f since Icon is fixed padding.
		const float image_pos_width_max = image_pos_width_min + image_size.x;
		const ImVec2 image_pos_min = ImVec2(image_pos_width_min, image_pos_height);
		const ImVec2 image_pos_max = ImVec2(image_pos_width_max, image_pos_height + image_size.y);

		// Render

		int vert_start_idx = window->DrawList->VtxBuffer.Size;
		window->DrawList->AddRectFilled(bb.Min, bb.Max, bg_color_1, g.Style.FrameRounding);
		int vert_end_idx = window->DrawList->VtxBuffer.Size;
		if (is_gradient)
			ImGui::ShadeVertsLinearColorGradientKeepAlpha(window->DrawList, vert_start_idx, vert_end_idx,
				bb.Min, bb.GetBL(), bg_color_1, bg_color_2);
		if (g.Style.FrameBorderSize > 0.0f)
			window->DrawList->AddRect(bb.Min, bb.Max, ImGui::GetColorU32(ImGuiCol_Border),
				g.Style.FrameRounding, 0, g.Style.FrameBorderSize);

		window->DrawList->AddImage(
			a_texture,
			image_pos_min,
			image_pos_max,
			ImVec2(0, 0),
			ImVec2(1, 1),
			image_color);

		// Custom behavior based on whether the button is in an expanded state or not.
		// This is determined externally by the container this button is in.
		// (If the size of the button > 4x the size of the image, it's considered expanded.)

		if (expanded) {
			ImGui::PushStyleColor(ImGuiCol_Text, image_color);

			ImGui::RenderTextClipped(ImVec2(image_pos_width_min + image_size.x + style.FramePadding.x,
										 bb.Min.y + style.FramePadding.y),
				ImVec2(bb.Max.x - style.FramePadding.x, bb.Max.y - style.FramePadding.y),
				a_text, NULL, &label_size, ImVec2(a_textMod, 0.5f),  // alignment
				&bb);
			ImGui::PopStyleColor();
		}

		IMGUI_TEST_ENGINE_ITEM_INFO(id, label, g.LastItemData.StatusFlags);
		return pressed;
	};

		// Draws a header bar inline with a square X button to the right to close the menu
	bool AddPopupMenuHeader(const char* label) 
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

	bool AddConfirmationButtons(bool &a_confirm, bool &a_cancel, bool m_navAccept)
	{
		const float button_width = (ImGui::GetContentRegionAvail().x - ImGui::GetStyle().ItemSpacing.x) * 0.5f;

		ImGui::PushStyleColor(ImGuiCol_Button, 
			m_navAccept ? ThemeConfig::GetColor("BUTTON_CONFIRM_HOVER") : ThemeConfig::GetColor("BUTTON_CONFIRM")
		);

		if (ImGui::GradientButton(Translate("CONFIRM"), ImVec2(button_width, ImGui::GetFrameHeightWithSpacing()))) {
			a_confirm = true;
			a_cancel = false;

			return true;
		}
		ImGui::PopStyleColor();
		ImGui::SameLine();

		ImGui::PushStyleColor(ImGuiCol_Button, 
			m_navAccept ? ThemeConfig::GetColor("BUTTON_CANCEL") : ThemeConfig::GetColor("BUTTON_CANCEL_HOVER")
		);

		if (ImGui::GradientButton(Translate("CANCEL"), ImVec2(button_width, ImGui::GetFrameHeightWithSpacing()))) {
			a_confirm = false;
			a_cancel = true;

			return true;
		}
		ImGui::PopStyleColor();

		return false;
	}
}