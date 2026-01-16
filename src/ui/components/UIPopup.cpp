// #include "include/U/UIPopup.h"
// #include "include/C/UIContainers.h"
// #include "include/S/Settings.h"

#include "UIPopup.h"
#include "UIContainers.h"
#include "imgui.h"
#include "localization/Locale.h"
#include "config/ThemeConfig.h"


namespace Modex
{    
    ////////////////////////////////////////////////////////////////////////////////
    // UIPopupHotkey
    ////////////////////////////////////////////////////////////////////////////////

    void UIPopupHotkey::Draw()
    {
        
        auto width = ImGui::GetMainViewport()->Size.x * 0.25f;
        auto height = ImGui::GetMainViewport()->Size.y * 0.20f;
        const float center_x = ImGui::GetMainViewport()->Size.x * 0.5f;
        const float center_y = ImGui::GetMainViewport()->Size.y * 0.5f;
        const float pos_x = center_x - (width * 0.5f);
        const float pos_y = center_y - (height * 0.5f);

        DrawPopupBackground(m_alpha);

        ImGui::SetNextWindowSize(ImVec2(width, height));
        ImGui::SetNextWindowPos(ImVec2(pos_x, pos_y));
        ImGui::SetNextWindowFocus();

        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, m_alpha);
        if (ImGui::Begin("##Modex::HotkeyPopup", NULL, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoFocusOnAppearing)) {
            if (ImGui::IsWindowAppearing()) {
                ImGui::GetIO().ClearInputKeys();
            }

            ImGui::SubCategoryHeader(Translate("HEADER_HOTKEY"), ImVec4(0.20f, 0.20f, 0.20f, 1.0f));

            ImGui::NewLine();

            ImGui::SetCursorPosX(ImGui::GetCenterTextPosX(Translate("CONFIG_HOTKEY_SET")));
            ImGui::Text("%s", Translate("CONFIG_HOTKEY_SET"));
            ImGui::NewLine();

            ImGui::SetCursorPosX(ImGui::GetCenterTextPosX(Translate("CONFIG_HOTKEY_RESET")));
            ImGui::Text("%s",Translate("CONFIG_HOTKEY_RESET"));
            ImGui::NewLine();

            ImGui::SetCursorPosX(ImGui::GetCenterTextPosX(Translate("CONFIG_KEY_CANCEL")));
            ImGui::Text("%s",Translate("CONFIG_KEY_CANCEL"));

            ImGui::SetCursorPosY(ImGui::GetWindowSize().y - ImGui::GetFrameHeightWithSpacing() - ImGui::GetStyle().WindowPadding.y);
            if (ImGui::GradientButton(Translate("Close"), ImVec2(ImGui::GetContentRegionAvail().x, ImGui::GetFrameHeightWithSpacing()))) {
                DeclineHotkey();
                m_close = true;
            }
        }

        ImGui::End();
        ImGui::PopStyleVar();
    }

    void UIPopupHotkey::PopupHotkey(uint32_t* a_hotkey, uint32_t a_default, std::function<void()> onConfirmHotkeyCallback)
    {
        m_hotkeyCurrent = a_hotkey;
        m_hotkeyDefault = a_default;
        m_onConfirmCallback = onConfirmHotkeyCallback;
        m_captureInput = true;
    }

    void UIPopupHotkey::AcceptHotkey(bool a_default)
    {
        if (a_default) {
            *m_hotkeyCurrent = m_hotkeyDefault;
        }

        if (m_onConfirmCallback) {
            m_onConfirmCallback();
        }

        this->CloseWindow();
    }

    void UIPopupHotkey::DeclineHotkey()
    {
        m_hotkeyCurrent = 0;
        m_hotkeyDefault = 0;
        m_onConfirmCallback = nullptr;

        this->CloseWindow();
    }

    ////////////////////////////////////////////////////////////////////////////////
    // UIPopupWarning
    ////////////////////////////////////////////////////////////////////////////////

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

            if (UICustom::AddPopupMenuHeader(m_pendingWarningTitle.c_str())) {
                ImGui::End();
                ImGui::PopStyleVar();
                DeclineWarning();
                return;
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
            if (UICustom::AddConfirmationButtons(_confirm, _cancel, m_navAccept)) {
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

        this->CloseWindow();
    }

    void UIPopupWarning::DeclineWarning()
    {
        this->CloseWindow();
    }

    ////////////////////////////////////////////////////////////////////////////////
    // UIPopupInput
    ////////////////////////////////////////////////////////////////////////////////

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
            if (UICustom::AddPopupMenuHeader(m_pendingInputTitle.c_str())) {
                ImGui::End();
                ImGui::PopStyleVar();
                DeclineInput();
                return;
            }

            ImGui::TextWrapped("%s",m_pendingInputMessage.c_str());
            ImGui::NewLine();

            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
            ImGui::InputText("##Modex::InputBoxPopup::InputText", m_inputBuffer, IM_ARRAYSIZE(m_inputBuffer));

            if (ImGui::IsWindowAppearing()) {
                ImGui::SetKeyboardFocusHere(-1);
            }

            ImGui::NewLine();
            ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal, 2.0f);

            bool _confirm, _cancel;
            if (UICustom::AddConfirmationButtons(_confirm, _cancel, m_navAccept)) {
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
        m_captureInput = true;

        ImFormatString(m_inputBuffer, IM_ARRAYSIZE(m_inputBuffer), "%s", a_hint.c_str());
    }

    void UIPopupInputBox::AcceptInput()
    {
        if (m_onConfirmCallback) {
            m_onConfirmCallback(m_inputBuffer);
        }

        this->CloseWindow();
    }

    void UIPopupInputBox::DeclineInput()
    {
        this->CloseWindow();
    }

    //////////////////////////////////////////////////////////////////////////////
    // UIPopupInfo
    //////////////////////////////////////////////////////////////////////////////

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

            if (UICustom::AddPopupMenuHeader(m_pendingInfoTitle.c_str())) {
                ImGui::End();
                ImGui::PopStyleVar();
                CloseInfo();
                return;
            }
            
	    ImGui::NewLine();
            ImGui::TextWrapped("%s", m_pendingInfoMessage.c_str());
            ImGui::NewLine();
            ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal);

	    ImGui::PushStyleColor(ImGuiCol_Button, ThemeConfig::GetColor("BUTTON_CONFIRM"));
	    if (ImGui::GradientButton(Translate("CONFIRM"), ImVec2(ImGui::GetContentRegionAvail().x, 0.f))) {
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
