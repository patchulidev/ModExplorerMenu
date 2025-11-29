#include "include/U/UIPopup.h"

namespace Modex
{
    void UIPopupHotkey::Draw()
    {
        if (m_close && m_alpha <= 0.0f) {
            manager->GetSingleton()->CloseWindow(this);
            return;
        }

        auto width = ImGui::GetMainViewport()->Size.x * 0.25f;
        auto height = ImGui::GetMainViewport()->Size.y * 0.20f;
        const float center_x = ImGui::GetMainViewport()->Size.x * 0.5f;
        const float center_y = ImGui::GetMainViewport()->Size.y * 0.5f;
        const float pos_x = center_x - (width * 0.5f);
        const float pos_y = center_y - (height * 0.5f);

        ImGui::SetNextWindowSize(ImVec2(width, height));
        ImGui::SetNextWindowPos(ImVec2(pos_x, pos_y));

        if (m_alpha < 1.0f && !m_close) {
            m_alpha += ImGui::GetIO().DeltaTime / FADE_IN;
            if (m_alpha > 1.0f) {
                m_alpha = 1.0f;
            }
        }

        if (m_alpha > 0.0f && m_close) {
            m_alpha -= ImGui::GetIO().DeltaTime / FADE_OUT;
            if (m_alpha < 0.0f) {
                m_alpha = 0.0f;
            }
        }

        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, m_alpha);
        if (ImGui::Begin("Modex::Hotkey", NULL, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar)) {
            if (ImGui::IsWindowAppearing()) {
                ImGui::GetIO().ClearInputKeys();
            }

            ImGui::SubCategoryHeader("Set a Hotkey", ImVec4(0.20f, 0.20f, 0.20f, 1.0f));

            ImGui::NewLine();

            ImGui::SetCursorPosX(ImGui::GetCenterTextPosX(Translate("CONFIG_HOTKEY_SET")));
            ImGui::Text(Translate("CONFIG_HOTKEY_SET"));
            ImGui::NewLine();

            ImGui::SetCursorPosX(ImGui::GetCenterTextPosX(Translate("CONFIG_HOTKEY_RESET")));
            ImGui::Text(Translate("CONFIG_HOTKEY_RESET"));
            ImGui::NewLine();

            ImGui::SetCursorPosX(ImGui::GetCenterTextPosX(Translate("CONFIG_KEY_CANCEL")));
            ImGui::Text(Translate("CONFIG_KEY_CANCEL"));

            ImGui::SetCursorPosY(ImGui::GetWindowSize().y - ImGui::GetFrameHeightWithSpacing() - ImGui::GetStyle().WindowPadding.y);
            if (ImGui::GradientButton(Translate("Close"), ImVec2(ImGui::GetContentRegionAvail().x, ImGui::GetFrameHeightWithSpacing()))) {
                DeclineHotkey();
                m_close = true;
            }
        }

        ImGui::End();
        ImGui::PopStyleVar();
    }

    void UIPopupHotkey::PopupHotkey(uint32_t* a_hotkey, const uint32_t& a_default, bool a_modifierOnly, std::function<void()> onConfirmHotkeyCallback)
    {
        m_hotkeyCurrent = a_hotkey;
        m_hotkeyDefault = a_default;
        m_modifierOnly = a_modifierOnly;
        m_onConfirmCallback = onConfirmHotkeyCallback;
        m_close = false;
        m_alpha = 0.0f;
    }

    void UIPopupHotkey::AcceptHotkey(bool a_default)
    {
        if (a_default) {
            *m_hotkeyCurrent = m_hotkeyDefault;
        }

        if (m_onConfirmCallback) {
            m_onConfirmCallback();
        }

        m_close = true;
    }

    void UIPopupHotkey::DeclineHotkey()
    {
        m_hotkeyCurrent = 0;
        m_hotkeyDefault = 0;
        m_onConfirmCallback = nullptr;

        m_close = true;
    }

    void UIPopupWarning::Draw()
    {
        if (m_close && m_alpha <= 0.0f) {
            manager->GetSingleton()->CloseWindow(this);
            return;
        }

        auto width = ImGui::GetMainViewport()->Size.x * 0.25f;
        auto height = ImGui::GetMainViewport()->Size.y * 0.20f;
        const float center_x = ImGui::GetMainViewport()->Size.x * 0.5f;
        const float center_y = ImGui::GetMainViewport()->Size.y * 0.5f;
        const float pos_x = center_x - (width * 0.5f);
        const float pos_y = center_y - (height * 0.5f);

        ImGui::SetNextWindowSize(ImVec2(width, height));
        ImGui::SetNextWindowPos(ImVec2(pos_x, pos_y));

        if (m_alpha < 1.0f && !m_close) {
            m_alpha += ImGui::GetIO().DeltaTime / FADE_IN;
            if (m_alpha > 1.0f) {
                m_alpha = 1.0f;
            }
        }

        if (m_alpha > 0.0f && m_close) {
            m_alpha -= ImGui::GetIO().DeltaTime / FADE_OUT;
            if (m_alpha < 0.0f) {
                m_alpha = 0.0f;
            }
        }

        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, m_alpha);
        if (ImGui::Begin("Modex::Warning", NULL, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar)) {
            if (ImGui::IsWindowAppearing()) {
                ImGui::GetIO().ClearInputKeys();
            }

            ImGui::SubCategoryHeader("! Warning !", ImVec4(0.35f, 0.20f, 0.20f, 1.0f));
            ImGui::TextWrapped(m_pendingWarningMessage.c_str());

            ImGui::SetCursorPosY(ImGui::GetWindowSize().y - (ImGui::GetFontSize() * 1.5f * 2) - 20.0f);  // subtract button size * 2 + separator
            ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal);
            const ImVec4 buttonColor = ImGui::GetStyleColorVec4(ImGuiCol_ButtonHovered);

            ImGui::PushStyleColor(ImGuiCol_Button, m_navAccept ? buttonColor : ImGui::GetStyleColorVec4(ImGuiCol_Button));
            if (ImGui::GradientButton("(Y)es", ImVec2(ImGui::GetContentRegionAvail().x, ImGui::GetFontSize() * 1.5f))) {
                AcceptWarning();
                m_close = true;
            }
            ImGui::PopStyleColor();

            ImGui::PushStyleColor(ImGuiCol_Button, m_navAccept ? ImGui::GetStyleColorVec4(ImGuiCol_Button) : buttonColor);
            if (ImGui::GradientButton("(N)o", ImVec2(ImGui::GetContentRegionAvail().x, ImGui::GetFontSize() * 1.5f))) {
                DeclineWarning();
                m_close = true;
            }
            ImGui::PopStyleColor();
            
        }
        ImGui::End();
        ImGui::PopStyleVar();
    }

    void UIPopupWarning::PopupWarning(const std::string& a_message, std::function<void()> a_onConfirmCallback)
    {
        m_pendingWarningMessage = a_message;
        m_onConfirmCallback = a_onConfirmCallback;
        m_close = false;
        m_alpha = 0.0f;
    }

    void UIPopupWarning::AcceptWarning()
    {
        if (m_onConfirmCallback) {
            m_onConfirmCallback();
        }

        m_close = true;
    }

    void UIPopupWarning::DeclineWarning()
    {
        m_close = true;
    }       
}