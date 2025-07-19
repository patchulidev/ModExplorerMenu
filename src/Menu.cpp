#include "include/M/Menu.h"
#include "include/C/Console.h"
#include "include/F/Frame.h"
#include "include/G/Graphic.h"
#include "include/I/InputManager.h"
#include "include/P/Persistent.h"
#include "include/U/UIManager.h"
#include "include/U/UserSettings.h"

#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"

namespace Modex
{
	bool Menu::AllowMenuOpen()
	{
		const auto& config = Settings::GetSingleton()->GetConfig();

		if (!config.disableInMenu)
			return true;

		if (config.showMenuModifier != 0)
			return true;

		if (Menu::GetSingleton()->isEnabled)
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

	void Menu::Open()
	{
		if (!AllowMenuOpen()) {
			return;
		}

		if (RE::Main* Game = RE::Main::GetSingleton()) {
			prevFreezeState = Game->freezeTime;

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

		isEnabled = true;
	}

	void Menu::Close()
	{
		if (RE::Main* Game = RE::Main::GetSingleton()) {
			Game->freezeTime = prevFreezeState;
		}

		if (ImGui::GetCurrentContext() != nullptr) {
			auto& io = ImGui::GetIO();

			io.MouseDrawCursor = false;
			io.ClearInputKeys();
		}

		PersistentData::GetSingleton()->SaveUserdata();

		isEnabled = false;
	}

	void Menu::Toggle()
	{
		if (isEnabled) {
			Close();
		} else {
			Open();
		}
	}

	void Menu::Draw()
	{
		Console::ProcessMainThreadTasks();
		InputManager::ProcessInput();

		if (!isEnabled) {
			return;
		}

		if (pendingFontChange) {
			RebuildFontAtlas();
			return;
		}

		ImGui_ImplWin32_NewFrame();
		ImGui_ImplDX11_NewFrame();

		// upscaler workaround. overrides the display size from ImplWin32_NewFrame() that uses ::GetClientRect()
		// instead, we pull the display size from the swapchain gathered during initialization. drawback is that
		// dynamic window resizing will no longer work.

		auto& io = ImGui::GetIO();
		io.DisplaySize = this->screenSize;

		ImGui::NewFrame();
		
		Frame::GetSingleton()->Draw();
		UIManager::GetSingleton()->Draw();

		ImGui::EndFrame();
		ImGui::Render();

		ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
	}

	void Menu::RefreshFont()
	{
		pendingFontChange = true;
	}

	void Menu::RebuildFontAtlas()
	{
		auto& io = ImGui::GetIO();
		io.Fonts->Clear();

		FontManager::GetSingleton()->SetStartupFont();

		io.Fonts->FontBuilderIO = ImGuiFreeType::GetBuilderForFreeType();
		io.Fonts->FontBuilderFlags |= ImGuiFreeTypeBuilderFlags_ForceAutoHint;

		io.Fonts->Build();
		ImGui_ImplDX11_InvalidateDeviceObjects();

		pendingFontChange = false;
	}

	void Menu::Init()
	{
		IMGUI_CHECKVERSION();

		const auto a_manager = RE::BSGraphics::Renderer::GetSingleton();
		const auto a_context = reinterpret_cast<ID3D11DeviceContext*>(a_manager->GetRuntimeData().context);
		const auto a_swapchain = reinterpret_cast<IDXGISwapChain*>(a_manager->GetRuntimeData().renderWindows->swapChain);
		const auto a_device = reinterpret_cast<ID3D11Device*>(a_manager->GetRuntimeData().forwarder);
		
		DXGI_SWAP_CHAIN_DESC desc;
		a_swapchain->GetDesc(&desc);

		this->screenSize = { static_cast<float>(desc.BufferDesc.Width), static_cast<float>(desc.BufferDesc.Height) };
		
		ImGui::CreateContext();
		auto& io = ImGui::GetIO();
		io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;
		io.DisplaySize = this->screenSize;

		ImGui_ImplWin32_Init(desc.OutputWindow);
		ImGui_ImplDX11_Init(a_device, a_context);

		this->device = a_device;
		this->context = a_context;
		this->swapchain = a_swapchain;

		PrettyLog::Debug("ImGui initialized with swap chain display size: {}x{}", desc.BufferDesc.Width, desc.BufferDesc.Height);
	}
}