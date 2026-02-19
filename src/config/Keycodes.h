
#pragma once

#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>

#define IM_VK_KEYPAD_ENTER (VK_RETURN + 256)

namespace KeyCode 
{
	static inline bool IsKeyModifier(uint32_t a_key)
	{
		return (a_key == 0x2A ||  // Left Shift key
				a_key == 0x36 ||  // Right Shift key
				a_key == 0x1D ||  // Left Control key
				a_key == 0x9D ||  // Right Control key
				a_key == 0x38 ||  // Left Alt key
				a_key == 0xB8     // Right Alt key
		);
	}

	static inline bool IsValidHotkey(uint32_t a_key)
	{
		return (a_key != 0x01 &&  // Escape key
				a_key != 0x0F &&  // Tab key
				a_key != 0x00 &&  // Invalid GFX key
				a_key != 0x1C &&  // Enter key
				a_key != 0x39 &&  // Space key
				a_key != 0x14     // T key (default)
		);
	}

	// Used to determine if a key is associated with text-input for conditional hotkey usage.
	static inline bool IsKeyboardTextShortcut(ImGuiKey a_key)
	{
		return (a_key == ImGuiKey_Backspace ||
				a_key == ImGuiKey_Delete ||
				a_key == ImGuiKey_Enter ||
				a_key == ImGuiKey_Space ||
				a_key == ImGuiKey_Tab ||
				a_key == ImGuiKey_End ||
				a_key == ImGuiKey_Home ||
				a_key == ImGuiKey_PageUp ||
				a_key == ImGuiKey_PageDown ||
				a_key == ImGuiKey_LeftArrow ||
				a_key == ImGuiKey_RightArrow ||
				a_key == ImGuiKey_UpArrow ||
				a_key == ImGuiKey_DownArrow);
	}

	// Convert ImGui native keycodes to Skyrim input scan codes.
	static inline uint32_t ImGuiKeyToScanCode(ImGuiKey a_key)
	{
		switch (a_key) {
		case ImGuiKey_Tab:
			return 0x0F;
		case ImGuiKey_LeftArrow:
			return 0xCB;
		case ImGuiKey_RightArrow:
			return 0xCD;
		case ImGuiKey_UpArrow:
			return 0xC8;
		case ImGuiKey_DownArrow:
			return 0xD0;
		case ImGuiKey_PageUp:
			return 0xC9;
		case ImGuiKey_PageDown:
			return 0xD1;
		case ImGuiKey_Home:
			return 0xC7;
		case ImGuiKey_End:
			return 0xCF;
		case ImGuiKey_Insert:
			return 0xD2;
		case ImGuiKey_Delete:
			return 0xD3;
		case ImGuiKey_Backspace:
			return 0x0E;
		case ImGuiKey_Space:
			return 0x39;
		case ImGuiKey_Enter:
			return 0x1C;
		case ImGuiKey_Escape:
			return 0x01;
		case ImGuiKey_Apostrophe:
			return 0x28;
		case ImGuiKey_Comma:
			return 0x33;
		case ImGuiKey_Minus:
			return 0x0C;
		case ImGuiKey_Period:
			return 0x34;
		case ImGuiKey_Slash:
			return 0x35;
		case ImGuiKey_Semicolon:
			return 0x27;
		case ImGuiKey_Equal:
			return 0x0D;
		case ImGuiKey_LeftBracket:
			return 0x1A;
		case ImGuiKey_Backslash:
			return 0x2B;
		case ImGuiKey_RightBracket:
			return 0x1B;
		case ImGuiKey_GraveAccent:
			return 0x29;
		case ImGuiKey_CapsLock:
			return 0x3A;
		case ImGuiKey_ScrollLock:
			return 0x46;
		case ImGuiKey_NumLock:
			return 0x45;
		case ImGuiKey_PrintScreen:
			return 0xB7;
		case ImGuiKey_Pause:
			return 0xC5;
		case ImGuiKey_Keypad0:
			return 0x52;
		case ImGuiKey_Keypad1:
			return 0x4F;
		case ImGuiKey_Keypad2:
			return 0x50;
		case ImGuiKey_Keypad3:
			return 0x51;
		case ImGuiKey_Keypad4:
			return 0x4B;
		case ImGuiKey_Keypad5:
			return 0x4C;
		case ImGuiKey_Keypad6:
			return 0x4D;
		case ImGuiKey_Keypad7:
			return 0x47;
		case ImGuiKey_Keypad8:
			return 0x48;
		case ImGuiKey_Keypad9:
			return 0x49;
		case ImGuiKey_KeypadDecimal:
			return 0x53;
		case ImGuiKey_KeypadDivide:
			return 0xB5;
		case ImGuiKey_KeypadMultiply:
			return 0x37;
		case ImGuiKey_KeypadSubtract:
			return 0x4A;
		case ImGuiKey_KeypadAdd:
			return 0x4E;
		case ImGuiKey_KeypadEnter:
			return 0x9C;
		case ImGuiMod_Shift:
		case ImGuiKey_RightShift:
		case ImGuiKey_LeftShift:
			return 0x2A;
		case ImGuiMod_Ctrl:
		case ImGuiKey_RightCtrl:
		case ImGuiKey_LeftCtrl:
			return 0x1D;
		case ImGuiMod_Alt:
		case ImGuiKey_RightAlt:
		case ImGuiKey_LeftAlt:
			return 0x38;
		case ImGuiMod_Super:
		case ImGuiKey_RightSuper:
		case ImGuiKey_LeftSuper:
			return 0x5B;
		case ImGuiKey_Menu:
			return 0x5D;
		case ImGuiKey_0:
			return 0x0B;
		case ImGuiKey_1:
			return 0x02;
		case ImGuiKey_2:
			return 0x03;
		case ImGuiKey_3:
			return 0x04;
		case ImGuiKey_4:
			return 0x05;
		case ImGuiKey_5:
			return 0x06;
		case ImGuiKey_6:
			return 0x07;
		case ImGuiKey_7:
			return 0x08;
		case ImGuiKey_8:
			return 0x09;
		case ImGuiKey_9:
			return 0x0A;
		case ImGuiKey_A:
			return 0x1E;
		case ImGuiKey_B:
			return 0x30;
		case ImGuiKey_C:
			return 0x2E;
		case ImGuiKey_D:
			return 0x20;
		case ImGuiKey_E:
			return 0x12;
		case ImGuiKey_F:
			return 0x21;
		case ImGuiKey_G:
			return 0x22;
		case ImGuiKey_H:
			return 0x23;
		case ImGuiKey_I:
			return 0x17;
		case ImGuiKey_J:
			return 0x24;
		case ImGuiKey_K:
			return 0x25;
		case ImGuiKey_L:
			return 0x26;
		case ImGuiKey_M:
			return 0x32;
		case ImGuiKey_N:
			return 0x31;
		case ImGuiKey_O:
			return 0x18;
		case ImGuiKey_P:
			return 0x19;
		case ImGuiKey_Q:
			return 0x10;
		case ImGuiKey_R:
			return 0x13;
		case ImGuiKey_S:
			return 0x1F;
		case ImGuiKey_T:
			return 0x14;
		case ImGuiKey_U:
			return 0x16;
		case ImGuiKey_V:
			return 0x2F;
		case ImGuiKey_W:
			return 0x11;
		case ImGuiKey_X:
			return 0x2D;
		case ImGuiKey_Y:
			return 0x15;
		case ImGuiKey_Z:
			return 0x2C;
		case ImGuiKey_F1:
			return 0x3B;
		case ImGuiKey_F2:
			return 0x3C;
		case ImGuiKey_F3:
			return 0x3D;
		case ImGuiKey_F4:
			return 0x3E;
		case ImGuiKey_F5:
			return 0x3F;
		case ImGuiKey_F6:
			return 0x40;
		case ImGuiKey_F7:
			return 0x41;
		case ImGuiKey_F8:
			return 0x42;
		case ImGuiKey_F9:
			return 0x43;
		case ImGuiKey_F10:
			return 0x44;
		case ImGuiKey_F11:
			return 0x57;
		case ImGuiKey_F12:
			return 0x58;
		default:
			return 0;
		}
	}

	// Map of Skyrim scan codes to Key Names. Used for displaying key names.
	static const std::unordered_map<int, const char*> SkyrimKeymap = {
		{ 0x0, "UnknownKey" },
		{ 0x01, "Esc" },
		{ 0x02, "1" },
		{ 0x03, "2" },
		{ 0x04, "3" },
		{ 0x05, "4" },
		{ 0x06, "5" },
		{ 0x07, "6" },
		{ 0x08, "7" },
		{ 0x09, "8" },
		{ 0x0A, "9" },
		{ 0x0B, "0" },
		{ 0x0C, "Hyphen" },
		{ 0x0D, "Equal" },
		{ 0x0E, "Backspace" },
		{ 0x0F, "Tab" },
		{ 0x10, "Q" },
		{ 0x11, "W" },
		{ 0x12, "E" },
		{ 0x13, "R" },
		{ 0x14, "T" },
		{ 0x15, "Y" },
		{ 0x16, "U" },
		{ 0x17, "I" },
		{ 0x18, "O" },
		{ 0x19, "P" },
		{ 0x1A, "Bracketleft" },
		{ 0x1B, "Bracketright" },
		{ 0x1C, "Enter" },
		{ 0x1D, "L-Ctrl" },
		{ 0x1E, "A" },
		{ 0x1F, "S" },
		{ 0x20, "D" },
		{ 0x21, "F" },
		{ 0x22, "G" },
		{ 0x23, "H" },
		{ 0x24, "J" },
		{ 0x25, "K" },
		{ 0x26, "L" },
		{ 0x27, "Semicolon" },
		{ 0x28, "Quotesingle" },
		{ 0x29, "Tilde" },
		{ 0x2A, "L-Shift" },
		{ 0x2B, "Backslash" },
		{ 0x2C, "Z" },
		{ 0x2D, "X" },
		{ 0x2E, "C" },
		{ 0x2F, "V" },
		{ 0x30, "B" },
		{ 0x31, "N" },
		{ 0x32, "M" },
		{ 0x33, "Comma" },
		{ 0x34, "Period" },
		{ 0x35, "Slash" },
		{ 0x36, "R-Shift" },
		{ 0x37, "NumPadMult" },
		{ 0x38, "L-Alt" },
		{ 0x39, "Space" },
		{ 0x3A, "CapsLock" },
		{ 0x3B, "F1" },
		{ 0x3C, "F2" },
		{ 0x3D, "F3" },
		{ 0x3E, "F4" },
		{ 0x3F, "F5" },
		{ 0x40, "F6" },
		{ 0x41, "F7" },
		{ 0x42, "F8" },
		{ 0x43, "F9" },
		{ 0x44, "F10" },
		{ 0x45, "NumLock" },
		{ 0x46, "ScrollLock" },
		{ 0x47, "7" },
		{ 0x48, "8" },
		{ 0x49, "9" },
		{ 0x4A, "NumPadMinus" },
		{ 0x4B, "4" },
		{ 0x4C, "5" },
		{ 0x4D, "6" },
		{ 0x4E, "NumPadPlus" },
		{ 0x4F, "1" },
		{ 0x50, "2" },
		{ 0x51, "3" },
		{ 0x52, "0" },
		{ 0x53, "NumPadDec" },
		{ 0x57, "F11" },
		{ 0x58, "F12" },
		{ 0x9D, "R-Ctrl" },
		{ 0xB5, "NumPadDivide" },
		{ 0xB7, "PrintScreen" },
		{ 0xB8, "R-Alt" },
		{ 0xC5, "Pause" },
		{ 0xC7, "Home" },
		{ 0xC8, "Up" },
		{ 0xC9, "PgUp" },
		{ 0xCB, "Left" },
		{ 0xCD, "Right" },
		{ 0xCF, "End" },
		{ 0xD0, "Down" },
		{ 0xD1, "PgDn" },
		{ 0xD2, "Insert" },
		{ 0xD3, "Delete" }
	};
}  // namespace ImGui
