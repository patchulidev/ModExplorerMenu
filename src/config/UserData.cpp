#include "UserData.h"

#include "ui/components/UINotification.h"

namespace Modex
{
	void UserData::Save()
	{
		auto data = m_userDataConfig.GetData();

		m_userDataConfig.Set<std::vector<SerializedObject>>("Recently Used List", m_recent.items);
		m_userDataConfig.Set<std::vector<SerializedObject>>("Favorite List", m_favorites.items);

		m_userDataConfig.Save();
	}

	void UserData::Load()
	{
		m_userDataConfig.SetFilePath(USERDATA_JSON_PATH);
		m_userDataConfig.Load(true);

		m_recent.items = m_userDataConfig.Get<std::vector<SerializedObject>>("Recently Used List", {});
		m_favorites.items = m_userDataConfig.Get<std::vector<SerializedObject>>("Favorite List", {});
	}

	bool UserData::IsFavorited(const std::string& a_editorid)
	{
		auto& favorites = UserData::GetFavorites();
		return std::find_if(favorites.items.begin(), favorites.items.end(), [&a_editorid](const auto& item) 
		{ return item.editorid == a_editorid; }) != favorites.items.end();
	}

	bool UserData::IsFavorited(RE::FormID a_refid)
	{
		auto& favorites = UserData::GetFavorites();
		return std::find_if(favorites.items.begin(), favorites.items.end(), [&a_refid](const auto& item) 
		{ return item.refid == a_refid; }) != favorites.items.end();
	}

	void AddToRecentList(const std::unique_ptr<BaseObject>& a_item)
	{
		auto& recent = UserData::GetRecent();

		SerializedObject newItem{ a_item->GetPluginName(), a_item->GetEditorID(), a_item->GetRefID(), a_item->GetOwnership() };
		recent.items.erase(std::remove(recent.items.begin(), recent.items.end(), newItem), recent.items.end());
		recent.items.insert(recent.items.begin(), newItem);

		if (recent.items.size() > recent.maxSize) {
			recent.items.pop_back();
		}
	}

	void RemoveFromRecentList(const std::unique_ptr<BaseObject>& a_item)
	{
		auto& recent = UserData::GetRecent();
		recent.items.erase(std::remove_if(recent.items.begin(), recent.items.end(), [&a_item](const auto& item) 
		{ 
			if (a_item->GetRefID() != 0) {
				return item.refid == a_item->GetRefID();
			} else {
				return item.editorid == a_item->GetEditorID();
			}
		}), recent.items.end());
	}

	void AddToFavorites(const std::unique_ptr<BaseObject>& a_item)
	{
		auto& favorites = UserData::GetFavorites();

		SerializedObject newItem{ a_item->GetPluginName(), a_item->GetEditorID(), a_item->GetRefID(), a_item->GetOwnership() };
		favorites.items.erase(std::remove(favorites.items.begin(), favorites.items.end(), newItem), favorites.items.end());
		favorites.items.insert(favorites.items.begin(), newItem);
	}

	void RemoveFromFavorites(const std::unique_ptr<BaseObject>& a_item)
	{
		auto& favorites = UserData::GetFavorites();
		favorites.items.erase(std::remove_if(favorites.items.begin(), favorites.items.end(), [&a_item](const auto& item) 
		{ 
			if (a_item->GetRefID() != 0) {
				return item.refid == a_item->GetRefID();
			} else {
				return item.editorid == a_item->GetEditorID();
			}
		}), favorites.items.end());
	}

	void DispatchEDIDToFavorites(ModexActionType a_actionType, const std::string& a_editorid, Ownership a_owner)
	{
		if (RE::TESForm* form = RE::TESForm::LookupByEditorID(a_editorid)) {
			UserData::SendEvent(a_actionType, std::make_unique<BaseObject>(form, a_owner));
		}
	}

	// Secondary overload to send events which are not specifically tied to Objects.
	// Overflows into the main SendEvent function if the event is not handled here.
	// assuming that we passed a_text as an editor_id. If not, it will void.
	void UserData::SendEvent(ModexActionType a_actionType, const std::string& a_text, Ownership a_owner)
	{
		switch (a_actionType) {
			case ModexActionType::ResetInventory:
				UINotification::ShowAction(Translate("RESET_INVENTORY"), a_text, ICON_LC_ROTATE_CCW, UIMessageType::Warning);
				break;
			case ModexActionType::ClearInventory:
				UINotification::ShowAction(Translate("CLEAR_INVENTORY"), a_text, ICON_LC_ROTATE_CW, UIMessageType::Warning);
				break;
			case ModexActionType::SaveKit:
				UINotification::ShowAction(Translate("KIT_SAVE"), a_text, ICON_LC_SAVE);
				break;
			case ModexActionType::CreateKit:
				UINotification::ShowAction(Translate("KIT_CREATE"), a_text, ICON_LC_SQUARE_PLUS);
				break;
			case ModexActionType::DeleteKit:
				UINotification::ShowAction(Translate("KIT_DELETE"), a_text, ICON_LC_SQUARE_MINUS, UIMessageType::Warning);
				break;
			case ModexActionType::RenameKit:
				UINotification::ShowAction(Translate("KIT_RENAME"), a_text, ICON_LC_ROTATE_CW_SQUARE);
				break;
			case ModexActionType::CopyKit:
				UINotification::ShowAction(Translate("KIT_COPY"), a_text, ICON_LC_SQUARE_PLUS);
				break;
			default:
				DispatchEDIDToFavorites(a_actionType, a_text, a_owner);
				break;
		}
	}

	// Necessary for sending events with only a object reference id. Resolves into a 
	// BaseObject by performing a LookupByID and then forwarded to the main SendEvent function.
	void UserData::SendEvent(ModexActionType a_actionType, RE::FormID a_refid, Ownership a_owner)
	{
		if (RE::TESForm* form = RE::TESForm::LookupByID(a_refid); form != nullptr) {
			if (const auto reference = form->As<RE::TESObjectREFR>(); reference != nullptr) {
				UserData::SendEvent(a_actionType, std::make_unique<BaseObject>(reference->GetBaseObject()->As<RE::TESForm>(), a_owner, 0, reference->GetFormID()));
			}
		}
	}

	// Primary end-of-line SendEvent function to dispatch notifications based on Objects.
	void UserData::SendEvent(ModexActionType a_actionType, const std::unique_ptr<BaseObject>& a_item)
	{
		if (!a_item) return;
		
		switch (a_actionType) {
			case ModexActionType::AddItem:
				AddToRecentList(a_item);
				UINotification::ShowAction(Translate("ADD_SELECTION"), a_item->GetName(), ICON_LC_PLUS);
				break;
			case ModexActionType::RemoveItem:
				AddToRecentList(a_item);
				UINotification::ShowAction(Translate("REMOVE_SELECTION"), a_item->GetName(), ICON_LC_MINUS, UIMessageType::Warning);
				break;
			case ModexActionType::EquipItem:
				AddToRecentList(a_item);
				UINotification::ShowAction(Translate("EQUIP_SELECTION"), a_item->GetName(), ICON_LC_SHIRT);
				break;
			case ModexActionType::PlaceAtMe:
				AddToRecentList(a_item);
				UINotification::ShowAction(Translate("PLACE_SELECTION"), a_item->GetName(), ICON_LC_MOUNTAIN);
				break;
			case ModexActionType::ReadBook:
				AddToRecentList(a_item);
				UINotification::ShowAction(Translate("READ"), a_item->GetName(), ICON_LC_BOOK);
				break;
			case ModexActionType::GotoReference:
				AddToRecentList(a_item);
				UINotification::ShowAction(Translate("GOTO_SELECTION"), a_item->GetName(), ICON_LC_MAP_PIN);
				break;
			case ModexActionType::BringReference:
				AddToRecentList(a_item);
				UINotification::ShowAction(Translate("BRING_SELECTION"), a_item->GetName(), ICON_LC_MAP_PIN);
				break;
			case ModexActionType::KillActor:
				AddToRecentList(a_item);
				UINotification::ShowAction(Translate("KILL_ACTOR"), a_item->GetName(), ICON_LC_SKULL, UIMessageType::Warning);
				break;
			case ModexActionType::ReviveActor:
				AddToRecentList(a_item);
				UINotification::ShowAction(Translate("RESURRECT_ACTOR"), a_item->GetName(), ICON_LC_USER);
				break;
			case ModexActionType::EnableReference:
				AddToRecentList(a_item);
				UINotification::ShowAction(Translate("ENABLE_REFERENCE"), a_item->GetName(), ICON_LC_CHECK);
				break;
			case ModexActionType::DisableReference:
				AddToRecentList(a_item);
				UINotification::ShowAction(Translate("DISABLE_REFERENCE"), a_item->GetName(), ICON_LC_CHECK);
				break;
			case ModexActionType::Favorited:
				AddToFavorites(a_item);
				UINotification::ShowAction(Translate("ADD_TO_FAVORITES"), a_item->GetName(), ICON_LC_HEART);
				break;
			case Modex::ModexActionType::SetDefaultOutfit:
				AddToRecentList(a_item);
				UINotification::ShowAction(Translate("SET_DEFAULT_OUTFIT"), a_item->GetEditorID(), ICON_LC_SHIRT);
				break;
			case Modex::ModexActionType::SetSleepOutfit:
				AddToRecentList(a_item);
				UINotification::ShowAction(Translate("SET_SLEEP_OUTFIT"), a_item->GetEditorID(), ICON_LC_SHIRT);
				break;
			case ModexActionType::Unfavorited:
				RemoveFromFavorites(a_item);
				UINotification::ShowAction(Translate("REMOVE_FROM_FAVORITES"), a_item->GetName(), ICON_LC_HEART_OFF);
				break;
			case ModexActionType::EquipOutfit:
				AddToRecentList(a_item);
				UINotification::ShowAction(Translate("EQUIP_OUTFIT_ITEMS"), a_item->GetName(), ICON_LC_SHIRT);
				break;
			case ModexActionType::AddOutfit:
				AddToRecentList(a_item);
				UINotification::ShowAction(Translate("ADD_OUTFIT_ITEMS"), a_item->GetName(), ICON_LC_SHIRT);
				break;
			case ModexActionType::CenterOnCell:
				AddToRecentList(a_item);
				UINotification::ShowAction(Translate("CENTER_ON_CELL"), a_item->GetName(), ICON_LC_PIN);
				break;
			default: Error("Missed switch case for SendEvent(ModexActionType a_actionType, const std::unique_ptr<BaseObject>& a_item)");
				break;
		}
	}
}
