#pragma once

#include <filesystem>
#include <fstream>

#include "external/json_serializers.cpp"

namespace Modex
{
    class ConfigManager
    {
    protected:
        nlohmann::json            m_data;
        std::filesystem::path     m_file_path;
        bool                      m_initialized = false;
        
    public:
        inline static const std::filesystem::path FILTER_DIRECTORY = std::filesystem::path("data") / "interface" / "modex" / "user" / "filters";
        inline static const std::filesystem::path EQUIPMENT_DIRECTORY = std::filesystem::path("data") / "interface" / "modex" / "user" / "equipment";
        inline static const std::filesystem::path THEME_DIRECTORY = std::filesystem::path("data") / "interface" / "modex" / "user" / "themes";

        virtual ~ConfigManager() = default;
        // ConfigManager(const ConfigManager&) = delete;
        // ConfigManager& operator=(const ConfigManager&) = delete;

        virtual bool Load(bool a_create);
        virtual bool Save();
        virtual void Clear();
        
        void Add(const std::string& a_key);
        bool Has(const std::string& a_key) const;
        void Remove(const std::string& a_key);

        void GetAsList(std::vector<std::string>& outList) const;
        static void CreateAndDumpJson(const std::filesystem::path& a_path, const nlohmann::json& a_data);

        template<typename T>
        T Get(const std::string& key, const T& defaultValue = T()) const;
        
        template<typename T>
        void Set(const std::string& key, const T& value);

        void SetFilePath(const std::filesystem::path& a_path) { m_file_path = a_path; }
        const std::filesystem::path& GetFilePath() const { return m_file_path; }

        nlohmann::json& GetData() { return m_data; }
        const nlohmann::json& GetData() const { return m_data; }
    };
}

template<typename T>
T Modex::ConfigManager::Get(const std::string& key, const T& defaultValue) const {
    auto it = m_data.find(key);
    if (it != m_data.end()) {
        try {
           return it->get<T>();
        } catch (const nlohmann::json::exception& e) {
            ASSERT_MSG(true, "ConfigManager -> Get: Failed to get key '{}' with error: {}", key, e.what());
            return defaultValue;
        }
    }
    return defaultValue;
}

template<typename T>
void Modex::ConfigManager::Set(const std::string& key, const T& value) {
    m_data[key] = value;
}
