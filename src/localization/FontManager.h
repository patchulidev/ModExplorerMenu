#pragma once

#include "pch.h"
#include "core/PathUtf8.h"

namespace Modex
{
	const static std::filesystem::path IMGUI_FONT_DIR = std::filesystem::path("data") / "interface" / "imguiicons" / "fonts";
	const static std::filesystem::path MODEX_FONT_DIR = std::filesystem::path("data") / "interface" / "modex" / "fonts";
	const static std::filesystem::path MODEX_ICON_FILE = MODEX_FONT_DIR / "lucide.ttf";

	// Thank you @cyfewlp
	static inline void DrawPreviewText()
	{
		static std::string PREVIEW_TEXT = R"(
			!@#$%^&*()_+-=[]{}|;':",.<>?/

			-- Unicode & Fallback --
			Latín: áéíóú ñ  |  FullWidth: ＡＢＣ１２３
			CJK: 繁體中文测试 / 简体中文测试 / 日本語 / 한국어

			-- Emoji & Variation --
			Icons: 🥰💀✌︎🌴🐢🐐🍄🍻👑📸😬👀🚨🏡
			New: 🐦‍🔥 🍋‍🟩 🍄‍🟫 🙂‍↕️ 🙂‍↔️

			-- Skyrim Immersion --
			Dovah: Dovahkiin, naal ok zin los vahriin!
			"I used to be an adventurer like you..."
		)";

		ImGui::TextWrapped("%s", PREVIEW_TEXT.c_str());
	}

	struct FontData
	{
		ImFont 			*font = nullptr;
		ImFontAtlas		*owner = nullptr;
		ImFontConfig		config;

		operator ImFont*() const { return font; };

		void Cleanup();

		FontData(bool a_bold)
		{
			config.OversampleH = 1;
			config.OversampleV = 1;
			config.PixelSnapH = 1;

			// These produce really clean and crisp results;
			config.FontLoaderFlags |= ImGuiFreeTypeLoaderFlags_ForceAutoHint;
			// config.FontLoaderFlags |= ImGuiFreeTypeLoaderFlags_MonoHinting;
			
			if (a_bold) {
				config.FontLoaderFlags |= ImGuiFreeTypeLoaderFlags_Bold;
			}
		}
	};

	struct FontInfo
	{
		std::string name;
		std::string filepath;

		FontInfo(const std::string& a_name, const std::filesystem::path& a_path)
			: name(a_name), filepath(PathToUtf8(a_path))
		{}
	};

	class FontManager
	{
	private:
		std::vector<FontInfo>	m_library;
		FontData 		m_base{ false };
		FontData		m_bold{ true };
		static inline bool 	dirty;

	public:
		FontManager() = default;
		~FontManager() = default;
		FontManager(const FontManager&) = delete;
		FontManager& operator=(const FontManager&) = delete;

		static inline FontManager* GetSingleton()
		{
			static FontManager singleton;
			return &singleton;
		}

		void Load();
		void Cleanup();
		void MergeIcons();
		void SetDefault();
		void SetFont(const std::filesystem::path& a_path);

		void BuildLocalFontLibrary();

		static FontData& GetBaseFont() { return GetSingleton()->m_base; }
		static FontData& GetBoldFont() { return GetSingleton()->m_bold; }

		std::vector<FontInfo>& GetFontLibrary() { return m_library; }
	};
}

namespace ImGui
{
	static inline void PushFontBold(float a_size = 0.0f)
	{
		ImGui::PushFont(Modex::FontManager::GetBoldFont(), a_size);
	}

	static inline void PushFontRegular(float a_size = 0.0f)
	{
		ImGui::PushFont(Modex::FontManager::GetBaseFont(), a_size);
	}
}

