#pragma once

// I could abstract widgets into their own class and introduce behaviors such as state tracking,
// undo history, etc. Would likely require UserConfig to require a struct for defining settings.

namespace Modex
{
	class SettingsModule
	{
	private:
		char                            m_modSearchBuffer[256];
		RE::FormType                    m_primaryFilter;
		std::vector<const RE::TESFile*> m_pluginList;
		std::vector<std::string>        m_pluginListVector;
		uint32_t                        m_sort;
		uint32_t                        m_type;

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
		void BuildBlacklistPlugins();

		enum class Viewport : uint8_t
		{
			UserSettings = 0,
			Blacklist,
			Count
		};

		Viewport m_activeViewport = Viewport::UserSettings;
	};
}
