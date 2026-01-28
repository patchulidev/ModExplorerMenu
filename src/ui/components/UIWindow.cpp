#include "UIWindow.h"
#include "ui/core/UIManager.h"

namespace Modex
{
	void UIWindow::OpenWindow(UIManager* a_manager)
	{
		manager = a_manager;
		m_state = WindowState::Opening;
		OnOpening();
	}

	void UIWindow::CloseWindow() 
	{
		m_state = WindowState::Closing;
		OnClosing();
	}
    
	void UIWindow::Update(float a_deltaTime) {
		Draw();

		if (ImGui::Shortcut(ImGuiKey_Escape, ImGuiInputFlags_RouteActive)) {
			manager->PopWindow();
		}
		
		switch (m_state) {
		case WindowState::Opening:
			m_alpha += a_deltaTime / FADE_IN;
			if (m_alpha >= 1.0f) {
				m_alpha = 1.0f;
				m_state = WindowState::Open;
				OnOpened();
			}
			break;
		case WindowState::Closing:
			m_alpha -= a_deltaTime / FADE_OUT;
			if (m_alpha <= 0.0f) {
				m_alpha = 0.0f;
				m_state = WindowState::Closed;
				m_close = true;
				OnClosed();
			}
			break;
		case WindowState::Closed:
			manager->DeleteWindow(this);
			break;
		default:
			break;
		}
	}
}
