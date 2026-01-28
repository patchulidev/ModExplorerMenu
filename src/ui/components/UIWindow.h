#pragma once

namespace Modex
{
	class UIManager;  // Forward declaration

	class UIWindow
	{
	public:
		virtual ~UIWindow() = default;
		virtual void Draw() = 0;
		virtual void OnOpening() {}
		virtual void OnClosing() {}
		virtual void OnOpened() {}
		virtual void OnClosed() {}

		virtual void OpenWindow(UIManager* a_manager);
		virtual void CloseWindow();
		virtual bool WantsInputCapture() const { return m_captureInput; }

		virtual void Update(float a_deltaTime);

		inline static constexpr float FADE_IN = 0.3f;
		inline static constexpr float FADE_OUT = 0.3f;

		UIManager* manager = nullptr;

		
	protected:
		float m_alpha = 0.0f;
		bool  m_close = false;
		bool  m_captureInput = false;

		enum class WindowState : uint8_t {
			Opening,
			Open,
			Closing,
			Closed
		};

		WindowState m_state = WindowState::Closed;
	};
}
