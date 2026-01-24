#pragma once

#include "config/Keycodes.h"

// Source: ersh1 alandtse flayan
// License: GPL-3.0
// https://github.com/ersh1/OpenAnimationReplacer/blob/786b286ffc0f680d9cbe3e70f59c5cf2387d3017/src/UI/UIManager.cpp

namespace Modex
{
	class CharEvent : public RE::InputEvent
	{
	public:
		uint32_t keyCode;  // 18 (ascii code)
	};

	struct KeyEvent
	{
		KeyEvent(const RE::ButtonEvent* a_event) :
			keyCode(a_event->GetIDCode()),
			device(a_event->GetDevice()),
			eventType(a_event->GetEventType()),
			value(a_event->Value()),
			heldDownSecs(a_event->HeldDuration()) {}

		KeyEvent(const CharEvent* a_event) :
			keyCode(a_event->keyCode),
			device(a_event->GetDevice()),
			eventType(a_event->GetEventType()) {}

		[[nodiscard]] constexpr bool IsPressed() const noexcept { return value > 0.0F; }
		[[nodiscard]] constexpr bool IsRepeating() const noexcept { return heldDownSecs > 0.0F; }
		[[nodiscard]] constexpr bool IsDown() const noexcept { return IsPressed() && (heldDownSecs == 0.0F); }
		[[nodiscard]] constexpr bool IsHeld() const noexcept { return IsPressed() && IsRepeating(); }
		[[nodiscard]] constexpr bool IsUp() const noexcept { return (value == 0.0F) && IsRepeating(); }

		uint32_t 				keyCode;
		RE::INPUT_DEVICE 		device;
		RE::INPUT_EVENT_TYPE 	eventType;
		float 					value;
		float 					heldDownSecs;
	};

	class InputManager
	{
	private:
		bool					_shiftDown;
		bool					_ctrlDown;
		bool					_altDown;
		
		mutable SharedLock		_inputLock;
		std::vector<KeyEvent>	inputQueue;

	public:
		static inline InputManager* GetSingleton()
		{
			static InputManager singleton;
			return std::addressof(singleton);
		}

		InputManager() = default;
		~InputManager() = default;
		InputManager(const InputManager&) = delete;
		InputManager& operator=(const InputManager&) = delete;

		void						Flush();
		void						UpdateSettings();
		void						AddEventToQueue(RE::InputEvent** a_event);
		void						AddKeyEvent(KeyEvent& a_keyEvent);
		void						ProcessInputEvents();
		bool						IsBoundModifierDown();
	};
}
