#pragma once

#include "ui/components/UIModule.h"

namespace Modex
{
	class ActorModule : public UIModule
	{
	public:
		ActorModule();
		~ActorModule();
		ActorModule(const ActorModule&) = delete;
		ActorModule(ActorModule&&) = delete;
		ActorModule& operator=(const ActorModule&) = delete;
		ActorModule& operator=(ActorModule&&) = delete;

		void Draw() override;
	};
}
