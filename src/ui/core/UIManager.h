#pragma once

#include "ui/Menu.h"
#include "config/Keycodes.h"
#include "ui/core/UIMenuImpl.h"
#include "ui/components/UIWindow.h"

namespace Modex
{

	class AddItemModule;
	class ActorModule;
	class EquipmentModule;
	class TeleportModule;
	class ObjectModule;
	class HomeModule;
	
	class UIManager
	{
	private:
		bool                          m_menuListener = false;
		bool                          m_wantTextInput = true;
		bool                          m_queueOpen = false;

		HWND                          m_hWnd = nullptr;
		std::atomic<bool>             m_initialized = false;
		std::unique_ptr<ModexGUIMenu> m_gui = nullptr;

		static inline WNDPROC RealWndProc;
		static auto MainWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) -> LRESULT;

		static void MouseHandler();
		static void ScrollHandler();
		static void ShortcutHandler();

		void TrySetAllowTextInput();
		static void AllowTextInput(const bool allow);
		static void AllowTextInput1(RE::ControlMap* controlMap, const bool allow);

	public:
		static inline UIManager* GetSingleton()
		{
			static UIManager singleton;
			return std::addressof(singleton);
		}

		void QueueOpen() noexcept 
		{ 
			m_queueOpen = !m_queueOpen;
			m_menuListener = false;
		}

		bool IsQueued() const { return m_queueOpen; }
		void SetMenuListener(bool a_enable) { m_menuListener = a_enable; }
		bool GetMenuListener() const { return m_menuListener; }
		
		bool DoInit(const RE::BSGraphics::RendererData &renderData, HWND hWnd);
		void DoD3DInit();
		
		void Open();
		void OnShow();
		void Close();
		void OnClose();
		void Shutdown();
		void Render();
		void PopWindow();
		void AddScrollEvent(float x, float y);

		bool IsMenuOpen() const;
		bool ShouldCaptureInput() const;
		void DeleteWindow(UIWindow* a_window);
		void SetActiveWindow(uint8_t a_module) { m_lastActiveWindow = a_module; }
		
		void ShowBanner();
		void ShowReferenceLookup(const std::string& a_title, const std::string& a_message, std::function<void(RE::FormID)> onSelectCallback = nullptr);
		void ShowWarning(const std::string& a_title, const std::string& a_message, bool a_showCondition = true, std::function<void()> onConfirmCallback = nullptr);
		void ShowHotkey(const char* a_title, const char* a_desc, uint32_t* a_hotkey, uint32_t& a_default, bool a_modifierOnly, std::function<void()> onConfirmHotkeyCallback = nullptr);
		void ShowInputBox(const std::string& a_title, const std::string& a_message, std::string a_hint = "", std::function<void(const std::string&)> onConfirmCallback = nullptr);
		void ShowBrowser(const std::string& a_title, const std::vector<std::string>& a_items, std::function<void(const std::string&)> onSelectCallback = nullptr);
		void ShowInfoBox(const std::string& a_title, const std::string& a_message);

		template<typename T>
		UIWindow* GetPopupWindowRef() const;

		int GetWindowCount() const { return static_cast<int>(m_windowStack.size()); }
		ImVec2 GetDisplaySize() const { return ImVec2(m_displayWidth, m_displayHeight); }

	private:
		UIManager();
		~UIManager() noexcept;

		UIManager(const UIManager&) = delete;
		UIManager(UIManager&&) = delete;
		UIManager& operator=(const UIManager&) = delete;
		UIManager& operator=(UIManager&&) = delete;

		Menu*                                   m_menu;
		std::unique_ptr<AddItemModule>          m_addItemWindow;
		std::unique_ptr<ActorModule>            m_npcWindow;
		std::unique_ptr<EquipmentModule>        m_equipmentWindow;
		std::unique_ptr<TeleportModule>         m_teleportWindow;
		std::unique_ptr<ObjectModule>           m_objectWindow;
		std::unique_ptr<HomeModule>             m_homeWindow;

		std::vector<std::unique_ptr<UIWindow>> 	m_windowStack;
		uint8_t                                 m_lastActiveWindow;
	
		ImVec2                                  m_scrollEnergy = ImVec2(0.0f, 0.0f);
		static constexpr float                  m_scrollMultiplier = 1.5f;
		static constexpr float                  m_scrollSmoothing = 10.0f;

		float                                   m_displayWidth;
		float                                   m_displayHeight;
		
	};
}
