#pragma once

#include "ui/components/UIModule.h"

namespace Modex
{
	class AddItemModule
	{
	private:
		std::unique_ptr<UITable> m_tableView;
		
	public:
		static inline AddItemModule* GetSingleton()
		{
			static AddItemModule singleton;
			return std::addressof(singleton);
		}

		AddItemModule();
		~AddItemModule() = default;

		AddItemModule(const AddItemModule&) = delete;
		AddItemModule(AddItemModule&&) = delete;
		AddItemModule& operator=(const AddItemModule&) = delete;
		AddItemModule& operator=(AddItemModule&&) = delete;

		void Draw(float a_offset);
		void Unload();
		void Load();

		void DrawAddItemActionPanel(const ImVec2 &a_pos, const ImVec2 &a_size, std::unique_ptr<UITable> &a_view);

		std::unique_ptr<UITable>& GetTableView() { 
			return m_tableView; 
		}

	private:
		void ShowActions();
		
		enum class Viewport
		{
			TableView = 0,
			BlacklistView,
			KitView,
			Count
		};
		
		Viewport m_viewport;
		bool m_playerToggle;
		int m_clickCount;
	};
}
