#pragma once

#include "ui/components/UIModule.h"

namespace Modex
{
	class AddItemModule : public UIModule
	{
	public:
		AddItemModule();
		~AddItemModule();
		AddItemModule(const AddItemModule&) = delete;
		AddItemModule(AddItemModule&&) = delete;
		AddItemModule& operator=(const AddItemModule&) = delete;
		AddItemModule& operator=(AddItemModule&&) = delete;

		void Draw() override;
	};
}
