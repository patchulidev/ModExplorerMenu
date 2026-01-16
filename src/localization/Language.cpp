#include "Language.h"
#include "external/icons/IconsLucide.h"

#include "config/UserConfig.h"
#include "external/magic_enum.hpp"

namespace Modex
{
	void FontManager::BuildFontLibrary()
	{
		if (!std::filesystem::exists(MODEX_FONT_DIR)) {
			ASSERT_MSG(true, "Modex Font directory does not exist at: {}", MODEX_FONT_DIR.string());
			return;
		}

		for (const auto& entry : std::filesystem::directory_iterator(MODEX_FONT_DIR)) {
			if (entry.path().filename().extension() != ".ttf" && entry.path().filename().extension() != ".otf") {
				continue;
			}

			FontData data;
			data.name = entry.path().stem().string();
			data.fullPath = entry.path().string();

			font_library[data.name] = data;
		}

		// Search for fonts relative to ImGui Icon Library directory.
		if (std::filesystem::exists(IMGUI_FONT_DIR)) {
			for (const auto& entry : std::filesystem::directory_iterator(IMGUI_FONT_DIR)) {
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
		a_size -= 2.0f;
		
		ImFontConfig config;
		config.MergeMode = true;
		config.GlyphMinAdvanceX = 10.0f;
		config.GlyphExtraSpacing.x = 5.0f;
		config.SizePixels = a_size;
		config.GlyphOffset.y = floorf(a_size / 4.0f);
		// config.GlyphOffset.y = (a_size / 2.0f) / 1.5f;

		static const ImWchar icon_ranges[] = { ICON_MIN_LC, ICON_MAX_LC, 0 };
		static const std::filesystem::path icons(MODEX_ICON_DIR / "lucide.ttf");

		if (!std::filesystem::exists(icons)) {
			ASSERT_MSG(true, "Unable to locate icon font file at: {}", icons.string());
		} else {
			io.Fonts->AddFontFromFileTTF(icons.string().c_str(), a_size + 3.0f, &config, icon_ranges);
		}
	}

	std::filesystem::path FontManager::GetSystemFontPath(const std::string& a_language)
	{		
    	size_t len = 0;
		char* sysroot = nullptr;
		static std::filesystem::path win_fonts;
		if (_dupenv_s(&sysroot, &len, "SystemRoot") == 0 && sysroot) {
			win_fonts = std::filesystem::path(sysroot) / "Fonts";
			free(sysroot);
		}

		if (win_fonts.empty()) {
			ASSERT_MSG(true, "Unable to locate Windows Fonts directory.");
			return std::filesystem::path();
		}

		static const std::map<std::string, std::string> system_fonts = {
			{ "English", "arial.ttf" },
			{ "ChineseFull", "simsun.ttc" },
			{ "ChineseSimplified", "simsun.ttc" },
			{ "Japanese", "msgothic.ttc" },
			{ "Korean", "malgun.ttf" },
			{ "Cyrillic", "arial.ttf" }
		};
		
		auto it = system_fonts.find(a_language);
		if (it != system_fonts.end()) {
			auto fullpath = win_fonts / it->second;
			if (std::filesystem::exists(fullpath)) {
				return fullpath;
			}
		}
		
		auto fallback = win_fonts / "arial.ttf";
		
		if (!std::filesystem::exists(fallback)) {
			PrettyLog::Warn("Default windows arial.ttf font not found. Please report this issue.", fallback. string());
		}
		
		return fallback;
	}

	void FontManager::AddDefaultFont()
	{
		auto& io = ImGui::GetIO();
		auto& config = UserConfig::Get();

		float size = config.globalFontSize;
		const auto glyph = magic_enum::enum_cast<Language::GlyphRanges>(config.glyphRange).value_or(Language::GlyphRanges::English);
		const auto glyph_name = magic_enum::enum_name(glyph).data();
		const auto imgui_glyphs = Language::GetUserGlyphRange(glyph);
		const auto fullpath = GetSystemFontPath(glyph_name);

		if (std::filesystem::exists(fullpath)) {
			PrettyLog::Info("Added default system font: \"{}\" at path: \"{}\"", fullpath.filename().string(), fullpath.string());
			io.Fonts->AddFontFromFileTTF(fullpath.string().c_str(), size, NULL, imgui_glyphs);
			MergeIconFont(io, size);
		} else {
			PrettyLog::Warn("System font not found: {}. Using ImGui proggy-font default. This is unexpected, but not critical.", MODEX_FONT_DIR.string());
			io.Fonts->AddFontDefault();
			MergeIconFont(io, size);
		}
	}

	// Do not call this function directly. See Menu::RebuildFontAtlas().
	// Builds a single font from file with appropriate glyph range.
	void FontManager::LoadCustomFont(FontData& a_font) noexcept
	{
		auto& io = ImGui::GetIO();
		const auto& config = UserConfig::Get();
		const auto glyph = magic_enum::enum_cast<Language::GlyphRanges>(config.glyphRange).value_or(Language::GlyphRanges::English);
		const auto glyph_range = Language::GetUserGlyphRange(glyph);

		float size = config.globalFontSize;
		io.Fonts->AddFontFromFileTTF(a_font.fullPath.string().c_str(), size, NULL, glyph_range);
		MergeIconFont(io, size);
	}

	// Runs after Settings are loaded.
	void FontManager::SetStartupFont()
	{
		auto& config = UserConfig::Get();
		auto data = GetFontData(config.globalFont);

		if (data.name.empty() || data.fullPath.empty() || data.name == "Default") {
			AddDefaultFont();
			return;
		}

		if (std::filesystem::exists(data.fullPath) == true) {
			PrettyLog::Info("Successfully detected custom font: \"{}\" at path: \"{}\"", data.name, data.fullPath.string());
			LoadCustomFont(data);
		} else {
			PrettyLog::Info("No custom font usage detected, defaulting to system font.");
			AddDefaultFont();
		}
	}

}