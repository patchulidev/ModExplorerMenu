#include "include/H/Hooks.h"
#include "include/I/InputManager.h"
#include "include/C/Console.h"
#include "include/U/UIManager.h"

// Addresses for hooks, in large part, derived from Open Animation Replacer by Ersh
// https://github.com/ersh1/OpenAnimationReplacer/blob/786b286ffc0f680d9cbe3e70f59c5cf2387d3017/src/Hooks.h
// License: GNU General Public License v3.0 (WITH) Modding Exception

namespace Hooks
{
	struct PollInputDevices_Hook
	{
		static void thunk(RE::BSTEventSource<RE::InputEvent*>* a_dispatcher, RE::InputEvent** a_events)
		{
			if (a_events) {
				Modex::InputManager::GetSingleton()->AddEventToQueue(a_events);
			}
			
			if (Modex::UIManager::GetSingleton()->IsEnabled()) {
				static RE::InputEvent* dummy[] = { nullptr };
				func(a_dispatcher, dummy);
			} else {
				func(a_dispatcher, a_events);
			}
		}

		static inline REL::Relocation<decltype(thunk)> func;
	};
	static inline REL::Relocation<uintptr_t> _input{ REL::VariantID(67315, 68617, 0xC519E0) };

	struct D3D11Create_Hook
	{
		static void thunk()
		{
			func();

			Modex::UIManager::GetSingleton()->Init();
		}

		static inline REL::Relocation<decltype(thunk)> func;
	};
	static inline REL::Relocation<uintptr_t> _create{ REL::VariantID(75595, 77226, 0xDC5530) };

	struct WndProcHandler_Hook
	{
		static LRESULT thunk(HWND a_hwnd, UINT a_msg, WPARAM a_wParam, LPARAM a_lParam)
		{
			switch (a_msg) {
			case WM_SETFOCUS:
				Modex::InputManager::GetSingleton()->OnFocusChange(true);
				break;
			case WM_KILLFOCUS:
				Modex::InputManager::GetSingleton()->OnFocusChange(false);
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

			Modex::Console::ProcessMainThreadTasks();
			Modex::InputManager::ProcessInput();
			Modex::UIManager::GetSingleton()->Render();
		}

		static inline REL::Relocation<decltype(thunk)> func;
	};
	static inline REL::Relocation<uintptr_t> _present{ REL::VariantID(75461, 77246, 0xDBBDD0) };
	
	void Install()
	{
		auto& trampoline = SKSE::GetTrampoline();

		SKSE::AllocTrampoline(14);
		PollInputDevices_Hook::func = trampoline.write_call<5>(_input.address() + REL::VariantOffset(0x7B, 0x7B, 0x81).offset(), PollInputDevices_Hook::thunk);
		PrettyLog::Info("Hooked PollInputDevices at {:#x}", _input.address() + REL::VariantOffset(0x7B, 0x7B, 0x81).offset());

		SKSE::AllocTrampoline(8); // 14?
		RegisterClassA_Hook::func = *(uintptr_t*)trampoline.write_call<6>(_register.address() + REL::VariantOffset(0x8E, 0x15C, 0x99).offset(), RegisterClassA_Hook::thunk);
		PrettyLog::Info("Hooked RegisterClassA at {:#x}", _register.address() + REL::VariantOffset(0x8E, 0x15C, 0x99).offset());

		SKSE::AllocTrampoline(14);
		D3D11Create_Hook::func = trampoline.write_call<5>(_create.address() + REL::VariantOffset(0x9, 0x275, 0x9).offset(), D3D11Create_Hook::thunk);
		PrettyLog::Info("Hooked D3D11 Create at {:#x}", _create.address() + REL::VariantOffset(0x9, 0x275, 0x9).offset());

		SKSE::AllocTrampoline(14);
		DXGIPresent_Hook::func = trampoline.write_call<5>(_present.address() + REL::VariantOffset(0x9, 0x9, 0x15).offset(), DXGIPresent_Hook::thunk);
		PrettyLog::Info("Hooked DXGI Present at {:#x}", _present.address() + REL::VariantOffset(0x9, 0x9, 0x15).offset());
	}
}