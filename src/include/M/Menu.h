#pragma once
#include "include/S/Settings.h"

// clang-format off
namespace Modex
{
	class Menu
	{
	public:
		void 					Draw();
		void 					Open();
		void 					Close();
		void 					Toggle();
		bool					AllowMenuOpen();

		void 					RefreshStyle();
		void 					RefreshFont();

		void 					Init();
		void 					SyncUserStyleToImGui(Settings::Style user);

		// constructor destructor
		static inline Menu* GetSingleton()
		{
			static Menu singleton;
			return std::addressof(singleton);
		}

		static inline bool IsEnabled()
		{
			return Menu::GetSingleton()->isEnabled;
		}

		Menu() = default;
		~Menu() = default;
		Menu(const Menu&) = delete;
		Menu& operator=(const Menu&) = delete;

		bool 					isEnabled;
		bool					pendingFontChange;
		bool 					prevFreezeState;
		bool					showSettingWindow;
		
		ID3D11Device* 			GetDevice() const { return device; };
		ID3D11DeviceContext* 	GetContext() const { return context; };
		IDXGISwapChain* 		GetSwapChain() const { return swapchain; };
		
	private:
		ImVec2 					screenSize;
		ID3D11Device* 			device;
		ID3D11DeviceContext* 	context;
		IDXGISwapChain* 		swapchain;
		
		void 					RebuildFontAtlas();
	};

}
// clang-format on