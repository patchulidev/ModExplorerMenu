#pragma once

#include "ui/components/UITable.h"

namespace Modex
{
	class EquipmentModule
	{
	private:
		std::unique_ptr<UITable>        m_tableView;
		std::unique_ptr<UITable>        m_inventoryView;
		std::unique_ptr<UITable>        m_kitTableView;
		std::unique_ptr<SearchSystem>   m_searchSystem;

	public:
		static inline EquipmentModule* GetSingleton()
		{
			static EquipmentModule singleton;
			return std::addressof(singleton);
		}

		EquipmentModule();
		~EquipmentModule() = default;

		EquipmentModule(const EquipmentModule&) = delete;
		EquipmentModule(EquipmentModule&&) = delete;
		EquipmentModule& operator=(const EquipmentModule&) = delete;
		EquipmentModule& operator=(EquipmentModule&&) = delete;

		void Draw(float a_offset);
		void Unload();
		void Load();
		void ShowKitBar();

		void DrawKitSelectionPanel(const ImVec2 &a_pos, const ImVec2 &a_size);

		std::unique_ptr<UITable>& GetTableView() {
			return m_tableView;
		}

		std::unique_ptr<UITable>& GetKitTableView() {
			return m_kitTableView;
		}

		std::unique_ptr<SearchSystem>& GetSearchSystem() {
			return m_searchSystem;
		}

		Kit& GetSelectedKit() {
			return m_selectedKit;
		}

		void SelectKit(const Kit& a_kit) {
			m_selectedKit = a_kit;
		}

	private:
		enum class Viewport
		{
			EquipmentView = 0,
			InventoryView,
			FollowerView,
			Count
		};

		char     m_searchBuffer[256];
		int      m_clickCount; // unused
		Kit      m_selectedKit;
		Viewport m_viewport;
	};
}
