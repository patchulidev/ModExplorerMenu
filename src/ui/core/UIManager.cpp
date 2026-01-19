#include "UIManager.h"

#include "ui/Menu.h"
#include "config/UserConfig.h"
#include "config/UserData.h"
#include "ui/components/UIBanner.h"
#include "ui/components/UIPopup.h"

#include "ui/modules/actor/ActorModule.h"
#include "ui/modules/home/HomeModule.h"
#include "ui/modules/object/ObjectModule.h"
#include "ui/modules/additem/AddItemModule.h"
#include "ui/modules/teleport/TeleportModule.h"
#include "ui/modules/equipment/EquipmentModule.h"

#include "localization/FontManager.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"

namespace Modex
{
	void UIManager::OnShow()
	{
		m_windowStack.clear();
		m_windowStack.push_back(std::make_unique<Menu>());
		m_gui = std::make_unique<ModexGUIMenu>();

		m_menu = static_cast<Menu*>(m_windowStack.back().get());
		m_menu->OpenWindow(this);

		// TODO: Determine how to patch / create compatibility for SkyrimSouls with new IMenu impl. 
		// if (LoadLibrary("Data/SKSE/Plugins/SkyrimSoulsRE.dll")) {
		//     if (RE::Main* Game = RE::Main::GetSingleton()) {
		// 		Game->freezeTime = true;
		// 	}
		// }
	}

	void UIManager::OnClose()
	{
		m_gui = nullptr;
		m_windowStack.clear();

		// OPTIMIZE: Is this a good location for this?
		UserData::Save();

		ImGui::GetIO().ClearInputKeys();
	}

	void UIManager::Open()
	{
		PrettyLog::Debug("UIManager::Open() called");
		if (auto *messageQueue = RE::UIMessageQueue::GetSingleton(); messageQueue != nullptr)
		{
			messageQueue->AddMessage(ModexGUIMenu::MENU_NAME, RE::UI_MESSAGE_TYPE::kShow, nullptr);
		}
	}

	void UIManager::Close()
	{
		PrettyLog::Debug("UIManager::Close() called");
		if (auto *messageQueue = RE::UIMessageQueue::GetSingleton(); messageQueue != nullptr)
		{
			messageQueue->AddMessage(ModexGUIMenu::MENU_NAME, RE::UI_MESSAGE_TYPE::kHide, nullptr);
		}
	}

	void UIManager::Shutdown()
	{
		UserData::Save();
		ImGui_ImplDX11_Shutdown();
		ImGui_ImplWin32_Shutdown();
		ImGui::DestroyContext();
	}

	void UIManager::DeleteWindow(UIWindow* a_window)
	{
		auto it = std::remove_if(m_windowStack.begin(), m_windowStack.end(),
			[a_window](const std::unique_ptr<UIWindow>& window) {
				return window.get() == a_window;
			});

		if (it != m_windowStack.end()) {
			m_windowStack.erase(it, m_windowStack.end());
		}

		if (m_windowStack.empty()) {
			Close();
		}
	}

	// FIX: Need to write-down and verify Tab button behavior in v2.0.
	void UIManager::PopWindow()
	{
		if (!m_windowStack.empty()) {
			m_windowStack.back()->CloseWindow();
		} else {
			Close();
		}

		// if (const auto& inputMgr = RE::BSInputDeviceManager::GetSingleton()) {
		// 	if (const auto& device = inputMgr->GetKeyboard()) {
		// 		device->Reset();
		// 	}
		// }
		
		// ImGui::GetIO().AddKeyEvent(ImGuiKey_Tab, false); // ?
		// ImGui::GetIO().AddKeyEvent(ImGuiKey_Escape, false);
		// ImGui::GetIO().AddKeyEvent(ImGuiKey_LeftAlt, false);
		// ImGui::GetIO().ClearInputKeys();
	}

	bool UIManager::IsMenuOpen() const
	{
		return m_gui != nullptr;
	}

	void UIManager::MouseHandler()
	{
		if (auto *ui = RE::UI::GetSingleton(); ui != nullptr)
		{
			POINT cursorPos;
			if (ui->IsMenuOpen(RE::CursorMenu::MENU_NAME))
			{
				const auto *menuCursor = RE::MenuCursor::GetSingleton();
				ImGui::GetIO().AddMouseSourceEvent(ImGuiMouseSource_Mouse);
				ImGui::GetIO().AddMousePosEvent(menuCursor->cursorPosX, menuCursor->cursorPosY);
			}
			else if (GetCursorPos(&cursorPos) != FALSE)
			{
				ImGui::GetIO().AddMousePosEvent(static_cast<float>(cursorPos.x), static_cast<float>(cursorPos.y));
			}
		}
	}

	void UIManager::ScrollHandler()
	{
		auto& io = ImGui::GetIO();
		const auto& manager = UIManager::GetSingleton();

		ImVec2 scroll_now = ImVec2(0.0f, 0.0f);
		if(std::abs(manager->m_scrollEnergy.x) > 0.01f) {
			scroll_now.x = manager->m_scrollEnergy.x * io.DeltaTime * manager->m_scrollSmoothing;
			manager->m_scrollEnergy.x -= scroll_now.x;
		} else {
			manager->m_scrollEnergy.x = 0.0f;
		}
		
		if(std::abs(manager->m_scrollEnergy.y) > 0.01f) {
			scroll_now.y = manager->m_scrollEnergy.y * io.DeltaTime * manager->m_scrollSmoothing;
			manager->m_scrollEnergy.y -= scroll_now.y;
		} else {
			manager->m_scrollEnergy.y = 0.0f;
		}
		
		io.MouseWheel = scroll_now.y;
		io.MouseWheelH = -scroll_now.x;
	}

	void UIManager::ShortcutHandler()
	{
		const auto& config = UserConfig::Get();
		const auto& io = ImGui::GetIO();

		if (!config.disableAlt && !io.WantCaptureKeyboard) {
			if (ImGui::IsKeyReleased(ImGuiMod_Alt)) {
				if (const auto& menu = UIManager::GetSingleton()->m_menu; menu) {
					menu->NextWindow();
				}
			}
		}
	}

	static inline bool build_atlas = true;
	void UIManager::Render()
	{
		ImGui_ImplDX11_NewFrame();
		ImGui_ImplWin32_NewFrame();

		MouseHandler();
		ScrollHandler();
		ShortcutHandler();

		ImGui::NewFrame();

		ImGui::ShowDemoWindow();

		TrySetAllowTextInput();
		ImGui::PushFont(NULL, UserConfig::Get().globalFontSize);
		for (const auto& window : m_windowStack) {
			if (window) {
				window->Update(ImGui::GetIO().DeltaTime);
			}
		}
		ImGui::PopFont();

		ImGui::Render();
		ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
	}

	auto UIManager::MainWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) -> LRESULT
	{
		switch (msg)
		{
			case WM_DESTROY: {
				PrettyLog::Trace("Window Destroyed. Modex shutting down..."); 
				UIManager::GetSingleton()->Shutdown();
				break;
			}
			default:
				break;
		}
		return RealWndProc(hWnd, msg, wParam, lParam);
	}

	bool UIManager::DoInit(const RE::BSGraphics::RendererData &renderData, HWND hWnd)
	{
		auto *device    = reinterpret_cast<ID3D11Device *>(renderData.forwarder);
		auto *context   = reinterpret_cast<ID3D11DeviceContext *>(renderData.context);

		IMGUI_CHECKVERSION();
		ImGui::CreateContext();

		if (!ImGui_ImplWin32_Init(hWnd))
			return false;

		if (!ImGui_ImplDX11_Init(device, context))
			return false;

		RECT     rect = {0, 0, 0, 0};
		::GetClientRect(hWnd, &rect);

		ImGuiIO &io   = ImGui::GetIO();
		io.ConfigNavMoveSetMousePos = false;
		io.DisplaySize = ImVec2(static_cast<float>(rect.right - rect.left), static_cast<float>(rect.bottom - rect.top));
		io.ConfigFlags &= ~ImGuiConfigFlags_NavEnableKeyboard;
		io.ConfigFlags &= ~ImGuiConfigFlags_NavEnableGamepad;
		io.IniFilename = NULL;

		ImGuiStyle &style = ImGui::GetStyle();
		style.HoverDelayNormal = 0.75f;
		style.HoverDelayShort = 0.50f;
		style.ButtonTextAlign = ImVec2(0.5f, 0.5f);
		style.SelectableTextAlign = ImVec2(0.5f, 0.5f);

		ImGui::StyleColorsDark(); // ?

		return true;
	}

	// Called from D3D11 Init Hook
	void UIManager::DoD3DInit()
	{
		if (m_initialized.load()) return;

		const auto* renderer = RE::BSGraphics::Renderer::GetSingleton();
		ASSERT_MSG(renderer == nullptr, "Failed to get BSGraphics Renderer!");

		const auto& renderData = renderer->GetRuntimeData();
		auto* swapChain = renderData.renderWindows[0].swapChain;
		ASSERT_MSG(swapChain == nullptr, "Failed to get SwapChain from Renderer!");

		REX::W32::DXGI_SWAP_CHAIN_DESC swapChainDesc;
		if (swapChain->GetDesc(&swapChainDesc) < S_OK) {
			ASSERT_MSG(true, "Failed to get SwapChain description!");
			return;
		}

		m_hWnd = reinterpret_cast<HWND>(swapChainDesc.outputWindow);
		if (!this->DoInit(renderData, m_hWnd)) {
			ASSERT_MSG(true, "Modex initialization failed!");
			return;
		}

		RealWndProc = reinterpret_cast<WNDPROC>(SetWindowLongPtrA(m_hWnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(MainWndProc)));
		ModexGUIMenu::RegisterMenu();

		m_initialized.store(true);
	}

	// Source: https://github.com/ocornut/imgui/issues/8391
	void UIManager::AddScrollEvent(float x, float y)
	{
		// Apply scroll multiplier
		float scroll_x = x * m_scrollMultiplier;
		float scroll_y = y * m_scrollMultiplier;

		// Immediately stop if direction changes
		if(m_scrollEnergy.x * scroll_x < 0.0f) {
			m_scrollEnergy.x = 0.0f;
		}
		if(m_scrollEnergy.y * scroll_y < 0.0f) {
			m_scrollEnergy.y = 0.0f;
		}

		m_scrollEnergy.x += scroll_x;
		m_scrollEnergy.y += scroll_y;
	}

	void UIManager::ShowBanner()
	{
		m_windowStack.push_back(std::make_unique<UIBanner>());
		UIBanner* banner = static_cast<UIBanner*>(m_windowStack.back().get());
		banner->OpenWindow(this);
		banner->Display();
	}

	void UIManager::ShowWarning(const std::string& a_title, const std::string& a_message, std::function<void()> onConfirmCallback)
	{
		m_windowStack.push_back(std::make_unique<UIPopupWarning>());
		UIPopupWarning* popup = static_cast<UIPopupWarning*>(m_windowStack.back().get());
		popup->PopupWarning(a_title, a_message, onConfirmCallback);
		popup->OpenWindow(this);
	}

	void UIManager::ShowHotkey(uint32_t* a_hotkey, uint32_t& a_default, std::function<void()> onConfirmHotkeyCallback)
	{
		m_windowStack.push_back(std::make_unique<UIPopupHotkey>());
		UIPopupHotkey* popup = static_cast<UIPopupHotkey*>(m_windowStack.back().get());
		popup->PopupHotkey(a_hotkey, a_default, onConfirmHotkeyCallback);
		popup->OpenWindow(this);
	}

	void UIManager::ShowInputBox(const std::string& a_title, const std::string& a_message, std::string a_hint, std::function<void(const std::string&)> onConfirmCallback)
	{
		m_windowStack.push_back(std::make_unique<UIPopupInputBox>());
		UIPopupInputBox* popup = static_cast<UIPopupInputBox*>(m_windowStack.back().get());
		popup->PopupInputBox(a_title, a_message, a_hint, onConfirmCallback);
		popup->OpenWindow(this);
	}

	void UIManager::ShowBrowser(const std::string& a_title, const std::vector<std::string>& a_items, std::function<void(const std::string&)> onSelectCallback)
	{
		m_windowStack.push_back(std::make_unique<UIPopupBrowser>());
		UIPopupBrowser* popup = static_cast<UIPopupBrowser*>(m_windowStack.back().get());
		popup->PopupBrowser(a_title, a_items, onSelectCallback);
		popup->OpenWindow(this);
	}

	void UIManager::ShowInfoBox(const std::string& a_title, const std::string& a_message)
	{
		m_windowStack.push_back(std::make_unique<UIPopupInfo>());
		UIPopupInfo* popup = static_cast<UIPopupInfo*>(m_windowStack.back().get());
		popup->PopupInfo(a_title, a_message);
		popup->OpenWindow(this);
	}

	template<typename T>
	UIWindow* UIManager::GetPopupWindowRef() const
	{
		for (auto& window : m_windowStack) {
			if (auto popup = dynamic_cast<T*>(window.get())) {
				return popup;
			}
		}

		return nullptr;
	}

	// OPTIMIZE: Probably not necessary anymore. Try handling via shortcuts in Popup component class.
	bool UIManager::InputHandler(ImGuiKey a_key)
	{
		if (auto infoBoxPopup = dynamic_cast<UIPopupInfo*>(GetPopupWindowRef<UIPopupInfo>()); infoBoxPopup) {
			if (a_key == ImGuiKey_Escape) {
				infoBoxPopup->CloseInfo();
				return true;
			}

			if (a_key == ImGuiKey_Enter) {
				infoBoxPopup->CloseInfo();
				return false;
			}
		}

		if (auto inputBoxPopup = dynamic_cast<UIPopupInputBox*>(GetPopupWindowRef<UIPopupInputBox>()); inputBoxPopup) {
			if (a_key == ImGuiKey_Enter) {
				inputBoxPopup->AcceptInput();  // Empty string, handled in callback
				return false;

			} else if (a_key == ImGuiKey_Escape) {
				inputBoxPopup->DeclineInput();
				return true;
			}
		}
		
		if (auto warningPopup = dynamic_cast<UIPopupWarning*>(GetPopupWindowRef<UIPopupWarning>()); warningPopup) {
			if (a_key == ImGuiKey_UpArrow || a_key == ImGuiKey_DownArrow) {
				*warningPopup->GetNavAccept() = !*warningPopup->GetNavAccept();
			}

			if (a_key == ImGuiKey_Enter) {
				if (warningPopup->GetNavAccept() && *warningPopup->GetNavAccept()) {
					warningPopup->AcceptWarning();
				} else {
					warningPopup->DeclineWarning();
				}
				return false;

			} else if (a_key == ImGuiKey_Escape) {
				warningPopup->DeclineWarning();
				return true;

			} else if (a_key == ImGuiKey_Y) {
				warningPopup->AcceptWarning();
				return false;

			} else if (a_key == ImGuiKey_N) {
				warningPopup->DeclineWarning();
				return false;
			}
		}

		// FIX: Probably need to specifically handle a whitelist for modifier keys for users wanting to set modifiers
		if (auto hotkeyPopup = dynamic_cast<UIPopupHotkey*>(GetPopupWindowRef<UIPopupHotkey>()); hotkeyPopup) {
			if (a_key == ImGuiKey_T) {
				hotkeyPopup->AcceptHotkey(true);
				return false;
			}

			if (a_key == ImGuiKey_Escape) {
				hotkeyPopup->DeclineHotkey();
				return true;
			}

			if (ImGui::ImGuiKeyToScanCode(a_key) == hotkeyPopup->GetCurrentHotkey()) {
				hotkeyPopup->DeclineHotkey();
				return false;
			} else {
				hotkeyPopup->SetCurrentHotkey(ImGui::ImGuiKeyToScanCode(a_key));
				hotkeyPopup->AcceptHotkey();
				return false;
			}
		}

		return false;
	}

	// Simple IME compatibility! <3
	void UIManager::TrySetAllowTextInput()
	{
		const bool cWantTextInput = ImGui::GetIO().WantTextInput;
		if (!m_wantTextInput && cWantTextInput)
		{
			AllowTextInput(true);
		}
		else if (m_wantTextInput && !cWantTextInput)
		{
			AllowTextInput(false);
		}

		m_wantTextInput = cWantTextInput;
	}

	void UIManager::AllowTextInput(const bool allow)
	{
		AllowTextInput1(RE::ControlMap::GetSingleton(), allow);
	}

	void UIManager::AllowTextInput1(RE::ControlMap *controlMap, bool allow)
	{
		using func_t = decltype(&UIManager::AllowTextInput1);
		static REL::Relocation<func_t> func{RELOCATION_ID(67252, 68552)};
		func(controlMap, allow);
	}

	UIManager::UIManager() try {
		m_homeWindow = std::make_unique<HomeModule>();
		m_addItemWindow = std::make_unique<AddItemModule>();
		m_equipmentWindow = std::make_unique<EquipmentModule>();
		m_npcWindow = std::make_unique<ActorModule>();
		m_objectWindow = std::make_unique<ObjectModule>();
		// m_teleportWindow = std::make_unique<TeleportModule>();
	} catch (const std::exception& e) {
		ASSERT_MSG(true, "UIManager constructor failed!\n\n" + std::string(e.what()));
	}

	UIManager::~UIManager() noexcept = default;
}

