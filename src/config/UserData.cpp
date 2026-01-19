#include "UserData.h"

namespace Modex
{
	UserData::UserData()
	{
		m_userDataConfig.SetFilePath(USERDATA_JSON_PATH);
		m_favoriteConfig.SetFilePath(FAVORITES_JSON_PATH);
		m_recentConfig.SetFilePath(RECENT_JSON_PATH);
	}

	void UserData::Save()
	{
		GetSingleton()->m_userDataConfig.Save();
		GetSingleton()->m_favoriteConfig.Save();
		GetSingleton()->m_recentConfig.Save();
	}

	void UserData::LoadAll()
	{
		GetSingleton()->m_userDataConfig.Load(true);
		GetSingleton()->m_favoriteConfig.Load(true);
		GetSingleton()->m_recentConfig.Load(true);
	}
}
