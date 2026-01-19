#include "InputManager.h"
#include "ui/core/UIManager.h"
#include "config/UserConfig.h"

namespace Modex
{
	// TODO: Determine whether this belongs here, or closer to Banner impl
	std::array<uint32_t, 2> InputManager::GetShowMenuKey() const
	{
		return { showMenuModifier, showMenuKey };
	}

	// TODO: Determine whether this belongs here, or closer to Banner impl
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

	// FIX: Remove showMenuKey member variable, reference config directly.
	void InputManager::UpdateSettings()
	{
		auto& config = UserConfig::Get();
		showMenuKey 	 = config.showMenuKey;
		showMenuModifier = config.showMenuModifier;
	}

	void InputManager::AddKeyEvent(KeyEvent& a_keyEvent)
	{
		WriteLocker locker(_inputLock);
		inputQueue.emplace_back(a_keyEvent);
	}

	// All other inputs are routed through the IMenu implementation.
	void InputManager::AddEventToQueue(RE::InputEvent** a_event)
	{
		if (a_event) {
			for (auto inputEvent = *a_event; inputEvent; inputEvent = inputEvent->next) {
				if (inputEvent->GetEventType() == RE::INPUT_EVENT_TYPE::kButton) {
					const auto buttonEvent = static_cast<const RE::ButtonEvent*>(inputEvent);
					KeyEvent keyEvent(buttonEvent);

					AddKeyEvent(keyEvent);
				}
			}
		}
	}
	
	bool InputManager::IsBoundModifierDown()
	{
		if (showMenuModifier == 0) 
			return true;
		
		if (showMenuModifier == 0x2A || showMenuModifier == 0x36) {
			return _shiftDown;
		} else if (showMenuModifier == 0x1D || showMenuModifier == 0x9D) {
			return _ctrlDown;
		} else if (showMenuModifier == 0x38 || showMenuModifier == 0xB8) {
			return _altDown;
		}

		return false;
	}

	void InputManager::Flush()
	{
		_altDown = false;
		_ctrlDown = false;
		_shiftDown = false;
	}

	// Called every frame from D3DPresent Hook. Acts as a listener to
	// open the menu when our showMenuKey + showMenuModifier is pressed.
	void InputManager::ProcessInputEvents()
	{
		if (ImGui::GetCurrentContext() == nullptr) 
			return;
		
		WriteLocker locker(_inputLock);

		auto* manager = UIManager::GetSingleton();
		for (auto& event : inputQueue) {
			if (event.device == RE::INPUT_DEVICE::kKeyboard) {
				_altDown = (event.keyCode == 0x38 || event.keyCode == 0xB8) ? event.IsPressed() : _altDown;
				_ctrlDown = (event.keyCode == 0x1D || event.keyCode == 0x9D) ? event.IsPressed() : _ctrlDown;
				_shiftDown = (event.keyCode == 0x2A || event.keyCode == 0x36) ? event.IsPressed() : _shiftDown;

				if (event.IsPressed()) {
					if (!manager->IsMenuOpen()) {
						if (event.keyCode == showMenuKey && IsBoundModifierDown() && event.IsDown()) {
							manager->Open();
							return;
						}
					}
				}
			}
		}
		
		inputQueue.clear();
	}
}
