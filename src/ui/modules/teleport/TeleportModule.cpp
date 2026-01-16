#include "TeleportModule.h"

// TODO: Re-Implement Teleport Module

namespace Modex
{
	void TeleportModule::Draw(float a_offset)
	{
		(void)a_offset;
	}

	void TeleportModule::Unload()
	{

	}

	void TeleportModule::Load()
	{

	}

	TeleportModule::TeleportModule() {
		m_tableView = std::make_unique<UITable>();
		m_tableView->Init();
	}

}