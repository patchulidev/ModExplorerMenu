#pragma once

namespace Modex
{
	class SettingsModule
	{
	public:
		static inline SettingsModule* GetSingleton()
		{
			static SettingsModule singleton;
			return std::addressof(singleton);
		}

		void Draw(float a_offset);
		void Load();

	private:
		void DrawGeneralSettings();
		void DrawBlacklistSettings();

		int   m_totalPlugins;
		int   m_totalBlacklisted;
		int   m_totalHidden;
		bool  m_updateHidden;

		char        m_modSearchBuffer[256];
		std::string m_selectedMod;

		RE::FormType                    m_primaryFilter;
		std::vector<const RE::TESFile*> m_pluginList;
		std::vector<std::string>        m_pluginListVector;

		enum class Viewport : uint8_t
		{
			UserSettings = 0,
			Blacklist,
			Count
		};

		Viewport m_activeViewport = Viewport::UserSettings;
	};
}
