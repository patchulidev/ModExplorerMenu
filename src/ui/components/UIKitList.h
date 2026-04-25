#pragma once

#include "data/BaseObject.h"

namespace Modex
{
	class UIKitList
	{
	public:
		enum class SelectionMode : uint8_t
		{
			Single,
			Multi
		};

		using SelectionChangedCallback = std::function<void(const std::vector<std::string>&)>;
		using KitActivatedCallback     = std::function<void(const std::string&)>;

		explicit UIKitList(const std::string& a_dataID, SelectionMode a_mode = SelectionMode::Single);
		~UIKitList() = default;
		UIKitList(const UIKitList&) = delete;
		UIKitList& operator=(const UIKitList&) = delete;

		void Draw(const ImVec2& a_size = ImVec2(0.0f, 0.0f));
		void Refresh();

		void SetSelectionMode(SelectionMode a_mode);
		SelectionMode GetSelectionMode() const { return m_mode; }

		void SetSelectedKey(const std::string& a_key);
		void SetSelectedKeys(const std::vector<std::string>& a_keys);
		void ClearSelection();
		const std::vector<std::string>& GetSelectedKeys() const { return m_selected; }
		bool IsSelected(const std::string& a_key) const;

		void SetSelectionChangedCallback(SelectionChangedCallback a_cb) { m_onSelectionChanged = std::move(a_cb); }
		void SetKitActivatedCallback(KitActivatedCallback a_cb) { m_onKitActivated = std::move(a_cb); }

		// Render a per-row trash button. Defaults to off so the selector use case
		// stays read-only.
		void SetShowDeleteAction(bool a_show) { m_showDeleteAction = a_show; }
		bool GetShowDeleteAction() const { return m_showDeleteAction; }

	private:
		struct Row
		{
			std::string key;
			std::string name;
			std::string collection;
			int weaponCount = 0;
			int armorCount  = 0;
			int totalCount  = 0;
			int totalValue  = 0;
		};

		void BuildRows();
		void ApplyFilter();
		void SortVisible(int a_columnUserID, ImGuiSortDirection a_dir);
		void DrawSearchBar(float a_width);
		void DrawTable(const ImVec2& a_size);
		void DrawEmptyState(const ImVec2& a_size, const char* a_message);
		void DrawFooter(float a_width);
		void HandleRowClick(const Row& a_row);
		void EmitSelectionChanged();

		std::string              m_data_id;
		SelectionMode            m_mode;
		std::vector<Row>         m_rows;
		std::vector<const Row*>  m_visible;
		std::vector<std::string> m_selected;

		char                     m_searchBuffer[MAX_PATH]{};
		int                      m_sortColumn    = 0;
		ImGuiSortDirection       m_sortDirection = ImGuiSortDirection_Ascending;

		SelectionChangedCallback m_onSelectionChanged;
		KitActivatedCallback     m_onKitActivated;

		bool                     m_showDeleteAction = false;
	};
}
