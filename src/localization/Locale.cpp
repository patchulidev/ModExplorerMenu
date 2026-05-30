#include "localization/Locale.h"
#include "core/PathUtf8.h"

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

	bool Locale::HasEntry(const char* a_localeString) const
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

	std::string Locale::TruncateText(const std::string& a_text, float a_width)
	{
		const float textWidth = ImGui::CalcTextSize(a_text.c_str()).x;

		if (textWidth > a_width) {
			const float ellipsisWidth = ImGui::CalcTextSize("...").x;

			std::string result = "";
			float resultWidth = 0.0f;

			for (const auto& c : a_text) {
				const float charWidth = ImGui::CalcTextSize(std::string(1, c).c_str()).x;

				if (resultWidth + charWidth + ellipsisWidth > a_width) {
					result += "...";
					break;
				}

				result += c;
				resultWidth += charWidth;
			}

			return result;
		}

		return a_text;
	}

	std::filesystem::path Locale::GetFilepath(const std::string& a_stem)
	{
		Debug("Conducting Lookup for Locale file '{}' in '{}'", a_stem, PathToUtf8(LOCALE_JSON_DIR));

		for (const auto& entry : std::filesystem::directory_iterator(LOCALE_JSON_DIR)) {
			if (entry.is_regular_file() && entry.path().extension() == ".json") {
				if (PathToUtf8(entry.path().stem()) == a_stem) {
					Debug(" - Found locale file: '{}'", PathToUtf8(entry.path().filename().stem()));
					return entry.path();
				}
			}
		}

		Debug(" - Error: Could not locate file: '{}'", a_stem);
		return std::filesystem::path();
	}

	void Locale::BuildLocaleList()
	{
		Debug("Building Locale List from: '{}'", PathToUtf8(LOCALE_JSON_DIR));

		for (const auto& entry : std::filesystem::directory_iterator(LOCALE_JSON_DIR)) {
			if (entry.is_regular_file() && entry.path().extension() == ".json") {
				Trace(" - Found locale file: '{}'", PathToUtf8(entry.path().filename()));

				const std::string language = PathToUtf8(entry.path().filename().stem());
				m_languages.push_back(language);
			}
		}
	}
}
