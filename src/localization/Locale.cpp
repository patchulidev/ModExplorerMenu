#include "localization/Locale.h"

namespace Modex
{
    static constexpr size_t hash_string(const char* str, size_t value = 5381) noexcept
	{
		return *str ? hash_string(str + 1, (value * 33) ^ static_cast<size_t>(*str)) : value;
	}

	template<size_t N>
	struct CompileTimeString
	{
		constexpr CompileTimeString(const char(&str)[N]) : hash(hash_string(str))
		{
			for (size_t i = 0; i < N; ++i) {
				data[i] = str[i];
			}
		}

		char data[N];
		size_t hash;
		static constexpr size_t size = N;
	};

	bool Locale::Load(bool a_create)
	{
		ASSERT_MSG(a_create, "Localization does not create files!");

		m_hash.clear();

		if (m_initialized == false) {
			BuildLocaleList();
		}

		ASSERT_MSG(!ConfigManager::Load(a_create), "Failed to load localization file!");

		if (!m_data.empty()) {        
			for (auto& [key, value] : m_data.items()) {
				size_t key_hash = hash_string(key.c_str());
				m_hash[key_hash] = value;
			}
		}

		return true;
	}

	const char* Locale::GetTranslation(const char* fallback_text) const
	{
		size_t text_hash = hash_string(fallback_text);

		auto it = m_hash.find(text_hash);
		if (it != m_hash.end()) {
			return it->second.c_str();
		}

		return fallback_text;
	}

	bool Locale::HasTooltip(const char* a_localeString) const
	{
		size_t tooltip_hash = hash_string(a_localeString);

		auto it = m_hash.find(tooltip_hash);
		if (it != m_hash.end()) {
			return true;
		}

		return false;
	}

	const char* Locale::GetTooltip(const char* a_localeString) const
	{
		std::string tooltip_key = std::string(a_localeString) + std::string("_TOOLTIP");
		size_t tooltip_hash = hash_string(tooltip_key.c_str());

		auto it = m_hash.find(tooltip_hash);
		if (it != m_hash.end()) {
			return it->second.c_str();
		}

		return "";
	}

	std::filesystem::path Locale::GetFilepath(const std::string& a_stem)
	{
		Debug("Conducting Lookup for Locale file '{}' in '{}'", a_stem, LOCALE_JSON_DIR.string());

		for (const auto& entry : std::filesystem::directory_iterator(LOCALE_JSON_DIR)) {
			if (entry.is_regular_file() && entry.path().extension() == ".json") {
				if (entry.path().stem().string() == a_stem) {
					Debug(" - Found locale file: '{}'", entry.path().filename().stem().string());
					return entry.path();
				}
			}
		}

		Debug(" - Error: Could not locate file: '{}'", a_stem);
		return std::filesystem::path();
	}

	void Locale::BuildLocaleList()
	{
		Debug("Building Locale List from: '{}'", LOCALE_JSON_DIR.string());

		for (const auto& entry : std::filesystem::directory_iterator(LOCALE_JSON_DIR)) {
			if (entry.is_regular_file() && entry.path().extension() == ".json") {
				Trace(" - Found locale file: '{}'", entry.path().filename().string());

				const std::string language = entry.path().filename().stem().string();
				m_languages.push_back(language);
			}
		}
	}
}
