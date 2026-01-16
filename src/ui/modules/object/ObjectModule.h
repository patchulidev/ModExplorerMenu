#pragma once

// #include "include/C/Columns.h"
// #include "include/G/Graphic.h"
// #include "include/I/InputManager.h"
// #include "include/T/Table.h"
// clang-format off

#include "ui/components/UITable.h"

namespace Modex
{
	class ObjectModule
	{
	private:
		std::unique_ptr<UITable>			m_tableView;

	public:
		static inline ObjectModule* GetSingleton()
		{
			static ObjectModule singleton;
			return std::addressof(singleton);
		}

		ObjectModule();
		~ObjectModule() = default;

		ObjectModule(const ObjectModule&) = delete;
		ObjectModule(ObjectModule&&) = delete;
		ObjectModule& operator=(const ObjectModule&) = delete;
		ObjectModule& operator=(ObjectModule&&) = delete;

		void 						Draw(float a_offset);
		void 						ShowActions();
		void						Unload();
		void						Load();
		
		std::unique_ptr<UITable>& GetTableView() {
			return m_tableView;
		}

	private:
		enum class Viewport
		{
			TableView = 0,
			BlacklistView,
			Count
			// Settings View
		};
		
		Viewport 					activeViewport;
		int 						m_clickCount;
	};
}