#include "include/U/UIManager.h"
#include "include/U/UserSettings.h"
#include "include/U/UIBanner.h"

#include "include/A/AddItem.h"
#include "include/N/NPC.h"
#include "include/T/Teleport.h"
#include "include/O/Object.h"
#include "include/B/Blacklist.h"
#include "include/H/Home.h"
#include "include/S/Settings.h"

#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"

namespace Modex
{
    void UIManager::Open()
	{
		if (!AllowMenuOpen()) {
			return;
		}

		if (RE::Main* Game = RE::Main::GetSingleton()) {
			m_prevFreezeState = Game->freezeTime;

			if (Settings::GetSingleton()->GetConfig().pauseGame) {
				Game->freezeTime = true;
			}
		}

		if (ImGui::GetCurrentContext() != nullptr) {
			auto& io = ImGui::GetIO();

			io.MouseDrawCursor = true;
			io.ClearInputKeys();

			// TODO: Revisit when assigning default focus on open/close.
			ImGui::SetWindowFocus("##ModexMenu");
		}

        m_windowStack.clear();
        m_windowStack.push_back(std::make_unique<Menu>());
        Menu* menu = static_cast<Menu*>(m_windowStack.back().get());
        menu->SetActiveWindow(m_lastActiveWindow);
        
		m_isEnabled = true;
	}

    void UIManager::Close()
	{
		if (RE::Main* Game = RE::Main::GetSingleton()) {
			Game->freezeTime = m_prevFreezeState;
		}

		if (ImGui::GetCurrentContext() != nullptr) {
			auto& io = ImGui::GetIO();

			io.MouseDrawCursor = false;
			io.ClearInputKeys();
		}

		PersistentData::GetSingleton()->SaveUserdata();

        if (m_windowStack.size() == 1 && dynamic_cast<Menu*>(m_windowStack.back().get())) {
            m_lastActiveWindow = static_cast<Menu*>(m_windowStack.back().get())->GetActiveWindow();
        }

        m_windowStack.clear();
		m_isEnabled = false;
	}

    void UIManager::Toggle()
	{
		if (m_isEnabled) {
			Close();
		} else {
			Open();
		}
	}

    bool UIManager::AllowMenuOpen()
	{
		const auto& config = Settings::GetSingleton()->GetConfig();

		if (!config.disableInMenu)
			return true;

		if (config.showMenuModifier != 0)
			return true;

		if (m_isEnabled)
			return true;

		// If the hotkey assigned to Modex doesn't overlap text-input behavioral keys, then we can process the event.
		if (config.showMenuKey != 0x0E &&  // Backspace
			config.showMenuKey != 0x0F &&  // Tab
			config.showMenuKey != 0x3A &&  // Caps-Lock
			config.showMenuKey != 0x45 &&  // Num-Lock
			config.showMenuKey != 0x46 &&  // Scroll-Lock
			config.showMenuKey != 0xB7 &&  // Prnt-Scrn
			config.showMenuKey != 0xC5 &&  // Pause
			config.showMenuKey != 0xC7 &&  // Home
			config.showMenuKey != 0xC8 &&  // Up
			config.showMenuKey != 0xC9 &&  // Page-Up
			config.showMenuKey != 0xCB &&  // Left
			config.showMenuKey != 0xCD &&  // Right
			config.showMenuKey != 0xCF &&  // End
			config.showMenuKey != 0xD0 &&  // Down
			config.showMenuKey != 0xD1 &&  // Page-Down
			config.showMenuKey != 0xD2 &&  // Insert
			config.showMenuKey != 0xD3) {  // Delete
			return true;
		}

		const auto UIManager = RE::UI::GetSingleton();

		if (UIManager->IsMenuOpen("Console") ||         // Text Input
			UIManager->IsMenuOpen("Dialogue Menu") ||   // Dialogue
			UIManager->IsMenuOpen("Crafting Menu") ||   // Text Input
			UIManager->IsMenuOpen("Training Menu") ||   // Just Incase
			UIManager->IsMenuOpen("MagicMenu") ||       // Text Input
			UIManager->IsMenuOpen("Quantity Menu") ||   // Text Input
			UIManager->IsMenuOpen("RaceSex Menu") ||    // Text Input
			UIManager->IsMenuOpen("BarterMenu") ||      // Text Input
			UIManager->IsMenuOpen("InventoryMenu") ||   // Text Input
			UIManager->IsMenuOpen("ContainerMenu") ||   // Text Input
			UIManager->IsMenuOpen("MessageBoxMenu")) {  // Text Input
			return false;
		}

		return true;
	}

    void UIManager::ShowBanner()
    {
        m_windowStack.push_back(std::make_unique<UIBanner>());
        UIBanner* banner = static_cast<UIBanner*>(m_windowStack.back().get());
        banner->Display();
    }

    void UIManager::Init()
	{
		IMGUI_CHECKVERSION();

		const auto a_manager = RE::BSGraphics::Renderer::GetSingleton();
		const auto a_context = reinterpret_cast<ID3D11DeviceContext*>(a_manager->GetRuntimeData().context);
		const auto a_swapchain = reinterpret_cast<IDXGISwapChain*>(a_manager->GetRuntimeData().renderWindows->swapChain);
		const auto a_device = reinterpret_cast<ID3D11Device*>(a_manager->GetRuntimeData().forwarder);
		
		DXGI_SWAP_CHAIN_DESC desc;
		a_swapchain->GetDesc(&desc);

		m_screenSize = { static_cast<float>(desc.BufferDesc.Width), static_cast<float>(desc.BufferDesc.Height) };
		
		ImGui::CreateContext();
		auto& io = ImGui::GetIO();
		io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;
		io.DisplaySize = m_screenSize;

		ImGui_ImplWin32_Init(desc.OutputWindow);
		ImGui_ImplDX11_Init(a_device, a_context);

		this->device = a_device;

        // Initalize elements
		AddItemWindow::GetSingleton()->Init();
		NPCWindow::GetSingleton()->Init();
		TeleportWindow::GetSingleton()->Init();
		ObjectWindow::GetSingleton()->Init();
		Blacklist::GetSingleton()->Init();
		HomeWindow::Init();
		SettingsWindow::Init();

        m_lastActiveWindow = Menu::ActiveWindow::Home;

		PrettyLog::Debug("ImGui initialized with swap chain display size: {}x{}", desc.BufferDesc.Width, desc.BufferDesc.Height);
	}

    void UIManager::Render()
	{
		// if (welcomeBannerPtr != nullptr) {
		// 	if (welcomeBannerPtr->ShouldDisplay()) {
		// 		ImGui_ImplWin32_NewFrame();
		// 		ImGui_ImplDX11_NewFrame();

		// 		ImGui::NewFrame();
		// 		welcomeBannerPtr->Draw();
		// 		ImGui::EndFrame();
				
		// 		ImGui::Render();
		// 		ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
		// 		return;
		// 	}
		// }

        if (m_pendingFontChange) {
            RebuildFontAtlas();
            return;
        }

        if (m_windowStack.empty()) {
            return;
        }
        
        ImGui_ImplWin32_NewFrame();
        ImGui_ImplDX11_NewFrame();

        auto& io = ImGui::GetIO();
        io.DisplaySize = m_screenSize;

        ImGui::NewFrame();

        for (const auto& window : m_windowStack) {
            if (window) {
                window->Draw();
            }
        }

        ImGui::EndFrame();
        ImGui::Render();

        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

		// upscaler workaround. overrides the display size from ImplWin32_NewFrame() that uses ::GetClientRect()
		// instead, we pull the display size from the swapchain gathered during initialization. drawback is that
		// dynamic window resizing will no longer work.


		
		//Frame::GetSingleton()->Draw();
		//UIManager::GetSingleton()->Draw();
	}

    void UIManager::RebuildFontAtlas()
	{
		auto& io = ImGui::GetIO();
		io.Fonts->Clear();

		FontManager::GetSingleton()->SetStartupFont();

		io.Fonts->FontBuilderIO = ImGuiFreeType::GetBuilderForFreeType();
		io.Fonts->FontBuilderFlags |= ImGuiFreeTypeBuilderFlags_ForceAutoHint;

		io.Fonts->Build();
		ImGui_ImplDX11_InvalidateDeviceObjects();

		m_pendingFontChange = false;
	}

    void UIManager::ShowWarning(const std::string& message, std::function<void()> onConfirmCallback)
    {
        m_pendingWarningMessage = message;
        m_onConfirmCallback = onConfirmCallback;
    }

    void UIManager::ShowHotkey(uint32_t* a_hotkey, const uint32_t& a_default, bool a_modifierOnly, std::function<void()> onConfirmHotkeyCallback)
    {
        m_hotkeyCurrent = a_hotkey;
        m_hotkeyDefault = a_default;
        m_modifierOnly = a_modifierOnly;
        m_onConfirmHotkeyCallback = onConfirmHotkeyCallback;
    }

    bool UIManager::IsWarningPending() const
    {
        return !m_pendingWarningMessage.empty();
    }

    bool UIManager::IsHotkeyPending() const
    {
        return m_onConfirmHotkeyCallback != nullptr;
    }

    void UIManager::AcceptWarning()
    {
        if (m_onConfirmCallback) {
            m_onConfirmCallback();
        }

        m_pendingWarningMessage.clear();
        m_onConfirmCallback = nullptr;
    }

    void UIManager::DeclineWarning()
    {
        m_pendingWarningMessage.clear();
        m_onConfirmCallback = nullptr;
    }

    void UIManager::AcceptHotkey()
    {
        // auto& config = Settings::GetSingleton()->GetConfig();
        // config.showMenuKey = m_hotkeyCurrent;

        if (m_onConfirmHotkeyCallback) {
            m_onConfirmHotkeyCallback();
        }

        m_hotkeyCurrent = 0;
        m_hotkeyDefault = 0;
        m_onConfirmHotkeyCallback = nullptr;
    }

    void UIManager::DeclineHotkey()
    {
        m_hotkeyCurrent = 0;
        m_hotkeyDefault = 0;
        m_onConfirmHotkeyCallback = nullptr;
    }

    // Called from InputManager with IsDown() == true;
    // Return true to prevent handling ESC key.
    bool UIManager::InputHandler(ImGuiKey a_key)
    {
        if (IsWarningPending()) {
            if (a_key == ImGuiKey_UpArrow || a_key == ImGuiKey_DownArrow) {
                nav_accept = !nav_accept;  // lol
            }

            if (a_key == ImGuiKey_Enter) {
                if (nav_accept) {
                    AcceptWarning();
                } else {
                    DeclineWarning();
                }
                return false;

            } else if (a_key == ImGuiKey_Escape) {
                DeclineWarning();
                return true;

            } else if (a_key == ImGuiKey_Y) {
                AcceptWarning();
                return false;

            } else if (a_key == ImGuiKey_N) {
                DeclineWarning();
                return false;
            }
        }

        if (IsHotkeyPending()) {
            if (a_key == ImGuiKey_T) {
                *m_hotkeyCurrent = m_hotkeyDefault;  // handle in callback as default
                AcceptHotkey();
                return false;
            }

            if (a_key == ImGuiKey_Escape) {
                DeclineHotkey();
                return true;
            }

            if (ImGui::ImGuiKeyToScanCode(a_key) == *m_hotkeyCurrent) {
                DeclineHotkey();
                return true;
            }

            if (!m_modifierOnly) {
                if (ImGui::IsKeyboardWhitelist(a_key)) {
                    *m_hotkeyCurrent = ImGui::ImGuiKeyToScanCode(a_key);
                    AcceptHotkey();
                    return true;
                }
            } else {
                if (ImGui::IsKeyboardModifier(a_key)) {
                    *m_hotkeyCurrent = ImGui::ImGuiKeyToScanCode(a_key);
                    AcceptHotkey();
                    return true;
                }
            }
        }

        return false;
    }

    // void UIManager::Draw()
    // {
    //     if (IsWarningPending()) {
    //         _warning();
    //     }

    //     if (IsHotkeyPending()) {
    //         _hotkey();
    //     }
    // }

    void UIManager::_hotkey()
    {
        ImGui::OpenPopup("Modex::Hotkey");

        // All this bullshit is to center the modal window, hopefully..
        auto width = ImGui::GetMainViewport()->Size.x * 0.25f;
        auto height = ImGui::GetMainViewport()->Size.y * 0.20f;
        const float center_x = ImGui::GetMainViewport()->Size.x * 0.5f;
        const float center_y = ImGui::GetMainViewport()->Size.y * 0.5f;
        const float pos_x = center_x - (width * 0.5f);
        const float pos_y = center_y - (height * 0.5f);
        // const float buttonHeight = ImGui::GetFontSize() * 1.5f;

        ImGui::SetNextWindowSize(ImVec2(width, height));
        ImGui::SetNextWindowPos(ImVec2(pos_x, pos_y));
        if (ImGui::BeginPopupModal("Modex::Hotkey", NULL, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar)) {
            if (ImGui::IsWindowAppearing()) {  // Needed still (?)
                ImGui::GetIO().ClearInputKeys();
            }

            ImGui::SubCategoryHeader("Set a Hotkey", ImVec4(0.20f, 0.20f, 0.20f, 1.0f));

            ImGui::NewLine();

            ImGui::SetCursorPosX(ImGui::GetCenterTextPosX(Translate("CONFIG_HOTKEY_SET")));
            ImGui::Text(Translate("CONFIG_HOTKEY_SET"));
            ImGui::NewLine();

            ImGui::SetCursorPosX(ImGui::GetCenterTextPosX(Translate("CONFIG_HOTKEY_RESET")));
            ImGui::Text(Translate("CONFIG_HOTKEY_RESET"));
            ImGui::NewLine();

            ImGui::SetCursorPosX(ImGui::GetCenterTextPosX(Translate("CONFIG_KEY_CANCEL")));
            ImGui::Text(Translate("CONFIG_KEY_CANCEL"));

            ImGui::SetCursorPosY(ImGui::GetWindowSize().y - ImGui::GetFrameHeightWithSpacing() - ImGui::GetStyle().WindowPadding.y);
            if (ImGui::GradientButton(Translate("Close"), ImVec2(ImGui::GetContentRegionAvail().x, ImGui::GetFrameHeightWithSpacing()))) {
                DeclineHotkey();
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
        }
    }

    void UIManager::_warning()
    {
        ImGui::OpenPopup("Modex::Warning");

        // All this bullshit is to center the modal window, hopefully..
        auto width = ImGui::GetMainViewport()->Size.x * 0.25f;
        auto height = ImGui::GetMainViewport()->Size.y * 0.20f;
        const float center_x = ImGui::GetMainViewport()->Size.x * 0.5f;
        const float center_y = ImGui::GetMainViewport()->Size.y * 0.5f;
        const float pos_x = center_x - (width * 0.5f);
        const float pos_y = center_y - (height * 0.5f);
        const float buttonHeight = ImGui::GetFontSize() * 1.5f;

        ImGui::SetNextWindowSize(ImVec2(width, height));
        ImGui::SetNextWindowPos(ImVec2(pos_x, pos_y));
        if (ImGui::BeginPopupModal("Modex::Warning", NULL, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar)) {
            ImGui::SubCategoryHeader("! Warning !", ImVec4(0.35f, 0.20f, 0.20f, 1.0f));

            ImGui::TextWrapped(m_pendingWarningMessage.c_str());

            ImGui::SetCursorPosY(ImGui::GetWindowSize().y - (buttonHeight * 2) - 20.0f);  // subtract button size * 2 + separator
            ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal);
            const ImVec4 buttonColor = ImGui::GetStyleColorVec4(ImGuiCol_ButtonHovered);
            ImGui::PushStyleColor(ImGuiCol_Button, nav_accept ? buttonColor : ImGui::GetStyleColorVec4(ImGuiCol_Button));
            if (ImGui::GradientButton("(Y)es", ImVec2(ImGui::GetContentRegionAvail().x, buttonHeight))) {
                AcceptWarning();

                ImGui::CloseCurrentPopup();
            }
            ImGui::PopStyleColor();

            ImGui::PushStyleColor(ImGuiCol_Button, nav_accept ? ImGui::GetStyleColorVec4(ImGuiCol_Button) : buttonColor);
            if (ImGui::GradientButton("(N)o", ImVec2(ImGui::GetContentRegionAvail().x, buttonHeight))) {
                DeclineWarning();

                ImGui::CloseCurrentPopup();
            }
            ImGui::PopStyleColor();

            ImGui::EndPopup();
        }
    }
}