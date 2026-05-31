#pragma once

#include "ui/components/UIModule.h"

namespace Modex
{
	class SpellModule : public UIModule
	{
	public:
		SpellModule();
		~SpellModule();
		SpellModule(const SpellModule&) = delete;
		SpellModule(SpellModule&&) = delete;
		SpellModule& operator=(const SpellModule&) = delete;
		SpellModule& operator=(SpellModule&&) = delete;

		void Draw() override;
		float GetContentRatio() const override;
	};
}
