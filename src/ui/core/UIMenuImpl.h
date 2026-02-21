#pragma once

namespace Modex
{
	class ModexGUIMenu : public RE::IMenu
	{
	private:
		bool m_fShow = false;
		bool m_weaponHandlerState = true;
		bool m_attackBlockHandlerState = true;

	public:
		static constexpr std::string_view MENU_NAME = "ModexGUIMenu";
		static inline std::vector<std::function<void(uint32_t)>> m_listeners = {};

		static void RegisterListener(std::function<void(uint32_t)>);
		static void FlushInputState();
		static void RegisterMenu();
		void PostDisplay() override;

		void OnShow();
		void OnHide();
		auto ProcessMessage(RE::UIMessage &a_message) -> RE::UI_MESSAGE_RESULTS override;

		constexpr auto IsShowing() const -> bool
		{
			return m_fShow;
		}

	private:
		static auto Creator() -> IMenu *;

		static void ProcessScaleformEvent(const RE::BSUIScaleformData *data);
		static void OnMouseEvent(RE::GFxEvent *event, bool down);
		static void OnMouseWheelEvent(RE::GFxEvent *event);
		static void OnKeyEvent(RE::GFxEvent *event, bool down);
		static void OnCharEvent(RE::GFxEvent *event);
		static void ForceCursor();

		void DisablePlayerControls();
		void EnablePlayerControls();

		static auto GFxKeyToImGuiKey(RE::GFxKey::Code keyCode) -> ImGuiKey;
	};
}
