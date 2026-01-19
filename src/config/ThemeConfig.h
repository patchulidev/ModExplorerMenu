#pragma once

#include "config/ConfigManager.h"
#include "core/Graphic.h"

namespace Modex
{
	static const std::filesystem::path THEMES_JSON_PATH = 
	std::filesystem::path("data") / "interface" / "modex" / "user" / "themes" / "default.json";

	struct ModexTheme
	{
		std::string m_name;
		std::filesystem::path m_filePath;

		ModexTheme(const std::filesystem::path& a_path) : m_filePath(a_path)
		{
			m_name = a_path.stem().string();
			m_filePath = a_path;
		}
	};

	class ThemeConfig : public ConfigManager
	{
	private:
		std::vector<ModexTheme> m_availableThemes;

	public:
		static inline ThemeConfig* GetSingleton()
		{
			static ThemeConfig singleton;
			return std::addressof(singleton);
		}

		virtual bool Load(bool a_create) override;
		void LoadTheme(const ModexTheme& a_theme);

		ThemeConfig();

		static std::optional<GraphicManager::Image> GetSplashLogo();

		static ImVec4 GetColor(const std::string& a_key, float a_alphaMult = 1.0f);
		static ImU32 GetColorU32(const std::string& a_key, float a_alphaMult = 1.0f);

		static const std::vector<ModexTheme>& GetAvailableThemes() { return GetSingleton()->m_availableThemes; }
	};
}
