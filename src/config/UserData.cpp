#include "UserData.h"

namespace Modex
{
	void UserData::Save()
	{
		auto data = m_userDataConfig.GetData();

		m_userDataConfig.Set<std::vector<std::string>>("Recent", m_recent.items);
		m_userDataConfig.Set<std::unordered_set<std::string>>("Favorites", m_favorites.items);

		m_userDataConfig.Save();
	}

	void UserData::Load()
	{
		m_userDataConfig.SetFilePath(USERDATA_JSON_PATH);
		m_userDataConfig.Load(true);

		m_recent.items = m_userDataConfig.Get<std::vector<std::string>>("Recent", {});
		m_favorites.items = m_userDataConfig.Get<std::unordered_set<std::string>>("Favorites", {});
	}

	void UserData::AddRecent(const std::unique_ptr<BaseObject>& a_item)
	{
		if (!a_item) return;

		const std::string edid = a_item->GetEditorID();
		auto& recent = m_recent.items;

		recent.erase(std::remove(recent.begin(), recent.end(), edid), recent.end());
		recent.insert(recent.begin(), edid);

		if (recent.size() > m_recent.maxSize) {
			recent.resize(m_recent.maxSize);
		}
	}

	void UserData::AddFavorite(const std::unique_ptr<BaseObject>& a_item)
	{
		if (!a_item) return;

		const std::string edid = a_item->GetEditorID();
		m_favorites.items.insert(edid);
	}
}
