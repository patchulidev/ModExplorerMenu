#include "ThemeConfig.h"

#include "config/UserConfig.h"
#include "external/json_serializers.cpp"

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
        
        return theme_found;
    }

    void ThemeConfig::LoadTheme(const ModexTheme& a_theme)
    {
        SetFilePath(a_theme.m_filePath);
        ConfigManager::Load(false);
    }

    ThemeConfig::ThemeConfig()
    {
        SetFilePath(THEMES_JSON_PATH);
    }
}
