#pragma once
#include "extern/IconsLucide.h"
#include <PCH.h>

namespace Modex
{

	// TODO: Purge wchar_t and string literals
	const static inline std::filesystem::path json_lang_path = "Data/Interface/Modex/LaNgUaGe/";
	const static inline std::filesystem::path json_font_path = "dAtA/intERFACE/MoDEX/user/FonTs/";

	class Translate
	{
	public:
		std::unordered_map<std::string, std::string> lang;

		static inline Translate* GetSingleton()
		{
			static Translate singleton;
			return &singleton;
		}

		void RefreshLanguage(std::string a_language)
		{
			lang.clear();
			LoadLanguage(a_language);
		}

		// TODO: Marked as duplicate, can be refactored in with PersistentData
		void LoadLanguage(std::string a_path)
		{
			if (!lang.empty()) {
				lang.clear();
			}

			std::filesystem::path full_path = json_lang_path / (a_path + ".json");

			std::ifstream file(full_path);
			if (!file.is_open()) {
				return;
			}

			// If the file is empty, don't bother parsing it.
			file.seekg(0, std::ios::end);
			if (file.tellg() == 0) {
				file.close();
				return;
			}

			// Reset pointer to beginning.
			file.seekg(0, std::ios::beg);

			try {
				nlohmann::json json;
				file >> json;
				file.close();

				for (auto& [key, value] : json.items()) {
					lang[key] = value;
				}

				logger::info("[Translation] Loaded language: {}", a_path);
			} catch (const nlohmann::json::parse_error& e) {
				file.close();
				stl::report_and_fail(std::string("[JSON] Error parsing language file:\n\nValidate your JSON formatting, and try again.\n") + e.what());
			} catch (const nlohmann::json::exception& e) {
				file.close();
				stl::report_and_fail(std::string("[JSON] Error Exception reading language file: ") + e.what());
			}
		}

		// Accessor func for translations. Intentionally return the translation key
		// if not found to offer a fallback for incorrect or missing translations.
		const char* GetTranslation(const std::string& key) const
		{
			auto it = lang.find(key);
			if (it != lang.end()) {
				return it->second.c_str();
			} else {
				return key.c_str();
			}
		}

	private:
		const static inline std::filesystem::path json_lang_path = "Data/Interface/Modex/LaNgUaGe/";

		Translate() = default;
		~Translate() = default;
		Translate(const Translate&) = delete;
		Translate& operator=(const Translate&) = delete;
	};

#define _T(key) Translate::GetSingleton()->GetTranslation(key)
#define _TFM(key, suffix) (std::string(_T(key)) + suffix).c_str()
#define _TICON(icon, key) ((std::string(icon) + _T(key)).c_str())
#define _TICONM(icon, key, suffix) ((std::string(icon) + _T(key) + suffix).c_str())

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
				stl::report_and_fail("No languages found in the language directory. Unable to Load Languages.");
			}

			return languages;
		}

		static const void BuildLanguageList()
		{
			for (const auto& entry : std::filesystem::directory_iterator(json_lang_path)) {
				if (entry.path().extension() == L".json") {
					std::string path = entry.path().filename().string();
					languages.insert(path.substr(0, path.find_last_of('.')));

					logger::info("[Language] Added language to master list: {}", path);
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

		[[nodiscard]] static std::string GetGlyphName(GlyphRanges a_range)
		{
			switch (a_range) {
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
		static inline const std::filesystem::path font_path = "Data/Interface/Modex/Fonts/";
		static inline const std::filesystem::path imgui_font_path = "Data/Interface/ImGuiIcons/Fonts/";
		static inline const std::filesystem::path icon_font_path = "Data/Interface/Modex/Icons/";

		std::filesystem::path GetSystemFontPath(const std::string& a_language);
		void AddDefaultFont();
		void LoadCustomFont(FontData& a_font) noexcept;
		void MergeIconFont(ImGuiIO& io, float a_size) noexcept;

		FontManager() = default;
		~FontManager() = default;
		FontManager(const FontManager&) = delete;
		FontManager& operator=(const FontManager&) = delete;
	};
}