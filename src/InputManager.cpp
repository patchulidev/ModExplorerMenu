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

		// Source: https://github.com/doodlum/skyrim-community-shaders/commit/8323dc521160c1d61ca9b9a872b7fb0e42078408 | License: GPL-3.0
		// Solves the alt+tab stuck issue, but disables tab after tabbing back in.
		if (const auto& inputMgr = RE::BSInputDeviceManager::GetSingleton()) {
			if (const auto& device = inputMgr->GetKeyboard()) {
				device->Reset();
			}
		}

		auto& io = ImGui::GetIO();
		io.ClearInputKeys();
		io.ClearEventsQueue();

		shiftDown = false;
		ctrlDown = false;
		altDown = false;
		modifierDown = false;
	}

	std::array<uint32_t, 2> InputManager::GetShowMenuKey() const
	{
		return { showMenuModifier, showMenuKey };
	}

	std::string InputManager::GetShowMenuKeyAsText() const
	{
		uint32_t scanCode = showMenuKey;
		uint32_t modifierScanCode = showMenuModifier;

		ImGuiKey imGuiKey = ImGui::ScanCodeToImGuiKey(scanCode);
		ImGuiKey modifierImGuiKey = ImGui::ScanCodeToImGuiKey(modifierScanCode);

		const char* keyName = ImGui::GetKeyName(imGuiKey);
		const char* modifierKeyName = ImGui::GetKeyName(modifierImGuiKey);

		if (modifierScanCode != 0) {
			return std::format("{} + {}", modifierKeyName, keyName);
		}
		
		if (scanCode != 0) {
			return std::string(keyName);
		}

		return "Unknown?";
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
						if (buttonEvent->IsDown()) {
							if (UIManager::GetSingleton()->InputHandler(imGuiKey)) {
								io.ClearInputKeys();
								break;
							}

							if (scanCode == 0x01) {
								if (!ImGui::GetIO().WantCaptureKeyboard) {
									Menu::GetSingleton()->Close();
								} else {
									ImGui::SetWindowFocus("##ModexMenu");
								}

								break;
							}

							bool isToggleKey = showMenuModifier != 0 ? (showMenuKey == scanCode && IsBoundModifierDown()) : (showMenuKey == scanCode);
							
							if (!Menu::IsEnabled() && isToggleKey) {
								Menu::GetSingleton()->Open();
								break;
							}

							if (Menu::IsEnabled() && isToggleKey) {
								if (!ImGui::GetIO().WantCaptureKeyboard) {
									Menu::GetSingleton()->Close();
									break;
								}
							}
						}

						// IsPressed() seems to perform better than IsDown()
						io.AddKeyEvent(imGuiKey, buttonEvent->IsPressed());
						
						break;
					}
					break;
				}
			}
		}

		inputQueue.clear();
	}
}