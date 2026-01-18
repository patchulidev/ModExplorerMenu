#include "FontManager.h"

#include "config/UserConfig.h"
#include "external/icons/IconsLucide.h"

namespace Modex
{
	void FontData::Cleanup()
	{
		if (owner && font) {
			owner->RemoveFont(font);
			font = nullptr;
			owner = nullptr;
		}
	}

	// Compile list of recognized custom fonts from interface directory.
	void FontManager::BuildLocalFontLibrary()
	{
		Debug("Building Font Library for font configuration...");

		if (!std::filesystem::exists(MODEX_ICON_FILE)) {
			Warn("Modex Icon Font not found at expected path: {}. Icons may not display correctly.", MODEX_ICON_FILE.string());
		}

		// Iterate through fonts relative to Modex Font directory.
		if (std::filesystem::exists(MODEX_FONT_DIR)) {
			for (const auto& entry : std::filesystem::directory_iterator(MODEX_FONT_DIR)) {
				if (entry.path().filename().extension() != ".ttf" && entry.path().filename().extension() != ".otf") {
					continue;
				}

				if (entry == MODEX_ICON_FILE) {
					continue;
				}

				const auto filename = entry.path().stem().string();
				const auto filepath = entry.path();

				m_library.emplace_back(filename, filepath);
				Debug("Registered Custom Font: {}", filename);
			}
		}

		// Search for fonts relative to ImGui Icon Library directory.
		if (std::filesystem::exists(IMGUI_FONT_DIR)) {
			for (const auto& entry : std::filesystem::directory_iterator(IMGUI_FONT_DIR)) {
				if (entry.path().filename().extension() != ".ttf" && entry.path().filename().extension() != ".otf") {
					continue;
				}

				const auto filename = entry.path().stem().string();
				const auto filepath = entry.path();

				m_library.emplace_back(filename, filepath);
				Debug("Registered ImGui Icon Font: {}", filename);
			}
		}

		Info("Finished local building font library. {} entries found.", m_library.size());
	}

	void FontManager::SetDefault()
	{
		auto &io = ImGui::GetIO();
		m_base = FontData(false);
		m_base.font = io.Fonts->AddFontDefault();
		m_base.owner = m_base.font->OwnerAtlas;

		MergeIcons();
	}

	void FontManager::Load()
	{
		BuildLocalFontLibrary();

		auto& io = ImGui::GetIO();
		io.Fonts->FontLoader = ImGuiFreeType::GetFontLoader();

		auto& user_font = UserConfig::Get().globalFont;
		for (const auto& font : m_library) {
			if (font.name == user_font) {
				SetFont(font.filepath);
				return;
			}	
		}

		Warn("User selected font '{}' not found in font library. Reverting to default font.", user_font);
		SetDefault();
	}
	
	void FontManager::MergeIcons()
	{
		auto& io = ImGui::GetIO();

		ImFontConfig icon_config;
		icon_config.MergeMode = true;
		icon_config.GlyphOffset.y = 3.0f;
		icon_config.DstFont = m_base.font;

		io.Fonts->AddFontFromFileTTF(MODEX_ICON_FILE.string().c_str(), 0.0f, &icon_config);
	}

	void FontManager::SetFont(const std::filesystem::path& a_path)
	{
		auto &io = ImGui::GetIO();
		auto _base = std::move(m_base);
		auto _bold = std::move(m_bold);

		m_base = FontData(false);
		m_base.font = io.Fonts->AddFontFromFileTTF(a_path.string().c_str(), 0.0f, &m_base.config);
		m_base.owner = m_base.font->OwnerAtlas;

		m_bold = FontData(true);
		m_bold.font = io.Fonts->AddFontFromFileTTF(a_path.string().c_str(), 0.0f, &m_bold.config);
		m_bold.owner = m_bold.font->OwnerAtlas;

		MergeIcons();

		_base.Cleanup();
		_bold.Cleanup();

		Info("Set font to: {}", a_path.string());
	}
}
