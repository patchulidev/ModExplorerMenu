#pragma once

#include "ui/components/UIModule.h"

namespace Modex
{	
	class TeleportModule : public UIModule
	{
	public:
		TeleportModule();
		~TeleportModule();
		TeleportModule(const TeleportModule&) = delete;
		TeleportModule(TeleportModule&&) = delete;
		TeleportModule& operator=(const TeleportModule&) = delete;
		TeleportModule& operator=(TeleportModule&&) = delete;

		void Draw() override;
	};
}
