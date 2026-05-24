#pragma once

#include "config/ThemeConfig.h"
#include "imgui.h"

namespace Modex::Style
{
	// Font-proportional layout tokens, recomputed each call from
	// ImGui::GetFontSize() and ThemeConfig::GetLayoutOverrides().
	// Generalizes the per-widget pattern in UIKitList::GetTokens()
	// and lets a future configurator overlay tune the proportions
	// at runtime via the _layout JSON block.
	//
	// At 16px font with default overrides: u=4, padX=12, padY=8,
	// gapMd=20, radiusSm=4, radiusMd=8, pillarWidth=5, settingsWidgetWidth=144.
	struct LayoutMetrics
	{
		float font;
		float u;                    // base unit = font * 0.25
		float padX;                 // u * overrides.padX
		float padY;                 // u * overrides.padY
		float gapSm;                // u * overrides.gapSm — tight stacking gap
		float gapMd;                // u * overrides.gapMd — badge / stats gap
		float radiusSm;             // u * overrides.radiusSm
		float radiusMd;             // u * overrides.radiusMd
		float pillarWidth;          // u * overrides.pillarWidth + 1px floor
		float settingsWidgetWidth;  // font * overrides.settingsWidgetWidth
	};

	inline LayoutMetrics Metrics()
	{
		const float font = ImGui::GetFontSize();
		const float u    = font * 0.25f;
		const auto& o    = ThemeConfig::GetLayoutOverrides();
		LayoutMetrics m{};
		m.font                 = font;
		m.u                    = u;
		m.padX                 = u * o.padX;
		m.padY                 = u * o.padY;
		m.gapSm                = u * o.gapSm;
		m.gapMd                = u * o.gapMd;
		m.radiusSm             = u * o.radiusSm;
		m.radiusMd             = u * o.radiusMd;
		m.pillarWidth          = u * o.pillarWidth + 1.0f;
		m.settingsWidgetWidth  = font * o.settingsWidgetWidth;
		return m;
	}

	// Layout split ratios. Inline accessors (not constexpr) so a future
	// configurator overlay can tune them at runtime through the
	// _layout JSON block. Used for table:panel splits and inline text
	// truncation widths.
	namespace Ratio
	{
		inline float TableEqual()         { return ThemeConfig::GetLayoutOverrides().ratioTableEqual; }
		inline float TableBalanced()      { return ThemeConfig::GetLayoutOverrides().ratioTableBalanced; }
		inline float TablePrimary()       { return ThemeConfig::GetLayoutOverrides().ratioTablePrimary; }
		inline float TruncateNameTight()  { return ThemeConfig::GetLayoutOverrides().ratioTruncateNameTight; }
		inline float TruncateNameMid()    { return ThemeConfig::GetLayoutOverrides().ratioTruncateNameMid; }
		inline float TruncateNameWide()   { return ThemeConfig::GetLayoutOverrides().ratioTruncateNameWide; }
	}

	// Tight indent (1u). Matches the historical 4.0f at 16px font.
	inline void GroupIndent()     { ImGui::Indent(Metrics().u); }
	inline void GroupUnindent()   { ImGui::Unindent(Metrics().u); }
}
