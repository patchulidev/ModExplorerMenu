#pragma once

#include "ui/components/UITable.h"

namespace Modex
{	
	class TeleportModule
	{
	private:
		std::unique_ptr<UITable>			m_tableView;

	public:
		static inline TeleportModule* GetSingleton()
		{
			static TeleportModule singleton;
			return std::addressof(singleton);
		}

		TeleportModule();
		~TeleportModule() = default;

		TeleportModule(const TeleportModule&) = delete;
		TeleportModule(TeleportModule&&) = delete;
		TeleportModule& operator=(const TeleportModule&) = delete;
		TeleportModule& operator=(TeleportModule&&) = delete;

		void 			Draw(float a_offset);
		void			Load();
		void			Unload();

		std::unique_ptr<UITable>& GetTableView() { 
			return m_tableView; 
		}

	private:
		void 						ShowActions();

		enum class Viewport
		{
			TableView = 0,
			Count
		};

		Viewport 					m_viewport;
	};
}