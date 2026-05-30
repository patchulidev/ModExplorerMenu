#pragma once

#include <functional>
#include <mutex>

#include "data/BaseObject.h"

namespace Modex
{
	class PlayerChestSpawn
	{
	public:
		enum class Placement
		{
			InFrontOfPlayer,
			HiddenBelowPlayer,
		};

		static inline PlayerChestSpawn* GetSingleton()
		{
			static PlayerChestSpawn singleton;
			return std::addressof(singleton);
		}

		void PopulateChestWithItems(const std::vector<std::unique_ptr<BaseObject>>& a_items, Placement a_placement = Placement::InFrontOfPlayer);
		void PopulateChestWithKit(const Modex::Kit& a_kit, Placement a_placement = Placement::InFrontOfPlayer);
		void PopulateChestWithOutfit(const RE::BGSOutfit* a_outfit, uint16_t a_level = 0, Placement a_placement = Placement::InFrontOfPlayer);
		void PopulateChestWithActorInventory(RE::TESObjectREFR* a_actor, Placement a_placement = Placement::HiddenBelowPlayer);
		void OpenChest();

		void OnModexClosed();
		void OnContainerMenuClosed();

	private:
		void InitializeBaseContainer();
		RE::TESObjectREFR* SpawnChestReference(Placement a_placement);
		void QueueOrDispatch(std::function<void()> a_action);

	private:
		RE::TESObjectCONT*      m_chestContainer = nullptr;
		RE::ObjectRefHandle     m_chestRefHandle;
		std::function<void()>   m_pendingAction;
		std::mutex              m_pendingMutex;
		bool                    m_cleanupOnClose = false;
	};
}
