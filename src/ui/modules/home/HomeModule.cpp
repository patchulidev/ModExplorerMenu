#include "HomeModule.h"
#include "config/UserData.h"
#include "localization/FontManager.h"
#include "localization/Locale.h"

namespace Modex
{
	void HomeModule::Draw()
	{
		DrawTabMenu();
	}

	void DrawHomeLayout(std::vector<std::unique_ptr<UITable>>& a_tables)
	{
		(void)a_tables;
		DrawPreviewText();
	}
	
	HomeModule::HomeModule()
	{
		m_name = Translate("MODULE_HOME");
		m_icon = ICON_LC_HOUSE;

		m_layouts.push_back({"Home", true, DrawHomeLayout});
	}
}
