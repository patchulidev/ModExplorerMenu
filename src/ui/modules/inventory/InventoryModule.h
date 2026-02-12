#pragma once

#include "ui/components/UIModule.h"

namespace Modex
{
	class InventoryModule : public UIModule
	{
	private:

	public:
		InventoryModule();
		~InventoryModule();
		InventoryModule(const InventoryModule&) = delete;
		InventoryModule(InventoryModule&&) = delete;
		InventoryModule& operator=(const InventoryModule&) = delete;
		InventoryModule& operator=(InventoryModule&&) = delete;

		void Draw() override;
	};
}
