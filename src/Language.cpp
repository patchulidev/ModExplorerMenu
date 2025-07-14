#include "include/L/Language.h"
#include "include/S/Settings.h"

namespace Modex
{
	void FontManager::BuildFontLibrary()
	{
		// Note to self: do not remove this fucking warning.
		if (std::filesystem::exists(font_path) == false) {
			stl::report_and_fail("FATAL ERROR: Font and/or Graphic asset directory not found. This is because Modex cannot locate the path 'Data/Interface/Modex/Fonts/'. Check your installation.");
			return;
		}

		for (const auto& entry : std::filesystem::directory_iterator(font_path)) {
			if (entry.path().filename().extension() != ".ttf" && entry.path().filename().extension() != ".otf") {
				continue;
			}

			FontData data;
			data.name = entry.path().stem().string();
			data.fullPath = entry.path().string();

			font_library[data.name] = data;
		}

		// Search for fonts relative to ImGui Icon Library directory.
		if (std::filesystem::exists(imgui_font_path)) {
			for (const auto& entry : std::filesystem::directory_iterator(imgui_font_path)) {
				if (entry.path().filename().extension() != ".ttf" && entry.path().filename().extension() != ".otf") {
					continue;
				}

				FontData data;
				data.name = entry.path().stem().string();
				data.fullPath = entry.path().string();

				font_library[data.name] = data;
			}
		}
	}

	// Subtracting -1 from size results in odd font sizes. Good idea?
	void FontManager::MergeIconFont(ImGuiIO& io, float a_size) noexcept
	{
		ImFontConfig config;
		config.MergeMode = true;
		config.GlyphMinAdvanceX = 10.0f;
		config.GlyphExtraSpacing.x = 5.0f;
		config.SizePixels = a_size;
		config.GlyphOffset.y = (a_size / 2.0f) / 1.5f;

		static const ImWchar icon_ranges[] = { ICON_MIN_LC, ICON_MAX_LC, 0 };
		static const std::filesystem::path icons(icon_font_path / "lucide.ttf");

		if (!std::filesystem::exists(icons)) {
			logger::error("[FontManager] Icon font file not found: {}", icons.string());
		} else {
			io.Fonts->AddFontFromFileTTF(icons.string().c_str(), a_size + 3.0f, &config, icon_ranges);
		}
	}

	std::filesystem::path FontManager::GetSystemFontPath(const std::string& a_language)
	{
		std::string windows = std::getenv("WINDIR") ? std::getenv("WINDIR") : "C:\\Windows";
		std::filesystem::path system_path = std::filesystem::path(windows) / "Fonts";

		static std::map<std::string, std::string> system_fonts = {
			{ "English", "arial.ttf" },
			{ "ChineseFull", "simsun.ttc" },
			{ "ChineseSimplified", "simsun.ttc" },
			{ "Japanese", "msgothic.ttc" },
			{ "Korean", "malgun.ttf" },
			{ "Cyrillic", "arial.ttf" }
		};

		auto it = system_fonts.find(a_language);
		if (it != system_fonts.end()) {
			auto match = system_path / it->second;
			if (std::filesystem::exists(match)) {
				return match;
			}
		}

		return std::filesystem::path(windows) / "Fonts" / "arial.ttf";  // Default fallback font
	}

	void FontManager::AddDefaultFont()
	{
		auto& config = Settings::GetSingleton()->GetConfig();
		auto& io = ImGui::GetIO();

		float size = config.globalFontSize;
		const auto glyphRange = Language::GetUserGlyphRange(config.glyphRange);
		const auto language = Language::GetGlyphName(config.glyphRange);
		const auto fontPath = GetSystemFontPath(language);

		if (std::filesystem::exists(fontPath)) {
			io.Fonts->AddFontFromFileTTF(fontPath.string().c_str(), size, NULL, glyphRange);
			MergeIconFont(io, size);
		} else {
			logger::error("[FontManager] System font not found: {}. Using ImGui default.", fontPath.string());
			io.Fonts->AddFontDefault();
			MergeIconFont(io, size);
		}
	}

	// Do not call this function directly. See Menu::RebuildFontAtlas().
	// Builds a single font from file with appropriate glyph range.
	void FontManager::LoadCustomFont(FontData& a_font) noexcept
	{
		auto& io = ImGui::GetIO();
		
		const auto& config = Settings::GetSingleton()->GetConfig();
		const auto& glyphRange = Language::GetUserGlyphRange(config.glyphRange);
		
		float size = config.globalFontSize;
		io.Fonts->AddFontFromFileTTF(a_font.fullPath.string().c_str(), size, NULL, glyphRange);

		MergeIconFont(io, size);
	}

	// Runs after Settings are loaded.
	void FontManager::SetStartupFont()
	{
		auto& config = Settings::GetSingleton()->GetConfig();
		auto data = GetFontData(config.globalFont);

		if (data.name.empty() || data.fullPath.empty() || data.name == "Default") {
			AddDefaultFont();
			return;
		}

		if (std::filesystem::exists(data.fullPath) == true) {
			logger::info("[Font Manager] Loading custom font: {}", data.name);
			LoadCustomFont(data);
		} else {
			logger::info("[Font Manager] No Custom font specified. Loading default font.");
			AddDefaultFont();
		}
	}

}