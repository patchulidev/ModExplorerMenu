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

	TeleportModule::~TeleportModule()
	{
		// Destructor
	}

	TeleportModule::TeleportModule() {
		m_layouts.push_back({ Translate("TAB_TELEPORT"), true, DrawTeleportLayout });

		// auto table = std::make_unique<UITable>();
	}
}
