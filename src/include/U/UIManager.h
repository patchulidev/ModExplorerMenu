#pragma once

#include "include/K/Keycode.h"
#include "include/S/Settings.h"
#include "include/U/Util.h"
#include "include/P/Persistent.h"
// #include "include/U/UIPopup.h"
#include <PCH.h>

namespace Modex
{
	class UIManager;
	class UIWindow
	{
	public:
		virtual ~UIWindow() = default;
		virtual void Draw() = 0;
		virtual void Close() { m_close = true; }

		inline static constexpr float FADE_IN = 0.3f;
		inline static constexpr float FADE_OUT = 0.3f;

		UIManager* manager = nullptr;

	protected:
		float m_alpha = 0.0f;
		bool  m_close = false;

	};

	class UIManager
	{
	public:
		static inline UIManager* GetSingleton()
		{
			static UIManager singleton;
			return std::addressof(singleton);
		}

		void Render();
		void Init();
		void Open();
		void Close();
		void Toggle();
		bool AllowMenuOpen();
		void ShowBanner();

		bool IsMenuOpen() const;
		void CloseWindow(UIWindow* a_window);
		void SetActiveModule(uint8_t a_module) { m_lastActiveWindow = a_module; }
		void ShowWarning(const std::string& message, std::function<void()> onConfirmCallback = nullptr);
		void ShowHotkey(uint32_t* a_hotkey, const uint32_t& a_default, bool a_modifierOnly, std::function<void()> onConfirmHotkeyCallback = nullptr);

		UIWindow* GetHotkeyPopup() const;
		UIWindow* GetWarningPopup() const;

		bool InputHandler(ImGuiKey a_key);
		void RebuildFontAtlas();

		void RefreshFont() { m_pendingFontChange = true; }
		bool IsEnabled() const { return m_isEnabled; }
		ID3D11Device* GetDevice() const { return device; }

	private:
		UIManager() = default;
		~UIManager() = default;

		// bool nav_accept = true;

		// void 		_warning();
		// void 		_hotkey();

		bool 									m_isEnabled;
		bool									m_pendingFontChange;
		bool 									m_prevFreezeState;
		bool									m_showSettingWindow;

		std::vector<std::unique_ptr<UIWindow>> 	m_windowStack;
		uint8_t									m_lastActiveWindow;

		ImVec2 									m_screenSize;
		ID3D11Device* 							device;
		
	};
}