#pragma once

#include "ui/components/UIModule.h"

namespace Modex
{
	class SettingsModule : public UIModule
	{
	public:
		char                            m_modSearchBuffer[256];
		std::vector<const RE::TESFile*> m_pluginList;
		std::vector<std::string>        m_pluginListVector;
		uint8_t                         m_sort;
		uint8_t                         m_type;

		SettingsModule();
		~SettingsModule();
		SettingsModule(const SettingsModule&) = delete;
		SettingsModule(SettingsModule&&) = delete;
		SettingsModule& operator=(const SettingsModule&) = delete;
		SettingsModule& operator=(SettingsModule&&) = delete;

		void Draw() override;
		void DrawBlacklistLayout(std::vector<std::unique_ptr<UITable>>& a_tables);
		void DrawBlacklistSettings();
		void BuildBlacklistPlugins();
	};
}
