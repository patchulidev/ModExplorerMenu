#pragma once

#include "config/ConfigManager.h"
#include "core/Graphic.h"

namespace Modex
{
	static const std::filesystem::path THEMES_JSON_PATH = 
	std::filesystem::path("data") / "interface" / "modex" / "user" / "themes" / "default.json";

	struct ModexTheme
	{
		std::string m_name;
		std::filesystem::path m_filePath;

		ModexTheme(const std::filesystem::path& a_path) : m_filePath(a_path)
		{
			m_name = a_path.stem().string();
			m_filePath = a_path;
		}
	};

	struct ThemeMeta
	{
		std::string name;
		std::string author;
		std::string description;
		std::string version;
	};

	// Tokens for widgets that draw their own frames/rounding/sizing outside
	// the stock ImGui style. Grouped per-widget so the configurator UI can
	// surface one section per family. Defaults preserve the original
	// hardcoded values — a theme that omits "_widgets" is pixel-identical
	// to before this refactor.
	struct WidgetStyle
	{
		// FancyInputText / FancyDropdown / FancyDropdownButton — the
		// rounded "search bar" look shared by the field, combo, and button.
		struct Fancy {
			float  frameRounding = 12.0f;
			ImVec2 framePadding  = ImVec2(8.0f, 8.0f);
			float  glyphFontSize = 18.0f;  // search / chevron overlay glyph
		} fancy;

		// FancyTooltip surface (the locale-keyed tooltip that follows the
		// mouse) — width is a fraction of the display, accent line sits at
		// a font-scaled offset under the first line of text.
		struct Tooltip {
			float rounding         = 5.0f;
			float widthFactor      = 0.20f;  // of GetIO().DisplaySize.x
			float accentLineYScale = 1.5f;   // font * scale = accent y offset
		} tooltip;

		// UICustom::ActionButton — used by all UIContainers action panels.
		struct ActionButton {
			float heightScale = 1.5f;  // font * scale
		} actionButton;

		// UIContainers::TabButton — the filter tree node buttons.
		struct TabButton {
			float  inactiveValueDelta = -0.20f;
			float  inactiveAlphaScale = 0.5f;
			float  borderSize         = 1.0f;
			ImVec4 borderColor        = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
		} tabButton;

		// UIPopup* dialogs + SearchSystem dropdown popup.
		struct Popup {
			float  widthFactor       = 0.25f;             // viewport fraction
			int    searchMaxRows     = 20;                // max popup height in rows
			ImVec2 searchItemPadding = ImVec2(5.0f, 5.0f);
			ImVec2 searchItemSpacing = ImVec2(5.0f, 5.0f);
		} popup;

		// ItemPreview component (the right-hand inspect panel).
		struct ItemPreview {
			float nameBarHeightScale   = 2.5f;   // font * scale
			float gradientHeightScale  = 2.0f;   // frameH * scale
			float inlineBarDivisor     = 3.0f;   // bar width = content / divisor
			float inlineBarMiniDivisor = 4.0f;
			float minTooltipWidth      = 200.0f;
			float desiredWidthPadFont  = 5.0f;   // padding term in getDesiredWidth
		} itemPreview;

		// UINotification message rows + transient tooltip strip.
		struct Notification {
			float tooltipHeightScale = 1.5f;   // frameH * scale
			float textTruncDivisor   = 1.5f;
		} notification;

		// UITable drag/drop visual overlay (the giant icon + label that
		// fills the drop target table) and the cursor-following payload tip.
		struct DragOverlay {
			float iconFontSize       = 72.0f;
			float labelFontDivisor   = 2.0f;   // iconFontSize / divisor
			float targetFontDivisor  = 2.5f;
			float payloadIconSize    = 24.0f;
			float payloadCountFont   = 36.0f;
			float payloadMinFrameH   = 5.0f;   // frameH * scale
			float payloadMaxFrameH   = 10.0f;
		} dragOverlay;

		// UITable layout knobs — row height, search bar splits, status bar.
		struct Table {
			float rowHeightScale       = 1.75f;
			float rowSpacingBase       = 3.0f;
			float statusBarHeightScale = 1.25f;
			float searchKeyRatio       = 0.45f;
			float searchModeDivisor    = 7.5f;
			float searchFormDivisor    = 2.5f;
		} table;

		// UIKitList — column widths, search button, popup dimensions.
		// Widths expressed as multipliers of the current font size.
		struct KitList {
			float searchBtnWidthFont     = 7.5f;
			float searchMinInputFont     = 8.0f;
			float emptyStateBtnFont      = 12.0f;
			float tagFilterListMaxFont   = 20.0f;
			float tagFilterListWidthFont = 14.0f;
			float colIconWidthU          = 5.0f;  // col = font + u * scale
			float colValueWidthU         = 5.0f;
		} kitList;
	};

	// Multipliers consumed by Modex::Style::Metrics() and Style::Ratio::*
	// (src/ui/style/LayoutMetrics.h). Defaults match the values baked
	// into the previous refactor, so a theme omitting "_layout" is
	// pixel-identical to before this change.
	struct LayoutOverrides
	{
		// Multiples of u (= font * 0.25)
		float padX                = 3.0f;
		float padY                = 2.0f;
		float gapSm               = 2.0f;
		float gapMd               = 5.0f;
		float radiusSm            = 1.0f;
		float radiusMd            = 2.0f;
		float pillarWidth         = 1.0f;  // applied as u * pillarWidth + 1.0f floor

		// Multiple of font directly
		float settingsWidgetWidth = 9.0f;

		// Layout split / truncation ratios (no font scaling)
		float ratioTableEqual         = 0.50f;
		float ratioTableBalanced      = 0.60f;
		float ratioTablePrimary       = 0.75f;
		float ratioTruncateNameTight  = 1.0f / 2.25f;
		float ratioTruncateNameMid    = 0.65f;
		float ratioTruncateNameWide   = 0.80f;
	};

	class ThemeConfig : public ConfigManager
	{
	private:
		std::vector<ModexTheme> m_availableThemes;
		WidgetStyle             m_widgetStyle;
		LayoutOverrides         m_layoutOverrides;

	public:
		static inline ThemeConfig* GetSingleton()
		{
			static ThemeConfig singleton;
			return std::addressof(singleton);
		}

		virtual bool Load(bool a_create) override;
		bool LoadTheme(const ModexTheme& a_theme);
		void ApplyThemeToImGui();

		ThemeMeta GetMeta() const;

		static const WidgetStyle&     GetWidgetStyle()     { return GetSingleton()->m_widgetStyle; }
		static const LayoutOverrides& GetLayoutOverrides() { return GetSingleton()->m_layoutOverrides; }

		// Mutable accessors for the in-game theme configurator. Edits take
		// effect immediately on the next frame (no re-apply needed for
		// _widgets/_layout, since the Style helpers re-read each call).
		// Call SaveCurrentTheme() to persist to disk.
		static WidgetStyle&     GetWidgetStyleMutable()     { return GetSingleton()->m_widgetStyle; }
		static LayoutOverrides& GetLayoutOverridesMutable() { return GetSingleton()->m_layoutOverrides; }

		// Serializes the current WidgetStyle + LayoutOverrides + ImGui
		// style back into _widgets / _layout / _style blocks on m_data
		// and writes to disk. Existing color keys and _meta are left intact.
		bool SaveCurrentTheme();

		// Restore all WidgetStyle / LayoutOverrides fields to compiled-in
		// defaults. Does not touch ImGui style or color keys.
		void ResetTokensToDefaults();

		ThemeConfig();

		static std::optional<GraphicManager::Image> GetSplashLogo();

		// Could generate and cache a struct to handle this behavior...
		static ImVec4 GetColor(const std::string& a_key, float a_alphaMult = 1.0f);
		static ImVec4 GetHover(const std::string& a_key, float a_alphaMult = 1.0f);
		static ImVec4 GetActive(const std::string& a_key, float a_alphaMult = 1.0f);

		static ImU32 GetColorU32(const std::string& a_key, float a_alphaMult = 1.0f);

		static const std::vector<ModexTheme>& GetAvailableThemes() { return GetSingleton()->m_availableThemes; }
	};
}
