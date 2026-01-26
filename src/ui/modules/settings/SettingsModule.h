#pragma once

#include "ui/components/UIModule.h"

namespace Modex
{
	class SettingsModule : public UIModule
	{
	public:
		static inline char                            m_modSearchBuffer[256];
		static inline std::vector<const RE::TESFile*> m_pluginList;
		static inline std::vector<std::string>        m_pluginListVector;
		static inline uint32_t                        m_sort;
		static inline uint32_t                        m_type;

		SettingsModule();
		~SettingsModule() = default;
		SettingsModule(const SettingsModule&) = delete;
		SettingsModule(SettingsModule&&) = delete;
		SettingsModule& operator=(const SettingsModule&) = delete;
		SettingsModule& operator=(SettingsModule&&) = delete;

		void Draw() override;
		void Load() override;
		void Unload() override;

		static void DrawBlacklistSettings();
	};
}
