#pragma once

#include "ui/components/UIKitList.h"
#include "ui/components/UIModule.h"

namespace Modex
{
	class EquipmentModule : public UIModule
	{
	private:
		Kit                             m_selectedKit;
		char                            m_searchBuffer[256];
		std::unique_ptr<SearchSystem>   m_searchSystem;
		std::unique_ptr<UIKitList>      m_kitList;

	public:
		EquipmentModule();
		~EquipmentModule();
		EquipmentModule(const EquipmentModule&) = delete;
		EquipmentModule(EquipmentModule&&) = delete;
		EquipmentModule& operator=(const EquipmentModule&) = delete;
		EquipmentModule& operator=(EquipmentModule&&) = delete;

		void Draw() override;
		float GetContentRatio() const override;
		void DrawEquipmentLayout(std::vector<std::unique_ptr<UITable>>& a_tables);
		void DrawKitBrowserLayout(std::vector<std::unique_ptr<UITable>>& a_tables);
		void DrawKitActionsPanel(const ImVec2 &a_pos, const ImVec2 &a_size);

		void SelectKitByKey(const std::string& a_key);
		void RefreshKitList();
		void OpenCreateKitDialog();
		void OpenCreateKitFromPlayerDialog(bool a_wornOnly);
		void OpenKitTagsPopup();
	};
}
