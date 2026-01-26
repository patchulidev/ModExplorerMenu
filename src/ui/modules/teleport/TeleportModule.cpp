#include "TeleportModule.h"
#include "localization/Locale.h"

// TODO: Re-Implement Teleport Module

namespace Modex
{
	void TeleportModule::Draw()
	{
		DrawTabMenu();
	}

	static inline void DrawTeleportLayout(std::vector<std::unique_ptr<UITable>>& a_tables)
	{
		(void)a_tables;
		ImGui::Text("Hello!");
	}

	TeleportModule::TeleportModule() {
		m_name = Translate("MODULE_TELEPORT");
		m_icon = ICON_LC_MAP_PIN;

		m_layouts.push_back({ "Teleport View", true, DrawTeleportLayout }); // TODO: Locale

		// auto table = std::make_unique<UITable>();
	}
}
