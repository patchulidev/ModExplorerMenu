#pragma once

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
		int                         m_reset;

		FilterProperty              m_currentSortFilter;
		FilterPropertyList          m_availableSortFilters;

	public:
		SortSystem(const std::filesystem::path& a_path) :
			m_currentSpecs(nullptr),
			m_ascending(true),
			m_reset(0),
			m_currentSortFilter(PropertyType::kNone)
		{
			ConfigManager::m_file_path = a_path;
		}

		virtual bool Load(bool a_create) override;
		
		bool SortFn(const std::unique_ptr<BaseObject>& a_lhs, const std::unique_ptr<BaseObject>& a_rhs) const;
		
		void SetSortSpecs(ImGuiTableSortSpecs* a_specs) {
			m_currentSpecs = a_specs;
		}

		void SetAscending(bool a_ascending) {
			m_ascending = a_ascending;
		}

		void ResetSort() {
			m_currentSpecs = nullptr;
			m_ascending = true;
			m_currentSortFilter = PropertyType::kNone;
		}

		int GetClicks() {
			return m_reset;
		}

		void ToggleAscending() {
			m_ascending = !m_ascending;
			m_reset++;

			if (m_reset > 2) {
				m_reset = 0;
				m_ascending = true;
				m_currentSortFilter = PropertyType::kNone;
			}
		}

		const FilterProperty& GetCurrentSortFilter() const {
			return m_currentSortFilter;
		}

		bool& GetSortAscending() {
			return m_ascending;
		}

		void SetCurrentSortFilter(FilterProperty a_filter) {
			if (m_currentSortFilter != a_filter) {
				m_reset = 0;
				m_ascending = true;
				m_currentSortFilter = std::move(a_filter);
			}
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
	};
}
