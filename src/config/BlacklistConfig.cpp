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
		ConfigManager::Save();
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
		ConfigManager::Save();
	}

	BlacklistConfig::BlacklistConfig()
	{
		SetFilePath(BLACKLIST_JSON_PATH);
	}
}
