#pragma once

// #include "include/D/DataTypes.h"

#include "data/BaseObject.h"
#include "config/ConfigManager.h"

namespace Modex
{
    using FilterPropertyList = std::vector<FilterProperty>;

    class SortSystem : public ConfigManager
    {
    private:
        ImGuiTableSortSpecs*        m_currentSpecs;
        bool                        m_ascending;

        FilterProperty              m_currentSortFilter;
        FilterPropertyList          m_availableSortFilters;

    public:
        SortSystem(const std::filesystem::path& a_path) :
            m_currentSpecs(nullptr),
            m_ascending(true),
            m_currentSortFilter(PropertyType::kNone)
        {
            ConfigManager::m_file_path = a_path;
        }

        virtual bool Load(bool a_create) override;
        
        bool          SortFn(const std::unique_ptr<BaseObject>& a_lhs, const std::unique_ptr<BaseObject>& a_rhs) const;
        bool          SortFnKit(const std::unique_ptr<Kit>& a_lhs, const std::unique_ptr<Kit>& a_rhs) const;
        const std::string   GetSortProperty(const BaseObject& a_object) const;
        
        void SetSortSpecs(ImGuiTableSortSpecs* a_specs) {
            m_currentSpecs = a_specs;
        }

        void SetAscending(bool a_ascending) {
            m_ascending = a_ascending;
        }

        void ToggleAscending() {
            m_ascending = !m_ascending;
        }

        const FilterProperty& GetCurrentSortFilter() const {
            return m_currentSortFilter;
        }

        bool& GetSortAscending() {
            return m_ascending;
        }

        void SetCurrentSortFilter(FilterProperty a_filter) {
            m_currentSortFilter = std::move(a_filter);
        }

        void AddAvailableFilter(FilterProperty a_filter) {
            m_availableSortFilters.push_back(std::move(a_filter));
        }

        const FilterPropertyList& GetAvailableFilters() const {
            return m_availableSortFilters;
        }

        void ClearAvailableFilters() {
            m_availableSortFilters.clear();
        }

    private:
        const int CompareEditorID(const BaseObject* a_lhs, const BaseObject* a_rhs);
        const int ComparePluginName(const BaseObject* a_lhs, const BaseObject* a_rhs);
        const int CompareFormType(const BaseObject* a_lhs, const BaseObject* a_rhs);
        const int CompareFormID(const BaseObject* a_lhs, const BaseObject* a_rhs);
        const int CompareReferenceID(const BaseObject* a_lhs, const BaseObject* a_rhs);
        const int CompareName(const BaseObject* a_lhs, const BaseObject* a_rhs);

        const int CompareClass(const BaseObject* a_lhs, const BaseObject* a_rhs);
        const int CompareGender(const BaseObject* a_lhs, const BaseObject* a_rhs);
        const int CompareRace(const BaseObject* a_lhs, const BaseObject* a_rhs);
        const int CompareLevel(const BaseObject* a_lhs, const BaseObject* a_rhs);
        const int CompareHealth(const BaseObject* a_lhs, const BaseObject* a_rhs);
        const int CompareMagicka(const BaseObject* a_lhs, const BaseObject* a_rhs);
        const int CompareStamina(const BaseObject* a_lhs, const BaseObject* a_rhs);

        const int CompareGoldValue(const BaseObject* a_lhs, const BaseObject* a_rhs);
        const int CompareWeight(const BaseObject* a_lhs, const BaseObject* a_rhs);

        const int CompareWeaponDamage(const BaseObject* a_lhs, const BaseObject* a_rhs);
        const int CompareWeaponSpeed(const BaseObject* a_lhs, const BaseObject* a_rhs);
        const int CompareWeaponCriticalDamage(const BaseObject* a_lhs, const BaseObject* a_rhs);
        const int CompareWeaponDamagePerSecond(const BaseObject* a_lhs, const BaseObject* a_rhs);
        const int CompareWeaponType(const BaseObject* a_lhs, const BaseObject* a_rhs);

        const int CompareArmorRating(const BaseObject* a_lhs, const BaseObject* a_rhs);
        const int CompareArmorSlot(const BaseObject* a_lhs, const BaseObject* a_rhs);
        const int CompareArmorType(const BaseObject* a_lhs, const BaseObject* a_rhs);

        const int CompareCellName(const BaseObject* a_lhs, const BaseObject* a_rhs);
    };
}