#include "UserData.h"

#include "ui/components/UINotification.h"

namespace Modex
{
	void UserData::Save()
	{
		auto data = m_userDataConfig.GetData();

		m_userDataConfig.Set<std::vector<std::string>>("Recent", m_recent.items);
		m_userDataConfig.Set<std::vector<std::string>>("Favorites", m_favorites.items);

		m_userDataConfig.Save();
	}

	void UserData::Load()
	{
		m_userDataConfig.SetFilePath(USERDATA_JSON_PATH);
		m_userDataConfig.Load(true);

		m_recent.items = m_userDataConfig.Get<std::vector<std::string>>("Recent", {});
		m_favorites.items = m_userDataConfig.Get<std::vector<std::string>>("Favorites", {});
	}

	void AddToRecentList(const std::string& a_editorid)
	{
		auto& recent = UserData::GetRecent();
		recent.items.erase(std::remove(recent.items.begin(), recent.items.end(), a_editorid), recent.items.end());
		recent.items.insert(recent.items.begin(), a_editorid);

		if (recent.items.size() > recent.maxSize) {
			recent.items.resize(recent.maxSize);
		}
	}

	void AddToFavorites(const std::string& a_editorid)
	{
		auto& favorites = UserData::GetFavorites();

		favorites.items.erase(std::remove(favorites.items.begin(), favorites.items.end(), a_editorid), favorites.items.end());
		favorites.items.insert(favorites.items.begin(), a_editorid);
	}

	void UserData::SendEvent(ModexActionType a_actionType, const std::string& a_editorid)
	{
		switch (a_actionType) {
			case ModexActionType::AddItem:
				AddToRecentList(a_editorid);
				UINotification::ShowAction(Translate("ADD_SELECTION"), a_editorid, ICON_LC_PLUS);
				break;
			case ModexActionType::RemoveItem:
				AddToRecentList(a_editorid);
				UINotification::ShowAction(Translate("REMOVE_SELECTION"), a_editorid, ICON_LC_MINUS, UIMessageType::Warning);
				break;
			case ModexActionType::EquipItem:
				AddToRecentList(a_editorid);
				UINotification::ShowAction(Translate("EQUIP_SELECTION"), a_editorid, ICON_LC_SHIRT);
				break;
			case ModexActionType::PlaceAtMe:
				AddToRecentList(a_editorid);
				UINotification::ShowAction(Translate("PLACE_SELECTION"), a_editorid, ICON_LC_MOUNTAIN);
				break;
			case ModexActionType::ReadBook:
				AddToRecentList(a_editorid);
				UINotification::ShowAction(Translate("READ"), a_editorid, ICON_LC_BOOK);
				break;
			case ModexActionType::ResetInventory:
				UINotification::ShowAction(Translate("RESET_INVENTORY"), a_editorid, ICON_LC_ROTATE_CCW, UIMessageType::Warning);
				break;
			case ModexActionType::ClearInventory:
				UINotification::ShowAction(Translate("CLEAR_INVENTORY"), a_editorid, ICON_LC_ROTATE_CW, UIMessageType::Warning);
				break;
			case ModexActionType::GotoReference:
				AddToRecentList(a_editorid);
				UINotification::ShowAction(Translate("GOTO_SELECTION"), a_editorid, ICON_LC_MAP_PIN);
				break;
			case ModexActionType::BringReference:
				AddToRecentList(a_editorid);
				UINotification::ShowAction(Translate("BRING_SELECTION"), a_editorid, ICON_LC_MAP_PIN);
				break;
			case ModexActionType::KillActor:
				AddToRecentList(a_editorid);
				UINotification::ShowAction(Translate("KILL_ACTOR"), a_editorid, ICON_LC_SKULL, UIMessageType::Warning);
				break;
			case ModexActionType::ReviveActor:
				AddToRecentList(a_editorid);
				UINotification::ShowAction(Translate("RESURRECT_ACTOR"), a_editorid, ICON_LC_USER);
				break;
			case ModexActionType::EnableReference:
				AddToRecentList(a_editorid);
				UINotification::ShowAction(Translate("ENABLE_REFERENCE"), a_editorid, ICON_LC_CHECK);
				break;
			case ModexActionType::DisableReference:
				AddToRecentList(a_editorid);
				UINotification::ShowAction(Translate("DISABLE_REFERENCE"), a_editorid, ICON_LC_CHECK);
				break;
			case ModexActionType::SaveKit:
				UINotification::ShowAction(Translate("KIT_SAVE"), a_editorid, ICON_LC_SAVE);
				break;
			case ModexActionType::CreateKit:
				UINotification::ShowAction(Translate("KIT_CREATE"), a_editorid, ICON_LC_SQUARE_PLUS);
				break;
			case ModexActionType::DeleteKit:
				UINotification::ShowAction(Translate("KIT_DELETE"), a_editorid, ICON_LC_SQUARE_MINUS, UIMessageType::Warning);
				break;
			case ModexActionType::RenameKit:
				UINotification::ShowAction(Translate("KIT_RENAME"), a_editorid, ICON_LC_ROTATE_CW_SQUARE);
				break;
			case ModexActionType::CopyKit:
				UINotification::ShowAction(Translate("KIT_COPY"), a_editorid, ICON_LC_SQUARE_PLUS);
				break;
			case ModexActionType::CenterOnCell:
				AddToRecentList(a_editorid);
				UINotification::ShowAction(Translate("CENTER_ON_CELL"), a_editorid, ICON_LC_PLUS);
				break;
			case ModexActionType::Favorited:
				AddToFavorites(a_editorid);
				UINotification::ShowAction(Translate("ADD_TO_FAVORITES"), a_editorid, ICON_LC_HEART);
				break;
			case ModexActionType::Total: break;
		}
	}

	void UserData::SendEvent(ModexActionType a_actionType, const std::unique_ptr<BaseObject>& a_item)
	{
		if (!a_item) return;
		UserData::SendEvent(a_actionType, a_item->GetEditorID());
	}

}
