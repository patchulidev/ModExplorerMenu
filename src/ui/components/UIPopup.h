#pragma once

#include "UIWindow.h"

namespace Modex
{    
    static inline void DrawPopupBackground(float a_alpha)
    {
        ImGui::SetNextWindowPos(ImGui::GetMainViewport()->Pos);
        ImGui::SetNextWindowSize(ImGui::GetMainViewport()->Size);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.0f, 0.0f, 0.0f, 0.55f * a_alpha));
        ImGui::Begin("##PopupBackground", nullptr,
            ImGuiWindowFlags_NoTitleBar |
            ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoScrollbar |
            ImGuiWindowFlags_NoSavedSettings);
        ImGui::End();
        ImGui::PopStyleVar(2);
        ImGui::PopStyleColor(2);
    }

    class UIPopupHotkey : public UIWindow
    {
    public:
        void Draw();

        void PopupHotkey(uint32_t* a_hotkey, uint32_t a_default, std::function<void()> onConfirmHotkeyCallback);
        void AcceptHotkey(bool a_default = false);
        void DeclineHotkey();

        uint32_t GetCurrentHotkey() const { return m_hotkeyCurrent ? *m_hotkeyCurrent : 0; }
        void SetCurrentHotkey(uint32_t a_hotkey) { if (m_hotkeyCurrent) *m_hotkeyCurrent = a_hotkey; }
    private:
        void RenderHotkeyPopup();

        uint32_t*                   m_hotkeyCurrent = nullptr;
        uint32_t                    m_hotkeyDefault = 0;

        std::function<void()>       m_onConfirmCallback;
    };

    class UIPopupInputBox : public UIWindow
    {
    public:
        void Draw();

        void PopupInputBox(const std::string& a_title, const std::string& a_message, std::string a_hint, std::function<void(const std::string&)> a_onConfirmCallback = nullptr);
        void AcceptInput();
        void DeclineInput();
    
    private:
        char                        m_inputBuffer[MAX_PATH] = { 0 };
        std::string                 m_pendingInputTitle;
        std::string                 m_pendingInputMessage;
        std::string                 m_pendingInputHint;
        bool                        m_navAccept = true;
        std::function<void(const std::string&)> m_onConfirmCallback;
    };


    class UIPopupWarning : public UIWindow
    {
    public:
        void Draw();

        void PopupWarning(const std::string& a_title, const std::string& a_message, std::function<void()> a_onConfirmCallback = nullptr);
        void AcceptWarning();
        void DeclineWarning();
        bool* GetNavAccept() { return &m_navAccept; }

    private:
        std::string                 m_pendingWarningTitle;
        std::string                 m_pendingWarningMessage;
        std::function<void()>       m_onConfirmCallback;
        bool                        m_navAccept = true;
    };

    class UIPopupInfo : public UIWindow
    {
    public:
        void Draw();

        void PopupInfo(const std::string& a_title, const std::string& a_message);
        void CloseInfo();
    private:
        std::string                 m_pendingInfoTitle;
        std::string                 m_pendingInfoMessage;
    };

    class UIPopupBrowser : public UIWindow
    {
    public:
        void Draw();

        void PopupBrowser(const std::string& a_title, const std::vector<std::string>& a_list, std::function<void(const std::string&)> a_onSelectCallback = nullptr);
        void AcceptSelection();
        void DeclineSelection();

        void NavigateUp();
        void NavigateDown();

        void PageUp();
        void PageDown();        
    private:
        std::string                                 m_pendingBrowserTitle;
        std::vector<std::string>                    m_pendingBrowserList;
        std::vector<std::string>                    m_filteredBrowserList;
        std::vector<std::string>                    m_tags;
        std::function<void(const std::string&)>     m_onSelectCallback;
        std::string                                 m_currentSelection;
    };
}