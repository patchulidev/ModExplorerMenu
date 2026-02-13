#pragma once

#include "data/BaseObject.h"
#include "config/ConfigManager.h"

namespace Modex
{
	using FilterPropertyList = std::vector<FilterProperty>;

	class SortSystem : public ConfigManager
	{
	private:
		bool                        m_ascending;
		bool                        m_usePrimary;
		std::function<void()>       m_sortSystemCallback;

		FilterProperty              m_primarySortFilter;
		FilterProperty              m_secondarySortFilter;
		FilterPropertyList          m_availableSortFilters;

	public:
		SortSystem(const std::filesystem::path& a_path)
			: m_ascending(true)
			, m_usePrimary(true)
			, m_primarySortFilter(PropertyType::kNone)
			, m_secondarySortFilter(PropertyType::kNone)
		{
			ConfigManager::m_file_path = a_path;
		}

		// overrides
		virtual bool Load(bool a_create) override;
		nlohmann::json SerializeState() const override;
		void DeserializeState(const nlohmann::json& a_state) override;
		
		// members
		bool SortFn(const std::unique_ptr<BaseObject>& a_lhs, const std::unique_ptr<BaseObject>& a_rhs) const;

		void SetAscending(bool a_ascending) {
			m_ascending = a_ascending;
		}

		void ResetSort() {
			m_ascending = true;
			m_secondarySortFilter = PropertyType::kNone;
		}

		void ToggleAscending() {
			m_ascending = !m_ascending;
		}

		const FilterProperty& GetPrimarySortFilter() const {
			return m_primarySortFilter;
		}

		const FilterProperty& GetSecondarySortFilter() const {
			return m_secondarySortFilter;
		}

		bool& GetSortAscending() {
			return m_ascending;
		}

		bool& GetUsePrimary() {
			return m_usePrimary;
		}

		void UsePrimary(bool a_use = true) {
			m_usePrimary = a_use;
		}

		void SetPrimarySortFilter(FilterProperty a_filter) {
			if (m_primarySortFilter != a_filter) {
				m_primarySortFilter = std::move(a_filter);
			}

			m_usePrimary = true;
		}

		void SetSecondarySortFilter(FilterProperty a_filter) {
			if (m_secondarySortFilter != a_filter) {
				m_ascending = true;
				m_secondarySortFilter = std::move(a_filter);
			}

			m_usePrimary = false;
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
