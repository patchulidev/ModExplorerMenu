#include "ThemeConfig.h"
#include "config/UserConfig.h"
#include "external/json_serializers.cpp"
#include "imgui.h"

#include <cctype>

namespace Modex
{
	ImVec4 ThemeConfig::GetColor(const std::string& a_key, float a_alphaMult)
	{
		auto& data = GetSingleton()->m_data;

		auto it = data.find(a_key);
		if (it != data.end()) {
			ImVec4 color = it->get<ImVec4>();
			color.w = std::clamp(color.w * a_alphaMult, 0.0f, 1.0f);
			return color;
		}

		return ImVec4(0.8f, 0.2f, 0.2f, 0.5f); // Default to RED if key not found
	}

	// Hover/active are derived in HSV space so warm/saturated palettes
	// (gold, copper) stay on-hue instead of washing out to beige.
	static ImVec4 LightenHSV(const ImVec4& a_color, float a_dV, float a_dS)
	{
		float h, s, v;
		ImGui::ColorConvertRGBtoHSV(a_color.x, a_color.y, a_color.z, h, s, v);
		v = std::clamp(v + a_dV, 0.0f, 1.0f);
		s = std::clamp(s + a_dS, 0.0f, 1.0f);
		ImVec4 out = a_color;
		ImGui::ColorConvertHSVtoRGB(h, s, v, out.x, out.y, out.z);
		return out;
	}

	ImVec4 ThemeConfig::GetHover(const std::string& a_key, float a_alphaMult)
	{
		return LightenHSV(GetColor(a_key, a_alphaMult), 0.10f, -0.05f);
	}

	ImVec4 ThemeConfig::GetActive(const std::string& a_key, float a_alphaMult)
	{
		return LightenHSV(GetColor(a_key, a_alphaMult), 0.18f, -0.08f);
	}

	ImU32 ThemeConfig::GetColorU32(const std::string& a_key, float a_alphaMult)
	{
		ImVec4 color = GetSingleton()->GetColor(a_key, a_alphaMult);
		return ImGui::ColorConvertFloat4ToU32(color);
	}

	std::optional<GraphicManager::Image> ThemeConfig::GetSplashLogo()
	{
		auto& data = GetSingleton()->m_data;
		auto it = data.find("SPLASH_FILE_PATH");

		if (it != data.end()) {
			std::string logo_path = it->get<std::string>();
			static GraphicManager::Image splash_image;

			if (std::filesystem::exists(logo_path) == false) {
				return std::nullopt;
			}

			if (splash_image.texture == nullptr) {
				GraphicManager::GetD3D11Texture(
					logo_path.c_str(),
					&splash_image.texture,
					splash_image.width,
					splash_image.height
				);
			}

			return splash_image;
		}

		return std::nullopt;
	}

	bool ThemeConfig::Load(bool a_create)
	{
		(void)a_create;
		bool theme_found = false;

		ASSERT_MSG(!std::filesystem::exists(THEMES_JSON_PATH), "Default Theme not found in Modex theme directory!\n{}", THEMES_JSON_PATH.string());

		RefreshAvailableThemes();
		for (const auto& theme : m_availableThemes) {
			if (theme.m_filePath == m_file_path) {
				const bool instantiate = theme.m_name == "default";
				theme_found = ConfigManager::Load(instantiate);
				break;
			}
		}

		if (!theme_found) {
			SetFilePath(THEMES_JSON_PATH);
			UserConfig::Get().theme = "default";
			theme_found = ConfigManager::Load(true);
		}

		ApplyThemeToImGui();
		return theme_found;
	}

	void ThemeConfig::RefreshAvailableThemes()
	{
		m_availableThemes.clear();
		const auto dir = THEMES_JSON_PATH.parent_path();
		if (!std::filesystem::exists(dir) || !std::filesystem::is_directory(dir)) {
			return;
		}
		for (const auto& entry : std::filesystem::directory_iterator(dir)) {
			if (entry.is_regular_file() && entry.path().extension() == ".json") {
				m_availableThemes.emplace_back(entry.path());
			}
		}
	}

	bool ThemeConfig::LoadTheme(const ModexTheme& a_theme)
	{
		Debug("Loading/Switching Theme to '{}'", a_theme.m_name);

		SetFilePath(a_theme.m_filePath);
		const bool ok = ConfigManager::Load(false);
		if (ok) {
			ApplyThemeToImGui();
		}
		return ok;
	}

	ThemeMeta ThemeConfig::GetMeta() const
	{
		ThemeMeta meta;
		auto it = m_data.find("_meta");
		if (it == m_data.end() || !it->is_object()) {
			return meta;
		}
		auto read = [&](const char* key, std::string& out) {
			auto f = it->find(key);
			if (f != it->end() && f->is_string()) {
				out = f->get<std::string>();
			}
		};
		read("name", meta.name);
		read("author", meta.author);
		read("description", meta.description);
		read("version", meta.version);
		return meta;
	}

	void ThemeConfig::ApplyThemeToImGui()
	{
		// Snapshot ImGui's defaults the first time we apply, so future theme
		// switches can restore any style fields that the next theme omits.
		// Without this, e.g. switching from a sharp theme (Rounding=0) back
		// to default would leave rounding stuck at 0.
		static ImGuiStyle s_baseline = ImGui::GetStyle();
		static bool s_baselineCaptured = false;
		auto& style = ImGui::GetStyle();
		if (!s_baselineCaptured) {
			s_baseline = style;
			s_baselineCaptured = true;
		} else {
			style = s_baseline;
		}

		RefreshImGuiColors();

		// _widgets block: tokens for manually-drawn widgets, grouped by
		// widget family. Each subsection is an object whose keys map to
		// fields on the matching WidgetStyle::* struct. Defaults reset
		// every time so a theme that omits sections reverts cleanly.
		m_widgetStyle = WidgetStyle{};
		if (auto wIt = m_data.find("_widgets"); wIt != m_data.end() && wIt->is_object()) {
			auto getSection = [&](const char* a_section) -> const nlohmann::json* {
				auto it = wIt->find(a_section);
				return (it != wIt->end() && it->is_object()) ? &*it : nullptr;
			};
			auto getF = [](const nlohmann::json& a_obj, const char* k, float& dst) {
				auto f = a_obj.find(k);
				if (f != a_obj.end() && f->is_number()) dst = f->get<float>();
			};
			auto getI = [](const nlohmann::json& a_obj, const char* k, int& dst) {
				auto f = a_obj.find(k);
				if (f != a_obj.end() && f->is_number_integer()) dst = f->get<int>();
			};
			auto getV2 = [](const nlohmann::json& a_obj, const char* k, ImVec2& dst) {
				auto f = a_obj.find(k);
				if (f != a_obj.end() && f->is_array() && f->size() == 2) {
					dst.x = (*f)[0].get<float>();
					dst.y = (*f)[1].get<float>();
				}
			};
			auto getV4 = [](const nlohmann::json& a_obj, const char* k, ImVec4& dst) {
				auto f = a_obj.find(k);
				if (f != a_obj.end() && f->is_array() && f->size() == 4) {
					dst.x = (*f)[0].get<float>();
					dst.y = (*f)[1].get<float>();
					dst.z = (*f)[2].get<float>();
					dst.w = (*f)[3].get<float>();
				}
			};

			if (auto* s = getSection("fancy")) {
				getF (*s, "frameRounding", m_widgetStyle.fancy.frameRounding);
				getV2(*s, "framePadding",  m_widgetStyle.fancy.framePadding);
				getF (*s, "glyphFontSize", m_widgetStyle.fancy.glyphFontSize);
			}
			if (auto* s = getSection("tooltip")) {
				getF(*s, "rounding",         m_widgetStyle.tooltip.rounding);
				getF(*s, "widthFactor",      m_widgetStyle.tooltip.widthFactor);
				getF(*s, "accentLineYScale", m_widgetStyle.tooltip.accentLineYScale);
			}
			if (auto* s = getSection("actionButton")) {
				getF(*s, "heightScale", m_widgetStyle.actionButton.heightScale);
			}
			if (auto* s = getSection("tabButton")) {
				getF (*s, "inactiveValueDelta", m_widgetStyle.tabButton.inactiveValueDelta);
				getF (*s, "inactiveAlphaScale", m_widgetStyle.tabButton.inactiveAlphaScale);
				getF (*s, "activeValueDelta",   m_widgetStyle.tabButton.activeValueDelta);
				getF (*s, "activeAlphaScale",   m_widgetStyle.tabButton.activeAlphaScale);
				getF (*s, "borderSize",         m_widgetStyle.tabButton.borderSize);
				getV4(*s, "borderColor",        m_widgetStyle.tabButton.borderColor);
			}
			if (auto* s = getSection("popup")) {
				getF (*s, "widthFactor",       m_widgetStyle.popup.widthFactor);
				getI (*s, "searchMaxRows",     m_widgetStyle.popup.searchMaxRows);
				getV2(*s, "searchItemPadding", m_widgetStyle.popup.searchItemPadding);
				getV2(*s, "searchItemSpacing", m_widgetStyle.popup.searchItemSpacing);
			}
			if (auto* s = getSection("itemPreview")) {
				getF(*s, "nameBarHeightScale",   m_widgetStyle.itemPreview.nameBarHeightScale);
				getF(*s, "gradientHeightScale",  m_widgetStyle.itemPreview.gradientHeightScale);
				getF(*s, "inlineBarDivisor",     m_widgetStyle.itemPreview.inlineBarDivisor);
				getF(*s, "inlineBarMiniDivisor", m_widgetStyle.itemPreview.inlineBarMiniDivisor);
				getF(*s, "minTooltipWidth",      m_widgetStyle.itemPreview.minTooltipWidth);
				getF(*s, "desiredWidthPadFont",  m_widgetStyle.itemPreview.desiredWidthPadFont);
			}
			if (auto* s = getSection("notification")) {
				getF(*s, "tooltipHeightScale", m_widgetStyle.notification.tooltipHeightScale);
				getF(*s, "textTruncDivisor",   m_widgetStyle.notification.textTruncDivisor);
			}
			if (auto* s = getSection("dragOverlay")) {
				getF(*s, "iconFontSize",      m_widgetStyle.dragOverlay.iconFontSize);
				getF(*s, "labelFontDivisor",  m_widgetStyle.dragOverlay.labelFontDivisor);
				getF(*s, "targetFontDivisor", m_widgetStyle.dragOverlay.targetFontDivisor);
				getF(*s, "payloadIconSize",   m_widgetStyle.dragOverlay.payloadIconSize);
				getF(*s, "payloadCountFont",  m_widgetStyle.dragOverlay.payloadCountFont);
				getF(*s, "payloadMinFrameH",  m_widgetStyle.dragOverlay.payloadMinFrameH);
				getF(*s, "payloadMaxFrameH",  m_widgetStyle.dragOverlay.payloadMaxFrameH);
			}
			if (auto* s = getSection("table")) {
				getF(*s, "rowHeightScale",       m_widgetStyle.table.rowHeightScale);
				getF(*s, "rowSpacingBase",       m_widgetStyle.table.rowSpacingBase);
				getF(*s, "statusBarHeightScale", m_widgetStyle.table.statusBarHeightScale);
				getF(*s, "searchKeyRatio",       m_widgetStyle.table.searchKeyRatio);
				getF(*s, "searchModeDivisor",    m_widgetStyle.table.searchModeDivisor);
				getF(*s, "searchFormDivisor",    m_widgetStyle.table.searchFormDivisor);
			}
			if (auto* s = getSection("kitList")) {
				getF(*s, "searchBtnWidthFont",     m_widgetStyle.kitList.searchBtnWidthFont);
				getF(*s, "searchMinInputFont",     m_widgetStyle.kitList.searchMinInputFont);
				getF(*s, "emptyStateBtnFont",      m_widgetStyle.kitList.emptyStateBtnFont);
				getF(*s, "tagFilterListMaxFont",   m_widgetStyle.kitList.tagFilterListMaxFont);
				getF(*s, "tagFilterListWidthFont", m_widgetStyle.kitList.tagFilterListWidthFont);
				getF(*s, "colIconWidthU",          m_widgetStyle.kitList.colIconWidthU);
				getF(*s, "colValueWidthU",         m_widgetStyle.kitList.colValueWidthU);
			}
		}

		// _layout block: multipliers consumed by Modex::Style::Metrics() and
		// Style::Ratio::* in src/ui/style/LayoutMetrics.h. Reset to defaults
		// first so a theme omitting keys reverts cleanly between switches.
		m_layoutOverrides = LayoutOverrides{};
		auto lIt = m_data.find("_layout");
		if (lIt != m_data.end() && lIt->is_object()) {
			auto getF = [&](const char* k, float& dst) {
				auto f = lIt->find(k);
				if (f != lIt->end() && f->is_number()) dst = f->get<float>();
			};
			getF("padX",                   m_layoutOverrides.padX);
			getF("padY",                   m_layoutOverrides.padY);
			getF("gapSm",                  m_layoutOverrides.gapSm);
			getF("gapMd",                  m_layoutOverrides.gapMd);
			getF("radiusSm",               m_layoutOverrides.radiusSm);
			getF("radiusMd",               m_layoutOverrides.radiusMd);
			getF("pillarWidth",            m_layoutOverrides.pillarWidth);
			getF("settingsWidgetWidth",    m_layoutOverrides.settingsWidgetWidth);
			getF("ratioTableEqual",        m_layoutOverrides.ratioTableEqual);
			getF("ratioTableBalanced",     m_layoutOverrides.ratioTableBalanced);
			getF("ratioTablePrimary",      m_layoutOverrides.ratioTablePrimary);
			getF("ratioTruncateNameTight", m_layoutOverrides.ratioTruncateNameTight);
			getF("ratioTruncateNameMid",   m_layoutOverrides.ratioTruncateNameMid);
			getF("ratioTruncateNameWide",  m_layoutOverrides.ratioTruncateNameWide);
		}

		// Optional _style block: per-theme rounding/border/padding overrides.
		// Anything omitted falls back to the captured ImGui defaults above.
		auto styleIt = m_data.find("_style");
		if (styleIt != m_data.end() && styleIt->is_object()) {
			const auto& s = *styleIt;
			auto getFloat = [&](const char* k, float& dst) {
				auto f = s.find(k);
				if (f != s.end() && f->is_number()) dst = f->get<float>();
			};
			auto getVec2 = [&](const char* k, ImVec2& dst) {
				auto f = s.find(k);
				if (f != s.end() && f->is_array() && f->size() == 2) {
					dst.x = (*f)[0].get<float>();
					dst.y = (*f)[1].get<float>();
				}
			};
			getFloat("WindowRounding", style.WindowRounding);
			getFloat("ChildRounding", style.ChildRounding);
			getFloat("FrameRounding", style.FrameRounding);
			getFloat("PopupRounding", style.PopupRounding);
			getFloat("ScrollbarRounding", style.ScrollbarRounding);
			getFloat("GrabRounding", style.GrabRounding);
			getFloat("TabRounding", style.TabRounding);
			getFloat("WindowBorderSize", style.WindowBorderSize);
			getFloat("ChildBorderSize", style.ChildBorderSize);
			getFloat("PopupBorderSize", style.PopupBorderSize);
			getFloat("FrameBorderSize", style.FrameBorderSize);
			getFloat("TabBorderSize", style.TabBorderSize);
			getFloat("ScrollbarSize", style.ScrollbarSize);
			getFloat("GrabMinSize", style.GrabMinSize);
			getFloat("IndentSpacing", style.IndentSpacing);
			getVec2("WindowPadding", style.WindowPadding);
			getVec2("FramePadding", style.FramePadding);
			getVec2("ItemSpacing", style.ItemSpacing);
			getVec2("ItemInnerSpacing", style.ItemInnerSpacing);
			getVec2("CellPadding", style.CellPadding);
		}
	}

	void ThemeConfig::ResetTokensToDefaults()
	{
		m_widgetStyle = WidgetStyle{};
		m_layoutOverrides = LayoutOverrides{};
	}

	void ThemeConfig::RefreshImGuiColors()
	{
		auto& style = ImGui::GetStyle();

		style.Colors[ImGuiCol_FrameBg]              = GetColor("BG");
		style.Colors[ImGuiCol_FrameBgHovered]       = GetHover("BG");
		style.Colors[ImGuiCol_FrameBgActive]        = GetActive("BG");

		style.Colors[ImGuiCol_Button]               = GetColor("PRIMARY");
		style.Colors[ImGuiCol_ButtonHovered]        = GetHover("PRIMARY");
		style.Colors[ImGuiCol_ButtonActive]         = GetActive("PRIMARY");

		style.Colors[ImGuiCol_Header]               = GetColor("PRIMARY");
		style.Colors[ImGuiCol_HeaderHovered]        = GetHover("PRIMARY");
		style.Colors[ImGuiCol_HeaderActive]         = GetActive("PRIMARY");

		style.Colors[ImGuiCol_SliderGrab]           = GetColor("PRIMARY");
		style.Colors[ImGuiCol_SliderGrabActive]     = GetActive("PRIMARY");

		style.Colors[ImGuiCol_ScrollbarBg]          = GetColor("BG");
		style.Colors[ImGuiCol_ScrollbarGrab]        = GetColor("PRIMARY");
		style.Colors[ImGuiCol_ScrollbarGrabHovered] = GetHover("PRIMARY");
		style.Colors[ImGuiCol_ScrollbarGrabActive]  = GetActive("PRIMARY");

		style.Colors[ImGuiCol_Separator]            = GetColor("PRIMARY");
		style.Colors[ImGuiCol_SeparatorHovered]     = GetHover("PRIMARY");
		style.Colors[ImGuiCol_SeparatorActive]      = GetActive("PRIMARY");

		style.Colors[ImGuiCol_ChildBg]              = GetColor("NONE");
		style.Colors[ImGuiCol_WindowBg]             = GetColor("FRAME");
		style.Colors[ImGuiCol_PopupBg]              = GetColor("FRAME");
		style.Colors[ImGuiCol_MenuBarBg]            = GetColor("BG");

		style.Colors[ImGuiCol_TitleBg]              = GetColor("BG");
		style.Colors[ImGuiCol_TitleBgActive]        = GetColor("PRIMARY");
		style.Colors[ImGuiCol_TitleBgCollapsed]     = GetColor("BG");

		style.Colors[ImGuiCol_Tab]                  = GetColor("BG_LIGHT");
		style.Colors[ImGuiCol_TabHovered]           = GetHover("PRIMARY");
		style.Colors[ImGuiCol_TabSelected]          = GetColor("PRIMARY");
		style.Colors[ImGuiCol_TabSelectedOverline]  = GetColor("TEXT_HEADER");
		style.Colors[ImGuiCol_TabDimmed]            = GetColor("BG_LIGHT");
		style.Colors[ImGuiCol_TabDimmedSelected]    = GetColor("SECONDARY");

		style.Colors[ImGuiCol_CheckMark]            = GetColor("TEXT_HEADER");

		style.Colors[ImGuiCol_ResizeGrip]           = GetColor("PRIMARY", 0.4f);
		style.Colors[ImGuiCol_ResizeGripHovered]    = GetHover("PRIMARY");
		style.Colors[ImGuiCol_ResizeGripActive]     = GetActive("PRIMARY");

		style.Colors[ImGuiCol_DragDropTarget]       = GetColor("TEXT_HEADER");
		style.Colors[ImGuiCol_TextSelectedBg]       = GetColor("PRIMARY", 0.5f);
		style.Colors[ImGuiCol_NavCursor]            = GetColor("TEXT_HEADER");

		style.Colors[ImGuiCol_TableHeaderBg]        = GetColor("BG_LIGHT");
		style.Colors[ImGuiCol_TableBorderStrong]    = GetColor("BORDER");
		style.Colors[ImGuiCol_TableBorderLight]     = GetColor("TABLE_BORDER");

		style.Colors[ImGuiCol_Text]                 = GetColor("TEXT");
		style.Colors[ImGuiCol_TextDisabled]         = GetColor("TEXT_DISABLED");

		style.Colors[ImGuiCol_TableRowBg]           = GetColor("TABLE_BG");
		style.Colors[ImGuiCol_TableRowBgAlt]        = GetHover("TABLE_BG_ALT");

		style.Colors[ImGuiCol_Border]               = GetColor("BORDER");

		// Overlay any explicit per-color overrides saved by the editor. These
		// take precedence over the token-derived assignments above so users
		// can pin exact ImGui colors independent of the theme palette.
		auto colIt = m_data.find("_imgui_colors");
		if (colIt != m_data.end() && colIt->is_object()) {
			for (int i = 0; i < ImGuiCol_COUNT; ++i) {
				const char* name = ImGui::GetStyleColorName(i);
				auto f = colIt->find(name);
				if (f != colIt->end() && f->is_array() && f->size() == 4) {
					style.Colors[i].x = (*f)[0].get<float>();
					style.Colors[i].y = (*f)[1].get<float>();
					style.Colors[i].z = (*f)[2].get<float>();
					style.Colors[i].w = (*f)[3].get<float>();
				}
			}
		}
	}

	void ThemeConfig::SetColor(const std::string& a_key, const ImVec4& a_color)
	{
		// Preserve on-disk format: int[0-255][4]. The from_json deserializer
		// expects ints in that range and divides by 255 on read.
		auto toByte = [](float v) {
			const float clamped = std::clamp(v, 0.0f, 1.0f);
			return static_cast<int>(std::round(clamped * 255.0f));
		};
		m_data[a_key] = nlohmann::json::array({
			toByte(a_color.x), toByte(a_color.y), toByte(a_color.z), toByte(a_color.w)
		});
		RefreshImGuiColors();
	}

	void ThemeConfig::ResetImGuiColorsOverride()
	{
		m_data.erase("_imgui_colors");
		RefreshImGuiColors();
	}

	// Helpers for SaveCurrentTheme — we write nested objects directly,
	// shadowing the existing _widgets / _layout / _style blocks on disk
	// so a re-load reproduces the in-memory state.
	static nlohmann::json ToJson(const ImVec2& v) { return nlohmann::json::array({ v.x, v.y }); }
	static nlohmann::json ToJson(const ImVec4& v) { return nlohmann::json::array({ v.x, v.y, v.z, v.w }); }

	bool ThemeConfig::SaveCurrentTheme()
	{
		const auto& w = m_widgetStyle;
		nlohmann::json widgets;
		widgets["fancy"] = {
			{ "frameRounding", w.fancy.frameRounding },
			{ "framePadding",  ToJson(w.fancy.framePadding) },
			{ "glyphFontSize", w.fancy.glyphFontSize },
		};
		widgets["tooltip"] = {
			{ "rounding",         w.tooltip.rounding },
			{ "widthFactor",      w.tooltip.widthFactor },
			{ "accentLineYScale", w.tooltip.accentLineYScale },
		};
		widgets["actionButton"] = {
			{ "heightScale", w.actionButton.heightScale },
		};
		widgets["tabButton"] = {
			{ "inactiveValueDelta", w.tabButton.inactiveValueDelta },
			{ "inactiveAlphaScale", w.tabButton.inactiveAlphaScale },
			{ "activeValueDelta",   w.tabButton.activeValueDelta },
			{ "activeAlphaScale",   w.tabButton.activeAlphaScale },
			{ "borderSize",         w.tabButton.borderSize },
			{ "borderColor",        ToJson(w.tabButton.borderColor) },
		};
		widgets["popup"] = {
			{ "widthFactor",       w.popup.widthFactor },
			{ "searchMaxRows",     w.popup.searchMaxRows },
			{ "searchItemPadding", ToJson(w.popup.searchItemPadding) },
			{ "searchItemSpacing", ToJson(w.popup.searchItemSpacing) },
		};
		widgets["itemPreview"] = {
			{ "nameBarHeightScale",   w.itemPreview.nameBarHeightScale },
			{ "gradientHeightScale",  w.itemPreview.gradientHeightScale },
			{ "inlineBarDivisor",     w.itemPreview.inlineBarDivisor },
			{ "inlineBarMiniDivisor", w.itemPreview.inlineBarMiniDivisor },
			{ "minTooltipWidth",      w.itemPreview.minTooltipWidth },
			{ "desiredWidthPadFont",  w.itemPreview.desiredWidthPadFont },
		};
		widgets["notification"] = {
			{ "tooltipHeightScale", w.notification.tooltipHeightScale },
			{ "textTruncDivisor",   w.notification.textTruncDivisor },
		};
		widgets["dragOverlay"] = {
			{ "iconFontSize",      w.dragOverlay.iconFontSize },
			{ "labelFontDivisor",  w.dragOverlay.labelFontDivisor },
			{ "targetFontDivisor", w.dragOverlay.targetFontDivisor },
			{ "payloadIconSize",   w.dragOverlay.payloadIconSize },
			{ "payloadCountFont",  w.dragOverlay.payloadCountFont },
			{ "payloadMinFrameH",  w.dragOverlay.payloadMinFrameH },
			{ "payloadMaxFrameH",  w.dragOverlay.payloadMaxFrameH },
		};
		widgets["table"] = {
			{ "rowHeightScale",       w.table.rowHeightScale },
			{ "rowSpacingBase",       w.table.rowSpacingBase },
			{ "statusBarHeightScale", w.table.statusBarHeightScale },
			{ "searchKeyRatio",       w.table.searchKeyRatio },
			{ "searchModeDivisor",    w.table.searchModeDivisor },
			{ "searchFormDivisor",    w.table.searchFormDivisor },
		};
		widgets["kitList"] = {
			{ "searchBtnWidthFont",     w.kitList.searchBtnWidthFont },
			{ "searchMinInputFont",     w.kitList.searchMinInputFont },
			{ "emptyStateBtnFont",      w.kitList.emptyStateBtnFont },
			{ "tagFilterListMaxFont",   w.kitList.tagFilterListMaxFont },
			{ "tagFilterListWidthFont", w.kitList.tagFilterListWidthFont },
			{ "colIconWidthU",          w.kitList.colIconWidthU },
			{ "colValueWidthU",         w.kitList.colValueWidthU },
		};
		m_data["_widgets"] = std::move(widgets);

		const auto& l = m_layoutOverrides;
		m_data["_layout"] = {
			{ "padX",                   l.padX },
			{ "padY",                   l.padY },
			{ "gapSm",                  l.gapSm },
			{ "gapMd",                  l.gapMd },
			{ "radiusSm",               l.radiusSm },
			{ "radiusMd",               l.radiusMd },
			{ "pillarWidth",            l.pillarWidth },
			{ "settingsWidgetWidth",    l.settingsWidgetWidth },
			{ "ratioTableEqual",        l.ratioTableEqual },
			{ "ratioTableBalanced",     l.ratioTableBalanced },
			{ "ratioTablePrimary",      l.ratioTablePrimary },
			{ "ratioTruncateNameTight", l.ratioTruncateNameTight },
			{ "ratioTruncateNameMid",   l.ratioTruncateNameMid },
			{ "ratioTruncateNameWide",  l.ratioTruncateNameWide },
		};

		// Snapshot every ImGui style color verbatim. Token-derived colors
		// match their tokens at save time; explicit user edits land here
		// alongside them and are reapplied last on load. ResetImGuiColors
		// drops the block when the user wants pure token derivation back.
		const auto& sty = ImGui::GetStyle();
		nlohmann::json imguiColors = nlohmann::json::object();
		for (int i = 0; i < ImGuiCol_COUNT; ++i) {
			const char* name = ImGui::GetStyleColorName(i);
			imguiColors[name] = ToJson(sty.Colors[i]);
		}
		m_data["_imgui_colors"] = std::move(imguiColors);

		const auto& s = ImGui::GetStyle();
		m_data["_style"] = {
			{ "WindowRounding",    s.WindowRounding },
			{ "ChildRounding",     s.ChildRounding },
			{ "FrameRounding",     s.FrameRounding },
			{ "PopupRounding",     s.PopupRounding },
			{ "ScrollbarRounding", s.ScrollbarRounding },
			{ "GrabRounding",      s.GrabRounding },
			{ "TabRounding",       s.TabRounding },
			{ "WindowBorderSize",  s.WindowBorderSize },
			{ "ChildBorderSize",   s.ChildBorderSize },
			{ "PopupBorderSize",   s.PopupBorderSize },
			{ "FrameBorderSize",   s.FrameBorderSize },
			{ "TabBorderSize",     s.TabBorderSize },
			{ "ScrollbarSize",     s.ScrollbarSize },
			{ "GrabMinSize",       s.GrabMinSize },
			{ "IndentSpacing",     s.IndentSpacing },
			{ "WindowPadding",     ToJson(s.WindowPadding) },
			{ "FramePadding",      ToJson(s.FramePadding) },
			{ "ItemSpacing",       ToJson(s.ItemSpacing) },
			{ "ItemInnerSpacing",  ToJson(s.ItemInnerSpacing) },
			{ "CellPadding",       ToJson(s.CellPadding) },
		};

		return Save();
	}

	void ThemeConfig::SetMeta(const ThemeMeta& a_meta)
	{
		nlohmann::json meta = nlohmann::json::object();
		meta["name"]        = a_meta.name;
		meta["author"]      = a_meta.author;
		meta["description"] = a_meta.description;
		meta["version"]     = a_meta.version;
		m_data["_meta"]     = std::move(meta);
	}

	// Stems map 1:1 to filenames, so reject anything the platform might choke
	// on. Spaces and dots are allowed but never as leading/trailing chars.
	std::string ThemeConfig::ValidateThemeName(const std::string& a_stem, const std::string& a_existingStem)
	{
		if (a_stem.empty()) {
			return "Theme name cannot be empty.";
		}
		for (char c : a_stem) {
			const unsigned char uc = static_cast<unsigned char>(c);
			const bool ok = std::isalnum(uc) || c == '_' || c == '-' || c == ' ' || c == '.';
			if (!ok) {
				return "Theme name may only contain letters, numbers, spaces, '_', '-', or '.'.";
			}
		}
		if (a_stem.front() == '.' || a_stem.back() == '.' ||
			a_stem.front() == ' ' || a_stem.back() == ' ') {
			return "Theme name cannot begin or end with a space or '.'.";
		}
		for (const auto& t : GetSingleton()->m_availableThemes) {
			if (t.m_name == a_stem && t.m_name != a_existingStem) {
				return "A theme with that name already exists.";
			}
		}
		return "";
	}

	bool ThemeConfig::CreateTheme(const std::string& a_stem)
	{
		if (!ValidateThemeName(a_stem).empty()) {
			return false;
		}
		const std::filesystem::path newPath = THEMES_JSON_PATH.parent_path() / (a_stem + ".json");
		if (std::filesystem::exists(newPath)) {
			return false;
		}

		// Carry the editor's current edits into the new theme by reusing the
		// existing serializer — it writes _widgets/_layout/_style from the
		// in-memory tokens + live ImGui style. _meta is mutated in place so
		// the new file lands with its own name.
		if (!m_data.contains("_meta") || !m_data["_meta"].is_object()) {
			m_data["_meta"] = nlohmann::json::object();
		}
		m_data["_meta"]["name"] = a_stem;

		SetFilePath(newPath);
		m_initialized = true;
		if (!SaveCurrentTheme()) {
			return false;
		}

		UserConfig::Get().theme = a_stem;
		UserConfig::GetSingleton()->SaveSettings();

		RefreshAvailableThemes();
		return true;
	}

	bool ThemeConfig::DeleteTheme(const ModexTheme& a_theme)
	{
		if (a_theme.m_name == "default") {
			return false;
		}
		if (!std::filesystem::exists(a_theme.m_filePath)) {
			return false;
		}

		const bool wasActive = (a_theme.m_filePath == m_file_path);

		try {
			std::filesystem::remove(a_theme.m_filePath);
		} catch (...) {
			return false;
		}

		if (wasActive) {
			const std::filesystem::path defaultPath = THEMES_JSON_PATH.parent_path() / "default.json";
			if (std::filesystem::exists(defaultPath)) {
				ModexTheme def(defaultPath);
				LoadTheme(def);
				UserConfig::Get().theme = "default";
				UserConfig::GetSingleton()->SaveSettings();
			}
		}

		RefreshAvailableThemes();
		return true;
	}

	bool ThemeConfig::RenameTheme(const ModexTheme& a_theme, const std::string& a_newStem)
	{
		if (a_theme.m_name == "default") {
			return false;
		}
		if (a_theme.m_name == a_newStem) {
			return true;  // no-op
		}
		if (!ValidateThemeName(a_newStem, a_theme.m_name).empty()) {
			return false;
		}

		std::filesystem::path newPath = a_theme.m_filePath;
		newPath.replace_filename(a_newStem + ".json");
		if (std::filesystem::exists(newPath)) {
			return false;
		}

		try {
			std::filesystem::rename(a_theme.m_filePath, newPath);
		} catch (...) {
			return false;
		}

		if (a_theme.m_filePath == m_file_path) {
			SetFilePath(newPath);
			UserConfig::Get().theme = a_newStem;
			UserConfig::GetSingleton()->SaveSettings();
		}

		RefreshAvailableThemes();
		return true;
	}

	ThemeConfig::ThemeConfig()
	{
		SetFilePath(THEMES_JSON_PATH);
	}
}
