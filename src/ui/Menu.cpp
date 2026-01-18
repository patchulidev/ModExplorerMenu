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

#include "core/Graphic.h"

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

	void Menu::Draw()
	{
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
		
		ImGui::PushStyleVar(ImGuiStyleVar_Alpha, m_alpha);
		ImGui::PushStyleColor(ImGuiCol_TableRowBg, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
		ImGui::PushStyleColor(ImGuiCol_TableRowBgAlt, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));

		if (!sidebar_initialized) {
			sidebar_w = expand_sidebar ? max_sidebar_w : min_sidebar_w;
			sidebar_initialized = true;
		}

		// TODO: Optionally disable this shortcut for users who don't want it. I.e. SimpleIME users.
		if (ImGui::IsKeyReleased(ImGuiMod_Alt)) {
			if (!ImGui::GetIO().WantCaptureKeyboard) {
				NextWindow();
			}
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
		
		ImGui::SetNextWindowSize(ImVec2(window_w, window_h));
		ImGui::SetNextWindowPos(ImVec2(center_x, center_y));

		if (ImGui::Begin("##ModexMenu", nullptr, WINDOW_FLAGS)) {
			ImGui::SetCursorPos(ImVec2(0, 0));

			ImGui::SetNextItemAllowOverlap();
			if (ImGui::BeginChild("##SideBar", ImVec2(sidebar_w, ImGui::GetContentRegionAvail().y + ImGui::GetStyle().WindowPadding.y), ImGuiChildFlags_Borders, SIDEBAR_FLAGS)) {
				const float button_width = ImGui::GetContentRegionAvail().x;
				constexpr float button_height = 40.0f;

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

					// ImGui::PopStyleVar();
					ImGui::PopStyleColor(5);

					// This is an alternative to doing the same UV scaling to the Y axis on a single image.
					// Instead, I just overlay a second image on the first one, which yields the same results.
					if (sidebar_w > min_sidebar_w) {
						ImTextureID overlay = reinterpret_cast<ImTextureID>(GraphicManager::image_library["new_logo_bottom"].texture);
						const float image_alpha = std::clamp(sidebar_w / max_sidebar_w, 0.0f, 1.0f);
						ImGui::SameLine();
						ImGui::SetCursorPos(backup_pos);
						ImGui::SetNextItemAllowOverlap();
						ImGui::Image(overlay, ImVec2(image_width * (sidebar_w / image_width) - 15.0f, image_height)); 
					}
				}

				ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal);

				SideBarImage home_image 		= { reinterpret_cast<ImTextureID>(GraphicManager::image_library["IconHome"].texture), ImVec2(32.0f, 32.0f) };
				SideBarImage additem_image 		= { reinterpret_cast<ImTextureID>(GraphicManager::image_library["IconAddItem"].texture), ImVec2(32.0f, 32.0f) };
				SideBarImage equipment_image 	= { reinterpret_cast<ImTextureID>(GraphicManager::image_library["IconAddItem"].texture), ImVec2(32.0f, 32.0f) };
				SideBarImage object_image 		= { reinterpret_cast<ImTextureID>(GraphicManager::image_library["IconObject"].texture), ImVec2(32.0f, 32.0f) };
				SideBarImage npc_image 			= { reinterpret_cast<ImTextureID>(GraphicManager::image_library["IconNPC"].texture), ImVec2(32.0f, 32.0f) };
				SideBarImage teleport_image 	= { reinterpret_cast<ImTextureID>(GraphicManager::image_library["IconTeleport"].texture), ImVec2(32.0f, 32.0f) };
				SideBarImage settings_image 	= { reinterpret_cast<ImTextureID>(GraphicManager::image_library["IconSettings"].texture), ImVec2(32.0f, 32.0f) };
				SideBarImage exit_image 		= { reinterpret_cast<ImTextureID>(GraphicManager::image_library["IconExit"].texture), ImVec2(32.0f, 32.0f) };

				if (UICustom::SidebarImageButton("Home", activeWindow == ActiveWindow::Home, home_image.texture, home_image.size, ImVec2(button_width, button_height), home_w)) {
					activeWindow = ActiveWindow::Home;
				}
			
				if (UICustom::SidebarImageButton("Item", activeWindow == ActiveWindow::AddItem, additem_image.texture, additem_image.size, ImVec2(button_width, button_height), additem_w)) {
					ReloadWindow(ActiveWindow::AddItem);
				}

				if (UICustom::SidebarImageButton("Equipment", activeWindow == ActiveWindow::Equipment, equipment_image.texture, equipment_image.size, ImVec2(button_width, button_height), equipment_w)) {
					ReloadWindow(ActiveWindow::Equipment);
				}

				if (UICustom::SidebarImageButton("Object", activeWindow == ActiveWindow::Object, object_image.texture, object_image.size, ImVec2(button_width, button_height), object_w)) {
					ReloadWindow(ActiveWindow::Object);
				}				

				if (UICustom::SidebarImageButton("NPC", activeWindow == ActiveWindow::NPC, npc_image.texture, npc_image.size, ImVec2(button_width, button_height), npc_w)) {
					ReloadWindow(ActiveWindow::NPC);
				}				

				if (UICustom::SidebarImageButton("Teleport", activeWindow == ActiveWindow::Teleport, teleport_image.texture, teleport_image.size, ImVec2(button_width, button_height), teleport_w)) {
					ReloadWindow(ActiveWindow::Teleport);
				}			

				if (UICustom::SidebarImageButton("Settings", activeWindow == ActiveWindow::Settings, settings_image.texture, settings_image.size, ImVec2(button_width, button_height), settings_w)) {
					ReloadWindow(ActiveWindow::Settings);
				}

				if (UICustom::SidebarImageButton("Exit", false, exit_image.texture, exit_image.size, ImVec2(button_width, button_height), exit_w)) {
					UIManager::GetSingleton()->Close();
				}

				if (this->expand_sidebar) {
					if (sidebar_w + 20.0f < max_sidebar_w) {
						sidebar_w += 20.0f;
					} else {
						sidebar_w = max_sidebar_w;
					}
				} else {
					if (sidebar_w - 20.0f > min_sidebar_w) {
						sidebar_w -= 20.0f;
					} else {
						sidebar_w = min_sidebar_w;
					}
				}
				
			}

			ImGui::EndChild();
			ImGui::SameLine();

			// Need to push a clip rect here so that sidebar overlaps contents while maintaining transparency.
			// Otherwise, the overlap will be transparent, and the overlapped content will show under.
			ImVec2 min = { sidebar_w + ImGui::GetWindowPos().x, ImGui::GetWindowPos().y };
			ImVec2 max = { min.x + ImGui::GetWindowSize().x, min.y + ImGui::GetWindowSize().y };
			ImGui::PushClipRect(min, max, true);

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

			ImGui::PopClipRect();

		}
		ImGui::End();
		ImGui::PopStyleVar();
		ImGui::PopStyleColor(2);
	}
	
}  // namespace Modex
