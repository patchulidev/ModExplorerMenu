#pragma once

#include "ui/components/UIModule.h"

namespace Modex
{
	class EquipmentModule : public UIModule
	{
	private:
		Kit                             m_selectedKit;
		char                            m_searchBuffer[256];
		std::unique_ptr<SearchSystem>   m_searchSystem;

	public:
		EquipmentModule();
		~EquipmentModule();
		EquipmentModule(const EquipmentModule&) = delete;
		EquipmentModule(EquipmentModule&&) = delete;
		EquipmentModule& operator=(const EquipmentModule&) = delete;
		EquipmentModule& operator=(EquipmentModule&&) = delete;

		void Draw() override;
		void DrawEquipmentLayout(std::vector<std::unique_ptr<UITable>>& a_tables);
		void DrawKitActionsPanel(const ImVec2 &a_pos, const ImVec2 &a_size);
	};
}
