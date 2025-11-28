#pragma once

#include "include/S/Settings.h"
#include "include/U/UIManager.h"

namespace Modex
{
	class Menu : public UIWindow
	{
	public:
		struct SideBarImage
		{
			ImTextureID texture;
			ImVec2 size;
		};

		enum ActiveWindow : uint8_t
		{
			Home = 0,
			AddItem,
			Object,
			NPC,
			Teleport,
			Settings,
			kTotal
		};

		Menu() = default;
		~Menu() = default;

		void 			Draw();
		void 			Load();

		void 			SetActiveWindow(uint8_t a_window)
		{
			activeWindow = static_cast<ActiveWindow>(a_window);
			ResetSelectable();
		}

		ActiveWindow 	GetActiveWindow() const
		{
			return activeWindow;
		}

		static const ImGuiWindowFlags	WINDOW_FLAGS =
			ImGuiWindowFlags_NoCollapse  			|
			ImGuiWindowFlags_NoScrollbar 			|
			ImGuiWindowFlags_NoScrollWithMouse		| 
			ImGuiWindowFlags_NoCollapse 			|
			ImGuiWindowFlags_NoTitleBar				|
			ImGuiWindowFlags_NoResize				|
			ImGuiWindowFlags_NoMove;

		static const ImGuiWindowFlags	SIDEBAR_FLAGS = 
			WINDOW_FLAGS;

		static const ImGuiTableFlags 	TABLE_FLAGS =
			ImGuiTableFlags_Reorderable 			| ImGuiTableFlags_Sortable |
			ImGuiTableFlags_Borders 				| ImGuiTableFlags_BordersOuterH |
			ImGuiTableFlags_Hideable 				| ImGuiTableFlags_BordersInnerH |
			ImGuiTableFlags_NoBordersInBody 		| ImGuiTableFlags_Resizable |
			ImGuiTableFlags_ScrollY 				| ImGuiTableFlags_SizingStretchProp;

		static const ImGuiTableFlags TELEPORT_FLAGS =
			ImGuiTableFlags_Reorderable 			| ImGuiTableFlags_RowBg |
			ImGuiTableFlags_Sortable 				| ImGuiTableFlags_Borders |
			ImGuiTableFlags_BordersOuterH 			| ImGuiTableFlags_Hideable |
			ImGuiTableFlags_BordersInnerH 			| ImGuiTableFlags_NoBordersInBody |
			ImGuiTableFlags_Resizable 				| ImGuiTableFlags_ScrollY;

		static const ImGuiInputTextFlags INPUT_FLAGS =
			ImGuiInputTextFlags_AutoSelectAll		| ImGuiInputTextFlags_EnterReturnsTrue;

		static const ImGuiTableFlags ACTIONBAR_FLAGS =
			ImGuiTableFlags_Borders 				| ImGuiTableFlags_RowBg |
			ImGuiTableFlags_ScrollY;

		static const ImGuiWindowFlags BACKGROUND_FLAGS =
			ImGuiWindowFlags_NoTitleBar 			| ImGuiWindowFlags_NoResize |
			ImGuiWindowFlags_NoMove 				| ImGuiWindowFlags_NoScrollbar |
			ImGuiWindowFlags_NoInputs 				| ImGuiWindowFlags_NoFocusOnAppearing |
			ImGuiWindowFlags_NoBringToFrontOnFocus 	| ImGuiWindowFlags_NoNav;
	private:

		bool			expand_sidebar;
		float			sidebar_w;
		float 			sidebar_h;

		float 			home_w 		= 0.0f;
		float			additem_w 	= 0.0f;
		float 			npc_w 		= 0.0f;
		float 			object_w 	= 0.0f;
		float 			teleport_w 	= 0.0f;
		float 			settings_w 	= 0.0f;
		float 			exit_w 		= 0.0f;

		bool 			b_Home 		= true;
		bool 			b_AddItem 	= false;
		bool 			b_Object 	= false;
		bool 			b_NPC 		= false;
		bool 			b_Settings 	= false;
		bool 			b_Teleport 	= false;

		ActiveWindow 	activeWindow = ActiveWindow::Home;

		void ResetSelectable()
		{
			b_Home 		= (activeWindow == Home);
			b_AddItem 	= (activeWindow == AddItem);
			b_Object 	= (activeWindow == Object);
			b_NPC 		= (activeWindow == NPC);
			b_Settings 	= (activeWindow == Settings);
			b_Teleport 	= (activeWindow == Teleport);
		}
	};
}