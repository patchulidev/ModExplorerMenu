#include "Menu.h"
#include "localization/Locale.h"
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
	void Menu::OnOpening()
	{
		uint8_t last_module = UserData::User().Get<uint8_t>("lastModule", 0);
		uint8_t last_layout = UserData::User().Get<uint8_t>("lastLayout", 0);

		auto& module = m_modules[last_module]; 

		LoadModule(module, last_layout);

		this->expand_sidebar = UserData::User().Get<bool>("Menu::Sidebar", true);
		this->sidebar_initialized = false;
	}

	void Menu::OnOpened()
	{
		this->m_captureInput = true;
	}

	void Menu::OnClosing()
	{
		uint8_t last_module = 0;
		uint8_t last_layout = 0;

		for (const auto& module : m_modules) {
			if (module->IsLoaded()) {
				last_module = static_cast<uint8_t>(&module - m_modules.data());
				last_layout = module->GetActiveLayoutIndex();
				break;
			}
		}

		UserData::User().Set("lastModule", last_module);
		UserData::User().Set("lastLayout", last_layout);

	}

	void Menu::OnClosed()
	{
		for (const auto& module : m_modules) {
			module->Unload();
		}

		this->m_captureInput = false;
	}

	void Menu::LoadModule(std::unique_ptr<UIModule>& a_module, uint8_t a_layoutIndex)
	{
		for (auto& module : m_modules) {
			if (module.get() == a_module.get()) {
				continue;
			}

			module->Unload();
		}

		a_module->Load();
		a_module->SetActiveLayout(a_layoutIndex);
	}

	void Menu::NextWindow()
	{
		// uint8_t index = m_activeModule + 1;
		// auto& next_module = m_modules[index % std::ssize(m_modules)];
		// LoadModule(next_module, 0);
		for (const auto& module : m_modules) {
			if (module->IsLoaded()) {
				uint8_t index = (static_cast<uint8_t>(&module - m_modules.data()) + 1) % std::ssize(m_modules);
				auto& next_module = m_modules[index];
				LoadModule(next_module, 0);
				break;
			}
		}
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

		
		// Push style for Modex Menu window
		ImGui::PushStyleColor(ImGuiCol_WindowBg, ThemeConfig::GetColor("WINDOW_BACKGROUND", m_alpha));
		ImGui::PushStyleColor(ImGuiCol_TableRowBg, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
		ImGui::PushStyleColor(ImGuiCol_TableRowBgAlt, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));

		ImGui::SetNextWindowSize(ImVec2(window_w, window_h));
		ImGui::SetNextWindowPos(ImVec2(center_x, center_y));

		if (ImGui::Begin("##Modex::Menu", nullptr, WINDOW_FLAGS)) {
			ImGui::SetCursorPos(ImVec2(0, 0));

			DrawSidebar();
			DrawModule();
		}

		ImGui::End();
		ImGui::PopStyleColor(3);

		DrawBackground(displaySize);

		ImGui::PopStyleVar(); // alpha
	}

	void Menu::DrawBackground(const ImVec2& a_displaySize)
	{
		ImGui::SetNextWindowPos(ImVec2(0, 0));
		ImGui::SetNextWindowSize(a_displaySize);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
		ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
		ImGui::PushStyleColor(ImGuiCol_WindowBg, ThemeConfig::GetColor("SCREEN_BACKGROUND", m_alpha));
		ImGui::Begin("##Modex::Background", nullptr, BACKGROUND_FLAGS);

		// Render a splash logo if theme contains a valid path.
		if (const auto splash_image = ThemeConfig::GetSplashLogo(); splash_image.has_value()) {
			if (splash_image->texture != nullptr) {
				ImGui::SetCursorPosX((a_displaySize.x / 2.0f) - (splash_image->width / 2.0f));
				ImGui::SetCursorPosY((a_displaySize.y / 2.0f) - (splash_image->height / 2.0f));
				ImGui::Image(reinterpret_cast<ImTextureID>(splash_image->texture), ImVec2(splash_image->width, splash_image->height));
			}
		}

		ImGui::End();
		ImGui::PopStyleVar(2);
		ImGui::PopStyleColor(2);
	}

	void Menu::DrawModule()
	{
		for (const auto& module : m_modules) {
			if (module->IsLoaded()) {
				module->SetOffset(sidebar_w);
				module->Draw();
				break;
			}
		}
	}

	void Menu::DrawSidebar()
	{
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

			for (auto& module : m_modules) {
				const bool is_active = module->IsLoaded();
				const std::string& module_name = module->GetName();
				const std::string& module_icon = module->GetIcon();

				if (UICustom::SidebarImageButton(module_name.c_str(), module_icon.c_str(), is_active, ImVec2(button_width, button_height), module->GetSidebarWidth(), expand_sidebar)) {
					LoadModule(module, module->GetActiveLayoutIndex());
				}
			}

			static constexpr std::string exit_icon = ICON_LC_LOG_OUT;
			if (UICustom::SidebarImageButton(Translate("MODULE_EXIT"), exit_icon, false, ImVec2(button_width, button_height), exit_w, expand_sidebar)) {
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
	}
	
	Menu::Menu()
	{
		m_modules.push_back(std::make_unique<HomeModule>());
		m_modules.push_back(std::make_unique<AddItemModule>());
		m_modules.push_back(std::make_unique<EquipmentModule>());
		m_modules.push_back(std::make_unique<ActorModule>());
		m_modules.push_back(std::make_unique<ObjectModule>());
		m_modules.push_back(std::make_unique<TeleportModule>());
		m_modules.push_back(std::make_unique<SettingsModule>());
	}


}
