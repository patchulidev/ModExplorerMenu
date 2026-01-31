#include "UIMenuImpl.h"
#include "ui/core/UIManager.h"
#include "core/InputManager.h"
#include "config/UserConfig.h"

// Credit toward cyfewlp of SimpleIME for introducing this alternative to the DX present draw method.
// https://github.com/cyfewlp/

namespace RE
{
	class GFxCharEvent : public RE::GFxEvent
	{
	public:
		GFxCharEvent() = default;

		explicit GFxCharEvent(UINT32 a_wcharCode, UINT8 a_keyboardIndex = 0)
			: GFxEvent(EventType::kCharEvent), wcharCode(a_wcharCode), keyboardIndex(a_keyboardIndex)
		{
		}

		// @members
		std::uint32_t wcharCode{};     // 04
		std::uint32_t keyboardIndex{}; // 08
	};

	static_assert(sizeof(GFxCharEvent) == 0x0C);
}

struct
{
	RE::GFxKey::Code gfxCode;
	ImGuiKey         imGuiKey; 
} GFxCodeToImGuiKeyTable[] = {
	{RE::GFxKey::kAlt,          ImGuiMod_Alt        },
	{RE::GFxKey::kControl,      ImGuiMod_Ctrl       },
	{RE::GFxKey::kShift,        ImGuiMod_Shift      },
	{RE::GFxKey::kCapsLock,     ImGuiKey_CapsLock      },
	// {RE::GFxKey::kTab,          ImGuiKey_Tab           }, // Don't sent tab key: bug when use tab close menu
	{RE::GFxKey::kHome,         ImGuiKey_Home          },
	{RE::GFxKey::kEnd,          ImGuiKey_End           },
	{RE::GFxKey::kPageUp,       ImGuiKey_PageUp        },
	{RE::GFxKey::kPageDown,     ImGuiKey_PageDown      },
	{RE::GFxKey::kComma,        ImGuiKey_Comma         },
	{RE::GFxKey::kPeriod,       ImGuiKey_Period        },
	{RE::GFxKey::kSlash,        ImGuiKey_Slash         },
	{RE::GFxKey::kBackslash,    ImGuiKey_Backslash     },
	{RE::GFxKey::kQuote,        ImGuiKey_Apostrophe    },
	{RE::GFxKey::kBracketLeft,  ImGuiKey_LeftBracket   },
	{RE::GFxKey::kBracketRight, ImGuiKey_RightBracket  },
	{RE::GFxKey::kReturn,       ImGuiKey_Enter         },
	{RE::GFxKey::kEqual,        ImGuiKey_Equal         },
	{RE::GFxKey::kMinus,        ImGuiKey_Minus         },
	{RE::GFxKey::kEscape,       ImGuiKey_Escape        },
	{RE::GFxKey::kLeft,         ImGuiKey_LeftArrow     },
	{RE::GFxKey::kUp,           ImGuiKey_UpArrow       },
	{RE::GFxKey::kRight,        ImGuiKey_RightArrow    },
	{RE::GFxKey::kDown,         ImGuiKey_DownArrow     },
	{RE::GFxKey::kSpace,        ImGuiKey_Space         },
	{RE::GFxKey::kBackspace,    ImGuiKey_Backspace     },
	{RE::GFxKey::kDelete,       ImGuiKey_Delete        },
	{RE::GFxKey::kInsert,       ImGuiKey_Insert        },
	{RE::GFxKey::kKP_Multiply,  ImGuiKey_KeypadMultiply},
	{RE::GFxKey::kKP_Add,       ImGuiKey_KeypadAdd     },
	{RE::GFxKey::kKP_Enter,     ImGuiKey_KeypadEnter   },
	{RE::GFxKey::kKP_Subtract,  ImGuiKey_KeypadSubtract},
	{RE::GFxKey::kKP_Decimal,   ImGuiKey_KeypadDecimal },
	{RE::GFxKey::kKP_Divide,    ImGuiKey_KeypadDivide  },
	{RE::GFxKey::kVoidSymbol,   ImGuiKey_None          }
};

// static void FlushInputState(); contains modified sources from:
// 		License: GPL-3.0
// 		Source: https://github.com/doodlum/skyrim-community-shaders/commit/8323dc521160c1d61ca9b9a872b7fb0e42078408
//
// 		License: MIT
// 		Source: https://github.com/BingusEx/AltTabFix/blob/master/src/Hooks/AltTabFix.hpp

namespace Modex
{
	void ModexGUIMenu::FlushInputState()
	{
		if (ImGui::GetCurrentContext() == nullptr)
			return; // Issue #48

		if (const auto& inputMgr = RE::BSInputDeviceManager::GetSingleton()) {
			if (const auto& device = inputMgr->GetKeyboard()) {
				device->Reset();
				device->Process(0);
			}
		}

		if (const auto& eventQueue = RE::BSInputEventQueue::GetSingleton()) {
			eventQueue->ClearInputQueue();
		}

		InputManager::GetSingleton()->Flush();

		auto& io = ImGui::GetIO();
		io.ClearInputKeys();
		io.ClearEventsQueue();
	}

	static ImGuiMouseSource ImGui_ImplWin32_GetMouseSourceFromMessageExtraInfo()
	{
		LPARAM extra_info = ::GetMessageExtraInfo();
		if ((extra_info & 0xFFFFFF80) == 0xFF515700) return ImGuiMouseSource_Pen;
		if ((extra_info & 0xFFFFFF80) == 0xFF515780) return ImGuiMouseSource_TouchScreen;
		return ImGuiMouseSource_Mouse;
	}

	void ModexGUIMenu::RegisterMenu()
	{
		if (auto *ui = RE::UI::GetSingleton(); ui != nullptr)
		{
			ui->Register(MENU_NAME, Creator);
		}
	}

	void ModexGUIMenu::RegisterListener(std::function<void(uint32_t)> a_func)
	{
		m_listeners.push_back(a_func);
	}

	void ModexGUIMenu::PostDisplay()
	{
		UIManager::GetSingleton()->Render();
	}

	// FIX: This isn't very reliable. If menu state is edited outside of our input mapping
	// the user can get stuck without controls. Return to this when implementing SkyrimSouls
	// compatability.

	void ModexGUIMenu::EnablePlayerControls()
	{
		// since we can't swallow input events in input manager.
		RE::PlayerControls::GetSingleton()->readyWeaponHandler->SetInputEventHandlingEnabled(m_weaponHandlerState);
		RE::PlayerControls::GetSingleton()->attackBlockHandler->SetInputEventHandlingEnabled(m_attackBlockHandlerState);
	}

	void ModexGUIMenu::DisablePlayerControls()
	{   
		if (RE::UI::GetSingleton()->IsMenuOpen(RE::MainMenu::MENU_NAME)) {
			m_weaponHandlerState = true;
			m_attackBlockHandlerState = true;
		} else {
			m_weaponHandlerState = RE::PlayerControls::GetSingleton()->readyWeaponHandler->IsInputEventHandlingEnabled();
			m_attackBlockHandlerState = RE::PlayerControls::GetSingleton()->attackBlockHandler->IsInputEventHandlingEnabled();
		}

		RE::PlayerControls::GetSingleton()->readyWeaponHandler->SetInputEventHandlingEnabled(false);
		RE::PlayerControls::GetSingleton()->attackBlockHandler->SetInputEventHandlingEnabled(false);
	}


	void ModexGUIMenu::OnShow()
	{
		m_fShow = true;
		DisablePlayerControls();
		UIManager::GetSingleton()->OnShow();
	}

	void ModexGUIMenu::OnHide()
	{
		m_fShow = false;
		EnablePlayerControls();
		UIManager::GetSingleton()->OnClose();
	}

	RE::UI_MESSAGE_RESULTS ModexGUIMenu::ProcessMessage(RE::UIMessage &a_message)
	{
		switch (a_message.type.get())
		{
			case RE::UI_MESSAGE_TYPE::kUpdate:
				break;
			case RE::UI_MESSAGE_TYPE::kUserEvent: {
				if (const auto &data = reinterpret_cast<RE::BSUIMessageData *>(a_message.data); data != nullptr) {
					if (data->fixedStr == "Cancel") {
						UIManager::GetSingleton()->PopWindow();
						return RE::UI_MESSAGE_RESULTS::kHandled;
					}
				}

				break;
			}
			case RE::UI_MESSAGE_TYPE::kShow:
				OnShow();
				break;
			case RE::UI_MESSAGE_TYPE::kHide:
				OnHide();
				break;
			case RE::UI_MESSAGE_TYPE::kScaleformEvent:
			{
				auto *scaleformData = reinterpret_cast<RE::BSUIScaleformData *>(a_message.data);
				if (scaleformData != nullptr && scaleformData->scaleformEvent != nullptr)
				{
					ProcessScaleformEvent(scaleformData);
					return RE::UI_MESSAGE_RESULTS::kHandled;
				}
				
				break;
			}
			default:;
		}

		return IMenu::ProcessMessage(a_message);
	}

	auto ModexGUIMenu::Creator() -> IMenu *
	{
		using Flags = RE::UI_MENU_FLAGS;
		auto *menu  = new ModexGUIMenu();
		menu->menuFlags.set(Flags::kPausesGame);
		menu->menuFlags.set(Flags::kUpdateUsesCursor, Flags::kUsesCursor);
		menu->menuFlags.set(Flags::kCustomRendering);
		menu->menuFlags.set(Flags::kUsesMenuContext);
		menu->depthPriority = 11;

		menu->inputContext.set(Context::kConsole);
		return menu;
	}

	void ModexGUIMenu::ProcessScaleformEvent(const RE::BSUIScaleformData *data)
	{
		switch (const auto &fxEvent = data->scaleformEvent; fxEvent->type.get())
		{
			case RE::GFxEvent::EventType::kMouseDown:
				OnMouseEvent(fxEvent, true);
				break;
			case RE::GFxEvent::EventType::kMouseUp:
				OnMouseEvent(fxEvent, false);
				break;
			case RE::GFxEvent::EventType::kMouseWheel:
				OnMouseWheelEvent(fxEvent);
				break;
			case RE::GFxEvent::EventType::kKeyDown:
				OnKeyEvent(fxEvent, true);
				break;
			case RE::GFxEvent::EventType::kKeyUp:
				OnKeyEvent(fxEvent, false);
				break;
			case RE::GFxEvent::EventType::kCharEvent:
				OnCharEvent(fxEvent);
				break;
			default:
				break;
		}
	}

	void ModexGUIMenu::OnMouseEvent(RE::GFxEvent *event, const bool down)
	{
		const auto &mouseSource = ImGui_ImplWin32_GetMouseSourceFromMessageExtraInfo();
		const auto *mouseEvent  = reinterpret_cast<RE::GFxMouseEvent *>(event);
		auto       &io          = ImGui::GetIO();

		if (down) {
			// FlushInputState(); // FIXME
		}

		io.AddMouseSourceEvent(mouseSource);
		io.AddMouseButtonEvent(static_cast<int>(mouseEvent->button), down);

	}

	void ModexGUIMenu::OnMouseWheelEvent(RE::GFxEvent *event)
	{
		const auto *mouseEvent = reinterpret_cast<RE::GFxMouseEvent *>(event);
		UIManager::GetSingleton()->AddScrollEvent(0, mouseEvent->scrollDelta);
	}

	void ModexGUIMenu::OnKeyEvent(RE::GFxEvent *event, const bool down)
	{
		const auto keyEvent = reinterpret_cast<RE::GFxKeyEvent *>(event);
		const auto imguiKey = GFxKeyToImGuiKey(keyEvent->keyCode);
		const auto showMenu = UserConfig::GetShowMenuKeys();
		const auto scanCode = ImGui::ImGuiKeyToScanCode(imguiKey);
		const auto modifier = InputManager::GetSingleton()->IsBoundModifierDown();

		if (m_listeners.size() > 0) {
			for (const auto& func : m_listeners) {
				func(scanCode);
			}

			m_listeners.clear();
			return;
		}

		// override behaviors for specific keys
		if (imguiKey == ImGuiKey_PageUp && down) { // TODO: Pass KeyEvent, handle if key is bound
			UIManager::GetSingleton()->AddScrollEvent(0, 1.0f);
			return;
		}
		else if (imguiKey == ImGuiKey_PageDown && down) {
			UIManager::GetSingleton()->AddScrollEvent(0, -1.0f);
			return;
		}
		else if (scanCode == showMenu[0] && down) {
			if ((showMenu[1] == 0) || (showMenu[1] != 0 && modifier)) {
				if (!ImGui::GetIO().WantCaptureKeyboard) {
					UIManager::GetSingleton()->PopWindow();
					return;
				}
			}
		}

		ImGui::GetIO().AddKeyEvent(imguiKey, down);
	}

	void ModexGUIMenu::OnCharEvent(RE::GFxEvent *event)
	{
		const auto charEvent = reinterpret_cast<RE::GFxCharEvent *>(event);
		ImGui::GetIO().AddInputCharacter(charEvent->wcharCode);
	}

	auto ModexGUIMenu::GFxKeyToImGuiKey(const RE::GFxKey::Code keyCode) -> ImGuiKey
	{
		ImGuiKey imguiKey = ImGuiKey_None;

		if (keyCode >= RE::GFxKey::kA && keyCode <= RE::GFxKey::kZ)
		{
			imguiKey = static_cast<ImGuiKey>(keyCode - RE::GFxKey::kA + ImGuiKey_A);
		}
		else if (keyCode >= RE::GFxKey::kF1 && keyCode <= RE::GFxKey::kF15)
		{
			imguiKey = static_cast<ImGuiKey>(keyCode - RE::GFxKey::kF1 + ImGuiKey_F1);
		}
		else if (keyCode >= RE::GFxKey::kNum0 && keyCode <= RE::GFxKey::kNum9)
		{
			imguiKey = static_cast<ImGuiKey>(keyCode - RE::GFxKey::kNum0 + ImGuiKey_0);
		}
		else if (keyCode >= RE::GFxKey::kKP_0 && keyCode <= RE::GFxKey::kKP_9)
		{
			imguiKey = static_cast<ImGuiKey>(keyCode - RE::GFxKey::kKP_0 + ImGuiKey_Keypad0);
		}
		else
		{
			for (const auto &[gfxCode, imGuiKey] : GFxCodeToImGuiKeyTable)
			{
				if (keyCode == gfxCode)
				{
					imguiKey = imGuiKey;
					break;
				}
			}
		}

		return imguiKey;
	}
}
