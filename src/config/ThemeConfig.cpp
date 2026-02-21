#include "ThemeConfig.h"
#include "config/UserConfig.h"
#include "external/json_serializers.cpp"
#include "imgui.h"

namespace Modex
{
	ImVec4 ThemeConfig::GetColor(const std::string& a_key, float a_alphaMult)
	{
		auto& data = GetSingleton()->m_data;

		auto it = data.find(a_key);
		if (it != data.end()) {
			ImVec4 color = it->get<ImVec4>();
			color.w *= a_alphaMult;
			return color;
		}

		return ImVec4(0.8f, 0.2f, 0.2f, 0.5f); // Default to RED if key not found
	}

	ImVec4 ThemeConfig::GetHover(const std::string& a_key, float a_alphaMult)
	{
		ImVec4 color = GetColor(a_key, a_alphaMult);
		return ImVec4(color.x + 0.05f, color.y + 0.05f, color.z + 0.10f, color.w);
	}

	ImVec4 ThemeConfig::GetActive(const std::string& a_key, float a_alphaMult)
	{
		ImVec4 color = GetColor(a_key, a_alphaMult);
		return ImVec4(color.x + 0.10f, color.y + 0.10f, color.z + 0.15f, color.w);
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
		
		m_availableThemes.clear();
		if (std::filesystem::exists(THEMES_JSON_PATH.parent_path()) && std::filesystem::is_directory(THEMES_JSON_PATH.parent_path())) {
			for (const auto& entry : std::filesystem::directory_iterator(THEMES_JSON_PATH.parent_path())) {
				if (entry.is_regular_file() && entry.path().extension() == ".json") {
					ModexTheme theme(entry.path());
					m_availableThemes.push_back(theme);

					if (theme.m_filePath == m_file_path) {
						const bool instantiate = theme.m_name == "default";
						theme_found = ConfigManager::Load(instantiate);
					}
				}
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

	bool ThemeConfig::LoadTheme(const ModexTheme& a_theme)
	{
		Debug("Loading/Switching Theme to '{}'", a_theme.m_name);

		SetFilePath(a_theme.m_filePath);
		return ConfigManager::Load(false);
	}

	void ThemeConfig::ApplyThemeToImGui()
	{
		auto& style = ImGui::GetStyle();
		style.Colors[ImGuiCol_FrameBg] = ThemeConfig::GetColor("BG");
		style.Colors[ImGuiCol_FrameBgHovered] = ThemeConfig::GetHover("BG");
		style.Colors[ImGuiCol_FrameBgActive] = ThemeConfig::GetActive("BG");

		style.Colors[ImGuiCol_Button] = ThemeConfig::GetColor("PRIMARY");
		style.Colors[ImGuiCol_ButtonHovered] = ThemeConfig::GetHover("PRIMARY");
		style.Colors[ImGuiCol_ButtonActive] = ThemeConfig::GetActive("PRIMARY");

		style.Colors[ImGuiCol_Header] = ThemeConfig::GetColor("PRIMARY");
		style.Colors[ImGuiCol_HeaderHovered] = ThemeConfig::GetHover("PRIMARY");
		style.Colors[ImGuiCol_HeaderActive] = ThemeConfig::GetActive("PRIMARY");

		style.Colors[ImGuiCol_SliderGrab] = ThemeConfig::GetColor("PRIMARY");
		style.Colors[ImGuiCol_SliderGrabActive] = ThemeConfig::GetActive("PRIMARY");

		style.Colors[ImGuiCol_ScrollbarBg] = ThemeConfig::GetColor("BG");
		style.Colors[ImGuiCol_ScrollbarGrab] = ThemeConfig::GetColor("PRIMARY");
		style.Colors[ImGuiCol_ScrollbarGrabHovered] = ThemeConfig::GetHover("PRIMARY");
		style.Colors[ImGuiCol_ScrollbarGrabActive] = ThemeConfig::GetActive("PRIMARY");

		style.Colors[ImGuiCol_Separator] = ThemeConfig::GetColor("PRIMARY");
		style.Colors[ImGuiCol_SeparatorHovered] = ThemeConfig::GetHover("PRIMARY");
		style.Colors[ImGuiCol_SeparatorActive] = ThemeConfig::GetActive("PRIMARY");

		style.Colors[ImGuiCol_ChildBg] = ThemeConfig::GetColor("NONE");
		style.Colors[ImGuiCol_WindowBg] = ThemeConfig::GetColor("FRAME");
		style.Colors[ImGuiCol_PopupBg] = ThemeConfig::GetColor("FRAME");

		style.Colors[ImGuiCol_Text] = ThemeConfig::GetColor("TEXT");
		style.Colors[ImGuiCol_TextDisabled] = ThemeConfig::GetColor("TEXT_DISABLED");

		style.Colors[ImGuiCol_TableRowBg] = ThemeConfig::GetColor("TABLE_BG");
		style.Colors[ImGuiCol_TableRowBgAlt] = ThemeConfig::GetHover("TABLE_BG_ALT");

		style.Colors[ImGuiCol_Border] = ThemeConfig::GetColor("BORDER");
	}

	ThemeConfig::ThemeConfig()
	{
		SetFilePath(THEMES_JSON_PATH);
	}
}
