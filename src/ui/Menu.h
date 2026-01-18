#pragma once

// #include "include/S/Settings.h"
// #include "include/U/UIManager.h"

#include "ui/components/UIWindow.h"

namespace Modex
{
	class Menu : public UIWindow
	{
	private:
		enum ActiveWindow : uint8_t
		{
			Home = 0,
			AddItem,
			Equipment,
			Object,
			NPC,
			Teleport,
			Settings,
			kTotal
		};

		ActiveWindow 	activeWindow = ActiveWindow::Home;

	public:

		Menu() = default;
		void 			Draw();
		void 			OnOpen();
		void			OnClose();

		void			ReloadWindow(ActiveWindow a_window);
		void			NextWindow();

		static constexpr ImGuiWindowFlags WINDOW_FLAGS =
			ImGuiWindowFlags_NoCollapse			|
			ImGuiWindowFlags_NoScrollbar			|
			ImGuiWindowFlags_NoScrollWithMouse		| 
			ImGuiWindowFlags_NoCollapse			|
			ImGuiWindowFlags_NoTitleBar			|
			ImGuiWindowFlags_NoResize			|
			ImGuiWindowFlags_NoMove;

		static constexpr ImGuiWindowFlags BACKGROUND_FLAGS =
			ImGuiWindowFlags_NoTitleBar			|
			ImGuiWindowFlags_NoResize			|
			ImGuiWindowFlags_NoMove				|
			ImGuiWindowFlags_NoScrollbar			|
			ImGuiWindowFlags_NoInputs			|
			ImGuiWindowFlags_NoFocusOnAppearing		|
			ImGuiWindowFlags_NoBringToFrontOnFocus 		| 
			ImGuiWindowFlags_NoNav;
	private:

		bool			expand_sidebar;
		bool			sidebar_initialized = false;
		float			sidebar_w;
		float 			sidebar_h;
		float			min_sidebar_w;
		float			max_sidebar_w;

		float 			home_w 		= 0.0f;
		float			additem_w 	= 0.0f;
		float 			equipment_w 	= 0.0f;
		float 			npc_w 		= 0.0f;
		float 			object_w 	= 0.0f;
		float 			teleport_w 	= 0.0f;
		float 			settings_w 	= 0.0f;
		float 			exit_w 		= 0.0f;

	};
}
