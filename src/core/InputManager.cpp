#include "InputManager.h"
#include "ui/core/UIManager.h"
#include "config/UserConfig.h"

namespace Modex
{

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
		auto& config = UserConfig::Get();

		if (config.showMenuModifier == 0) 
			return true;
		
		if (config.showMenuModifier == 0x2A || config.showMenuModifier == 0x36) {
			return _shiftDown;
		} else if (config.showMenuModifier == 0x1D || config.showMenuModifier == 0x9D) {
			return _ctrlDown;
		} else if (config.showMenuModifier == 0x38 || config.showMenuModifier == 0xB8) {
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
		auto& config = UserConfig::Get();

		for (auto& event : inputQueue) {
			if (event.device == RE::INPUT_DEVICE::kKeyboard) {
				_altDown = (event.keyCode == 0x38 || event.keyCode == 0xB8) ? event.IsPressed() : _altDown;
				_ctrlDown = (event.keyCode == 0x1D || event.keyCode == 0x9D) ? event.IsPressed() : _ctrlDown;
				_shiftDown = (event.keyCode == 0x2A || event.keyCode == 0x36) ? event.IsPressed() : _shiftDown;

				if (event.IsPressed()) {
					if (!manager->IsMenuOpen()) {
						if (event.keyCode == config.showMenuKey && IsBoundModifierDown() && event.IsDown()) {
							if (!manager->IsMenuOpen()) {
								manager->Open();
							}

							return;
						}
					}
				}
			}
		}
		
		inputQueue.clear();
	}
}
