#include "Menu.h"
#include "ui/core/UIManager.h"
#include "ui/components/UICustom.h"
#include "ui/modules/actor/ActorModule.h"
#include "ui/modules/home/HomeModule.h"
#include "ui/modules/object/ObjectModule.h"
#include "ui/modules/additem/AddItemModule.h"
#include "ui/modules/teleport/TeleportModule.h"
#include "ui/modules/equipment/EquipmentModule.h"
#include "ui/modules/settings/SettingsModule.h"

#include "config/UserData.h"
#include "config/UserConfig.h"
#include "config/ThemeConfig.h"

#include "core/Graphic.h"
#include "localization/FontManager.h"

namespace Modex
{
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
		if (a_window == ActiveWindow::kTotal) {
			ReloadWindow(ActiveWindow::Home);
			return;
		}

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
			// TeleportModule::GetSingleton()->Load();
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
		uint8_t next_window = static_cast<uint8_t>(activeWindow) + 1;
		ReloadWindow(static_cast<ActiveWindow>(next_window));
	}

	void Menu::Draw()
	{
		ImGui::PushStyleVar(ImGuiStyleVar_Alpha, m_alpha);

		const auto& config = UserConfig::Get();
		const auto displaySize = ImGui::GetMainViewport()->Size;

		// Predefined window size.
		const float size_x = displaySize.x * 0.90f;
		const float size_y = displaySize.y * 0.775f;

		// User defined window scale.
		const float scale_x = (config.uiScaleHorizontal / 100.0f);
		const float scale_y = (config.uiScaleVertical / 100.0f);

		// Prevent users from scaling window outside the display bounds.
		const float size_w = std::clamp(size_x * scale_x, displaySize.x / 4.0f, displaySize.x);
		const float size_h = std::clamp(size_y * scale_y, displaySize.y / 4.0f, displaySize.y);

		// Resolve final window resolution conditionally.
		const float window_w = config.fullscreen ? displaySize.x : size_w;
		const float window_h = config.fullscreen ? displaySize.y : size_h;

		min_sidebar_w = 64.0f + (ImGui::GetStyle().WindowPadding.x * 2);
		max_sidebar_w = size_w * 0.12f;

		const float center_x = (displaySize.x * 0.5f) - (window_w * 0.5f);
		const float center_y = (displaySize.y * 0.5f) - (window_h * 0.5f);

		if (!sidebar_initialized) {
			sidebar_w = expand_sidebar ? max_sidebar_w : min_sidebar_w;
			sidebar_initialized = true;
		}

		// Draw a transparent black background.
		ImGui::SetNextWindowPos(ImVec2(0, 0));
		ImGui::SetNextWindowSize(displaySize);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
		ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
		ImGui::PushStyleColor(ImGuiCol_WindowBg, ThemeConfig::GetColor("SCREEN_BACKGROUND", m_alpha));
		ImGui::Begin("##Modex::Background", nullptr, BACKGROUND_FLAGS);

		// Render a splash logo if theme contains a valid path.
		if (const auto splash_image = ThemeConfig::GetSplashLogo(); splash_image.has_value()) {
			if (splash_image->texture != nullptr) {
				ImGui::SetCursorPosX((displaySize.x / 2.0f) - (splash_image->width / 2.0f));
				ImGui::SetCursorPosY((displaySize.y / 2.0f) - (splash_image->height / 2.0f));
				ImGui::Image(reinterpret_cast<ImTextureID>(splash_image->texture), ImVec2(splash_image->width, splash_image->height));
			}
		}

		ImGui::End();
		ImGui::PopStyleVar(2);
		ImGui::PopStyleColor(2);
		// End black background
		

		// Push style for Modex Menu window
		ImGui::PushStyleColor(ImGuiCol_WindowBg, ThemeConfig::GetColor("WINDOW_BACKGROUND", m_alpha));
		ImGui::PushStyleColor(ImGuiCol_TableRowBg, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
		ImGui::PushStyleColor(ImGuiCol_TableRowBgAlt, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));

		ImGui::SetNextWindowSize(ImVec2(window_w, window_h));
		ImGui::SetNextWindowPos(ImVec2(center_x, center_y));

		if (ImGui::Begin("##Modex::Menu", nullptr, WINDOW_FLAGS)) {
			ImGui::SetCursorPos(ImVec2(0, 0));

			ImGui::SetNextItemAllowOverlap();
			if (ImGui::BeginChild("##Modex::Menu::SideBar", ImVec2(sidebar_w, ImGui::GetContentRegionAvail().y + ImGui::GetStyle().WindowPadding.y), ImGuiChildFlags_Borders, WINDOW_FLAGS)) {
				const float button_width = ImGui::GetContentRegionAvail().x;
				const float button_height = ImGui::GetFontSize() * 2.0f;

				{
					// This is ridiculous, but is required due to Skyrim Upscaler Plugin. Typically, I could just
					// create a clip rect, or just hide the image behind the child window outside the sidebar. However,
					// when using Skyrim Upscaler Plugin. The texture clips over the bounds for some reason...
					ImTextureID texture = reinterpret_cast<ImTextureID>(GraphicManager::image_library["new_logo"].texture);

					// Using a fixed image width and height, since the sidebar is also a fixed width and height.
					constexpr float image_height = 54.0f;
					const float image_width = max_sidebar_w - ImGui::GetStyle().WindowPadding.x;
					const ImVec2 backup_pos = ImGui::GetCursorPos();

					// Calculate the UV for the image based on the sidebar width.
					float uv_x = 1.0f - (std::min)(0.35f, 1.0f - (sidebar_w / max_sidebar_w));
					uv_x *= (sidebar_w / image_width);

					// Minor tweak because of bad UV calculation.
					ImGui::SetCursorPosX(ImGui::GetCursorPosX() - 2.0f);
					ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.f, 0.f, 0.f, 0.f));
					ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.f, 0.f, 0.f, 0.f));
					ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.f, 0.f, 0.f, 0.f));
					ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.f, 0.f, 0.f, 0.f));
					ImGui::PushStyleColor(ImGuiCol_BorderShadow, ImVec4(0.f, 0.f, 0.f, 0.f));

					if (ImGui::ImageButton("Modex::Sidebar::Expand", texture, ImVec2(image_width * (sidebar_w / image_width) - 15.0f, image_height), ImVec2(0, 0), ImVec2(uv_x, 1.0f))) {
						this->expand_sidebar = !this->expand_sidebar;
						UserData::User().Set<bool>("Menu::Sidebar", this->expand_sidebar);
					}

					ImGui::PopStyleColor(5);

					if (expand_sidebar) {
						const char* title = "A Mod Explorer Menu";
						ImGui::PushFont(NULL, 16.0f);
						ImGui::SetCursorPosY(ImGui::GetCursorPosY() - image_height / 2.75f);
						ImGui::SetCursorPosX(UICustom::GetCenterTextPosX(title));
						ImGui::Text("%s", title);
						ImGui::PopFont();
					}
				}

				ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal);


				static constexpr std::string home_icon = ICON_LC_HOUSE; 
				if (UICustom::SidebarImageButton("Home", home_icon, activeWindow == ActiveWindow::Home, ImVec2(button_width, button_height), home_w, expand_sidebar)) {
					activeWindow = ActiveWindow::Home;
				}
			
				static constexpr std::string item_icon = ICON_LC_PLUS;
				if (UICustom::SidebarImageButton("Item", item_icon, activeWindow == ActiveWindow::AddItem, ImVec2(button_width, button_height), additem_w, expand_sidebar)) {
					ReloadWindow(ActiveWindow::AddItem);
				}

				static constexpr std::string equip_icon = ICON_LC_PACKAGE;
				if (UICustom::SidebarImageButton("Equipment", equip_icon, activeWindow == ActiveWindow::Equipment, ImVec2(button_width, button_height), equipment_w, expand_sidebar)) {
					ReloadWindow(ActiveWindow::Equipment);
				}

				static constexpr std::string object_icon = ICON_LC_SHAPES;
				if (UICustom::SidebarImageButton("Object", object_icon, activeWindow == ActiveWindow::Object, ImVec2(button_width, button_height), object_w, expand_sidebar)) {
					ReloadWindow(ActiveWindow::Object);
				}				

				static constexpr std::string actor_icon = ICON_LC_USER_PLUS;
				if (UICustom::SidebarImageButton("Actor", actor_icon, activeWindow == ActiveWindow::NPC, ImVec2(button_width, button_height), npc_w, expand_sidebar)) {
					ReloadWindow(ActiveWindow::NPC);
				}				

				static constexpr std::string teleport_icon = ICON_LC_MAP_PIN;
				if (UICustom::SidebarImageButton("Teleport", teleport_icon, activeWindow == ActiveWindow::Teleport, ImVec2(button_width, button_height), teleport_w, expand_sidebar)) {
					ReloadWindow(ActiveWindow::Teleport);
				}			

				static constexpr std::string settings_icon = ICON_LC_SETTINGS;
				if (UICustom::SidebarImageButton("Settings", settings_icon, activeWindow == ActiveWindow::Settings, ImVec2(button_width, button_height), settings_w, expand_sidebar)) {
					ReloadWindow(ActiveWindow::Settings);
				}

				static constexpr std::string exit_icon = ICON_LC_LOG_OUT;
				if (UICustom::SidebarImageButton("Exit", exit_icon, false, ImVec2(button_width, button_height), exit_w, expand_sidebar)) {
					UIManager::GetSingleton()->Close();
				}

				static const float step = 5.0f;
				if (this->expand_sidebar) {
					if (sidebar_w + step < max_sidebar_w) {
						sidebar_w += step;
					} else {
						sidebar_w = max_sidebar_w;
					}
				} else {
					if (sidebar_w - step > min_sidebar_w) {
						sidebar_w -= step;
					} else {
						sidebar_w = min_sidebar_w;
					}
				}
				
			}

			ImGui::EndChild();
			ImGui::SameLine();

			switch (activeWindow) {
			case ActiveWindow::Home:
				HomeModule::Draw();
				break;
			case ActiveWindow::AddItem:
				AddItemModule::GetSingleton()->Draw(sidebar_w);
				break;
			case ActiveWindow::Equipment:
				EquipmentModule::GetSingleton()->Draw(sidebar_w);
				break;
			case ActiveWindow::Object:
				ObjectModule::GetSingleton()->Draw(sidebar_w);
				break;
			case ActiveWindow::NPC:
				ActorModule::GetSingleton()->Draw(sidebar_w);
				break;
			case ActiveWindow::Teleport:
				TeleportModule::GetSingleton()->Draw(sidebar_w);
				break;
			case ActiveWindow::Settings:
				SettingsModule::GetSingleton()->Draw(sidebar_w);
				break;
			default:
				break;
			}
		}

		ImGui::End();
		ImGui::PopStyleColor(3);
		ImGui::PopStyleVar(); // alpha
	}
	
}  // namespace Modex
