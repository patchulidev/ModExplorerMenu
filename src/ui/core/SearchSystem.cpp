#include "SearchSystem.h"
#include "imgui.h"

#include "localization/Locale.h"
#include "config/ThemeConfig.h"
#include "ui/components/UICustom.h"

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

			SetupDefaultKey();

			return m_initialized = true;
		}

		// If we ensure JSON creation (a_create == true), we assume that the absence of SearchProperty is an error!
		ASSERT_MSG(a_create, "No SearchProperty found in JSON.\nFile: {}", m_file_path.string());

		return m_initialized = !a_create;
	}

	nlohmann::json SearchSystem::SerializeState() const
	{
		nlohmann::json j;

		j["CurrentSearchProperty"] = m_searchKey.ToString();

	    // Debug: Check what's actually in the buffer
		std::string buffer_str(m_searchBuffer);
		Info("SerializeState - Buffer contents: '{}' (length: {})", buffer_str, buffer_str.length());

		j["CurrentSearchBuffer"] = buffer_str;

		return j;
	}

	void SearchSystem::DeserializeState(const nlohmann::json& a_state)
	{
		Info("Deserializing SearchSystem state...\n\n{}", a_state.dump());
		if (a_state.contains("CurrentSearchProperty") && a_state["CurrentSearchProperty"].is_string()) {
			Info("Trying to assign Search Key");
			std::string prop_str = a_state["CurrentSearchProperty"].get<std::string>();
			auto filter = FilterProperty::FromString(prop_str);

			if (filter.has_value()) {
				Info("Succeeded");
				m_searchKey = filter.value();
			} else {
				Info("Failed");
				m_searchKey = SearchItem(PropertyType::kNone);
			}
		}

		if (a_state.contains("CurrentSearchBuffer") && a_state["CurrentSearchBuffer"].is_string()) {
			std::string buffer_str = a_state["CurrentSearchBuffer"].get<std::string>();
			Info("Restoring search buffer: {}", buffer_str);
			ImFormatString(m_searchBuffer, sizeof(m_searchBuffer), "%s", buffer_str.c_str());
		}
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

	// Backwards compatible for tables, use for extracting kit names.
	std::string SearchSystem::ExtractDisplayName(const std::string& a_fullName)
	{
		if (a_fullName.empty()) {
			return "";
		}
		
		std::string name = a_fullName.substr(a_fullName.find_last_of("/\\") + 1);
		
		size_t dotPos = name.find_last_of('.');
		if (dotPos != std::string::npos) {
			name = name.substr(0, dotPos);
		}
		
		return name;
	}

	void SearchSystem::FilterItems(const std::vector<std::string>& a_items, const char* a_buffer)
	{
		m_filteredList.clear();
		m_navList.clear();
		m_topComparisonWeight = std::string::npos;
		m_topComparisonIdx = -1;

		std::string input = a_buffer;
		std::transform(input.begin(), input.end(), input.begin(), ::tolower);

		for (size_t i = 0; i < a_items.size(); i++) {
			std::string compare = a_items[i];
			std::transform(compare.begin(), compare.end(), compare.begin(), ::tolower);
			
			size_t weight = compare.find(input);
			bool show = (weight != std::string::npos);

			SearchItem item(m_searchKey.GetPropertyType());
			item.name = a_items[i];
			item.weight = weight;
			item.idx = static_cast<int>(i);
			item.show = show;

			m_filteredList.push_back(item);

			if (show) {
				m_navList.push_back(item);
				
				// Track the best match
				if (weight < m_topComparisonWeight) {
					m_topComparisonWeight = weight;
					m_topComparisonIdx = static_cast<int>(i);
				}
			}
		}
	}

	bool SearchSystem::RenderPopupItems(const char* a_buffer, const std::string& a_preview, bool a_popupIsAppearing, int& a_cursorIdx)
	{
		if (m_navList.size() <= 0 && !m_forceDropdown) {
			ImGui::TextWrapped("No Results Found, try clearing your search.");
			return false;
		}

		int filtered_idx = 0;
		bool bufferIsEmpty = (strlen(a_buffer) == 0);
		bool clicked = false;

		for (size_t i = 0; i < m_filteredList.size(); i++) {
			const auto& item = m_filteredList[i];
			
			if ((item.show || m_forceDropdown) && !item.name.empty()) {
				std::string displayName = ExtractDisplayName(item.name);
				bool isSelected = (filtered_idx == a_cursorIdx);

				// Auto-select the best match when popup appears
				if (a_popupIsAppearing) {
					if (bufferIsEmpty && displayName == a_preview) {
						m_navSelection = item;
						a_cursorIdx = filtered_idx;
						ImGui::SetScrollHereY();
					} else if (!bufferIsEmpty && m_topComparisonIdx == item.idx) {
						m_navSelection = item;
						a_cursorIdx = filtered_idx;
						ImGui::SetScrollHereY();
					}
				}

				if (ImGui::Selectable(displayName.c_str(), isSelected)) {
					clicked = true;
					m_navSelection = item;
					a_cursorIdx = filtered_idx;
				}

				filtered_idx++;
			}
		}

		return clicked;
	}

	bool SearchSystem::HandleKeyboardNavigation(char* a_buffer, size_t a_size, ImGuiID a_inputTextID, int& a_cursorIdx, bool& a_unlockScroll)
	{
		bool shouldClosePopup = false;
		bool bufferChanged = false;
		
		ImGuiInputTextState* inputState = ImGui::GetInputTextState(a_inputTextID);

		// Down arrow -- Increment with wrap-around
		if (ImGui::Shortcut(ImGuiKey_DownArrow, ImGuiInputFlags_Repeat, a_inputTextID) && !m_navList.empty()) {
			a_cursorIdx = (a_cursorIdx + 1) % static_cast<int>(m_navList.size());
			m_navSelection = m_navList[a_cursorIdx];
			a_unlockScroll = false;
			m_lastNavKey = ImGuiKey_DownArrow;
			bufferChanged = true;
		}

		// Up arrow -- Decrement with wrap-around
		if (ImGui::Shortcut(ImGuiKey_UpArrow, ImGuiInputFlags_Repeat, a_inputTextID) && !m_navList.empty()) {
			a_cursorIdx = (a_cursorIdx - 1 + static_cast<int>(m_navList.size())) % static_cast<int>(m_navList.size());
			m_navSelection = m_navList[a_cursorIdx];
			a_unlockScroll = false;
			m_lastNavKey = ImGuiKey_UpArrow;
			bufferChanged = true;
		}

		// Update buffer with selected item (preview only - don't commit yet)
		// This emulates the virtual selection / auto completion behavior.
		if (bufferChanged && m_lastNavKey != ImGuiKey_None) {
			std::string displayName = ExtractDisplayName(m_navSelection.name);
			ImFormatString(a_buffer, a_size, "%s", displayName.c_str());
			
			if (inputState) {
				inputState->ReloadUserBufAndSelectAll();
			}
		}

		// Only capture Enter when we're rendering a navable list.
		if (ImGui::Shortcut(ImGuiKey_Enter, ImGuiInputFlags_RouteAlways, a_inputTextID) && !m_navList.empty()) {
			ImFormatString(a_buffer, a_size, "%s", m_navSelection.name.c_str());
			
			if (inputState) {
				inputState->ReloadUserBufAndMoveToEnd();
			}
			
			ImGui::ClearActiveID();
			shouldClosePopup = true;
			return true; // Signal that selection was committed
		}

		// TEST: Leftover from v1.x, not sure why we disabled these.
		if (ImGui::Shortcut(ImGuiKey_PageUp, 0, a_inputTextID)) {}
		if (ImGui::Shortcut(ImGuiKey_PageDown, 0, a_inputTextID)) {}

		return shouldClosePopup;
	}

	// Source: https://github.com/ocornut/imgui/issues/718
	// Modified with additional features to best fit our use-case.
	bool SearchSystem::InputTextComboBox(const char* a_label, char* a_buffer,
		std::string& a_preview, 
		size_t a_size, 
		std::vector<std::string> a_items, 
		float a_width)
	{
		// Generate unique IDs for this widget instance
		auto suffix = std::string("##InputTextCombo::") + a_label;
		auto icon_pos = ImGui::GetCursorScreenPos();
		auto popupID = suffix + "Popup";
		bool result = false;

		ImGui::SetNextItemWidth(a_width);
		ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 12.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8.0f, 8.0f));

		bool enterPressed = ImGui::InputTextWithHint(a_label, a_preview.c_str(), a_buffer, a_size, ImGuiInputTextFlags_EnterReturnsTrue);

		// Draw Search Icon;
		ImGui::PushFont(NULL, 18.0f);
		const auto& DrawList = ImGui::GetWindowDrawList();
		icon_pos.x += a_width - ImGui::GetFrameHeightWithSpacing() + ImGui::GetStyle().FramePadding.x;
		icon_pos.y += (ImGui::GetItemRectSize().y / 2.0f) - (ImGui::GetFontSize() / 2.0f);

		DrawList->AddText(icon_pos, ThemeConfig::GetColorU32("TEXT"), ICON_LC_SEARCH);
		ImGui::PopFont();
		ImGui::PopStyleVar(2);

		ImGuiID inputTextID = ImGui::GetItemID();
		const bool inputTextActive = ImGui::IsItemActive();
		const bool inputTextDeactivated = ImGui::IsItemDeactivated();

		// Global keyboard shortcut to focus search (Ctrl+Shift+F)
		if (ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiMod_Shift | ImGuiKey_F, ImGuiInputFlags_RouteAlways)) {
			ImGui::SetKeyboardFocusHere(-1);
		}

		// Store rect info for popup positioning
		auto prevItemRectMin = ImGui::GetItemRectMin();
		auto prevItemRectMax = ImGui::GetItemRectMax();
		auto prevItemRectSize = ImGui::GetItemRectSize();

		// Open popup when input is active
		if (inputTextActive) {
			m_forceDropdown = false;
			ImGui::OpenPopup(popupID.c_str());
		}

		// Don't touch these flags without understanding their implications!
		ImGuiWindowFlags popup_flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings;
		popup_flags |= ImGuiWindowFlags_NoFocusOnAppearing;
		popup_flags |= ImGuiWindowFlags_NoNav;

		auto minPopupSize = ImVec2(prevItemRectSize.x, 0);
		auto maxPopupSize = ImVec2(prevItemRectSize.x, prevItemRectSize.y * 20);

		ImGui::SetNextWindowPos(ImVec2(prevItemRectMin.x, prevItemRectMax.y + ImGui::GetStyle().ItemSpacing.y));
		ImGui::SetNextWindowSizeConstraints(minPopupSize, maxPopupSize);
		ImGui::SetNextWindowBgAlpha(1.0f);

		// Begin popup
		if (ImGui::BeginPopupEx(ImGui::GetID(popupID.c_str()), popup_flags)) {
			const bool popup_is_appearing = ImGui::IsWindowAppearing();

			ImGuiStorage* storage = ImGui::GetStateStorage();
			const int cursor_idx_prev = storage->GetInt(ImGui::GetID("CursorIdx"), -1);
			const bool unlock_scroll_prev = storage->GetBool(ImGui::GetID("UnlockScroll"), false);
			
			int cursor_idx = cursor_idx_prev;
			bool unlock_scroll = unlock_scroll_prev;

			// NOTE: Detect if user is typing (reset navigation state). No Simplified
			// logic from v1.x to no longer align cursor to best match. Makes navigation
			// heuristics more predictable, and safer.

			bool userIsTyping = false;
			for (int i = ImGuiKey_NamedKey_BEGIN; i < ImGuiKey_NamedKey_END; i++) {
				auto key = static_cast<ImGuiKey>(i);
				
				// Skip navigation keys
				if (key == ImGuiKey_DownArrow || key == ImGuiKey_UpArrow || 
					key == ImGuiKey_Enter || key == ImGuiKey_MouseLeft || 
					key == ImGuiKey_MouseRight) {
					continue;
				}

				// Mouse wheel unlocks scroll
				if (key == ImGuiKey_MouseWheelX || key == ImGuiKey_MouseWheelY) {
					if (ImGui::IsKeyPressed(key)) {
						unlock_scroll = true;
					}
					continue;
				}

				if (ImGui::IsKeyDown(key)) {
					userIsTyping = true;
					m_lastNavKey = ImGuiKey_None;
					cursor_idx = -1; // Reset cursor when typing
					break;
				}
			}

			// Filter items when typing or when popup first appears
			if (userIsTyping || m_lastNavKey == ImGuiKey_None || popup_is_appearing) {
				FilterItems(a_items, a_buffer);
			}

			// Handle keyboard navigation
			bool selectionCommitted = HandleKeyboardNavigation(a_buffer, a_size, inputTextID, cursor_idx, unlock_scroll);
			
			if (selectionCommitted) {
				result = true;
				ImGui::CloseCurrentPopup();
			} else {
				ImGui::PushStyleVar(ImGuiStyleVar_SelectableTextAlign, ImVec2(0.f, 0.5f));
				ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(5.0f, 5.0f));
				ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(5.0f, 5.0f));
				
				// NOTE: Bisected logic for handling keyboard navigation vs autocompletion.
				// Now the two congruently work without interfering with each other. This is based
				// on unlock_scroll state.

				if (m_lastNavKey == ImGuiKey_DownArrow || m_lastNavKey == ImGuiKey_UpArrow) {
					int filtered_idx = 0;
					for (const auto& item : m_filteredList) {
						if (item.show && !item.name.empty()) {
							std::string displayName = ExtractDisplayName(item.name);
							bool isSelected = (filtered_idx == cursor_idx);
							
							// Make sure we store the full name in buffer, not the display name.
							if (ImGui::Selectable(displayName.c_str(), isSelected)) {
								m_navSelection = item;
								ImFormatString(a_buffer, a_size, "%s", item.name.c_str());
								ImGui::ClearActiveID();
								ImGui::CloseCurrentPopup();
								result = true;
								break;
							}

							// Auto-scroll to cursor position
							if (!unlock_scroll && isSelected) {
								ImGui::SetScrollHereY();
							}

							filtered_idx++;
						}
					}
				} else {
					if (RenderPopupItems(a_buffer, a_preview, popup_is_appearing, cursor_idx)) {
						m_navSelection = m_navList[cursor_idx];
						ImFormatString(a_buffer, a_size, "%s", m_navSelection.name.c_str());
						ImGui::ClearActiveID();
						ImGui::CloseCurrentPopup();
						result = true;
					}
				}
				
				ImGui::PopStyleVar(3);
			}

			// Close popup when appropriate
			if (!inputTextActive && !ImGui::IsWindowFocused() && !m_forceDropdown) {
				m_lastNavKey = ImGuiKey_None;
				ImGui::CloseCurrentPopup();
			}

			// Handle force dropdown close
			if (m_forceDropdown && !ImGui::IsWindowFocused() && 
				ImGui::IsMouseClicked(0) && !ImGui::IsAnyItemHovered()) {
				m_forceDropdown = false;
				ImGui::CloseCurrentPopup();
			}

			// Persist state
			if (cursor_idx != cursor_idx_prev) {
				storage->SetInt(ImGui::GetID("CursorIdx"), cursor_idx);
			}
			if (unlock_scroll != unlock_scroll_prev) {
				storage->SetBool(ImGui::GetID("UnlockScroll"), unlock_scroll);
			}

			ImGui::EndPopup();
		} else {
			// Popup closed - reset navigation state
			m_lastNavKey = ImGuiKey_None;
		}

		// Handle enter key press from input field using nav selection or default=0.
		if (enterPressed) {
			if (!m_navList.empty()) {
				if (m_navSelection.name.empty()) {
					m_navSelection = m_navList[0];
				}
				
				ImFormatString(a_buffer, a_size, "%s", m_navSelection.name.c_str());
				result = true;
			} else {
				a_preview = "(Warning: No Results Found \"" + std::string(a_buffer) + "\")";
				ImFormatString(a_buffer, a_size, "%s", "");
				result = false;
			}
		}

		return result;
	}
}
