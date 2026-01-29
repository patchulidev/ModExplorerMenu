#pragma once

#include "ui/core/UIManager.h"
#include "ui/components/UICustom.h"
#include "ui/components/UITable.h"

namespace Modex
{
    namespace UIContainers
    {
        static inline char s_sharedInputSearchBuffer[256] = ""; // FIX: Go back to module ownership.

		void DrawKitSelectionPanel(const ImVec2 &a_pos, const ImVec2 &a_size);
		void DrawAddItemActionPanel(const ImVec2 &a_pos, const ImVec2 &a_size, std::unique_ptr<UITable> &a_view);
        void DrawItemSearchPanel(const ImVec2 &a_pos, const ImVec2 &a_size, std::unique_ptr<UITable> &a_view);
        void DrawKitActionsPanel(const ImVec2 &a_pos, const ImVec2 &a_size, std::unique_ptr<UITable> &a_kitView, std::unique_ptr<UITable> &a_mainView, Kit& a_kit);
        void DrawBasicTablePanel(const ImVec2 &a_pos, const ImVec2 &a_size, std::unique_ptr<UITable> &a_view);
        void DrawKitTablePanel(const ImVec2 &a_pos, const ImVec2 &a_size, std::unique_ptr<UITable> &a_view);
        void DrawInventoryTablePanel(const ImVec2 &a_pos, const ImVec2 &a_size, std::unique_ptr<UITable> &a_view);
        void DrawTableStatusPanel(const ImVec2 &a_pos, const ImVec2 &a_size, std::unique_ptr<UITable> &a_view);
        void DrawPopupBackground();

        bool TabButton(const char* a_label, const ImVec2& a_size, const bool a_condition, const ImVec4& a_color);

        bool QueryCheck(const bool a_condition);
    }
}
