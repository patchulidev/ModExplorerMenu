#pragma once

#include "ui/components/UIModule.h"

namespace Modex
{
	class OutfitModule : public UIModule
	{
	public:
		OutfitModule();
		~OutfitModule();
		OutfitModule(const OutfitModule&) = delete;
		OutfitModule(OutfitModule&&) = delete;
		OutfitModule& operator=(const OutfitModule&) = delete;
		OutfitModule& operator=(OutfitModule&&) = delete;

		void Draw() override;
	};
}
