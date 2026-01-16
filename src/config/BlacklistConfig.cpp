#include "BlacklistConfig.h"

namespace Modex
{
	void BlacklistConfig::LoadBlacklist()
	{
		ASSERT_MSG(!Load(true), "Failed to load blacklist config!");

		for (auto& [filename, nil] : m_data.items()) {
			m_blacklist.insert(RE::TESDataHandler::GetSingleton(false)->LookupModByName(filename));
		}
	}
	
	bool BlacklistConfig::Has(const RE::TESFile* a_plugin)
	{
		if (a_plugin == nullptr) {
			return false;
		}

		if (a_plugin->fileName[0] == '\0') {
			return false;
		}

		return ConfigManager::Has(a_plugin->fileName);
	}

	void BlacklistConfig::AddPluginToBlacklist(const RE::TESFile* a_plugin)
	{
		if (a_plugin == nullptr) {
			return;
		}

		if (a_plugin->fileName[0] == '\0') {
			return;
		}

		ConfigManager::Add(a_plugin->fileName);
	}

	void BlacklistConfig::RemovePluginFromBlacklist(const RE::TESFile* a_plugin)
	{
		if (a_plugin == nullptr) {
			return;
		}

		if (a_plugin->fileName[0] == '\0') {
			return;
		}

		ConfigManager::Remove(a_plugin->fileName);
	}

	BlacklistConfig::BlacklistConfig()
	{
		SetFilePath(BLACKLIST_JSON_PATH);
	}
}


/*
	void EquipmentConfig::LoadBlacklist()
	{
		nlohmann::json JSON = OpenJSONFile(json_user_path + "blacklist.json");

		if (!JSON.contains("Blacklist")) {
			return;
		}

		for (auto& [filename, nil] : JSON["Blacklist"].items()) {
			m_blacklist.insert(RE::TESDataHandler::GetSingleton(false)->LookupModByName(filename));
		}
	}

	void EquipmentConfig::AddPluginToBlacklist(const RE::TESFile* a_plugin)
	{
		nlohmann::json JSON = OpenJSONFile(json_user_path + "blacklist.json");
		const std::string plugin_name = ValidateTESFileName(a_plugin);

		JSON["Blacklist"][plugin_name] = true;

		if (!m_blacklist.contains(a_plugin)) {
			m_blacklist.insert(a_plugin);
		}

		if (!SaveJSONFile(json_user_path + "blacklist.json", JSON)) {
			PrettyLog::Warn("Failed to add plugin to blacklist.json during save.");
		}
	}
    
    
    void EquipmentConfig::RemovePluginFromBlacklist(const RE::TESFile* a_plugin)
	{
		nlohmann::json JSON = OpenJSONFile(json_user_path + "blacklist.json");
		const std::string plugin_name = ValidateTESFileName(a_plugin);

		if (JSON["Blacklist"].contains(plugin_name)) {
			JSON["Blacklist"].erase(plugin_name);
		}

		if (m_blacklist.contains(a_plugin)) {
			m_blacklist.erase(a_plugin);
		}

		if (!SaveJSONFile(json_user_path + "blacklist.json", JSON)) {
			PrettyLog::Warn("Failed to remove plugin from blacklist.json during save.");
		}
	}
        
    */