#pragma once

#include <string>

namespace Modex
{
	struct FormSelectorOptions
	{
		bool singleSelect{ false };
		bool showTotalCost{ false };
		bool requireTotalCost{ false };
		int maxCost{ 0 };              // 0 = unlimited
		int maxCount{ 0 };             // 0 = unlimited
		float costMultiplier{ 1.0f };
		std::string title{};

		void Reset()
		{
			singleSelect = false;
			showTotalCost = false;
			requireTotalCost = false;
			maxCost = 0;
			maxCount = 0;
			costMultiplier = 1.0f;
			title.clear();
		}
	};
}
