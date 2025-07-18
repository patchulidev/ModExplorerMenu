#pragma once

#include "include/G/Graphic.h"

namespace Modex
{

	class Settings
	{
	public:
		// void LoadStyleTheme(ImGuiStyle a_theme); - DEPRECATED

		void GetIni(const std::filesystem::path& a_path, const std::function<void(CSimpleIniA&)> a_func);
		void LoadSettings(const std::filesystem::path& a_path);
		void LoadUserFontSetting();
		void SaveSettings();
		void LoadMasterIni(CSimpleIniA& a_ini);

		void CreateDefaultMaster();
		static void FormatMasterIni(CSimpleIniA& a_ini);

		static inline Settings* GetSingleton()
		{
			static Settings singleton;
			return std::addressof(singleton);
		}

		static inline const std::filesystem::path ini_theme_path = "Data/Interface/Modex/themes/";
		static inline const std::filesystem::path ini_main_path = "Data/Interface/Modex/Modex.ini";

		enum SECTION
		{
			Window,
			Frame,
			Text,
			Table,
			Widgets,
			Images,
			Main,
			Modules,
			AddItem,
			Object,
			FormLookup,
			NPC,
			Teleport,
		};

		static inline std::map<SECTION, const char*> rSections = {
			{ Window, "Window" },
			{ Text, "Text" },
			{ Table, "Table" },
			{ Widgets, "Widgets" },
			{ Images, "Images & Icons" },
			{ Main, "Main" },
			{ Modules, "Modules" },
			{ AddItem, "AddItem" },
			{ Object, "Object" },
			{ FormLookup, "FormLookup" },
			{ NPC, "NPC" },
			{ Teleport, "Teleport" }
		};

		struct Config
		{
			// General
			std::string theme = "Default";
			uint32_t showMenuKey = 211;
			uint32_t showMenuModifier = 0;
			int modListSort = 0;        // 0 = Alphabetical, 1 = Load Order (ASC) 2 = Load Order (DESC)
			int uiScaleVertical = 100;  // 80, 90, 100, 110, 120 (Should be a slider..)
			int uiScaleHorizontal = 100;
			bool fullscreen = false;
			bool pauseGame = false;
			bool disableInMenu = false;

			// Font Stuff
			std::string language = "English";
			Language::GlyphRanges glyphRange = Language::GlyphRanges::English;
			std::string globalFont = "Default";
			float globalFontSize = 16.0f;

			// Modules
			int defaultShow = 1;  // 0 = Home, 1 = AddItem, 2 = Object, 3 = NPC, 4 = Teleport, 5 = Settings
			bool showHomeMenu = false;
			bool showAddItemMenu = true;
			bool showObjectMenu = true;
			bool showNPCMenu = true;
			bool showTeleportMenu = true;

			// Hiden from User
			ImVec2 screenScaleRatio;
		};

		struct Setting
		{
			Config config;
		};

		Setting def;
		Setting user;

		// https://github.com/powerof3/PhotoMode | License: MIT
		template <class T>
		static std::string ToString(const T& a_style, bool a_hex = false)
		{
			if constexpr (std::is_same_v<ImVec4, T>) {
				if (a_hex) {
					return std::format("#{:02X}{:02X}{:02X}{:02X}", std::uint8_t(255.0f * a_style.x), std::uint8_t(255.0f * a_style.y), std::uint8_t(255.0f * a_style.z), std::uint8_t(255.0f * a_style.w));
				}
				return std::format("{}{},{},{},{}{}", "{", std::uint8_t(255.0f * a_style.x), std::uint8_t(255.0f * a_style.y), std::uint8_t(255.0f * a_style.z), std::uint8_t(255.0f * a_style.w), "}");
			} else if constexpr (std::is_same_v<ImVec2, T>) {
				return std::format("{}{},{}{}", "{", a_style.x, a_style.y, "}");
			} else if constexpr (std::is_same_v<std::string, T>) {
				return a_style;
			} else if constexpr (std::is_same_v<FontManager::FontData, T>) {
				return a_style.name;
			} else if constexpr (std::is_same_v<GraphicManager::Image, T>) {
				return GraphicManager::GetImageName(a_style);
			} else if constexpr (std::is_same_v<Language::GlyphRanges, T>) {
				return Language::GetGlyphName(a_style);
			} else if constexpr (std::is_same_v<float, T>) {
				return std::format("{:.3f}", a_style);
			} else if constexpr (std::is_same_v<bool, T>) {
				return a_style ? "true" : "false";
			} else if constexpr (std::is_same_v<int, T>) {
				return std::to_string(a_style);
			} else {
				stl::report_and_fail("Unsupported type for ToString");  // FIXME: static_assert?
			}
		}

		// https://github.com/powerof3/PhotoMode | License: MIT
		template <class T>
		static std::pair<T, bool> GetColor(std::string& a_str)
		{
			if constexpr (std::is_same_v<ImVec4, T>) {
				static std::regex rgb_pattern("\\{([0-9]+),([0-9]+),([0-9]+),([0-9]+)\\}");
				static std::regex hex_pattern("#([0-9a-fA-F]{2})([0-9a-fA-F]{2})([0-9a-fA-F]{2})([0-9a-fA-F]{2})");

				std::smatch rgb_matches;
				std::smatch hex_matches;

				if (std::regex_match(a_str, rgb_matches, rgb_pattern)) {
					auto red = std::stoi(rgb_matches[1]);
					auto green = std::stoi(rgb_matches[2]);
					auto blue = std::stoi(rgb_matches[3]);
					auto alpha = std::stoi(rgb_matches[4]);

					return { ImVec4(red / 255.0f, green / 255.0f, blue / 255.0f, alpha / 255.0f), true };  //{ { red / 255.0f, green / 255.0f, blue / 255.0f, alpha / 255.0f }, false };
				} else if (std::regex_match(a_str, hex_matches, hex_pattern)) {
					auto red = std::stoi(hex_matches[1], 0, 16);
					auto green = std::stoi(hex_matches[2], 0, 16);
					auto blue = std::stoi(hex_matches[3], 0, 16);
					auto alpha = std::stoi(hex_matches[4], 0, 16);

					return { ImVec4(red / 255.0f, green / 255.0f, blue / 255.0f, alpha / 255.0f), true };
				}
			}

			logger::error("[Settings] Failed to parse color from .ini file! Ensure you're using the correct format!");
			logger::error("[Settings] ImVec4 input: {}", a_str);
			return { T(), false };
		}

		static std::pair<ImVec2, bool> GetVec2(std::string& a_str)
		{
			static std::regex pattern("\\{([0-9]*\\.?[0-9]+),([0-9]*\\.?[0-9]+)\\}");
			std::smatch matches;

			if (std::regex_match(a_str, matches, pattern)) {
				float x = std::stof(matches[1]);
				float y = std::stof(matches[2]);

				return { ImVec2(x, y), true };
			}

			logger::error("[Settings] Failed to parse ImVec2 from .ini file! Ensure you're using the correct format!");
			logger::error("[Settings] ImVec2 input: {}", a_str);
			return { ImVec2(), false };
		}

		// Horrendous de-serialization.
		[[nodiscard]] static inline float 		GetFloat(std::string& a_str) { return std::stof(a_str); };
		[[nodiscard]] static inline int 		GetInt(std::string& a_str) { return std::stoi(a_str); };
		[[nodiscard]] static inline uint32_t 	GetUInt(std::string& a_str) { return std::stoul(a_str); };
		[[nodiscard]] static inline bool 		GetBool(std::string& a_str) { return a_str == "true"; };
		[[nodiscard]] static inline std::string GetString(std::string& a_str) { return a_str; };  // lol

		[[nodiscard]] inline Config& GetConfig() { return user.config; };

	private:
	};
}