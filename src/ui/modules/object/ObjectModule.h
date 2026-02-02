#pragma once

#include "ui/components/UIModule.h"

namespace Modex
{
	class ObjectModule : public UIModule
	{
	public:
		ObjectModule();
		~ObjectModule();
		ObjectModule(const ObjectModule&) = delete;
		ObjectModule(ObjectModule&&) = delete;
		ObjectModule& operator=(const ObjectModule&) = delete;
		ObjectModule& operator=(ObjectModule&&) = delete;

		void Draw() override;
	};
}
