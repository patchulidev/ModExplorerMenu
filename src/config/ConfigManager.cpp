#include "ConfigManager.h"

namespace Modex
{
    void ConfigManager::CreateAndDumpJson(const std::filesystem::path& a_path, const nlohmann::json& a_data)
    {
        PrettyLog::Debug("Creating new config file at '{}'", a_path.string());
        std::filesystem::create_directories(a_path.parent_path());

        std::ofstream file(a_path);
        if (file.is_open()) {
            file << a_data.dump(4);
            file.close();
        }
    }

    // a_create ensures local JSON file is created to prevent CTD. Still propogate errors upwards.
    // we pass false if we don't need a JSON file at load time.

    bool ConfigManager::Load(bool a_create)
    {        
        ASSERT_MSG(a_create && m_file_path.empty(), "Called before setting file path!");
            
        m_data = nlohmann::json::object();

        if (!std::filesystem::exists(m_file_path)) {
            if (a_create) {
                CreateAndDumpJson(m_file_path, m_data);
                return m_initialized = true;
            }

            return m_initialized = false;
        }
        
        try {
            std::ifstream file(m_file_path);
            if (!file.is_open()) {
                return m_initialized = false;
            }
            file >> m_data;
            return m_initialized = true;
        } catch (...) {
            ASSERT_MSG(true, "Failed to load file {}", m_file_path.string());
            return m_initialized = false;
        }
    }

    bool ConfigManager::Save()
    {
        ASSERT_MSG(!m_initialized, "Attempted to save uninitialized ConfigManager!");

        try {
            // Create directories if they don't exist
            std::filesystem::create_directories(m_file_path.parent_path());

            std::ofstream file(m_file_path);
            if (!file.is_open()) {
                return false;
            }
            file << m_data.dump(4);
            return true;
        } catch (...) {
            return false;
        }
    }

    void ConfigManager::GetAsList(std::vector<std::string>& outList) const
    {        
        outList.clear();
        for (auto& [key, value] : m_data.items()) {
            outList.push_back(key);
        }
    }

    void ConfigManager::Clear()
    {
        m_data = nlohmann::json::object();
    }

    void ConfigManager::Add(const std::string& a_key)
    {
        m_data[a_key] = nlohmann::json::object();
    }

    bool ConfigManager::Has(const std::string& a_key) const
    {
        return m_data.contains(a_key);
    }

    void ConfigManager::Remove(const std::string& a_key)
    {
        m_data.erase(a_key);
    }
}