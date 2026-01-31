#include "UIPopup.h"
#include "UIContainers.h"
#include "config/Keycodes.h"
#include "localization/Locale.h"
#include "config/ThemeConfig.h"
#include "ui/components/UICustom.h"

namespace Modex
{
	void UIPopupHotkey::Draw()
	{
		static float height;
		auto width = ImGui::GetMainViewport()->Size.x * 0.25f;
		const float center_x = ImGui::GetMainViewport()->Size.x * 0.5f;
		const float center_y = ImGui::GetMainViewport()->Size.y * 0.5f;
		const float pos_x = center_x - (width * 0.5f);
		const float pos_y = center_y - (height * 0.5f);

		DrawPopupBackground(m_alpha);

		ImGui::SetNextWindowSize(ImVec2(width, 0));
		ImGui::SetNextWindowPos(ImVec2(pos_x, pos_y));
		ImGui::SetNextWindowFocus();

		ImGui::PushStyleVar(ImGuiStyleVar_Alpha, m_alpha);
		if (ImGui::Begin("##Modex::HotkeyPopup", NULL, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoFocusOnAppearing)) {
			if (ImGui::IsWindowAppearing()) {
				ImGui::GetIO().ClearInputKeys();
			}

			if (UICustom::Popup_MenuHeader(m_pendingHotkeyTitle.c_str())) {
				DeclineHotkey();
			}

			ImGui::NewLine();
			ImGui::TextWrapped("%s", m_pendingHotkeyDesc.c_str());
			ImGui::NewLine();

			ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal);
			ImGui::PushStyleColor(ImGuiCol_Button, ThemeConfig::GetColor("BUTTON_DECLINE"));
			if (ImGui::Button(Translate("CLOSE"), ImVec2(ImGui::GetContentRegionAvail().x, 0.f))) {
				DeclineHotkey();
			}
			ImGui::PopStyleColor();

			height = ImGui::GetWindowSize().y;
		}

		ImGui::End();
		ImGui::PopStyleVar();
	}

	void UIPopupHotkey::PopupHotkey(const char* a_title, const char* a_desc, uint32_t* a_hotkey, uint32_t a_default, bool a_modifierOnly, std::function<void()> onConfirmHotkeyCallback)
	{
		m_pendingHotkeyTitle = a_title;
		m_pendingHotkeyDesc = a_desc;
		m_hotkeyModifierOnly = a_modifierOnly;
		m_hotkeyCurrent = a_hotkey;
		m_hotkeyDefault = a_default;
		m_onConfirmCallback = onConfirmHotkeyCallback;
		m_captureInput = true;

		ModexGUIMenu::RegisterListener([this](uint32_t a_key) { AcceptHotkey(a_key); });
	}

	void UIPopupHotkey::AcceptHotkey(uint32_t a_key)
	{
		bool success = false;
		if (ImGui::IsValidHotkey(a_key)) {
			if (m_hotkeyModifierOnly) {
				if (ImGui::IsKeyModifier(a_key)) {
					*m_hotkeyCurrent = a_key;
					success = true;
				}
			}

			if (!m_hotkeyModifierOnly) {
				if (!ImGui::IsKeyModifier(a_key)) {
					*m_hotkeyCurrent = a_key;
					success = true;
				}
			}
		}

		if (success == false && a_key == 0x14) {
			*m_hotkeyCurrent = m_hotkeyDefault;
			success = true;
		}

		if (success) {
			if (m_onConfirmCallback) {
				m_onConfirmCallback();
			}

			CloseWindow();
		} else {
			UIManager::GetSingleton()->ShowWarning(
				m_pendingHotkeyTitle,
				Translate("SETTINGS_MENU_KEYBIND_INVALID")
			);

			CloseWindow();
		}
	}

	void UIPopupHotkey::DeclineHotkey()
	{
		CloseWindow();
	}

	void UIPopupWarning::Draw()
	{
		static float height;
		auto width = ImGui::GetMainViewport()->Size.x * 0.25f;
		const float center_x = ImGui::GetMainViewport()->Size.x * 0.5f;
		const float center_y = ImGui::GetMainViewport()->Size.y * 0.5f;
		const float pos_x = center_x - (width * 0.5f);
		const float pos_y = center_y - (height * 0.5f);

		DrawPopupBackground(m_alpha);

		ImGui::SetNextWindowSize(ImVec2(width, 0));
		ImGui::SetNextWindowPos(ImVec2(pos_x, pos_y));
		ImGui::SetNextWindowFocus();

		ImGui::PushStyleVar(ImGuiStyleVar_Alpha, m_alpha);
		if (ImGui::Begin("##Modex::WarningPopup", NULL, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoFocusOnAppearing)) {
			if (ImGui::IsWindowAppearing()) {
				ImGui::GetIO().ClearInputKeys();
			}

			if (UICustom::Popup_MenuHeader(m_pendingWarningTitle.c_str())) {
				DeclineWarning();
			}

			if (ImGui::IsKeyPressed(ImGuiKey_LeftArrow) || ImGui::IsKeyPressed(ImGuiKey_RightArrow)) {
				if (!ImGui::GetIO().WantTextInput) {
					m_navAccept = !m_navAccept;
				}
			}
			
			ImGui::TextWrapped("%s", m_pendingWarningMessage.c_str());
			ImGui::NewLine();
			ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal);

			bool _confirm, _cancel;
			if (UICustom::Popup_ConfirmDeclineButtons(_confirm, _cancel, m_navAccept)) {
				if (_confirm) {
					AcceptWarning();
				} else if (_cancel) {
					DeclineWarning();
				}
			}

			if (ImGui::IsKeyPressed(ImGuiKey_Enter) && ImGui::GetIO().WantTextInput == false) {
				if (m_navAccept) {
					AcceptWarning();
				} else {
					DeclineWarning();
				}
			}

			height = ImGui::GetWindowSize().y;
		}

		ImGui::End();
		ImGui::PopStyleVar();
	}

	void UIPopupWarning::PopupWarning(const std::string& a_title, const std::string& a_message, std::function<void()> a_onConfirmCallback)
	{
		m_pendingWarningTitle = a_title;
		m_pendingWarningMessage = a_message;
		m_onConfirmCallback = a_onConfirmCallback;
		m_captureInput = true;
	}

	void UIPopupWarning::AcceptWarning()
	{
		if (m_onConfirmCallback) {
			m_onConfirmCallback();
		}

		CloseWindow();
	}

	void UIPopupWarning::DeclineWarning()
	{
		CloseWindow();
	}

	void UIPopupInputBox::Draw()
	{
		static float height;
		auto width = ImGui::GetMainViewport()->Size.x * 0.25f;
		const float center_x = ImGui::GetMainViewport()->Size.x * 0.5f;
		const float center_y = ImGui::GetMainViewport()->Size.y * 0.5f;
		const float pos_x = center_x - (width * 0.5f);
		const float pos_y = center_y - (height * 0.5f);

		DrawPopupBackground(m_alpha);

		ImGui::SetNextWindowSize(ImVec2(width, 0));
		ImGui::SetNextWindowPos(ImVec2(pos_x, pos_y));
		ImGui::SetNextWindowFocus();

		ImGui::PushStyleVar(ImGuiStyleVar_Alpha, m_alpha);
		if (ImGui::Begin("##Modex::InputBoxPopup", NULL, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar)) {
			if (ImGui::IsWindowAppearing()) {
				ImGui::GetIO().ClearInputKeys();
			}

			if (ImGui::IsKeyPressed(ImGuiKey_LeftArrow) || ImGui::IsKeyPressed(ImGuiKey_RightArrow)) {
				if (!ImGui::GetIO().WantTextInput) {
					m_navAccept = !m_navAccept;
				}
			}

			ImGui::SetNextItemWidth(500.0f);
			if (UICustom::Popup_MenuHeader(m_pendingInputTitle.c_str())) {
				DeclineInput();
			}

			ImGui::TextWrapped("%s",m_pendingInputMessage.c_str());
			ImGui::NewLine();

			ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
			if (ImGui::InputText("##Modex::InputBoxPopup::InputText", m_inputBuffer, IM_ARRAYSIZE(m_inputBuffer), ImGuiInputTextFlags_EnterReturnsTrue)) {
				AcceptInput();
			}

			if (ImGui::IsWindowAppearing()) {
				ImGui::SetKeyboardFocusHere(-1);
			}

			ImGui::NewLine();
			ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal, 2.0f);

			bool _confirm, _cancel;
			if (UICustom::Popup_ConfirmDeclineButtons(_confirm, _cancel, m_navAccept)) {
				if (_confirm) {
					AcceptInput();
				} else if (_cancel) {
					DeclineInput();
				}
			}

			if (ImGui::IsKeyPressed(ImGuiKey_Enter) && ImGui::GetIO().WantTextInput == false) {
				if (m_navAccept) {
					AcceptInput();
				} else {
					DeclineInput();
				}
			}

			height = ImGui::GetWindowSize().y;
		}
		ImGui::End();
		ImGui::PopStyleVar();
	}

	void UIPopupInputBox::PopupInputBox(const std::string& a_title, const std::string& a_message, std::string a_hint, std::function<void(const std::string&)> a_onConfirmCallback)
	{
		m_pendingInputTitle = a_title;
		m_pendingInputMessage = a_message;
		m_onConfirmCallback = a_onConfirmCallback;
		m_navAccept = true;
		m_captureInput = true;

		ImFormatString(m_inputBuffer, IM_ARRAYSIZE(m_inputBuffer), "%s", a_hint.c_str());
	}

	void UIPopupInputBox::AcceptInput()
	{
		if (m_onConfirmCallback) {
			m_onConfirmCallback(m_inputBuffer);
		}

		CloseWindow();
	}

	void UIPopupInputBox::DeclineInput()
	{
		CloseWindow();
	}

	void UIPopupInfo::Draw()
	{
		static float height;
		auto width = ImGui::GetMainViewport()->Size.x * 0.25f;
		const float center_x = ImGui::GetMainViewport()->Size.x * 0.5f;
		const float center_y = ImGui::GetMainViewport()->Size.y * 0.5f;
		const float pos_x = center_x - (width * 0.5f);
		const float pos_y = center_y - (height * 0.5f);

		DrawPopupBackground(m_alpha);

		ImGui::SetNextWindowSize(ImVec2(width, 0));
		ImGui::SetNextWindowPos(ImVec2(pos_x, pos_y));
		ImGui::SetNextWindowFocus();

		ImGui::PushStyleVar(ImGuiStyleVar_Alpha, m_alpha);
		if (ImGui::Begin("##Modex::InfoPopup", NULL, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoFocusOnAppearing)) {
			if (ImGui::IsWindowAppearing()) {
				ImGui::GetIO().ClearInputKeys();
			}

			if (UICustom::Popup_MenuHeader(m_pendingInfoTitle.c_str())) {
				CloseInfo();
			}
			
		ImGui::NewLine();
			ImGui::TextWrapped("%s", m_pendingInfoMessage.c_str());
			ImGui::NewLine();
			ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal);

		ImGui::PushStyleColor(ImGuiCol_Button, ThemeConfig::GetColor("BUTTON_CONFIRM"));
		if (ImGui::Button(Translate("CONFIRM"), ImVec2(ImGui::GetContentRegionAvail().x, 0.f))) {
			CloseInfo();
		}
		ImGui::PopStyleColor();

			if (ImGui::IsKeyPressed(ImGuiKey_Enter) && ImGui::GetIO().WantTextInput == false) {
		CloseInfo();
			}

			height = ImGui::GetWindowSize().y;
		}

		ImGui::End();
		ImGui::PopStyleVar();
	}

	void UIPopupInfo::PopupInfo(const std::string& a_title, const std::string& a_message)
	{
		m_pendingInfoTitle = a_title;
		m_pendingInfoMessage = a_message;
		m_captureInput = true;
	}

	void UIPopupInfo::CloseInfo()
	{
		CloseWindow();
	}
}
