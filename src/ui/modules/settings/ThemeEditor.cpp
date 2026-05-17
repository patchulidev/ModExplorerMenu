#include "config/ThemeConfig.h"
#include "external/icons/IconsLucide.h"
#include "imgui.h"
#include "localization/Locale.h"
#include "ui/components/UICustom.h"
#include "ui/components/UITable.h"
#include "ui/modules/settings/SettingsModule.h"

// In-game theme configurator. Exposes WidgetStyle, LayoutOverrides, and
// ImGui native style as live controls. Edits mutate the active singleton
// directly, so changes are visible on the next frame; "Save" persists to
// the active theme JSON.
//
// Not exposed: color keys ("PRIMARY", "BG", ...) — those need a string
// table editor and HSV-derived hover/active reapply, both out of scope
// for this first pass.

namespace Modex
{
	namespace
	{
		bool s_dirty = false;

		// Visual indent used to nest the controls under each collapsing
		// header. Kept local so the rest of the settings layout stays
		// unaffected by changes here.
		constexpr float kIndentPx = 12.0f;

		void TouchIfChanged(bool a_changed)
		{
			if (a_changed) s_dirty = true;
		}

		void DrawFloat(const char* a_label, float& a_value, float a_min, float a_max, const char* a_fmt = "%.2f")
		{
			ImGui::PushID(&a_value);
			ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * 0.55f);
			TouchIfChanged(ImGui::SliderFloat(a_label, &a_value, a_min, a_max, a_fmt));
			ImGui::PopID();
		}

		void DrawInt(const char* a_label, int& a_value, int a_min, int a_max)
		{
			ImGui::PushID(&a_value);
			ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * 0.55f);
			TouchIfChanged(ImGui::SliderInt(a_label, &a_value, a_min, a_max));
			ImGui::PopID();
		}

		void DrawVec2(const char* a_label, ImVec2& a_v, float a_min, float a_max, const char* a_fmt = "%.1f")
		{
			ImGui::PushID(&a_v);
			ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * 0.55f);
			float arr[2] = { a_v.x, a_v.y };
			if (ImGui::SliderFloat2(a_label, arr, a_min, a_max, a_fmt)) {
				a_v.x = arr[0];
				a_v.y = arr[1];
				s_dirty = true;
			}
			ImGui::PopID();
		}

		void DrawColor(const char* a_label, ImVec4& a_c)
		{
			ImGui::PushID(&a_c);
			TouchIfChanged(ImGui::ColorEdit4(a_label, &a_c.x, ImGuiColorEditFlags_AlphaBar));
			ImGui::PopID();
		}

		// Collapsing header that auto-indents its body and emits a trailing
		// separator so the sections read like a stack of cards.
		bool BeginSection(const char* a_label)
		{
			ImGui::PushID(a_label);
			const bool open = ImGui::CollapsingHeader(a_label, ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_SpanAvailWidth);
			if (open) {
				ImGui::Indent(kIndentPx);
			}
			return open;
		}

		void EndSection(bool a_open)
		{
			if (a_open) ImGui::Unindent(kIndentPx);
			ImGui::PopID();
		}

		void DrawMetaSection()
		{
			const auto meta = ThemeConfig::GetSingleton()->GetMeta();
			if (!BeginSection("Theme Info")) {
				EndSection(false);
				return;
			}
			ImGui::TextDisabled("Name:");
			ImGui::SameLine();
			ImGui::TextUnformatted(meta.name.empty() ? "(unnamed)" : meta.name.c_str());

			if (!meta.author.empty()) {
				ImGui::TextDisabled("Author:");
				ImGui::SameLine();
				ImGui::TextUnformatted(meta.author.c_str());
			}
			if (!meta.version.empty()) {
				ImGui::TextDisabled("Version:");
				ImGui::SameLine();
				ImGui::TextUnformatted(meta.version.c_str());
			}
			if (!meta.description.empty()) {
				ImGui::Spacing();
				ImGui::TextWrapped("%s", meta.description.c_str());
			}
			EndSection(true);
		}

		void DrawLayoutSection()
		{
			auto& l = ThemeConfig::GetLayoutOverridesMutable();
			const bool open = BeginSection("Layout (font-proportional)");
			if (!open) { EndSection(false); return; }

			ImGui::TextDisabled("Multipliers of u (= font * 0.25)");
			DrawFloat("Pad X",                  l.padX,           0.0f, 10.0f);
			DrawFloat("Pad Y",                  l.padY,           0.0f, 10.0f);
			DrawFloat("Gap Sm",                 l.gapSm,          0.0f, 10.0f);
			DrawFloat("Gap Md",                 l.gapMd,          0.0f, 12.0f);
			DrawFloat("Radius Sm",              l.radiusSm,       0.0f, 8.0f);
			DrawFloat("Radius Md",              l.radiusMd,       0.0f, 12.0f);
			DrawFloat("Pillar Width",           l.pillarWidth,    0.0f, 5.0f);

			ImGui::Spacing();
			ImGui::TextDisabled("Multiplier of font");
			DrawFloat("Settings Widget Width",  l.settingsWidgetWidth, 4.0f, 24.0f);

			ImGui::Spacing();
			ImGui::TextDisabled("Layout split + truncation ratios");
			DrawFloat("Ratio Table Equal",       l.ratioTableEqual,       0.20f, 0.80f, "%.3f");
			DrawFloat("Ratio Table Balanced",    l.ratioTableBalanced,    0.20f, 0.80f, "%.3f");
			DrawFloat("Ratio Table Primary",     l.ratioTablePrimary,     0.40f, 0.95f, "%.3f");
			DrawFloat("Truncate Name Tight",     l.ratioTruncateNameTight, 0.20f, 0.95f, "%.3f");
			DrawFloat("Truncate Name Mid",       l.ratioTruncateNameMid,   0.20f, 0.95f, "%.3f");
			DrawFloat("Truncate Name Wide",      l.ratioTruncateNameWide,  0.20f, 0.95f, "%.3f");

			EndSection(true);
		}

		void DrawImGuiStyleSection()
		{
			auto& s = ImGui::GetStyle();
			const bool open = BeginSection("ImGui Style");
			if (!open) { EndSection(false); return; }

			ImGui::TextDisabled("Rounding");
			DrawFloat("Window",     s.WindowRounding,    0.0f, 16.0f);
			DrawFloat("Child",      s.ChildRounding,     0.0f, 16.0f);
			DrawFloat("Frame",      s.FrameRounding,     0.0f, 16.0f);
			DrawFloat("Popup",      s.PopupRounding,     0.0f, 16.0f);
			DrawFloat("Scrollbar",  s.ScrollbarRounding, 0.0f, 16.0f);
			DrawFloat("Grab",       s.GrabRounding,      0.0f, 16.0f);
			DrawFloat("Tab",        s.TabRounding,       0.0f, 16.0f);

			ImGui::Spacing();
			ImGui::TextDisabled("Borders");
			DrawFloat("Window Border",    s.WindowBorderSize, 0.0f, 4.0f);
			DrawFloat("Child Border",     s.ChildBorderSize,  0.0f, 4.0f);
			DrawFloat("Popup Border",     s.PopupBorderSize,  0.0f, 4.0f);
			DrawFloat("Frame Border",     s.FrameBorderSize,  0.0f, 4.0f);
			DrawFloat("Tab Border",       s.TabBorderSize,    0.0f, 4.0f);
			DrawFloat("Scrollbar Size",   s.ScrollbarSize,    4.0f, 32.0f);
			DrawFloat("Grab Min Size",    s.GrabMinSize,      4.0f, 32.0f);
			DrawFloat("Indent Spacing",   s.IndentSpacing,    0.0f, 32.0f);

			ImGui::Spacing();
			ImGui::TextDisabled("Padding / Spacing");
			DrawVec2("Window Padding",      s.WindowPadding,     0.0f, 24.0f);
			DrawVec2("Frame Padding",       s.FramePadding,      0.0f, 24.0f);
			DrawVec2("Item Spacing",        s.ItemSpacing,       0.0f, 24.0f);
			DrawVec2("Item Inner Spacing",  s.ItemInnerSpacing,  0.0f, 16.0f);
			DrawVec2("Cell Padding",        s.CellPadding,       0.0f, 16.0f);

			EndSection(true);
		}

		void DrawWidgetSection()
		{
			auto& w = ThemeConfig::GetWidgetStyleMutable();

			if (BeginSection("Fancy (search + dropdown)")) {
				DrawFloat("Frame Rounding",  w.fancy.frameRounding, 0.0f, 24.0f);
				DrawVec2 ("Frame Padding",   w.fancy.framePadding,  0.0f, 24.0f);
				DrawFloat("Glyph Font Size", w.fancy.glyphFontSize, 8.0f, 48.0f);
				EndSection(true);
			} else EndSection(false);

			if (BeginSection("Tooltip")) {
				DrawFloat("Rounding",            w.tooltip.rounding,         0.0f, 16.0f);
				DrawFloat("Width Factor",        w.tooltip.widthFactor,      0.05f, 0.5f, "%.3f");
				DrawFloat("Accent Line Y Scale", w.tooltip.accentLineYScale, 0.5f, 3.0f);
				EndSection(true);
			} else EndSection(false);

			if (BeginSection("Action Button")) {
				DrawFloat("Height Scale (font)", w.actionButton.heightScale, 1.0f, 3.0f);
				EndSection(true);
			} else EndSection(false);

			if (BeginSection("Tab Button")) {
				DrawFloat("Inactive Value Delta", w.tabButton.inactiveValueDelta, -0.5f, 0.5f);
				DrawFloat("Inactive Alpha Scale", w.tabButton.inactiveAlphaScale,  0.0f, 1.0f);
				DrawFloat("Border Size",          w.tabButton.borderSize,          0.0f, 4.0f);
				DrawColor("Border Color",         w.tabButton.borderColor);
				EndSection(true);
			} else EndSection(false);

			if (BeginSection("Popup")) {
				DrawFloat("Width Factor (viewport)", w.popup.widthFactor,       0.10f, 0.80f, "%.3f");
				DrawInt  ("Search Max Rows",         w.popup.searchMaxRows,     4, 40);
				DrawVec2 ("Search Item Padding",     w.popup.searchItemPadding, 0.0f, 16.0f);
				DrawVec2 ("Search Item Spacing",     w.popup.searchItemSpacing, 0.0f, 16.0f);
				EndSection(true);
			} else EndSection(false);

			if (BeginSection("Item Preview")) {
				DrawFloat("Name Bar Height (font)",     w.itemPreview.nameBarHeightScale,   1.0f, 5.0f);
				DrawFloat("Gradient Height (frameH)",   w.itemPreview.gradientHeightScale,  1.0f, 5.0f);
				DrawFloat("Inline Bar Divisor",         w.itemPreview.inlineBarDivisor,     1.5f, 8.0f);
				DrawFloat("Inline Bar Mini Divisor",    w.itemPreview.inlineBarMiniDivisor, 1.5f, 8.0f);
				DrawFloat("Min Tooltip Width (px)",     w.itemPreview.minTooltipWidth,      100.0f, 600.0f, "%.0f");
				DrawFloat("Desired Width Pad (font)",   w.itemPreview.desiredWidthPadFont,  1.0f, 12.0f);
				EndSection(true);
			} else EndSection(false);

			if (BeginSection("Notification")) {
				DrawFloat("Tooltip Height (frameH)", w.notification.tooltipHeightScale, 0.5f, 4.0f);
				DrawFloat("Text Trunc Divisor",      w.notification.textTruncDivisor,   1.0f, 4.0f);
				EndSection(true);
			} else EndSection(false);

			if (BeginSection("Drag Overlay")) {
				DrawFloat("Icon Font Size",       w.dragOverlay.iconFontSize,      24.0f, 144.0f);
				DrawFloat("Label Font Divisor",   w.dragOverlay.labelFontDivisor,  1.0f, 6.0f);
				DrawFloat("Target Font Divisor",  w.dragOverlay.targetFontDivisor, 1.0f, 6.0f);
				DrawFloat("Payload Icon Size",    w.dragOverlay.payloadIconSize,   12.0f, 64.0f);
				DrawFloat("Payload Count Font",   w.dragOverlay.payloadCountFont,  16.0f, 72.0f);
				DrawFloat("Payload Min (frameH)", w.dragOverlay.payloadMinFrameH,  2.0f, 20.0f);
				DrawFloat("Payload Max (frameH)", w.dragOverlay.payloadMaxFrameH,  2.0f, 20.0f);
				EndSection(true);
			} else EndSection(false);

			if (BeginSection("Table")) {
				DrawFloat("Row Height Scale",       w.table.rowHeightScale,       1.0f, 3.0f);
				DrawFloat("Row Spacing Base",       w.table.rowSpacingBase,       0.0f, 12.0f);
				DrawFloat("Status Bar Height",      w.table.statusBarHeightScale, 1.0f, 3.0f);
				DrawFloat("Search Key Ratio",       w.table.searchKeyRatio,       0.20f, 0.80f, "%.3f");
				DrawFloat("Search Mode Divisor",    w.table.searchModeDivisor,    3.0f, 16.0f);
				DrawFloat("Search Form Divisor",    w.table.searchFormDivisor,    1.5f, 8.0f);
				EndSection(true);
			} else EndSection(false);

			if (BeginSection("Kit List")) {
				DrawFloat("Search Btn Width (font)",     w.kitList.searchBtnWidthFont,     4.0f, 16.0f);
				DrawFloat("Search Min Input (font)",     w.kitList.searchMinInputFont,     4.0f, 24.0f);
				DrawFloat("Empty State Btn (font)",      w.kitList.emptyStateBtnFont,      6.0f, 24.0f);
				DrawFloat("Tag Filter Max (font)",       w.kitList.tagFilterListMaxFont,   8.0f, 40.0f);
				DrawFloat("Tag Filter Width (font)",     w.kitList.tagFilterListWidthFont, 8.0f, 32.0f);
				DrawFloat("Col Icon Width (u)",          w.kitList.colIconWidthU,          1.0f, 12.0f);
				DrawFloat("Col Value Width (u)",         w.kitList.colValueWidthU,         1.0f, 12.0f);
				EndSection(true);
			} else EndSection(false);
		}

		void DrawActionFooter()
		{
			ImGui::Spacing();
			ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal);
			ImGui::Spacing();

			const float full = ImGui::GetContentRegionAvail().x;
			const float gap  = ImGui::GetStyle().ItemSpacing.x;
			const float btn  = (full - gap) * 0.5f;

			const ImVec4 saveColor = s_dirty
				? ThemeConfig::GetColor("CONFIRM")
				: ThemeConfig::GetColor("PRIMARY");
			ImGui::PushStyleColor(ImGuiCol_Button,        saveColor);
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ThemeConfig::GetHover(s_dirty ? "CONFIRM" : "PRIMARY"));
			ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ThemeConfig::GetActive(s_dirty ? "CONFIRM" : "PRIMARY"));
			const std::string saveLabel = s_dirty
				? std::string(ICON_LC_SAVE) + "  Save Theme*"
				: std::string(ICON_LC_SAVE) + "  Save Theme";
			if (ImGui::Button(saveLabel.c_str(), ImVec2(btn, 0.0f))) {
				if (ThemeConfig::GetSingleton()->SaveCurrentTheme()) {
					s_dirty = false;
				}
			}
			ImGui::PopStyleColor(3);

			ImGui::SameLine();

			ImGui::PushStyleColor(ImGuiCol_Button,        ThemeConfig::GetColor("DECLINE"));
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ThemeConfig::GetHover("DECLINE"));
			ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ThemeConfig::GetActive("DECLINE"));
			if (ImGui::Button(ICON_LC_ROTATE_CCW "  Reset Tokens", ImVec2(btn, 0.0f))) {
				ThemeConfig::GetSingleton()->ResetTokensToDefaults();
				s_dirty = true;
			}
			ImGui::PopStyleColor(3);

			if (s_dirty) {
				ImGui::TextDisabled(ICON_LC_TRIANGLE_ALERT "  Unsaved changes — Save to persist to %s",
					ThemeConfig::GetSingleton()->GetFilePath().filename().string().c_str());
			}
		}
	}

	void DrawThemeLayout(std::vector<std::unique_ptr<UITable>>& a_tables)
	{
		(void)a_tables;

		if (!ImGui::BeginChild("##Modex::ThemeEditor::Layout", ImVec2(0, 0), false)) {
			ImGui::EndChild();
			return;
		}

		ImGui::Spacing();
		ImGui::TextWrapped(
			"Tweak token values live. Edits apply immediately to the running UI. "
			"Click Save Theme to persist the active theme JSON, or Reset Tokens to "
			"restore compiled-in defaults (does not touch colors or ImGui style).");
		ImGui::Spacing();

		DrawMetaSection();
		DrawImGuiStyleSection();
		DrawLayoutSection();
		DrawWidgetSection();
		DrawActionFooter();

		ImGui::EndChild();
	}
}
