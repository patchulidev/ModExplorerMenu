#pragma once

#include "ui/components/UIWindow.h"
#include "ui/components/UIModule.h"

namespace Modex
{
	class Menu : public UIWindow
	{
	public:

		Menu();

		Menu(const Menu&) = delete;
		Menu(Menu&&) = delete;
		Menu& operator=(const Menu&) = delete;
		Menu& operator=(Menu&&) = delete;

		void Draw();
		void OnOpen();
		void OnClose();

		void LoadModule(std::unique_ptr<UIModule>& a_module, uint8_t a_layoutIndex);
		void NextWindow();

		static constexpr ImGuiWindowFlags WINDOW_FLAGS =
			ImGuiWindowFlags_NoCollapse         |
			ImGuiWindowFlags_NoScrollbar        |
			ImGuiWindowFlags_NoScrollWithMouse  | 
			ImGuiWindowFlags_NoCollapse         |
			ImGuiWindowFlags_NoTitleBar         |
			ImGuiWindowFlags_NoResize           |
			ImGuiWindowFlags_NoBringToFrontOnFocus | 
			ImGuiWindowFlags_NoMove;

		static constexpr ImGuiWindowFlags BACKGROUND_FLAGS =
			ImGuiWindowFlags_NoTitleBar         |
			ImGuiWindowFlags_NoResize           |
			ImGuiWindowFlags_NoMove             |
			ImGuiWindowFlags_NoScrollbar        |
			ImGuiWindowFlags_NoInputs           |
			ImGuiWindowFlags_NoFocusOnAppearing |
			ImGuiWindowFlags_NoNav              |
			ImGuiWindowFlags_NoBringToFrontOnFocus;

	private:
		void DrawSidebar();
		void DrawModule();
		void DrawBackground(const ImVec2& a_displaySize);

		bool expand_sidebar;
		bool sidebar_initialized = false;

		float sidebar_w;
		float sidebar_h;
		float min_sidebar_w;
		float max_sidebar_w;

		float home_w       = 0.0f;
		float additem_w    = 0.0f;
		float equipment_w  = 0.0f;
		float npc_w        = 0.0f;
		float object_w     = 0.0f;
		float teleport_w   = 0.0f;
		float settings_w   = 0.0f;
		float exit_w       = 0.0f;

		std::vector<std::unique_ptr<UIModule>> m_modules;
	};
}
