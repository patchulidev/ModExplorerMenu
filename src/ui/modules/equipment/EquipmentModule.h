#pragma once

#include "ui/components/UIModule.h"

namespace Modex
{
	class EquipmentModule : public UIModule
	{
	private:

	public:
		EquipmentModule();
		~EquipmentModule();
		EquipmentModule(const EquipmentModule&) = delete;
		EquipmentModule(EquipmentModule&&) = delete;
		EquipmentModule& operator=(const EquipmentModule&) = delete;
		EquipmentModule& operator=(EquipmentModule&&) = delete;

		void Draw() override;

		// These don't need to be static members...
		static inline std::unique_ptr<SearchSystem>   m_searchSystem;
		static void DrawKitSelectionPanel(const ImVec2 &a_pos, const ImVec2 &a_size, std::unique_ptr<UITable> &a_kitTable);

		static inline Kit      m_selectedKit;
		static inline char     m_searchBuffer[256];

		static std::unique_ptr<SearchSystem>& GetSearchSystem() {
			return m_searchSystem;
		}

		static Kit& GetSelectedKit() {
			return m_selectedKit;
		}

		static void SelectKit(const Kit& a_kit) {
			m_selectedKit = a_kit;
		}
	};
}
