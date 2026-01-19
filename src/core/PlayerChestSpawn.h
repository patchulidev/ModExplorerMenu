#pragma once

#include "data/BaseObject.h"

namespace Modex
{
	class PlayerChestSpawn
	{
	public:
		static inline PlayerChestSpawn* GetSingleton()
		{
			static PlayerChestSpawn singleton;
			return std::addressof(singleton);
		}

		void PopulateChestWithItems(const std::vector<std::unique_ptr<BaseObject>>& a_items);
		void PopulateChestWithKit(const Modex::Kit& a_kit);

	private:
		void Reset();
		void InitializeChest();
		void InitializeChestHandle();
		void OpenChest();

	private:
		RE::TESObjectCONT*      m_chestContainer = nullptr;
		RE::ObjectRefHandle     m_chestHandle{};
	};
}
