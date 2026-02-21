#pragma once

#include "UIWindow.h"
#include "data/Data.h"

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

	struct ReferenceLookupCache
	{
		std::vector<const BaseObject*> objects;
		ImGuiID sortColumn = 0;
		ImGuiSortDirection sortDirection = ImGuiSortDirection_None;
	};

	class UIPopupReferenceLookup : public UIWindow
	{
	public:
		void Draw();
		void PopupReferenceLookup(const std::string& a_title, const std::string& a_message, std::function<void(RE::FormID)> a_onSelectCallback = nullptr);
	
	private:
		void AcceptEntry();
		void DeclineEntry();

		void FilterList(const std::string& a_filter);
		void SortTable();
		void DrawTable();

		static inline ReferenceLookupCache s_cache;

		bool									m_navAccept = true;
		std::string								m_pendingFuzzyListTitle;
		std::string								m_pendingFuzzyDesc;
		RE::FormID								m_currentSelection;
		std::function<void(RE::FormID)>  		m_onSelectCallback;
	};

	// BUG: PgUp and PgDn don't wory for hotkeys. Need to explicitly warn about this, or fix it.

	class UIPopupHotkey : public UIWindow
	{
	public:
		void Draw();
		void PopupHotkey(const char* a_title, const char* a_desc, uint32_t* a_hotkey, uint32_t a_default, bool a_modifierOnly, std::function<void()> onConfirmHotkeyCallback);

	private:
		void AcceptHotkey(uint32_t a_key);
		void DeclineHotkey();

		bool                        m_hotkeyModifierOnly;
		std::string                 m_pendingHotkeyTitle;
		std::string                 m_pendingHotkeyDesc;
		uint32_t*                   m_hotkeyCurrent;
		uint32_t                    m_hotkeyDefault;

		std::function<void()>       m_onConfirmCallback;
	};

	class UIPopupInputBox : public UIWindow
	{
	public:
		void Draw();
		void PopupInputBox(const std::string& a_title, const std::string& a_message, std::string a_hint, std::function<void(const std::string&)> a_onConfirmCallback = nullptr);

	private:
		void AcceptInput();
		void DeclineInput();

		char                        m_inputBuffer[MAX_PATH] = { 0 };
		std::string                 m_pendingInputTitle;
		std::string                 m_pendingInputMessage;
		std::string                 m_pendingInputHint;
		bool                        m_navAccept;

		std::function<void(const std::string&)> m_onConfirmCallback;
	};


	class UIPopupWarning : public UIWindow
	{
	public:
		void Draw();
		void PopupWarning(const std::string& a_title, const std::string& a_message, std::function<void()> a_onConfirmCallback = nullptr);

	private:
		void AcceptWarning();
		void DeclineWarning();

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

	private:
		void CloseInfo();

		std::string                 m_pendingInfoTitle;
		std::string                 m_pendingInfoMessage;
	};

	class UIPopupBrowser : public UIWindow
	{
	public:
		void Draw();
		void PopupBrowser(const std::string& a_title, const std::vector<std::string>& a_list, std::function<void(const std::string&)> a_onSelectCallback = nullptr);

	private:
		void AcceptSelection();
		void DeclineSelection();
		void NavigateUp();
		void NavigateDown();
		void PageUp();
		void PageDown();

		std::string                                 m_pendingBrowserTitle;
		std::vector<std::string>                    m_pendingBrowserList;
		std::vector<std::string>                    m_filteredBrowserList;
		std::vector<std::string>                    m_tags;
		std::function<void(const std::string&)>     m_onSelectCallback;
		std::string                                 m_currentSelection;
	};
}
