#pragma once

#include "include/K/Keycode.h"
#include "include/S/Settings.h"
#include "include/U/Util.h"
#include "include/P/Persistent.h"
#include <PCH.h>

namespace Modex
{
	class UIWindow
	{
	public:
		virtual void Draw() = 0;
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

		void ShowWarning(const std::string& message, std::function<void()> onConfirmCallback = nullptr);
		void ShowHotkey(uint32_t* a_hotkey, const uint32_t& a_default, bool a_modifierOnly, std::function<void()> onConfirmHotkeyCallback = nullptr);
		bool IsWarningPending() const;
		bool IsHotkeyPending() const;
		void AcceptWarning();
		void DeclineWarning();
		void AcceptHotkey();
		void DeclineHotkey();
		bool InputHandler(ImGuiKey a_key);
		void RebuildFontAtlas();

		void RefreshFont() { m_pendingFontChange = true; }
		bool IsEnabled() const { return m_isEnabled; }
		ID3D11Device* GetDevice() const { return device; }

	private:
		UIManager() = default;
		~UIManager() = default;

		bool nav_accept = true;

		void 		_warning();
		void 		_hotkey();

		bool 									m_isEnabled;
		bool									m_pendingFontChange;
		bool 									m_prevFreezeState;
		bool									m_showSettingWindow;

		std::string 							m_pendingWarningMessage;
		std::function<void()> 					m_onConfirmCallback;

		std::vector<std::unique_ptr<UIWindow>> 	m_windowStack;
		uint8_t									m_lastActiveWindow;

		bool 									m_modifierOnly;
		uint32_t 								m_hotkeyDefault;
		uint32_t* 								m_hotkeyCurrent = nullptr;
		std::function<void()> 					m_onConfirmHotkeyCallback;

		ImVec2 									m_screenSize;
		ID3D11Device* 							device;
		// ID3D11DeviceContext* 					context;
		// IDXGISwapChain* 						swapchain;
		
	};
}