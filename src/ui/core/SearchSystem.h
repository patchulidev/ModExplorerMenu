#pragma once

#include "data/BaseObject.h"
#include "config/ConfigManager.h"

namespace Modex
{
	struct SearchItem : public FilterProperty
	{
		std::string name;
		size_t      weight;
		int         idx;
		bool        show;

		SearchItem(PropertyType a_type) :
			FilterProperty(a_type),

			name(""),
			weight(0),
			idx(-1),
			show(false)
		{}

		SearchItem(FilterProperty a_property) :
			FilterProperty(a_property),

			name(""),
			weight(0),
			idx(-1),
			show(false)
		{}
	};

	using SearchList = std::vector<SearchItem>;

	class SearchSystem : public ConfigManager
	{
	private:
		SearchItem     m_searchKey;
		ImGuiKey       m_lastNavKey;
		char           m_searchBuffer[MAX_PATH];
		// char           m_lastSearchBuffer[MAX_PATH];

		bool           m_forceDropdown;
		int            m_topComparisonIdx;
		size_t         m_topComparisonWeight;

		SearchList     m_availableSearchKeys;
		SearchList     m_filteredList;
		SearchList     m_navList;
		SearchItem     m_navSelection;
		
	public:
		SearchSystem(const std::filesystem::path& a_path) :
			m_searchKey(PropertyType::kNone),
			m_lastNavKey(ImGuiKey_None),
			m_searchBuffer(),
			// m_lastSearchBuffer(),

			m_forceDropdown(false),
			m_topComparisonIdx(-1),
			m_topComparisonWeight(0),
			
			m_navSelection(PropertyType::kNone)
		{
			m_file_path = a_path;
		}
		
		// overrides
		virtual bool Load(bool a_create) override;
		nlohmann::json SerializeState() const override;
		void DeserializeState(const nlohmann::json& a_state) override;

		//members
		const SearchList& GetAvailableKeys() const {
			return m_availableSearchKeys;
		}

		const std::vector<std::string> GetAvailableKeysVector() const {
			std::vector<std::string> keys;

			for (const auto& key : m_availableSearchKeys) {
				keys.push_back(key.ToString());
			}

			return keys;
		}

		char* GetSearchBuffer() {
			return m_searchBuffer;
		}

		std::string GetSearchBufferString() const {
			return std::string(m_searchBuffer);
		}

		const SearchItem& GetSearchKey() const {
			return m_searchKey;
		}

		int GetSearchKeyIndex() const {
			for (size_t i = 0; i < m_availableSearchKeys.size(); i++) {
				if (m_availableSearchKeys[i] == m_searchKey) {
					return static_cast<int>(i);
				}
			}

			return -1;
		}

		void SetSearchKey(SearchItem a_key) {
			m_searchKey = std::move(a_key);
		}

		void SetSearchKeyByIndex(int a_index) {
			if (a_index >= 0 && a_index < static_cast<int>(m_availableSearchKeys.size())) {
				SetSearchKey(m_availableSearchKeys[a_index]);
			}
		}

		std::string GetSearchKeyString(const SearchItem& a_key) const {
			return a_key.ToString();
		}

		std::string GetCurrentKeyString() const {
			return m_searchKey.ToString();
		}

		void SetupDefaultKey()
		{
			const auto& keys = GetAvailableKeys();
			
			if (!keys.empty()) {
				SetSearchKey(keys[0]);
			}
		}

		bool CompareInputToObject(const BaseObject* a_object);
		bool InputTextComboBox(const char* a_label, char* a_buffer, std::string& a_preview, size_t a_size, std::vector<std::string> a_items, float a_width);

	private:
		std::string ExtractDisplayName(const std::string& a_fullName);
		void FilterItems(const std::vector<std::string>& a_items, const char* a_buffer);
		bool RenderPopupItems(const char* a_buffer, const std::string& a_preview, bool a_popupIsAppearing, int& a_cursorIdx);
		bool HandleKeyboardNavigation(char* a_buffer, size_t a_size, ImGuiID a_inputTextID, int& a_cursorIdx, bool& a_unlockScroll);
    };
}
