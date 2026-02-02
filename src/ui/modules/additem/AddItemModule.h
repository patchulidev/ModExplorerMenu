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
		void DrawAddItemActionPanel(const ImVec2 &a_pos, const ImVec2 &a_size, std::unique_ptr<UITable> &a_view);
	};
}
