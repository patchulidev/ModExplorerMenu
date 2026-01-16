#include "core/Hooks.h"

#include "core/InputManager.h"
#include "ui/core/UIManager.h"
#include "ui/core/UIMenuImpl.h"

// Addresses for hooks, in large part, derived from Open Animation Replacer by Ersh
// https://github.com/ersh1/OpenAnimationReplacer/blob/786b286ffc0f680d9cbe3e70f59c5cf2387d3017/src/Hooks.h
// License: GNU General Public License v3.0 (WITH) Modding Exception

namespace Hooks
{
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

	struct PollInputDevices_Hook
	{
		static void thunk(RE::BSTEventSource<RE::InputEvent*>* a_dispatcher, RE::InputEvent** a_events)
		{
			if (const auto& inputManager = Modex::InputManager::GetSingleton(); inputManager != nullptr) {
				inputManager->AddEventToQueue(a_events);
			}

			func(a_dispatcher, a_events);
		}

		static inline REL::Relocation<decltype(thunk)> func;
	};
	static inline REL::Relocation<uintptr_t> _input{ REL::VariantID(67315, 68617, 0xC519E0) };

	struct D3D11Create_Hook
	{
		static void thunk() {
			func();
			
			if (const auto& manager = Modex::UIManager::GetSingleton(); manager != nullptr) {
				manager->DoD3DInit();
			}
		}

		static inline REL::Relocation<decltype(thunk)> func;
	};
	static inline REL::Relocation<uintptr_t> _create{ REL::VariantID(75595, 77226, 0xDC5530) };

	struct WndProcHandler_Hook
	{
		// Only WM_ACTIVATE && WM_ACTIVATEAPP are fired for Windowed
		static LRESULT thunk(HWND a_hwnd, UINT a_msg, WPARAM a_wParam, LPARAM a_lParam)
		{
			switch (a_msg)
			{
				case WM_ACTIVATE: {
					WORD activationType = LOWORD(a_wParam);
					if (activationType != WA_INACTIVE) {
						Modex::ModexGUIMenu::FlushInputState();
					}
					break;
				}
				case WM_SETFOCUS: { // ?
					Modex::ModexGUIMenu::FlushInputState();
					break;
				}
				default: 
					break;
			}

			return func(a_hwnd, a_msg, a_wParam, a_lParam);
		}

		static inline REL::Relocation<decltype(thunk)> func;
	};
	
	struct RegisterClassA_Hook
	{
		static ATOM thunk(WNDCLASSA* a_wndClass)
		{
			WndProcHandler_Hook::func = reinterpret_cast<uintptr_t>(a_wndClass->lpfnWndProc);
			a_wndClass->lpfnWndProc = &WndProcHandler_Hook::thunk;
			
			return func(a_wndClass);
		}
		
		static inline REL::Relocation<decltype(thunk)> func;
	};
	static inline REL::Relocation<uintptr_t> _register{ REL::VariantID(75591, 77226, 0xDC4B90) };

	struct DXGIPresent_Hook
	{
		static void thunk(std::uint32_t a_p1)
		{
			func(a_p1);

			if (const auto& inputManager = Modex::InputManager::GetSingleton(); inputManager != nullptr) {
				inputManager->ProcessInputEvents();
			}

			if (const auto& manager = Modex::UIManager::GetSingleton(); manager != nullptr) {
				if (manager->IsQueued()) {
					manager->QueueOpen();
					manager->Open();
				}
			}
		}

		static inline REL::Relocation<decltype(thunk)> func;
	};
	static inline REL::Relocation<uintptr_t> _present{ REL::VariantID(75461, 77246, 0xDBBDD0) };
	
	void Install()
	{
		auto& trampoline = SKSE::GetTrampoline();

		SKSE::AllocTrampoline(14);
		PollInputDevices_Hook::func = trampoline.write_call<5>(_input.address() + REL::VariantOffset(0x7B, 0x7B, 0x81).offset(), PollInputDevices_Hook::thunk);
		Modex::PrettyLog::Trace("Hooked PollInputDevices at {:#x}", _input.address() + REL::VariantOffset(0x7B, 0x7B, 0x81).offset());

		SKSE::AllocTrampoline(8); // 14?
		RegisterClassA_Hook::func = *(uintptr_t*)trampoline.write_call<6>(_register.address() + REL::VariantOffset(0x8E, 0x15C, 0x99).offset(), RegisterClassA_Hook::thunk);
		Modex::PrettyLog::Trace("Hooked RegisterClassA at {:#x}", _register.address() + REL::VariantOffset(0x8E, 0x15C, 0x99).offset());
		SKSE::AllocTrampoline(14);
		D3D11Create_Hook::func = trampoline.write_call<5>(_create.address() + REL::VariantOffset(0x9, 0x275, 0x9).offset(), D3D11Create_Hook::thunk);
		Modex::PrettyLog::Trace("Hooked D3D11 Create at {:#x}", _create.address() + REL::VariantOffset(0x9, 0x275, 0x9).offset());

		SKSE::AllocTrampoline(14);
		DXGIPresent_Hook::func = trampoline.write_call<5>(_present.address() + REL::VariantOffset(0x9, 0x9, 0x15).offset(), DXGIPresent_Hook::thunk);
		Modex::PrettyLog::Trace("Hooked DXGI Present at {:#x}", _present.address() + REL::VariantOffset(0x9, 0x9, 0x15).offset());
	}
}