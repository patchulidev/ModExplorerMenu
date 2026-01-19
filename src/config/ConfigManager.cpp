#include "ConfigManager.h"

namespace Modex
{
	// Used by Load() to populate an empty JSON at specified location.
	void ConfigManager::CreateAndDumpJson(const std::filesystem::path& a_path, const nlohmann::json& a_data)
	{
		Debug("Creating new config file at '{}'", a_path.string());

		std::filesystem::create_directories(a_path.parent_path());

		std::ofstream file(a_path);
		if (file.is_open()) {
			file << a_data.dump(4);
			file.close();
		}
	}

	// a_create ensures json file is created to prevent CTD.
	bool ConfigManager::Load(bool a_create)
	{        
		ASSERT_MSG(a_create && m_file_path.empty(), "ConfigManager::Load() Called before setting file path!");
		Trace("ConfigManager Load called for file '{}", m_file_path.stem().string());
			
		m_data = nlohmann::json::object();

		if (!std::filesystem::exists(m_file_path)) {
			if (a_create) {
				Trace("  ^ Config file does not exist..");
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
		} catch (std::exception& e) {
			ASSERT_MSG(true, "Failed to load file '{}'", m_file_path.stem().string(), e.what());
			return m_initialized = false;
		}

		Trace("Config file '{}' loaded successfully", m_file_path.stem().string());
	}

	bool ConfigManager::Save()
	{
		ASSERT_MSG(!m_initialized, "Attempted to save uninitialized ConfigManager Class!");

		try {
			// Create directories if they don't exist
			if (!std::filesystem::exists(m_file_path.parent_path())) {
				Trace("Creating directory to save Config File '{}'", m_file_path.string());
				std::filesystem::create_directories(m_file_path.parent_path());
			}

			std::ofstream file(m_file_path);
			if (!file.is_open()) {
				return false;
			}
			file << m_data.dump(4);
			return true;
		} catch (...) {
			return false;
		}

		Trace("Config file '{}' saved successfully", m_file_path.stem().string());
	}

	// Export vector of JSON keys, does not include values!
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
