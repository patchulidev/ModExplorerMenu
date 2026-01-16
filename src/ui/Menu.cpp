	void Menu::OnOpen()
	{
		uint8_t last_window = UserData::User().Get<uint8_t>("lastActiveWindow", 0);
		ReloadWindow(static_cast<ActiveWindow>(last_window));

		this->expand_sidebar = UserData::User().Get<bool>("Menu::Sidebar", true);
		this->sidebar_initialized = false;
		this->m_captureInput = true;
	}

	void Menu::OnClose()
	{
		uint8_t last_window = static_cast<uint8_t>(activeWindow);
		UserData::User().Set("lastActiveWindow", last_window);
		this->m_captureInput = false;
	}

	void Menu::ReloadWindow(ActiveWindow a_window)
	{
		this->activeWindow = a_window;

		switch (a_window) {
		case Home:
			// HomeWindow::GetSingleton()->Load();
			break;
		case AddItem:
			AddItemModule::GetSingleton()->Load();
			break;
		case Equipment:
			EquipmentModule::GetSingleton()->Load();
			break;
		case Object:
			ObjectModule::GetSingleton()->Load();
			break;
		case NPC:
			ActorModule::GetSingleton()->Load();
			break;
		case Teleport:
			TeleportModule::GetSingleton()->Load();
			break;
		case Settings:
			SettingsModule::GetSingleton()->Load();
			break;
		case kTotal:
			break;
		}
	}

	void Menu::NextWindow()
	{
		uint8_t next_window = (static_cast<uint8_t>(activeWindow) + 1) % static_cast<uint8_t>(ActiveWindow::kTotal);
		ReloadWindow(static_cast<ActiveWindow>(next_window));
	}

		// TODO: Optionally disable this shortcut for users who don't want it. I.e. SimpleIME users.
		if (ImGui::IsKeyReleased(ImGuiMod_Alt)) {
			if (!ImGui::GetIO().WantCaptureKeyboard) {
				NextWindow();
			}
		}
				if (UICustom::SidebarButton("Home", activeWindow == ActiveWindow::Home, home_image.texture, home_image.size, ImVec2(button_width, button_height), home_w)) {
					activeWindow = ActiveWindow::Home;
				}
			
				if (UICustom::SidebarButton("Item", activeWindow == ActiveWindow::AddItem, additem_image.texture, additem_image.size, ImVec2(button_width, button_height), additem_w)) {
					ReloadWindow(ActiveWindow::AddItem);
				}

				if (UICustom::SidebarButton("Equipment", activeWindow == ActiveWindow::Equipment, equipment_image.texture, equipment_image.size, ImVec2(button_width, button_height), equipment_w)) {
					ReloadWindow(ActiveWindow::Equipment);
				}

				if (UICustom::SidebarButton("Object", activeWindow == ActiveWindow::Object, object_image.texture, object_image.size, ImVec2(button_width, button_height), object_w)) {
					ReloadWindow(ActiveWindow::Object);
				}				

				if (UICustom::SidebarButton("NPC", activeWindow == ActiveWindow::NPC, npc_image.texture, npc_image.size, ImVec2(button_width, button_height), npc_w)) {
					ReloadWindow(ActiveWindow::NPC);
				}				

				if (UICustom::SidebarButton("Teleport", activeWindow == ActiveWindow::Teleport, teleport_image.texture, teleport_image.size, ImVec2(button_width, button_height), teleport_w)) {
					ReloadWindow(ActiveWindow::Teleport);
				}			

				if (UICustom::SidebarButton("Settings", activeWindow == ActiveWindow::Settings, settings_image.texture, settings_image.size, ImVec2(button_width, button_height), settings_w)) {
					ReloadWindow(ActiveWindow::Settings);
				}

				if (UICustom::SidebarButton("Exit", false, exit_image.texture, exit_image.size, ImVec2(button_width, button_height), exit_w)) {
					UIManager::GetSingleton()->Close();
				}
