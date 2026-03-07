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
		void PopulateChestWithOutfit(const RE::BGSOutfit* a_outfit, uint16_t a_level = 0);

	private:
		void InitializeBaseContainer();
		RE::TESObjectREFR* SpawnChestReference();
		void DestroyChestReference();
		void OpenChest();

	private:
		RE::TESObjectCONT*      m_chestContainer = nullptr;
		RE::TESObjectREFR*      m_chestRef = nullptr;
	};
}
