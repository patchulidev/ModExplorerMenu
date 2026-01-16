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
        char           m_searchBuffer[256];
	char		m_lastSearchBuffer[256];

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
            m_searchBuffer{ 0 },
	    m_lastSearchBuffer { 0 },

            m_forceDropdown(false),
            m_topComparisonIdx(-1),
            m_topComparisonWeight(0),
            
            m_navSelection(PropertyType::kNone)
        {
            // ConfigManager::m_file_path = a_path.empty() ? std::filesystem::path() : a_path;
            m_file_path = a_path;
        }

        const SearchList& GetAvailableKeys() const {
            return m_availableSearchKeys;
        }

        char* GetSearchBuffer() {
		return m_searchBuffer;
        }

	char* GetLastSearchBuffer() {
		return m_lastSearchBuffer;
	}

        void ResetSearchBuffer() {
            m_searchBuffer[0] = '\0';
        }

        const SearchItem& GetSearchKey() const {
            return m_searchKey;
        }

        void SetSearchKey(SearchItem a_key) {
            m_searchKey = std::move(a_key);
        }

        std::string GetSearchKeyString(const SearchItem& a_key) const {
            return a_key.ToString();
        }

        void SetupDefaultKey()
        {
            const auto& keys = GetAvailableKeys();
            
            if (!keys.empty()) {
                SetSearchKey(keys[0]);
            }
        }

        virtual bool Load(bool a_create) override;

        bool CompareInputToObject(const BaseObject* a_object);
        
        bool InputTextComboBox(const char* a_label, char* a_buffer, std::string& a_preview, size_t a_size, 
                                std::vector<std::string> a_items, float a_width, bool a_showArrow = true);

    private:
        
    };
}
