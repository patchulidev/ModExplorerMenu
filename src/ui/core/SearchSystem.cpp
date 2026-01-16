#pragma once

#include "SearchSystem.h"

// #include "include/P/Persistent.h"
// #include "extern/magic_enum.hpp"

namespace Modex
{
	bool SearchSystem::Load(bool a_create)
	{
		ASSERT_MSG(a_create && m_file_path.empty(), "Called before setting file path!");

		if (!ConfigManager::Load(a_create))
			return m_initialized;

		if (m_data.contains("SearchProperty")) {
			const auto& key_array = m_data["SearchProperty"];

			for (const auto& key_entry : key_array) {
				std::string key_name = key_entry.get<std::string>();
				auto filter = FilterProperty::FromString(key_name);

				if (filter.has_value()) {
					m_availableSearchKeys.emplace_back(filter.value());
				}
			}

			return m_initialized = true;
		}

		// If we ensure JSON creation, we assume that the absence of SearchProperty is an error!
		ASSERT_MSG(a_create, "No SearchProperty found in JSON.\nFile: {}", m_file_path.string());

		return m_initialized = !a_create;
	}

	bool SearchSystem::CompareInputToObject(const BaseObject* a_object)
	{
		std::string compareString;
		std::string input = m_searchBuffer;
		std::transform(input.begin(), input.end(), input.begin(),
			[](unsigned char c) { return static_cast<char>(std::tolower(c)); });

		if (!input.empty()) {
			compareString = a_object->GetProperty(m_searchKey);

			std::transform(compareString.begin(), compareString.end(), compareString.begin(),
				[](unsigned char c) { return static_cast<char>(std::tolower(c)); });

			// If the input is wrapped in quotes, we do an exact match across all parameters.
			if (!input.empty() && input.front() == '"' && input.back() == '"') {
				std::string match = input.substr(1, input.size() - 2);

				if (compareString == match) {
					return true;
				}
			}

			// If the input begins with > or < we do a greater than / less than comparison.
			if (!input.empty() && (input.front() == '>' || input.front() == '<')) {
				char comparator = input.front();
				std::string match = input.substr(1);

				// Attempt to convert both strings to numbers.
				try {
					double compareValue = std::stod(compareString);
					double matchValue = std::stod(match);

					if (comparator == '>' && compareValue > matchValue) {
						return true;
					} else if (comparator == '<' && compareValue < matchValue) {
						return true;
					}
				} catch (const std::invalid_argument&) {
					return false;
				}
			}

			// If the input contains a '==' we do an equality comparison.
			size_t equalityPos = input.find("==");
			if (equalityPos != std::string::npos) {
				std::string match = input.substr(equalityPos + 2);

				if (compareString == match) {
					return true;
				} else {
					return false;
				}
			}

			// If the input contains a '!=' we do an inequality comparison.
			size_t inequalityPos = input.find("!=");
			if (inequalityPos != std::string::npos) {
				std::string match = input.substr(inequalityPos + 2);

				if (compareString != match) {
					return true;
				} else {
					return false;
				}
			}

			// If the input starts with '%' we do a full regex comparison.
			if (!input.empty() && input.front() == '%') {
				std::string pattern = input.substr(1);
				try {
					std::regex re(pattern, std::regex::icase);
					if (std::regex_search(compareString, re)) {
						return true;
					}
				} catch (const std::regex_error&) {
					return false;
				}
			}
		}

		return compareString.find(input) != std::string::npos;
	}

	// TODO: The size_t argument on InputTextComboBox is kind of irrelevant if we pass a buffer.

    // Source: https://github.com/ocornut/imgui/issues/718
    // Modified with additional features to best fit our use-case.
	bool SearchSystem::InputTextComboBox(const char* a_label, char* a_buffer, std::string& a_preview, size_t a_size, std::vector<std::string> a_items, float a_width, bool a_showArrow)
	{
		bool result = false;
		ImGuiContext& g = *GImGui;

		auto arrowSize = ImGui::GetFrameHeight();
		ImGui::SetNextItemWidth(a_width - arrowSize);
		result = ImGui::InputTextWithHint(a_label, a_preview.c_str(), a_buffer, a_size, ImGuiInputTextFlags_EnterReturnsTrue);

		if (ImGui::IsItemDeactivated() && !result) {
			if (a_buffer && std::strcmp(a_buffer, a_preview.c_str()) != 0) {
				ImFormatString(a_buffer, a_size, "%s", "");
			}
		}
		
		if (ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiMod_Shift | ImGuiKey_F, ImGuiInputFlags_RouteAlways)) {
			ImGui::SetKeyboardFocusHere(-1);
		}

		auto suffix = std::string("##InputTextCombo::");
		auto prevItemRectMax = ImGui::GetItemRectMax();
		auto prevItemRectMin = ImGui::GetItemRectMin();
		auto prevItemRectSize = ImGui::GetItemRectSize();

		ImGuiID inputTextID = ImGui::GetItemID();
		const bool inputTextActive = ImGui::IsItemActive();
		ImGuiInputTextState* inputState = inputTextActive ? ImGui::GetInputTextState(inputTextID) : NULL;

		// Block ImGui ItemSpacing to re-create ComboBox style.
		
		if (a_showArrow) {
			ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
			ImGui::SameLine();
			if (ImGui::ArrowButton((suffix + a_label).c_str(), ImGuiDir_Down)) {
				if (ImGui::IsPopupOpen((suffix + a_label + "Popup").c_str())) {
					ImGui::CloseCurrentPopup();
					m_forceDropdown = false;
				} else {
					ImGui::OpenPopup((suffix + a_label + "Popup").c_str());
					m_forceDropdown = true;
				}
			}
			ImGui::PopStyleVar();
		}

		if (inputTextActive) {
			m_forceDropdown = false;
			ImGui::OpenPopup((suffix + a_label + "Popup").c_str());
		}

		// Position and size popup
		ImGui::SetNextWindowPos(ImVec2(prevItemRectMin.x, prevItemRectMax.y + ImGui::GetStyle().ItemSpacing.y));

		ImGuiWindowFlags popup_window_flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings;
		popup_window_flags |= ImGuiWindowFlags_NoFocusOnAppearing;
		popup_window_flags |= ImGuiWindowFlags_NoNav;
		popup_window_flags |= ImGuiWindowFlags_AlwaysAutoResize;

		auto minPopupSize = ImVec2(prevItemRectSize.x, 0);
		auto maxPopupSize = ImVec2(prevItemRectSize.x, prevItemRectSize.y * 20);  // 20 items max (?)
		ImGui::SetNextWindowSizeConstraints(minPopupSize, maxPopupSize);
		ImGui::SetNextWindowBgAlpha(1.0f);
		if (ImGui::BeginPopupEx(ImGui::GetID((suffix + a_label + "Popup").c_str()), popup_window_flags)) {
			ImGuiWindow* popup_window = g.CurrentWindow;
			const bool popup_is_appearing = ImGui::IsWindowAppearing();

			// https://github.com/ocornut/imgui/issues/4461
			if (popup_is_appearing) {
				ImGui::BringWindowToDisplayFront(popup_window);
			}

			// Important: Tracks navigation cursor for keyboard input.
			const int cursor_idx_prev = ImGui::GetStateStorage()->GetInt(ImGui::GetID("CursorIdx"), -1);
			int cursor_idx = cursor_idx_prev;

			// Important: Tracks navigation state so that the user can switch between Arrow Keys and Mousewheel seamlessly.
			const bool unlock_scroll_prev = ImGui::GetStateStorage()->GetBool(ImGui::GetID("UnlockScroll"), false);
			bool unlock_scroll = unlock_scroll_prev;

			// TODO: Locale
			if (m_navList.size() <= 0) {
				ImGui::TextWrapped("No Results Found, try clearing your search.");
				// ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.3f, 1.0f), "No Results Found, try clearing your search.");
			}

			// If we are typing, reset navigation state and cursor. The filtered list
			// will also be reset further down in the code.
			for (int i = ImGuiKey_NamedKey_BEGIN; i < ImGuiKey_NamedKey_END; i++) {
				auto key = static_cast<ImGuiKey>(i);
				if (key == ImGuiKey_DownArrow ||
					key == ImGuiKey_UpArrow ||
					key == ImGuiKey_Tab ||
					key == ImGuiKey_Enter ||
					key == ImGuiKey_MouseLeft ||
					key == ImGuiKey_MouseRight)
					continue;

				if (key == ImGuiKey_MouseWheelX || key == ImGuiKey_MouseWheelY) {
					unlock_scroll = true;
					break;
				}

				if (ImGui::IsKeyDown(key)) {
					m_lastNavKey = ImGuiKey_None;
					break;
				}
			}

			// Custom keyboard navigation.
			// - Keyboard navigation relies on navList which is a cached SearchList with hidden SearchItems omitted.
			// - This is important: We have to feed navigation a linear list of items to match cursor idx.
			bool rewrite_buf = false;
			if (ImGui::Shortcut(ImGuiKey_DownArrow, ImGuiInputFlags_Repeat, inputTextID) && (m_navList.size() > 0)) {
				cursor_idx = (cursor_idx + 1) % m_navList.size();
				m_navSelection = m_navList.at(cursor_idx);

				rewrite_buf = true;
				unlock_scroll = false;
				m_lastNavKey = ImGuiKey_DownArrow;
			}
			if (ImGui::Shortcut(ImGuiKey_UpArrow, ImGuiInputFlags_Repeat, inputTextID) && (m_navList.size() > 0)) {
				cursor_idx = (cursor_idx - 1 + static_cast<int>(m_navList.size())) % static_cast<int>(m_navList.size());
				m_navSelection = m_navList.at(cursor_idx);
				rewrite_buf = true;
				unlock_scroll = false;
				m_lastNavKey = ImGuiKey_UpArrow;
			}

			// Makeshift auto-completion to circumvent the need for ImGui's implementation.
			if (ImGui::Shortcut(ImGuiKey_Tab, ImGuiInputFlags_RouteActive, inputTextID) && (m_navList.size() > 0)) {
				ImFormatString(a_buffer, a_size, "%s", m_navSelection.ToString().c_str());
				if (inputState) {
					inputState->ReloadUserBufAndMoveToEnd();
					ImGui::ClearActiveID();
				} else {
					ImGui::ActivateItemByID(inputTextID);
				}

				result = true;
				ImGui::CloseCurrentPopup();
			}

            // TODO: Why did we disable these?
			// Disable PageUp/PageDown keys
			if (ImGui::Shortcut(ImGuiKey_PageUp, 0, inputTextID)) {}
			if (ImGui::Shortcut(ImGuiKey_PageDown, 0, inputTextID)) {}
			if (rewrite_buf) {
				ImFormatString(a_buffer, a_size, "%s", m_navSelection.ToString().c_str());

				if (inputState) {
					inputState->ReloadUserBufAndSelectAll();
				} else {
					ImGui::ActivateItemByID(inputTextID);
				}
			}

			// If we are ever using the navigation keys, we do not want to modify the original filterList copy.
			// This is because we want to keep the original list intact for future filtering.
			// Only when accepting input do we clear the previous filterList and navList cache and rebuild.
			ImGui::PushStyleVar(ImGuiStyleVar_SelectableTextAlign, ImVec2(0.f, 0.5f));
			if (m_lastNavKey == ImGuiKey_DownArrow || m_lastNavKey == ImGuiKey_UpArrow) {
				int filtered_idx = 0;
				for (int i = 0; i < m_filteredList.size(); i++) {
					auto item = m_filteredList.at(i);
					if (item.show && !item.name.empty()) {
						// const char* item_name = item.name.c_str();
						const ImVec2 item_pos = popup_window->DC.CursorPos;

						std::string item_name = item.name;
						item_name = item_name.substr(item_name.find_last_of("/\\") + 1);
						item_name = item_name.substr(0, item_name.find_last_of('.'));

						if (ImGui::Selectable(item_name.c_str(), filtered_idx == cursor_idx)) {
							ImGui::ClearActiveID();
							// ImFormatString(a_buffer, a_size, "%s", item_name);
							m_navSelection = item;
							ImGui::CloseCurrentPopup();
							result = true;
							break;
						}

						// TODO: Flickering while scrolling
						if (!unlock_scroll && filtered_idx == cursor_idx) {
							ImGui::SetScrollHereY();
						}

						filtered_idx++;
					}
				}

			} else {
				m_navList.clear();
				m_filteredList.clear();
				m_topComparisonWeight = std::string::npos;

				// (First Pass)
				// The purpose of doing two passes is to first observe the entire list and create
				// a cache of each item. I do this so I can track the userdata and state across those
				// items. The second pass (mostly through the filtered list) does the actual drawing.
				for (int current_item_idx = 0; current_item_idx < a_items.size(); current_item_idx++) {
					std::string compare = a_items.at(current_item_idx);
					std::string input = a_buffer;
					size_t comparator_weight = 0;

					// TODO: Is this an issue for Non-ASCII characters?
					std::transform(compare.begin(), compare.end(), compare.begin(), ::tolower);
					std::transform(input.begin(), input.end(), input.begin(), ::tolower);
					comparator_weight = compare.find(input);

					// Set top match to true item_idx position
					if (comparator_weight < m_topComparisonWeight) {
						m_topComparisonWeight = comparator_weight;
						m_topComparisonIdx = current_item_idx;
					}

					SearchItem a_item(m_searchKey.GetPropertyType());
					a_item.name = a_items.at(current_item_idx);
					a_item.weight = comparator_weight;
					a_item.idx = current_item_idx;
					a_item.show = (comparator_weight == std::string::npos) ? false : true;

					// a_temp.push_back(a_item);
					m_filteredList.push_back(a_item);

					if (a_item.show) {
						m_navList.push_back(a_item);
					}
				}

				// (Second Pass)
				// Need to keep track of the filtered index for cursor position since we don't use a vector and
				// don't step through it incrementally. (Because we track shown state based on SearchItem::show).
				// forceDropDown is used to keep bypass the filter and show all items if ArrowButton is clicked.
				int filtered_idx = 0;
				for (int i = 0; i < m_filteredList.size(); i++) {
					auto item = m_filteredList.at(i);
					if ((item.show or m_forceDropdown) && !item.name.empty()) {
						// const char* item_name = item.name.c_str();
						const ImVec2 item_pos = popup_window->DC.CursorPos;

						std::string item_name = item.name;
						item_name = item_name.substr(item_name.find_last_of("/\\") + 1);
						item_name = item_name.substr(0, item_name.find_last_of('.'));

						if (strlen(a_buffer) == 0) {  // maybe utf-8 safe?
							if (item_name == a_preview) {
								if (popup_is_appearing) {
									ImGui::SetScrollHereY();
								}
								m_navSelection = item;
								cursor_idx = filtered_idx;
							}
						} else if (m_topComparisonIdx == item.idx) {
							if (popup_is_appearing) {
								ImGui::SetScrollHereY();
							}
							m_navSelection = item;
							cursor_idx = filtered_idx;
						}

						if (ImGui::Selectable(item_name.c_str(), cursor_idx == filtered_idx)) {
							ImGui::ClearActiveID();
							//ImFormatString(a_buffer, a_size, "%s", item_name);
							m_navSelection = item;
							ImGui::CloseCurrentPopup();
							result = true;
							break;
						}

						filtered_idx++;
					}
				}
			}
			ImGui::PopStyleVar();

			// Close popup on deactivation (unless we are mouse-clicking in our popup)
			if (!inputTextActive && !ImGui::IsWindowFocused() && !m_forceDropdown) {
				m_lastNavKey = ImGuiKey_None;
				ImGui::CloseCurrentPopup();
			}

			// Additional state handling for ArrowButton drop-down.
			if (m_forceDropdown && !ImGui::IsWindowFocused() && ImGui::IsMouseClicked(0) && !ImGui::IsAnyItemHovered()) {
				m_forceDropdown = false;
				ImGui::CloseCurrentPopup();
			}

			// Store cursor position.
			if (cursor_idx != cursor_idx_prev) {
				ImGui::GetStateStorage()->SetInt(ImGui::GetID("CursorIdx"), cursor_idx);
			}

			// Store scroll position.
			if (unlock_scroll != unlock_scroll_prev) {
				ImGui::GetStateStorage()->SetBool(ImGui::GetID("UnlockScroll"), unlock_scroll);
			}

			ImGui::EndPopup();
		}

		// Because we introduced the InputTextFlags_EnterReturnsTrue flag, we need to manually handle some of these
		// exceptions with fallbacks to prevent undefined behavior or unexpected results.
		// We use navList here since it's `.size()` reflects the number of plugins that are shown in the dropdown.

		// Fallback in-case the user presses enter without a matching plugin or selection.
		if ((result && !m_forceDropdown) && m_navList.size() <= 0) {
			a_preview = "(Warning: No Results Found \"" + std::string(a_buffer) + "\")";
			ImFormatString(a_buffer, a_size, "%s", "");
			ImGui::CloseCurrentPopup();
			result = false;
		}

		// Fallback in-case the user presses enter with an empty buffer, but with a navList selection.
		if (result && m_navList.size() > 0) {
			ImFormatString(a_buffer, a_size, "%s", m_navSelection.name.c_str());
			ImGui::CloseCurrentPopup();
			result = true;
		}

		return result;
	}
}