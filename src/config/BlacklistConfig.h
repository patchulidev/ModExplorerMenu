#pragma once

#include "ConfigManager.h"

namespace Modex
{
    inline const std::filesystem::path BLACKLIST_JSON_PATH = 
	std::filesystem::path("data") / "interface" / "modex" / "user" / "blacklist.json";

    class BlacklistConfig : public ConfigManager
    {
    private:
        std::unordered_set<const RE::TESFile*> m_blacklist;

    public:
        static inline BlacklistConfig* GetSingleton()
        {
            static BlacklistConfig singleton;
            return std::addressof(singleton);
        }

        BlacklistConfig();

        void LoadBlacklist();

        bool Has(const RE::TESFile* a_plugin);
        void AddPluginToBlacklist(const RE::TESFile* a_plugin);
        void RemovePluginFromBlacklist(const RE::TESFile* a_plugin);

        [[nodiscard]] static std::unordered_set<const RE::TESFile*>& Get() { return GetSingleton()->m_blacklist; };
    };
}