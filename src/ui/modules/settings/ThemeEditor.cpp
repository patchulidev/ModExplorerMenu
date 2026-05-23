#include "config/ThemeConfig.h"
#include "config/UserConfig.h"
#include "external/icons/IconsLucide.h"
#include "imgui.h"
#include "localization/Locale.h"
#include "ui/components/UICustom.h"
#include "ui/components/UITable.h"
#include "ui/core/UIManager.h"
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

		// Build "<icon>  <translated text>" — returned as std::string so the
		// temporary outlives the .c_str() call within the same full-expression.
		std::string IconLabel(const char* a_icon, const char* a_key)
		{
			return std::string(a_icon) + "  " + Translate(a_key);
		}

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

		// Editable meta backing buffers. Re-sync from m_data whenever the
		// active theme file changes (theme switch, create, rename, delete) so
		// the editor never shows fields from the previous theme.
		struct MetaBuffers {
			char name[128]        = {};
			char author[128]      = {};
			char description[512] = {};
			char version[64]      = {};
		};

		MetaBuffers&            s_metaBuf()      { static MetaBuffers b; return b; }
		std::filesystem::path&  s_metaSyncPath() { static std::filesystem::path p; return p; }

		void SyncMetaBuffersFromConfig()
		{
			const auto meta = ThemeConfig::GetSingleton()->GetMeta();
			auto& b = s_metaBuf();
			ImFormatString(b.name,        IM_ARRAYSIZE(b.name),        "%s", meta.name.c_str());
			ImFormatString(b.author,      IM_ARRAYSIZE(b.author),      "%s", meta.author.c_str());
			ImFormatString(b.description, IM_ARRAYSIZE(b.description), "%s", meta.description.c_str());
			ImFormatString(b.version,     IM_ARRAYSIZE(b.version),     "%s", meta.version.c_str());
			s_metaSyncPath() = ThemeConfig::GetSingleton()->GetFilePath();
		}

		void FlushMetaBuffersToConfig()
		{
			const auto& b = s_metaBuf();
			ThemeMeta meta;
			meta.name        = b.name;
			meta.author      = b.author;
			meta.description = b.description;
			meta.version     = b.version;
			ThemeConfig::GetSingleton()->SetMeta(meta);
		}

		const ModexTheme* FindActiveTheme()
		{
			const auto& themes = ThemeConfig::GetAvailableThemes();
			const auto& active = ThemeConfig::GetSingleton()->GetFilePath();
			for (const auto& t : themes) {
				if (t.m_filePath == active) return &t;
			}
			return nullptr;
		}

		void DrawThemePicker()
		{
			const auto* active = FindActiveTheme();
			const char* preview = active ? active->m_name.c_str() : Translate("THEME_EDITOR_PICKER_NONE");

			ImGui::TextDisabled("%s", Translate("THEME_EDITOR_PICKER_LABEL"));
			ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
			if (ImGui::BeginCombo("##Modex::ThemeEditor::ThemePicker", preview)) {
				const auto themes = ThemeConfig::GetAvailableThemes();  // copy: list may mutate mid-loop on switch
				for (const auto& t : themes) {
					const bool selected = active && t.m_filePath == active->m_filePath;
					if (ImGui::Selectable(t.m_name.c_str(), selected)) {
						if (!selected) {
							if (ThemeConfig::GetSingleton()->LoadTheme(t)) {
								UserConfig::Get().theme = t.m_name;
								UserConfig::GetSingleton()->SaveSettings();
								// Loaded fresh state — buffers and dirty flag must reset.
								SyncMetaBuffersFromConfig();
								s_dirty = false;
							}
						}
					}
				}
				ImGui::EndCombo();
			}
		}

		void DrawThemeActionRow()
		{
			const auto* active = FindActiveTheme();
			const bool isDefault = active && active->m_name == "default";

			const float full = ImGui::GetContentRegionAvail().x;
			const float gap  = ImGui::GetStyle().ItemSpacing.x;
			const float btn  = (full - gap * 2.0f) / 3.0f;

			if (ImGui::Button(IconLabel(ICON_LC_PLUS, "THEME_EDITOR_BTN_NEW").c_str(), ImVec2(btn, 0.0f))) {
				UIManager::GetSingleton()->ShowInputBox(
					Translate("THEME_EDITOR_POPUP_NEW_TITLE"),
					Translate("THEME_EDITOR_POPUP_NEW_MSG"),
					"",
					[](const std::string& a_input) {
						const std::string reason = ThemeConfig::ValidateThemeName(a_input);
						if (!reason.empty()) {
							UIManager::GetSingleton()->ShowInfoBox(Translate("THEME_EDITOR_POPUP_INVALID_TITLE"), reason);
							return;
						}
						if (!ThemeConfig::GetSingleton()->CreateTheme(a_input)) {
							UIManager::GetSingleton()->ShowInfoBox(
								Translate("THEME_EDITOR_POPUP_CREATE_FAIL_TITLE"),
								Translate("THEME_EDITOR_POPUP_CREATE_FAIL_MSG"));
							return;
						}
						SyncMetaBuffersFromConfig();
						s_dirty = false;
					});
			}

			ImGui::SameLine();
			if (isDefault) ImGui::BeginDisabled();
			if (ImGui::Button(IconLabel(ICON_LC_PENCIL, "THEME_EDITOR_BTN_RENAME").c_str(), ImVec2(btn, 0.0f))) {
				const std::string currentStem = active ? active->m_name : "";
				UIManager::GetSingleton()->ShowInputBox(
					Translate("THEME_EDITOR_POPUP_RENAME_TITLE"),
					Translate("THEME_EDITOR_POPUP_RENAME_MSG"),
					currentStem,
					[currentStem](const std::string& a_input) {
						if (a_input == currentStem) return;
						const std::string reason = ThemeConfig::ValidateThemeName(a_input, currentStem);
						if (!reason.empty()) {
							UIManager::GetSingleton()->ShowInfoBox(Translate("THEME_EDITOR_POPUP_INVALID_TITLE"), reason);
							return;
						}
						// Look up by stem so we always operate on the on-disk entry.
						const ModexTheme* match = nullptr;
						for (const auto& t : ThemeConfig::GetAvailableThemes()) {
							if (t.m_name == currentStem) { match = &t; break; }
						}
						if (!match) return;
						if (!ThemeConfig::GetSingleton()->RenameTheme(*match, a_input)) {
							UIManager::GetSingleton()->ShowInfoBox(
								Translate("THEME_EDITOR_POPUP_RENAME_FAIL_TITLE"),
								Translate("THEME_EDITOR_POPUP_RENAME_FAIL_MSG"));
							return;
						}
						SyncMetaBuffersFromConfig();
					});
			}
			if (isDefault) ImGui::EndDisabled();

			ImGui::SameLine();
			if (isDefault) ImGui::BeginDisabled();
			ImGui::PushStyleColor(ImGuiCol_Button,        ThemeConfig::GetColor("DECLINE"));
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ThemeConfig::GetHover("DECLINE"));
			ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ThemeConfig::GetActive("DECLINE"));
			if (ImGui::Button(IconLabel(ICON_LC_TRASH_2, "THEME_EDITOR_BTN_DELETE").c_str(), ImVec2(btn, 0.0f))) {
				const std::string stem = active ? active->m_name : "";
				const std::string msg  = std::string(Translate("THEME_EDITOR_POPUP_DELETE_MSG")) + "\n\n" + stem;
				UIManager::GetSingleton()->ShowWarning(
					Translate("THEME_EDITOR_POPUP_DELETE_TITLE"),
					msg,
					true,
					[stem]() {
						const ModexTheme* match = nullptr;
						for (const auto& t : ThemeConfig::GetAvailableThemes()) {
							if (t.m_name == stem) { match = &t; break; }
						}
						if (!match) return;
						if (!ThemeConfig::GetSingleton()->DeleteTheme(*match)) {
							UIManager::GetSingleton()->ShowInfoBox(
								Translate("THEME_EDITOR_POPUP_DELETE_FAIL_TITLE"),
								Translate("THEME_EDITOR_POPUP_DELETE_FAIL_MSG"));
							return;
						}
						SyncMetaBuffersFromConfig();
						s_dirty = false;
					});
			}
			ImGui::PopStyleColor(3);
			if (isDefault) ImGui::EndDisabled();
		}

		void DrawMetaSection()
		{
			// Resync local buffers if the active theme file changed beneath us
			// (e.g. via the Settings tab's theme dropdown, or after Create).
			if (s_metaSyncPath() != ThemeConfig::GetSingleton()->GetFilePath()) {
				SyncMetaBuffersFromConfig();
			}

			if (!BeginSection(Translate("THEME_EDITOR_SECT_INFO"))) {
				EndSection(false);
				return;
			}

			DrawThemePicker();
			ImGui::Spacing();
			DrawThemeActionRow();
			ImGui::Spacing();
			ImGui::Separator();
			ImGui::Spacing();

			auto& b = s_metaBuf();
			const float labelW = ImGui::CalcTextSize(Translate("THEME_EDITOR_META_DESCRIPTION")).x + ImGui::GetStyle().ItemSpacing.x * 2.0f;

			auto field = [&](const char* a_label, char* a_buf, size_t a_bufSize, bool a_multiline) {
				ImGui::PushID(a_label);
				ImGui::AlignTextToFramePadding();
				ImGui::TextDisabled("%s", a_label);
				ImGui::SameLine(labelW);
				ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
				bool changed = false;
				if (a_multiline) {
					const ImVec2 size(ImGui::GetContentRegionAvail().x, ImGui::GetFrameHeight() * 2.5f);
					changed = ImGui::InputTextMultiline("##field", a_buf, a_bufSize, size);
				} else {
					changed = ImGui::InputText("##field", a_buf, a_bufSize);
				}
				if (changed) {
					FlushMetaBuffersToConfig();
					s_dirty = true;
				}
				ImGui::PopID();
			};

			field(Translate("THEME_EDITOR_META_NAME"),        b.name,        IM_ARRAYSIZE(b.name),        false);
			field(Translate("THEME_EDITOR_META_AUTHOR"),      b.author,      IM_ARRAYSIZE(b.author),      false);
			field(Translate("THEME_EDITOR_META_VERSION"),     b.version,     IM_ARRAYSIZE(b.version),     false);
			field(Translate("THEME_EDITOR_META_DESCRIPTION"), b.description, IM_ARRAYSIZE(b.description), true);

			EndSection(true);
		}

		void DrawLayoutSection()
		{
			auto& l = ThemeConfig::GetLayoutOverridesMutable();
			const bool open = BeginSection(Translate("THEME_EDITOR_SECT_LAYOUT"));
			if (!open) { EndSection(false); return; }

			ImGui::TextDisabled("%s", Translate("THEME_EDITOR_HINT_U_MULT"));
			DrawFloat(Translate("THEME_EDITOR_LBL_PAD_X"),         l.padX,        0.0f, 10.0f);
			DrawFloat(Translate("THEME_EDITOR_LBL_PAD_Y"),         l.padY,        0.0f, 10.0f);
			DrawFloat(Translate("THEME_EDITOR_LBL_GAP_SM"),        l.gapSm,       0.0f, 10.0f);
			DrawFloat(Translate("THEME_EDITOR_LBL_GAP_MD"),        l.gapMd,       0.0f, 12.0f);
			DrawFloat(Translate("THEME_EDITOR_LBL_RADIUS_SM"),     l.radiusSm,    0.0f, 8.0f);
			DrawFloat(Translate("THEME_EDITOR_LBL_RADIUS_MD"),     l.radiusMd,    0.0f, 12.0f);
			DrawFloat(Translate("THEME_EDITOR_LBL_PILLAR_WIDTH"),  l.pillarWidth, 0.0f, 5.0f);

			ImGui::Spacing();
			ImGui::TextDisabled("%s", Translate("THEME_EDITOR_HINT_FONT_MULT"));
			DrawFloat(Translate("THEME_EDITOR_LBL_SETTINGS_WIDGET_WIDTH"), l.settingsWidgetWidth, 4.0f, 24.0f);

			ImGui::Spacing();
			ImGui::TextDisabled("%s", Translate("THEME_EDITOR_HINT_RATIOS"));
			DrawFloat(Translate("THEME_EDITOR_LBL_RATIO_TABLE_EQUAL"),     l.ratioTableEqual,        0.20f, 0.80f, "%.3f");
			DrawFloat(Translate("THEME_EDITOR_LBL_RATIO_TABLE_BALANCED"),  l.ratioTableBalanced,     0.20f, 0.80f, "%.3f");
			DrawFloat(Translate("THEME_EDITOR_LBL_RATIO_TABLE_PRIMARY"),   l.ratioTablePrimary,      0.40f, 0.95f, "%.3f");
			DrawFloat(Translate("THEME_EDITOR_LBL_TRUNCATE_NAME_TIGHT"),   l.ratioTruncateNameTight, 0.20f, 0.95f, "%.3f");
			DrawFloat(Translate("THEME_EDITOR_LBL_TRUNCATE_NAME_MID"),     l.ratioTruncateNameMid,   0.20f, 0.95f, "%.3f");
			DrawFloat(Translate("THEME_EDITOR_LBL_TRUNCATE_NAME_WIDE"),    l.ratioTruncateNameWide,  0.20f, 0.95f, "%.3f");

			EndSection(true);
		}

		void DrawImGuiStyleSection()
		{
			auto& s = ImGui::GetStyle();
			const bool open = BeginSection(Translate("THEME_EDITOR_SECT_IMGUI"));
			if (!open) { EndSection(false); return; }

			ImGui::TextDisabled("%s", Translate("THEME_EDITOR_HINT_ROUNDING"));
			DrawFloat(Translate("THEME_EDITOR_LBL_WINDOW"),    s.WindowRounding,    0.0f, 16.0f);
			DrawFloat(Translate("THEME_EDITOR_LBL_CHILD"),     s.ChildRounding,     0.0f, 16.0f);
			DrawFloat(Translate("THEME_EDITOR_LBL_FRAME"),     s.FrameRounding,     0.0f, 16.0f);
			DrawFloat(Translate("THEME_EDITOR_LBL_POPUP"),     s.PopupRounding,     0.0f, 16.0f);
			DrawFloat(Translate("THEME_EDITOR_LBL_SCROLLBAR"), s.ScrollbarRounding, 0.0f, 16.0f);
			DrawFloat(Translate("THEME_EDITOR_LBL_GRAB"),      s.GrabRounding,      0.0f, 16.0f);
			DrawFloat(Translate("THEME_EDITOR_LBL_TAB"),       s.TabRounding,       0.0f, 16.0f);

			ImGui::Spacing();
			ImGui::TextDisabled("%s", Translate("THEME_EDITOR_HINT_BORDERS"));
			DrawFloat(Translate("THEME_EDITOR_LBL_WINDOW_BORDER"), s.WindowBorderSize, 0.0f, 4.0f);
			DrawFloat(Translate("THEME_EDITOR_LBL_CHILD_BORDER"),  s.ChildBorderSize,  0.0f, 4.0f);
			DrawFloat(Translate("THEME_EDITOR_LBL_POPUP_BORDER"),  s.PopupBorderSize,  0.0f, 4.0f);
			DrawFloat(Translate("THEME_EDITOR_LBL_FRAME_BORDER"),  s.FrameBorderSize,  0.0f, 4.0f);
			DrawFloat(Translate("THEME_EDITOR_LBL_TAB_BORDER"),    s.TabBorderSize,    0.0f, 4.0f);
			DrawFloat(Translate("THEME_EDITOR_LBL_SCROLLBAR_SIZE"),s.ScrollbarSize,    4.0f, 32.0f);
			DrawFloat(Translate("THEME_EDITOR_LBL_GRAB_MIN_SIZE"), s.GrabMinSize,      4.0f, 32.0f);
			DrawFloat(Translate("THEME_EDITOR_LBL_INDENT_SPACING"),s.IndentSpacing,    0.0f, 32.0f);

			ImGui::Spacing();
			ImGui::TextDisabled("%s", Translate("THEME_EDITOR_HINT_PADDING"));
			DrawVec2(Translate("THEME_EDITOR_LBL_WINDOW_PADDING"),     s.WindowPadding,    0.0f, 24.0f);
			DrawVec2(Translate("THEME_EDITOR_LBL_FRAME_PADDING"),      s.FramePadding,     0.0f, 24.0f);
			DrawVec2(Translate("THEME_EDITOR_LBL_ITEM_SPACING"),       s.ItemSpacing,      0.0f, 24.0f);
			DrawVec2(Translate("THEME_EDITOR_LBL_ITEM_INNER_SPACING"), s.ItemInnerSpacing, 0.0f, 16.0f);
			DrawVec2(Translate("THEME_EDITOR_LBL_CELL_PADDING"),       s.CellPadding,      0.0f, 16.0f);

			EndSection(true);
		}

		void DrawWidgetSection()
		{
			auto& w = ThemeConfig::GetWidgetStyleMutable();

			if (BeginSection(Translate("THEME_EDITOR_SECT_FANCY"))) {
				DrawFloat(Translate("THEME_EDITOR_LBL_FRAME_ROUNDING"),  w.fancy.frameRounding, 0.0f, 24.0f);
				DrawVec2 (Translate("THEME_EDITOR_LBL_FRAME_PADDING"),   w.fancy.framePadding,  0.0f, 24.0f);
				DrawFloat(Translate("THEME_EDITOR_LBL_GLYPH_FONT_SIZE"), w.fancy.glyphFontSize, 8.0f, 48.0f);
				EndSection(true);
			} else EndSection(false);

			if (BeginSection(Translate("THEME_EDITOR_SECT_TOOLTIP"))) {
				DrawFloat(Translate("THEME_EDITOR_LBL_ROUNDING"),            w.tooltip.rounding,         0.0f, 16.0f);
				DrawFloat(Translate("THEME_EDITOR_LBL_WIDTH_FACTOR"),        w.tooltip.widthFactor,      0.05f, 0.5f, "%.3f");
				DrawFloat(Translate("THEME_EDITOR_LBL_ACCENT_LINE_Y_SCALE"), w.tooltip.accentLineYScale, 0.5f, 3.0f);
				EndSection(true);
			} else EndSection(false);

			if (BeginSection(Translate("THEME_EDITOR_SECT_ACTION_BUTTON"))) {
				DrawFloat(Translate("THEME_EDITOR_LBL_HEIGHT_SCALE_FONT"), w.actionButton.heightScale, 1.0f, 3.0f);
				EndSection(true);
			} else EndSection(false);

			if (BeginSection(Translate("THEME_EDITOR_SECT_TAB_BUTTON"))) {
				DrawFloat(Translate("THEME_EDITOR_LBL_INACTIVE_VALUE_DELTA"), w.tabButton.inactiveValueDelta, -0.5f, 0.5f);
				DrawFloat(Translate("THEME_EDITOR_LBL_INACTIVE_ALPHA_SCALE"), w.tabButton.inactiveAlphaScale,  0.0f, 1.0f);
				DrawFloat(Translate("THEME_EDITOR_LBL_ACTIVE_VALUE_DELTA"),   w.tabButton.activeValueDelta,   -0.5f, 0.5f);
				DrawFloat(Translate("THEME_EDITOR_LBL_ACTIVE_ALPHA_SCALE"),   w.tabButton.activeAlphaScale,    0.0f, 1.0f);
				DrawFloat(Translate("THEME_EDITOR_LBL_BORDER_SIZE"),         w.tabButton.borderSize,          0.0f, 4.0f);
				DrawColor(Translate("THEME_EDITOR_LBL_BORDER_COLOR"),        w.tabButton.borderColor);
				EndSection(true);
			} else EndSection(false);

			if (BeginSection(Translate("THEME_EDITOR_SECT_POPUP"))) {
				DrawFloat(Translate("THEME_EDITOR_LBL_WIDTH_FACTOR_VIEWPORT"), w.popup.widthFactor,       0.10f, 0.80f, "%.3f");
				DrawInt  (Translate("THEME_EDITOR_LBL_SEARCH_MAX_ROWS"),       w.popup.searchMaxRows,     4, 40);
				DrawVec2 (Translate("THEME_EDITOR_LBL_SEARCH_ITEM_PADDING"),   w.popup.searchItemPadding, 0.0f, 16.0f);
				DrawVec2 (Translate("THEME_EDITOR_LBL_SEARCH_ITEM_SPACING"),   w.popup.searchItemSpacing, 0.0f, 16.0f);
				EndSection(true);
			} else EndSection(false);

			if (BeginSection(Translate("THEME_EDITOR_SECT_ITEM_PREVIEW"))) {
				DrawFloat(Translate("THEME_EDITOR_LBL_NAME_BAR_HEIGHT_FONT"),    w.itemPreview.nameBarHeightScale,   1.0f, 5.0f);
				DrawFloat(Translate("THEME_EDITOR_LBL_GRADIENT_HEIGHT_FRAMEH"),  w.itemPreview.gradientHeightScale,  1.0f, 5.0f);
				DrawFloat(Translate("THEME_EDITOR_LBL_INLINE_BAR_DIVISOR"),      w.itemPreview.inlineBarDivisor,     1.5f, 8.0f);
				DrawFloat(Translate("THEME_EDITOR_LBL_INLINE_BAR_MINI_DIVISOR"), w.itemPreview.inlineBarMiniDivisor, 1.5f, 8.0f);
				DrawFloat(Translate("THEME_EDITOR_LBL_MIN_TOOLTIP_WIDTH_PX"),    w.itemPreview.minTooltipWidth,      100.0f, 600.0f, "%.0f");
				DrawFloat(Translate("THEME_EDITOR_LBL_DESIRED_WIDTH_PAD_FONT"),  w.itemPreview.desiredWidthPadFont,  1.0f, 12.0f);
				EndSection(true);
			} else EndSection(false);

			if (BeginSection(Translate("THEME_EDITOR_SECT_NOTIFICATION"))) {
				DrawFloat(Translate("THEME_EDITOR_LBL_TOOLTIP_HEIGHT_FRAMEH"), w.notification.tooltipHeightScale, 0.5f, 4.0f);
				DrawFloat(Translate("THEME_EDITOR_LBL_TEXT_TRUNC_DIVISOR"),    w.notification.textTruncDivisor,   1.0f, 4.0f);
				EndSection(true);
			} else EndSection(false);

			if (BeginSection(Translate("THEME_EDITOR_SECT_DRAG_OVERLAY"))) {
				DrawFloat(Translate("THEME_EDITOR_LBL_ICON_FONT_SIZE"),      w.dragOverlay.iconFontSize,      24.0f, 144.0f);
				DrawFloat(Translate("THEME_EDITOR_LBL_LABEL_FONT_DIVISOR"),  w.dragOverlay.labelFontDivisor,  1.0f, 6.0f);
				DrawFloat(Translate("THEME_EDITOR_LBL_TARGET_FONT_DIVISOR"), w.dragOverlay.targetFontDivisor, 1.0f, 6.0f);
				DrawFloat(Translate("THEME_EDITOR_LBL_PAYLOAD_ICON_SIZE"),   w.dragOverlay.payloadIconSize,   12.0f, 64.0f);
				DrawFloat(Translate("THEME_EDITOR_LBL_PAYLOAD_COUNT_FONT"),  w.dragOverlay.payloadCountFont,  16.0f, 72.0f);
				DrawFloat(Translate("THEME_EDITOR_LBL_PAYLOAD_MIN_FRAMEH"),  w.dragOverlay.payloadMinFrameH,  2.0f, 20.0f);
				DrawFloat(Translate("THEME_EDITOR_LBL_PAYLOAD_MAX_FRAMEH"),  w.dragOverlay.payloadMaxFrameH,  2.0f, 20.0f);
				EndSection(true);
			} else EndSection(false);

			if (BeginSection(Translate("THEME_EDITOR_SECT_TABLE"))) {
				DrawFloat(Translate("THEME_EDITOR_LBL_ROW_HEIGHT_SCALE"),    w.table.rowHeightScale,       1.0f, 3.0f);
				DrawFloat(Translate("THEME_EDITOR_LBL_ROW_SPACING_BASE"),    w.table.rowSpacingBase,       0.0f, 12.0f);
				DrawFloat(Translate("THEME_EDITOR_LBL_STATUS_BAR_HEIGHT"),   w.table.statusBarHeightScale, 1.0f, 3.0f);
				DrawFloat(Translate("THEME_EDITOR_LBL_SEARCH_KEY_RATIO"),    w.table.searchKeyRatio,       0.20f, 0.80f, "%.3f");
				DrawFloat(Translate("THEME_EDITOR_LBL_SEARCH_MODE_DIVISOR"), w.table.searchModeDivisor,    3.0f, 16.0f);
				DrawFloat(Translate("THEME_EDITOR_LBL_SEARCH_FORM_DIVISOR"), w.table.searchFormDivisor,    1.5f, 8.0f);
				EndSection(true);
			} else EndSection(false);

			if (BeginSection(Translate("THEME_EDITOR_SECT_KIT_LIST"))) {
				DrawFloat(Translate("THEME_EDITOR_LBL_SEARCH_BTN_WIDTH_FONT"),  w.kitList.searchBtnWidthFont,     4.0f, 16.0f);
				DrawFloat(Translate("THEME_EDITOR_LBL_SEARCH_MIN_INPUT_FONT"),  w.kitList.searchMinInputFont,     4.0f, 24.0f);
				DrawFloat(Translate("THEME_EDITOR_LBL_EMPTY_STATE_BTN_FONT"),   w.kitList.emptyStateBtnFont,      6.0f, 24.0f);
				DrawFloat(Translate("THEME_EDITOR_LBL_TAG_FILTER_MAX_FONT"),    w.kitList.tagFilterListMaxFont,   8.0f, 40.0f);
				DrawFloat(Translate("THEME_EDITOR_LBL_TAG_FILTER_WIDTH_FONT"), w.kitList.tagFilterListWidthFont, 8.0f, 32.0f);
				DrawFloat(Translate("THEME_EDITOR_LBL_COL_ICON_WIDTH_U"),       w.kitList.colIconWidthU,          1.0f, 12.0f);
				DrawFloat(Translate("THEME_EDITOR_LBL_COL_VALUE_WIDTH_U"),      w.kitList.colValueWidthU,         1.0f, 12.0f);
				EndSection(true);
			} else EndSection(false);
		}

		// Theme color tokens — grouped to match the structure of the on-disk
		// JSON. Keys not present in the active theme are silently skipped so
		// custom palettes that omit a category don't error out.
		void DrawColorTokenGroup(const char* a_label, std::initializer_list<const char*> a_keys)
		{
			auto* tc = ThemeConfig::GetSingleton();
			const auto& data = tc->GetData();

			bool anyPresent = false;
			for (const auto* k : a_keys) {
				if (data.contains(k) && data.at(k).is_array()) {
					anyPresent = true;
					break;
				}
			}
			if (!anyPresent) return;

			ImGui::Spacing();
			ImGui::TextDisabled("%s", a_label);
			for (const auto* k : a_keys) {
				if (!data.contains(k) || !data.at(k).is_array()) continue;
				ImVec4 c = ThemeConfig::GetColor(k);
				ImGui::PushID(k);
				if (ImGui::ColorEdit4(k, &c.x, ImGuiColorEditFlags_AlphaBar)) {
					tc->SetColor(k, c);
					s_dirty = true;
				}
				ImGui::PopID();
			}
		}

		void DrawColorsSection()
		{
			if (!BeginSection(Translate("THEME_EDITOR_SECT_COLORS"))) {
				EndSection(false);
				return;
			}

			if (ImGui::TreeNodeEx(Translate("THEME_EDITOR_COLORS_THEME"),
					ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_SpanAvailWidth)) {
				DrawColorTokenGroup(Translate("THEME_EDITOR_COLORS_STATUS"),
					{ "ERROR", "WARN", "SUCCESS", "CONFIRM", "DECLINE" });
				DrawColorTokenGroup(Translate("THEME_EDITOR_COLORS_FRAME"),
					{ "NONE", "FRAME", "BORDER", "BG", "BG_LIGHT", "WINDOW_BACKGROUND", "SCREEN_BACKGROUND" });
				DrawColorTokenGroup(Translate("THEME_EDITOR_COLORS_BRAND"),
					{ "PRIMARY", "SECONDARY" });
				DrawColorTokenGroup(Translate("THEME_EDITOR_COLORS_TEXT"),
					{ "TEXT", "TEXT_HEADER", "TEXT_DISABLED", "TEXT_ENCHANTED",
					  "TEXT_UNIQUE", "TEXT_ESSENTIAL", "TEXT_UNIQUE_ESSENTIAL" });
				DrawColorTokenGroup(Translate("THEME_EDITOR_COLORS_TABLE"),
					{ "TABLE_BG", "TABLE_BG_ALT", "TABLE_BORDER", "TABLE_SELECTED", "TABLE_HOVER" });
				DrawColorTokenGroup(Translate("THEME_EDITOR_COLORS_FILTERS"),
					{ "FILTER_0", "FILTER_1", "FILTER_2", "FILTER_3", "FILTER_4",
					  "FILTER_5", "FILTER_6", "FILTER_7", "FILTER_8", "FILTER_9" });
				DrawColorTokenGroup(Translate("THEME_EDITOR_COLORS_FORMTYPES"),
					{ "ARMO", "ALCH", "AMMO", "BOOK", "INGR", "KEYM", "MISC", "SCRL",
					  "WEAP", "NPC_", "TREE", "STAT", "CONT", "ACTI", "LIGH", "DOOR",
					  "FURN", "OTFT", "LVLI", "ANIO", "GRAS" });
				ImGui::TreePop();
			}

			if (ImGui::TreeNodeEx(Translate("THEME_EDITOR_COLORS_IMGUI"),
					ImGuiTreeNodeFlags_SpanAvailWidth)) {
				ImGui::TextDisabled("%s", Translate("THEME_EDITOR_COLORS_IMGUI_HINT"));
				ImGui::Spacing();
				auto& style = ImGui::GetStyle();
				for (int i = 0; i < ImGuiCol_COUNT; ++i) {
					const char* name = ImGui::GetStyleColorName(i);
					ImGui::PushID(i);
					if (ImGui::ColorEdit4(name, &style.Colors[i].x, ImGuiColorEditFlags_AlphaBar)) {
						s_dirty = true;
					}
					ImGui::PopID();
				}
				ImGui::TreePop();
			}

			EndSection(true);
		}

		void DrawActionFooter()
		{
			ImGui::Spacing();
			ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal);
			ImGui::Spacing();

			const float full = ImGui::GetContentRegionAvail().x;
			const float gap  = ImGui::GetStyle().ItemSpacing.x;
			// Row 1: Save (full) | Reset Tokens (half) | Reset ImGui Colors (half)
			const float saveBtn  = full;
			const float halfBtn  = (full - gap) * 0.5f;

			const ImVec4 saveColor = s_dirty
				? ThemeConfig::GetColor("CONFIRM")
				: ThemeConfig::GetColor("PRIMARY");
			ImGui::PushStyleColor(ImGuiCol_Button,        saveColor);
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ThemeConfig::GetHover(s_dirty ? "CONFIRM" : "PRIMARY"));
			ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ThemeConfig::GetActive(s_dirty ? "CONFIRM" : "PRIMARY"));
			const std::string saveLabel = IconLabel(ICON_LC_SAVE, s_dirty ? "THEME_EDITOR_BTN_SAVE_DIRTY" : "THEME_EDITOR_BTN_SAVE");
			if (ImGui::Button(saveLabel.c_str(), ImVec2(saveBtn, 0.0f))) {
				if (ThemeConfig::GetSingleton()->SaveCurrentTheme()) {
					s_dirty = false;
				}
			}
			ImGui::PopStyleColor(3);

			ImGui::PushStyleColor(ImGuiCol_Button,        ThemeConfig::GetColor("DECLINE"));
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ThemeConfig::GetHover("DECLINE"));
			ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ThemeConfig::GetActive("DECLINE"));
			if (ImGui::Button(IconLabel(ICON_LC_ROTATE_CCW, "THEME_EDITOR_BTN_RESET").c_str(), ImVec2(halfBtn, 0.0f))) {
				ThemeConfig::GetSingleton()->ResetTokensToDefaults();
				s_dirty = true;
			}
			ImGui::SameLine();
			if (ImGui::Button(IconLabel(ICON_LC_ROTATE_CCW, "THEME_EDITOR_BTN_RESET_IMGUI_COLORS").c_str(), ImVec2(halfBtn, 0.0f))) {
				ThemeConfig::GetSingleton()->ResetImGuiColorsOverride();
				s_dirty = true;
			}
			ImGui::PopStyleColor(3);

			if (s_dirty) {
				const std::string fmt = std::string(ICON_LC_TRIANGLE_ALERT) + "  " + Translate("THEME_EDITOR_UNSAVED_FORMAT");
				ImGui::TextDisabled(fmt.c_str(),
					ThemeConfig::GetSingleton()->GetFilePath().filename().string().c_str());
			}
		}
	}

	// Shared between the Settings > Theme Editor tab and the floating popout
	// window. Caller is responsible for whatever surrounding container (a
	// BeginChild for the tab, an ImGui::Begin for the popout) the body lives
	// inside — DrawThemeEditorBody only draws content.
	void DrawThemeEditorBody()
	{
		ImGui::Spacing();
		ImGui::TextWrapped("%s", Translate("THEME_EDITOR_INTRO"));
		ImGui::Spacing();

		DrawMetaSection();
		DrawImGuiStyleSection();
		DrawLayoutSection();
		DrawWidgetSection();
		DrawColorsSection();
		DrawActionFooter();
	}

	void DrawThemeLayout(std::vector<std::unique_ptr<UITable>>& a_tables)
	{
		(void)a_tables;

		if (!ImGui::BeginChild("##Modex::ThemeEditor::Layout", ImVec2(0, 0), false)) {
			ImGui::EndChild();
			return;
		}

		// Toggle row — opens a floating, always-topmost mirror of this editor
		// so the user can preview token changes while navigating other modules.
		const bool popped = UIManager::GetSingleton()->IsThemeEditorOpen();
		const char* key   = popped ? "THEME_EDITOR_POPOUT_CLOSE" : "THEME_EDITOR_POPOUT_OPEN";
		const char* icon  = popped ? ICON_LC_PICTURE_IN_PICTURE_2 : ICON_LC_PICTURE_IN_PICTURE;
		if (ImGui::Button((std::string(icon) + "  " + Translate(key)).c_str(),
				ImVec2(ImGui::GetContentRegionAvail().x, 0.0f))) {
			UIManager::GetSingleton()->ToggleThemeEditor();
		}

		if (popped) {
			// Editor body is rendered in the floating window — avoid duplicate
			// drawing here so the user isn't editing two mirrors of the same state.
			ImGui::Spacing();
			ImGui::Spacing();
			ImGui::TextWrapped("%s", Translate("THEME_EDITOR_POPOUT_HINT"));
		} else {
			DrawThemeEditorBody();
		}

		ImGui::EndChild();
	}
}
