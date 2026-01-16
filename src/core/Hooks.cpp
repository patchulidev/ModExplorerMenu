	RE::BSEventNotifyControl IMenuOpenCloseEvent::ProcessEvent(const RE::MenuOpenCloseEvent* event, RE::BSTEventSource<RE::MenuOpenCloseEvent>*) {
		if (!event->opening) {
			if (const auto& manager = Modex::UIManager::GetSingleton(); manager != nullptr) {
				if (event->menuName == RE::ContainerMenu::MENU_NAME) {
					if (manager->GetMenuListener()) {
						manager->QueueOpen();
					}
				}
			}
		}
		
		return RE::BSEventNotifyControl::kContinue;
	}
