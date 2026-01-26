#pragma once

#include "ui/components/UIModule.h"

namespace Modex
{
	class HomeModule : public UIModule
	{
	public:
		HomeModule();
		~HomeModule() = default;
		HomeModule(const HomeModule&) = delete;
		HomeModule(HomeModule&&) = delete;
		HomeModule& operator=(const HomeModule&) = delete;
		HomeModule& operator=(HomeModule&&) = delete;

		void Draw() override;
	};
}
