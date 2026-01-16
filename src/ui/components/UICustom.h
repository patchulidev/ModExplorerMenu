#pragma once

namespace Modex::UICustom
{
    bool AddSliderPicker(const char* a_text, float& a_valRef, float a_min, float a_max);
    bool AddDualSlider(const char* a_text, float& a_valRef_a, float& a_valRef_b, float a_min, float a_max);\
    bool AddKeybind(const char* a_text, uint32_t& a_keybind, uint32_t defaultKey, ImVec4& a_hover);
    bool AddToggleButton(const char* a_text, bool& a_boolRef);
    bool AddSelectionDropdown(const char* a_text, int& a_value, const std::vector<std::string>& a_options);
    bool AddFontDropdown(const char* a_text, std::string* a_font);

    bool AddIconButton(const char* a_icon, const char* a_tooltip, bool& condition);
    bool AddIconButtonBg(const char* a_icon, const char* a_tooltip, bool& a_toggle, const ImVec2& a_size, const ImVec4& a_bgColor, float a_offset);
    void AddFancyTooltip(const char* a_text);
	bool AddPopupMenuHeader(const char* a_text);
	bool AddConfirmationButtons(bool& a_confirm, bool& a_cancel, bool m_navAccept = true);

    bool SidebarButton(const char* a_text, bool a_selected, ImTextureID a_texture, ImVec2 a_imageSize, ImVec2 a_buttonSize, float& a_textMod);
}

namespace ImGui
{
	[[nodiscard]] inline static float GetCenterTextPosX(const char* text)
	{
		return ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x / 2 - ImGui::CalcTextSize(text).x / 2;
	};

	[[nodiscard]] inline static float GetCenterTextPosY(const char* text)
	{
		return ImGui::GetCursorPosY() + ImGui::GetContentRegionAvail().y / 2 - ImGui::CalcTextSize(text).y / 2;
	}

	static inline void SubCategoryHeader(const char* label, ImVec4 color = ImVec4(0.22f, 0.22f, 0.22f, 0.9f))
	{
		ImGui::PushStyleColor(ImGuiCol_Button, color);
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, color);
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, color);
		ImGui::Button(label, ImVec2(ImGui::GetContentRegionAvail().x, ImGui::GetFrameHeightWithSpacing()));
		ImGui::PopStyleColor(3);
	}

	// Source: https://github.com/ocornut/imgui/issues/4722
	static inline bool ColoredButtonV1(const char* label, const ImVec2& size_arg, ImU32 text_color, ImU32 bg_color_1, ImU32 bg_color_2)
	{
		ImGuiWindow* window = GetCurrentWindow();
		if (window->SkipItems)
			return false;

		ImGuiContext& g = *GImGui;
		const ImGuiStyle& style = g.Style;
		const ImGuiID id = window->GetID(label);
		const ImVec2 label_size = CalcTextSize(label, NULL, true);

		ImVec2 pos = window->DC.CursorPos;
		ImVec2 size = CalcItemSize(size_arg, label_size.x + style.FramePadding.x * 2.0f, label_size.y + style.FramePadding.y * 2.0f);

		const ImRect bb(pos, ImVec2(pos.x + size.x, pos.y + size.y));
		ItemSize(size, style.FramePadding.y);
		if (!ItemAdd(bb, id))
			return false;

		ImGuiButtonFlags flags = ImGuiButtonFlags_None;

		bool hovered, held;
		bool pressed = ButtonBehavior(bb, id, &hovered, &held, flags);

		// Render
		const bool is_gradient = bg_color_1 != bg_color_2;
		if (hovered && !held) {
			// Modify colors (ultimately this can be prebaked in the style)
			float h_increase = (held || hovered) ? 0.02f : 0.02f;
			float v_increase = (held || hovered) ? 10.0f : 0.07f;

			ImVec4 bg1f = ColorConvertU32ToFloat4(bg_color_1);
			ColorConvertRGBtoHSV(bg1f.x, bg1f.y, bg1f.z, bg1f.x, bg1f.y, bg1f.z);
			bg1f.x = ImMin(bg1f.x + h_increase, 1.0f);
			bg1f.z = ImMin(bg1f.z + v_increase, 1.0f);
			ColorConvertHSVtoRGB(bg1f.x, bg1f.y, bg1f.z, bg1f.x, bg1f.y, bg1f.z);
			bg_color_1 = GetColorU32(bg1f);
			if (is_gradient) {
				ImVec4 bg2f = ColorConvertU32ToFloat4(bg_color_2);
				ColorConvertRGBtoHSV(bg2f.x, bg2f.y, bg2f.z, bg2f.x, bg2f.y, bg2f.z);
				bg2f.z = ImMin(bg2f.z + h_increase, 1.0f);
				bg2f.z = ImMin(bg2f.z + v_increase, 1.0f);
				ColorConvertHSVtoRGB(bg2f.x, bg2f.y, bg2f.z, bg2f.x, bg2f.y, bg2f.z);
				bg_color_2 = GetColorU32(bg2f);
			} else {
				bg_color_2 = bg_color_1;
			}
		} else if (hovered && held) {
			ImVec4 bg1f = ColorConvertU32ToFloat4(bg_color_1);
			bg1f.x = ImMin(bg1f.x + 0.25f, 1.0f);
			bg1f.y = ImMin(bg1f.y + 0.25f, 1.0f);
			bg1f.z = ImMin(bg1f.z + 0.25f, 1.0f);
			bg_color_1 = GetColorU32(bg1f);
		}
		RenderNavHighlight(bb, id);

#if 0
    // V1 : faster but prevents rounding
    window->DrawList->AddRectFilledMultiColor(bb.Min, bb.Max, bg_color_1, bg_color_1, bg_color_2, bg_color_2);
    if (g.Style.FrameBorderSize > 0.0f)
        window->DrawList->AddRect(bb.Min, bb.Max, GetColorU32(ImGuiCol_Border), 0.0f, 0, g.Style.FrameBorderSize);
#endif

		// V2
		int vert_start_idx = window->DrawList->VtxBuffer.Size;
		window->DrawList->AddRectFilled(bb.Min, bb.Max, bg_color_1, g.Style.FrameRounding);
		int vert_end_idx = window->DrawList->VtxBuffer.Size;
		if (is_gradient && !held)
			ShadeVertsLinearColorGradientKeepAlpha(window->DrawList, vert_start_idx, vert_end_idx, bb.Min, bb.GetBL(), bg_color_1, bg_color_2);
		if (g.Style.FrameBorderSize > 0.0f)
			window->DrawList->AddRect(bb.Min, bb.Max, GetColorU32(ImGuiCol_Border), g.Style.FrameRounding, 0, g.Style.FrameBorderSize);

		if (g.LogEnabled)
			LogSetNextTextDecoration("[", "]");
		PushStyleColor(ImGuiCol_Text, text_color);
		RenderTextClipped(
			ImVec2(bb.Min.x + style.FramePadding.x, bb.Min.y + style.FramePadding.y),
			ImVec2(bb.Max.x - style.FramePadding.x, bb.Max.y - style.FramePadding.y),
			label, NULL,
			&label_size,
			style.ButtonTextAlign,
			&bb);
		PopStyleColor();

		IMGUI_TEST_ENGINE_ITEM_INFO(id, label, g.LastItemData.StatusFlags);
		return pressed;
	}

	inline static bool ToggleButton(const char* str_id, bool* v, const float width)
	{
		ImVec2 p = ImGui::GetCursorScreenPos();
		ImDrawList* draw_list = ImGui::GetWindowDrawList();

		float height = ImGui::GetFrameHeight();
		// float width = height * 3.55f;
		float radius = height * 0.50f;
		bool clicked = false;

		ImGui::InvisibleButton(str_id, ImVec2(width, height));
		if (ImGui::IsItemClicked()) {
			*v = !*v;
			clicked = true;
		}

		float t = *v ? 1.0f : 0.0f;

		ImGuiContext& g = *GImGui;
		float ANIM_SPEED = 0.08f;
		if (g.LastActiveId == g.CurrentWindow->GetID(str_id))  // && g.LastActiveIdTimer < ANIM_SPEED)
		{
			float t_anim = ImSaturate(g.LastActiveIdTimer / ANIM_SPEED);
			t = *v ? (t_anim) : (1.0f - t_anim);
		}

		ImU32 col_bg;
		ImU32 col_grab;
		if (ImGui::IsItemHovered()) {
			col_bg = ImGui::GetColorU32(ImLerp(ImGui::GetStyleColorVec4(ImGuiCol_FrameBgHovered), ImVec4(0.40f, 0.70f, 0.40f, 1.0f), t));
			col_grab = ImGui::GetColorU32(ImLerp(ImGui::GetStyleColorVec4(ImGuiCol_SliderGrab), ImVec4(0.78f, 0.78f, 0.78f, 1.0f), t));
		} else {
			col_bg = ImGui::GetColorU32(ImLerp(ImGui::GetStyleColorVec4(ImGuiCol_FrameBg), ImVec4(0.40f, 0.70f, 0.40f, 1.0f), t));
			col_grab = ImGui::GetColorU32(ImLerp(ImGui::GetStyleColorVec4(ImGuiCol_SliderGrab), ImVec4(0.85f, 0.85f, 0.85f, 1.0f), t));
		}

		draw_list->AddRectFilled(p, ImVec2(p.x + width, p.y + height), col_bg, height * 0.5f);
		draw_list->AddCircleFilled(ImVec2(p.x + radius + t * (width - radius * 2.0f), p.y + radius), radius - 1.5f, col_grab);

		return clicked;
	}

	// TODO: Are we even using this?
	inline static void ShowLanguagePopup()
	{
		// auto& style = Modex::Settings::GetSingleton()->GetStyle();

		auto width = ImGui::GetMainViewport()->Size.x * 0.25f;
		auto height = ImGui::GetMainViewport()->Size.y * 0.20f;
		const float center_x = ImGui::GetMainViewport()->Size.x * 0.5f;
		const float center_y = ImGui::GetMainViewport()->Size.y * 0.5f;

		const float pos_x = center_x - (width * 0.5f);
		const float pos_y = center_y - (height * 0.5f);

		const float buttonHeight = ImGui::GetFontSize() * 1.5f;

		ImGui::SetNextWindowSize(ImVec2(width, height));
		ImGui::SetNextWindowPos(ImVec2(pos_x, pos_y));
		if (ImGui::BeginPopupModal("Non-latin Alphabetical Language", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar)) {
			ImGui::SetCursorPosX(ImGui::GetCenterTextPosX("Non-latin Alphabetical Language"));
			// ImGui::PushFont(style.font.normal);
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.5f, 0.5f, 1.0f));
			ImGui::Text("Non-latin Alphabetical Language");
			ImGui::PopStyleColor(1);
			// ImGui::PopFont();

			ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal);

			ImGui::TextWrapped(
				"You have selected a non-latin alphabetical language. As a result, you"
				" may need to restart the game for the changes to take effect.");

			ImGui::NewLine();
			ImGui::Text("Do you understand, and wish to proceed?");
			ImGui::NewLine();

			ImGui::SetCursorPosY(ImGui::GetWindowSize().y - (buttonHeight * 2) - 20.0f);  // subtract button size * 2 + separator
			ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal);
			if (ImGui::Button("Yes, I understand", ImVec2(ImGui::GetContentRegionAvail().x, buttonHeight))) {
				ImGui::CloseCurrentPopup();
			}

			if (ImGui::Button("No, take me back!", ImVec2(ImGui::GetContentRegionAvail().x, buttonHeight))) {
				ImGui::CloseCurrentPopup();
			}
			ImGui::EndPopup();
		}
	}

	inline static bool GradientButton(const char* label, const ImVec2& size = ImVec2(0, 0))
	{
		auto col_button = ImGui::GetStyle().Colors[ImGuiCol_Button];
		auto col_a = ImGui::GetColorU32(col_button);
		auto col_b = ImGui::GetColorU32(ImVec4(col_button.x * 0.7f, col_button.y * 0.7f, col_button.z * 0.7f, col_button.w));
		auto text_color = ImGui::GetColorU32(ImGuiCol_Text);
		return ImGui::ColoredButtonV1(label, size, text_color, col_a, col_b);
	}

	inline static bool GradientSelectableEX(const char* label, bool& selected, const ImVec2& size = ImVec2(0, 0))
	{
		auto col_button = selected ? ImGui::GetStyle().Colors[ImGuiCol_Button] : ImGui::GetStyle().Colors[ImGuiCol_FrameBg];
		auto alpha = selected ? 1.0f : 0.5f;
		auto col_a = ImGui::GetColorU32(ImVec4(col_button.x, col_button.y, col_button.z, col_button.w * alpha));
		auto col_b = ImGui::GetColorU32(ImVec4(col_button.x * 0.6f, col_button.y * 0.6f, col_button.z * 0.6f, col_button.w * alpha));

		auto innerPadding = ImGui::GetStyle().FramePadding.y;
		auto newSize = ImVec2(size.x, size.y + innerPadding);
		auto pressed = ImGui::ColoredButtonV1(label, newSize, IM_COL32(255, 255, 255, 255 * alpha), col_a, col_b);

		if (pressed) {
			selected = !selected;
		}

		return pressed;
	}
}
