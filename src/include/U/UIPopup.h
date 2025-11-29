#pragma once

#include "include/U/UIManager.h"

namespace Modex
{    
    class UIPopupHotkey : public UIWindow
    {
    public:
        void Draw();

        void PopupHotkey(uint32_t* a_hotkey, const uint32_t& a_default, bool a_modifierOnly, std::function<void()> onConfirmHotkeyCallback);
        void AcceptHotkey(bool a_default = false);
        void DeclineHotkey();

        uint32_t* GetCurrentHotkey() const { return m_hotkeyCurrent; }
        bool IsModifierOnly() const { return m_modifierOnly; }
    private:
        void RenderHotkeyPopup();

        uint32_t*                   m_hotkeyCurrent = nullptr;
        uint32_t                    m_hotkeyDefault = 0;
        bool                        m_modifierOnly = false;

        std::function<void()>       m_onConfirmCallback;
    };


    class UIPopupWarning : public UIWindow
    {
    public:
        void Draw();

        void PopupWarning(const std::string& a_message, std::function<void()> a_onConfirmCallback = nullptr);
        void AcceptWarning();
        void DeclineWarning();
        bool* GetNavAccept() { return &m_navAccept; }

    private:
        std::string                 m_pendingWarningMessage;
        std::function<void()>       m_onConfirmCallback;
        bool                        m_navAccept = true;
    };
}