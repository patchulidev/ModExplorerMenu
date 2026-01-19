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
}
