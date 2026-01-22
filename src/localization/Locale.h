#pragma once

#include "config/ConfigManager.h"

namespace Modex
{
    static inline const std::filesystem::path LOCALE_JSON_DIR = 
    std::filesystem::path("data") / "interface" / "modex" / "language";

    class Locale : public ConfigManager
    {
    private:
        std::unordered_map<size_t, std::string> m_hash;
		std::vector<std::string> m_languages;

    public:
        static inline Locale* GetSingleton()
        {
            static Locale singleton;
            return std::addressof(singleton);
        }

        virtual bool Load(bool a_create) override;

		void BuildLocaleList();
		// void LoadFromPath(std::filesystem::path a_path);

		std::vector<std::string> GetLanguages() noexcept { return m_languages; }
		std::filesystem::path GetFilepath(const std::string& a_stem);
		const char* GetTranslation(const char* fallback_text) const;
    };

	#define Translate(text) Locale::GetSingleton()->GetTranslation(text)
    #define TranslateFormat(text, suffix) (std::string(Translate(text)) + suffix).c_str()
    #define TranslateIcon(icon, text) ((std::string(icon) + Translate(text)).c_str())
    #define TranslateIconFormat(icon, text, suffix) ((std::string(icon) + Translate(text) + suffix).c_str())

}
