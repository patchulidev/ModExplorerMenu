#pragma once

#include "data/BaseObject.h"
#include "config/ConfigManager.h"

namespace Modex
{
	using FilterPropertyList = std::vector<FilterProperty>;

	class SortSystem : public ConfigManager
	{
	public:
		struct SortQuery
		{
			int column = -1;
			FilterProperty property = PropertyType::kNone;

			friend void to_json(nlohmann::json& j, const SortQuery& q)
			{
				j = nlohmann::json{
					{"Column", q.column},
					{"Property", q.property.ToString()}
				};
			}

			friend void from_json(const nlohmann::json& j, SortQuery& q)
			{
				if (j.contains("Column") && j["Column"].is_number_integer()) {
					q.column = j["Column"].get<int>();
				}

				if (j.contains("Property") && j["Property"].is_string()) {
					auto filter = FilterProperty::FromString(j["Property"].get<std::string>());
					q.property = filter.has_value() ? filter.value() : FilterProperty(PropertyType::kNone);
				}
			}
		};

	private:
		bool                        m_ascending;
		bool                        m_usePrimary;
		std::function<void()>       m_sortSystemCallback;

		std::vector<SortQuery>      m_columns {
			SortQuery{0, PropertyType::kPlugin},
			SortQuery{1, PropertyType::kEditorID},
			SortQuery{2, PropertyType::kNone}
		};

		SortQuery                   m_currentSort;
		FilterPropertyList          m_availableSortFilters;

		// FilterProperty              m_primarySortFilter;
		// FilterProperty              m_secondarySortFilter;

	public:
		SortSystem(const std::filesystem::path& a_path)
			: m_ascending(true)
			, m_usePrimary(true)
			, m_currentSort()
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

		void SetSortData(SortQuery a_query)
		{
			for (int i = 0; i < std::ssize(m_columns); i++) {
				if (i == a_query.column) {
					m_columns[i].property = a_query.property;
				}
			}

			m_currentSort = std::move(a_query);
		}

		void SetSortedColumn(int a_column)
		{
			if (a_column < 0 || a_column > std::ssize(m_columns)) {
				m_currentSort = {0, FilterProperty(PropertyType::kNone)};
			} else {
				m_currentSort = {a_column, m_columns[a_column].property};
			}
		}

		bool IsColumnSorted(int a_column)
		{
			return m_currentSort.column == a_column;
		}

		FilterProperty GetCurrentSortProperty()
		{
			return m_currentSort.property;
		}

		std::vector<SortQuery> GetColumns()
		{
			return m_columns;
		}

		void SetupColumns(const std::vector<SortQuery> a_columns)
		{
			m_columns = std::move(a_columns);
		}

		void Reset()
		{
			m_columns = {
				{0, PropertyType::kPlugin},
				{1, PropertyType::kEditorID},
				{2, PropertyType::kNone}
			};
			
			m_currentSort = m_columns[1];
			m_ascending = true;
		}

		FilterProperty GetColumnPropertyType(int a_column)
		{
			if (m_columns.empty()) return PropertyType::kNone;
			if (a_column < 0 || a_column > std::ssize(m_columns)) return PropertyType::kNone;
			return m_columns[a_column].property;
		}

		// void ResetSort() {
		// 	m_ascending = true;
		// 	m_secondarySortFilter = PropertyType::kNone;
		// }

		void ToggleAscending() {
			m_ascending = !m_ascending;
		}

		// const FilterProperty& GetPrimarySortFilter() const {
		// 	return m_primarySortFilter;
		// }
		//
		// const FilterProperty& GetSecondarySortFilter() const {
		// 	return m_secondarySortFilter;
		// }

		bool& GetSortAscending() {
			return m_ascending;
		}

		// bool& GetUsePrimary() {
		// 	return m_usePrimary;
		// }

		// void UsePrimary(bool a_use = true) {
		// 	m_usePrimary = a_use;
		// }

		// void SetPrimarySortFilter(FilterProperty a_filter) {
		// 	if (m_primarySortFilter != a_filter) {
		// 		m_primarySortFilter = std::move(a_filter);
		// 	}
		//
		// 	m_usePrimary = true;
		// }
		//
		// void SetSecondarySortFilter(FilterProperty a_filter) {
		// 	if (m_secondarySortFilter != a_filter) {
		// 		m_ascending = true;
		// 		m_secondarySortFilter = std::move(a_filter);
		// 	}
		//
		// 	m_usePrimary = false;
		// }

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
