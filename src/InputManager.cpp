#include "include/I/InputManager.h"
#include "include/U/UIManager.h"

namespace Modex
{
	void InputManager::OnFocusChange(bool a_focus)
	{
		// Issue #48 - Fix focus kill on menu close for Simple IME compatability.
		if (ImGui::GetCurrentContext() == nullptr) {
			return;
		}

		auto& io = ImGui::GetIO();
		io.ClearInputKeys();
		io.ClearEventsQueue();
		io.AddFocusEvent(a_focus);

		shiftDown = false;
		ctrlDown = false;
		altDown = false;
		modifierDown = false;
	}

	void InputManager::UpdateSettings()
	{
		auto& config = Settings::GetSingleton()->GetConfig();
		showMenuKey = config.showMenuKey;
		showMenuModifier = config.showMenuModifier;
	}

	void InputManager::Init()
	{
		UpdateSettings();
	}

	void InputManager::AddEventToQueue(RE::InputEvent** a_event)
	{
		if (a_event) {
			for (auto inputEvent = *a_event; inputEvent; inputEvent = inputEvent->next) {
				WriteLocker locker(_inputLock);
				inputQueue.emplace_back(inputEvent);
			}
		}
	}

	bool InputManager::IsBoundModifierDown()
	{
		if (showMenuModifier == 0x2A || showMenuModifier == 0x36) {
			return shiftDown;
		} else if (showMenuModifier == 0x1D || showMenuModifier == 0x9D) {
			return ctrlDown;
		} else if (showMenuModifier == 0x38 || showMenuModifier == 0xB8) {
			return altDown;
		}

		return false;
	}

	void InputManager::ProcessInputEvents()
	{
		WriteLocker locker(_inputLock);

		if (inputQueue.empty()) {
			return;
		}

		if (ImGui::GetCurrentContext() == nullptr) {
			return;
		}

		auto& io = ImGui::GetIO();

		for (const auto& event : inputQueue) {
			switch (event->GetEventType()) {
			case RE::INPUT_EVENT_TYPE::kChar:
				if (Menu::IsEnabled()) {
					io.AddInputCharacter(static_cast<const RE::CharEvent*>(event)->keyCode);
				}
				break;
			case RE::INPUT_EVENT_TYPE::kButton:
			{
				const RE::ButtonEvent* buttonEvent = static_cast<const RE::ButtonEvent*>(event);
				const uint32_t scanCode = buttonEvent->GetIDCode();
				const ImGuiKey imGuiKey = ImGui::ScanCodeToImGuiKey(scanCode);

				// Left/Right agnostic modifier key handler for better experience.
				if (imGuiKey == ImGuiKey_LeftShift || imGuiKey == ImGuiKey_RightShift) {
					shiftDown = buttonEvent->IsPressed();
				} else if (imGuiKey == ImGuiKey_LeftCtrl || imGuiKey == ImGuiKey_RightCtrl) {
					ctrlDown = buttonEvent->IsPressed();
				} else if (imGuiKey == ImGuiKey_LeftAlt || imGuiKey == ImGuiKey_RightAlt) {
					altDown = buttonEvent->IsPressed();
				}

				modifierDown = shiftDown || ctrlDown || altDown;

				if (Menu::IsEnabled()) {
					io.AddKeyEvent(ImGuiKey_ModShift, shiftDown);
					io.AddKeyEvent(ImGuiKey_ModCtrl, ctrlDown);
					io.AddKeyEvent(ImGuiKey_ModAlt, altDown);
				}

				switch (buttonEvent->device.get()) {
					case RE::INPUT_DEVICE::kMouse:
						if (Menu::IsEnabled()) {
							if (scanCode > 7)  // Middle Scroll
								io.AddMouseWheelEvent(0, buttonEvent->Value() * (scanCode == 8 ? 1 : -1));
							else {
								if (scanCode > 5) {
									io.AddMouseButtonEvent(5, buttonEvent->IsPressed());
								} else {
									io.AddMouseButtonEvent(scanCode, buttonEvent->IsPressed());
								}
							}
						}
						break;
					case RE::INPUT_DEVICE::kKeyboard:
						if (!Menu::IsEnabled() && buttonEvent->IsDown()) {
							bool isMenuKey = showMenuModifier != 0 ? (showMenuKey == scanCode && IsBoundModifierDown()) : (showMenuKey == scanCode);
							if (isMenuKey) {
								Menu::GetSingleton()->Toggle();
								break;
							}
						}

						if (Menu::IsEnabled()) {
							if (scanCode == 0x01 && buttonEvent->IsDown()) { // esc
								Menu::GetSingleton()->Close();
								break;
							}

							io.AddKeyEvent(imGuiKey, buttonEvent->IsDown());
						}
						break;
					}
					break;
				}
			}
		}

		inputQueue.clear();
	}
}