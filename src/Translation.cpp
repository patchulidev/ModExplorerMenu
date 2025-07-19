#pragma once

#include "include/P/Persistent.h"
#include "include/L/Language.h"

namespace Modex
{
    constexpr size_t hash_string(const char* str, size_t value = 5381) noexcept
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

    void PersistentData::RefreshTranslation(const std::string& a_language)
    {
        hash.clear();
        LoadTranslation(a_language);
    }

    void PersistentData::LoadTranslation(std::string a_path)
    {
        if (!hash.empty()) {
            hash.clear();
        }

        std::filesystem::path full_path = lang_path / (a_path + ".json");

        std::ifstream file(full_path);
        if (!file.is_open()) {
            return;
        }

        file.seekg(0, std::ios::end);
        if (file.tellg() == 0) {
            file.close();
            return;
        }

        file.seekg(0, std::ios::beg);

        try {
            nlohmann::json json;
            file >> json;
            file.close();

            for (auto& [key, value] : json.items()) {
                size_t key_hash = hash_string(key.c_str());
                hash[key_hash] = value;
            }

            PrettyLog::Info("Successfully loaded language from file: \"{}\"", a_path);
        } catch (const nlohmann::json::parse_error& e) {
            PrettyLog::Warn("Error parsing language file: {}", e.what());
            file.close();
        } catch (const nlohmann::json::exception& e) {
            PrettyLog::Error("Error Exception reading language file: {}", e.what());
            file.close();
        }
    }

    template<size_t N>
    const char* PersistentData::GetTranslation(const char (&fallback_text)[N]) const
    {
        constexpr size_t text_hash = hash_string(fallback_text);
        
        auto it = hash.find(text_hash);
        if (it != hash.end()) {
            return it->second.c_str();
        }
        
        return fallback_text;
    }

    const char* PersistentData::GetTranslation(const char* fallback_text) const
    {
        size_t text_hash = hash_string(fallback_text);

        auto it = hash.find(text_hash);
        if (it != hash.end()) {
            return it->second.c_str();
        }

        return fallback_text;
    }
}