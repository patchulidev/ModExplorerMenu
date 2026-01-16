#pragma once

#include "pch.h"

namespace Modex
{
	const static std::filesystem::path LANGUAGE_JSON_DIR = std::filesystem::path("data") / "interface" / "modex" / "language";
	const static std::filesystem::path IMGUI_FONT_DIR = std::filesystem::path("data") / "interface" / "imguiicons" / "fonts";
	const static std::filesystem::path MODEX_FONT_DIR = std::filesystem::path("data") / "interface" / "modex" / "fonts";
	const static std::filesystem::path MODEX_ICON_DIR = std::filesystem::path("data") / "interface" / "modex" / "icons";

	class Language
	{
	private:
		Language() = default;
		~Language() = default;
		Language(const Language&) = delete;
		Language& operator=(const Language&) = delete;

	public:
		static inline Language* GetSingleton()
		{
			static Language singleton;
			return &singleton;
		}

		enum GlyphRanges
		{
			English,
			Greek,
			Korean,
			Japanese,
			ChineseFull,
			ChineseSimplified,
			Cyrillic,
			Thai,
			Vietnamese
		};

		static inline std::set<std::string> languages;

		static inline std::set<std::string> GetLanguages()
		{
			if (languages.empty()) {
				return {};
			}

			return languages;
		}

		static void BuildLanguageList()
		{
			if (!std::filesystem::exists(LANGUAGE_JSON_DIR)) {
				ASSERT_MSG(true, "Language directory does not exist at: {}", LANGUAGE_JSON_DIR.string());
				return;
			}

			languages.clear();

			for (const auto& entry : std::filesystem::directory_iterator(LANGUAGE_JSON_DIR)) {
				if (entry.path().extension() == L".json") {
					std::string path = entry.path().filename().string();
					languages.insert(path.substr(0, path.find_last_of('.')));

					PrettyLog::Info("Added {} to language list.", path);
				}
			}
		}

		static const ImWchar* GetUserGlyphRange(GlyphRanges a_range)
		{
			auto& io = ImGui::GetIO();

			switch (a_range) {
			case GlyphRanges::English:
				return io.Fonts->GetGlyphRangesDefault();  // Basic Latin, Extended Latin
			case GlyphRanges::Greek:
				return io.Fonts->GetGlyphRangesGreek();  // Default + Greek and Coptic
			case GlyphRanges::Korean:
				return io.Fonts->GetGlyphRangesKorean();  // Default + Korean characters
			case GlyphRanges::Japanese:
				return io.Fonts->GetGlyphRangesJapanese();  // Default + Hiragana, Katakana, Half-Width, Selection of 2999 Ideographs
			case GlyphRanges::ChineseFull:
				return io.Fonts->GetGlyphRangesChineseFull();  // Default + Half-Width + Japanese Hiragana/Katakana + full set of about 21000 CJK Unified Ideographs
			case GlyphRanges::ChineseSimplified:
				return io.Fonts->GetGlyphRangesChineseSimplifiedCommon();  // Default + Half-Width + Japanese Hiragana/Katakana + set of 2500 CJK Unified Ideographs for common simplified Chinese
			case GlyphRanges::Cyrillic:
				return io.Fonts->GetGlyphRangesCyrillic();  // Default + about 400 Cyrillic characters
			case GlyphRanges::Thai:
				return io.Fonts->GetGlyphRangesThai();  // Default + Thai characters
			case GlyphRanges::Vietnamese:
				return io.Fonts->GetGlyphRangesVietnamese();  // Default + Vietnamese characters
			default:
				return io.Fonts->GetGlyphRangesDefault();  // Basic Latin, Extended Latin
			};
		}

		[[nodiscard]] static GlyphRanges GetGlyphRange(std::string a_language)
		{
			if (a_language.compare("English") == 0) {
				return GlyphRanges::English;
			} else if (a_language.compare("Greek") == 0) {
				return GlyphRanges::Greek;
			} else if (a_language.compare("Korean") == 0) {
				return GlyphRanges::Korean;
			} else if (a_language.compare("Japanese") == 0) {
				return GlyphRanges::Japanese;
			} else if (a_language.compare("ChineseFull") == 0) {
				return GlyphRanges::ChineseFull;
			} else if (a_language.compare("ChineseSimplified") == 0) {
				return GlyphRanges::ChineseSimplified;
			} else if (a_language.compare("Cyrillic") == 0) {
				return GlyphRanges::Cyrillic;
			} else if (a_language.compare("Thai") == 0) {
				return GlyphRanges::Thai;
			} else if (a_language.compare("Vietnamese") == 0) {
				return GlyphRanges::Vietnamese;
			} else {
				return GlyphRanges::English;
			}
		}

		// TODO: Yuck
		[[nodiscard]] static std::string GetGlyphName(int a_range)
		{
			const auto range = magic_enum::enum_cast<GlyphRanges>(a_range);
			switch (range.value_or(GlyphRanges::English)) {
			case GlyphRanges::English:
				return "English";
			case GlyphRanges::Greek:
				return "Greek";
			case GlyphRanges::Korean:
				return "Korean";
			case GlyphRanges::Japanese:
				return "Japanese";
			case GlyphRanges::ChineseFull:
				return "ChineseFull";
			case GlyphRanges::ChineseSimplified:
				return "ChineseSimplified";
			case GlyphRanges::Cyrillic:
				return "Cyrillic";
			case GlyphRanges::Thai:
				return "Thai";
			case GlyphRanges::Vietnamese:
				return "Vietnamese";
			default:
				return "English";
			};
		}

		// Not going to bother making this fancy.
		[[nodiscard]] static std::set<std::string> GetListOfGlyphNames()
		{
			return { "English", "Greek", "Korean", "Japanese", "ChineseFull", "ChineseSimplified", "Cyrillic", "Thai", "Vietnamese" };
		}
	};

	class FontManager
	{
	public:
		enum class FontSize
		{
			Small,
			Medium,
			Large,
		};

		struct FontData
		{
			std::string name;
			std::filesystem::path fullPath;
		};

		static inline FontManager* GetSingleton()
		{
			static FontManager singleton;
			return &singleton;
		}

		FontManager() = default;
		~FontManager() = default;
		FontManager(const FontManager&) = delete;
		FontManager& operator=(const FontManager&) = delete;

		// Returns std::map<std::string, FontData>
		[[nodiscard]] static inline FontData& GetFontData(const std::string& a_name)
		{
			return font_library[a_name];
		}

		// Returns a sorted list of registered fonts.
		[[nodiscard]] static inline std::vector<std::string> GetFontLibrary()
		{
			std::vector<std::string> fonts;
			for (const auto& [key, value] : font_library) {
				fonts.push_back(key);
			}

			std::sort(fonts.begin(), fonts.end());

			if (font_library.find("Default") == font_library.end()) {
				fonts.insert(fonts.begin(), "Default"); // ?
			}

			return fonts;
		}

		void SetStartupFont();
		void BuildFontLibrary();
		
	private:
		static inline std::map<std::string, FontData> font_library;

		std::filesystem::path GetSystemFontPath(const std::string& a_language);
		void AddDefaultFont();
		void LoadCustomFont(FontData& a_font) noexcept;
		void MergeIconFont(ImGuiIO& io, float a_size) noexcept;
	};
}