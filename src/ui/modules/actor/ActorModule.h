#pragma once

// #include "extern/DescriptionFrameworkAPI.h"
// #include "include/C/Columns.h"
// #include "include/G/Graphic.h"
// #include "include/I/InputManager.h"
// #include "include/T/Table.h"

#include "ui/components/UITable.h"

namespace Modex
{
	class ActorModule
	{
	private:
		std::unique_ptr<UITable> m_tableView;

	public:
		static inline ActorModule* GetSingleton()
		{
			static ActorModule singleton;
			return std::addressof(singleton);
		}

		ActorModule();
		~ActorModule() = default;
		
		ActorModule(const ActorModule&) = delete;
		ActorModule(ActorModule&&) = delete;
		ActorModule& operator=(const ActorModule&) = delete;
		ActorModule& operator=(ActorModule&&) = delete;

		void 					Draw(float a_offset);
		void 					ShowActions();
		void					Unload();
		void					Load();
		
		std::unique_ptr<UITable>& GetTableView() {
			return m_tableView;
		}

	private:
		enum class Viewport
		{
			TableView = 0,
			BlacklistView,
			Count
		};
		
		Viewport 				m_viewport;
		int 					m_clickCount;
	};
}