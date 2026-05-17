#include "UIPopup.h"
#include "UIContainers.h"
#include "config/EquipmentConfig.h"
#include "config/Keycodes.h"
#include "external/icons/IconsLucide.h"
#include "imgui.h"
#include "localization/Locale.h"
#include "config/ThemeConfig.h"
#include "ui/components/UICustom.h"

namespace Modex
{
	void UIPopupHotkey::Draw()
	{
		static float height;
		auto width = ImGui::GetMainViewport()->Size.x * ThemeConfig::GetWidgetStyle().popup.widthFactor;
		const float center_x = ImGui::GetMainViewport()->Size.x * 0.5f;
		const float center_y = ImGui::GetMainViewport()->Size.y * 0.5f;
		const float pos_x = center_x - (width * 0.5f);
		const float pos_y = center_y - (height * 0.5f);

		DrawPopupBackground(m_alpha);

		ImGui::SetNextWindowSize(ImVec2(width, 0));
		ImGui::SetNextWindowPos(ImVec2(pos_x, pos_y));
		ImGui::SetNextWindowFocus();

		ImGui::PushStyleVar(ImGuiStyleVar_Alpha, m_alpha);
		if (ImGui::Begin("##Modex::HotkeyPopup", NULL, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoFocusOnAppearing)) {
			if (ImGui::IsWindowAppearing()) {
				ImGui::GetIO().ClearInputKeys();
			}

			if (UICustom::Popup_MenuHeader(m_pendingHotkeyTitle.c_str())) {
				DeclineHotkey();
			}

			ImGui::NewLine();
			ImGui::TextWrapped("%s", m_pendingHotkeyDesc.c_str());
			ImGui::NewLine();

			ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal);

			ImGui::PushStyleColor(ImGuiCol_Button, ThemeConfig::GetColor("DECLINE"));
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ThemeConfig::GetHover("DECLINE"));
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, ThemeConfig::GetActive("DECLINE"));
			if (ImGui::Button(Translate("CLOSE"), ImVec2(ImGui::GetContentRegionAvail().x, 0.f))) {
				DeclineHotkey();
			}
			ImGui::PopStyleColor(3);

			height = ImGui::GetWindowSize().y;
		}

		ImGui::End();
		ImGui::PopStyleVar();
	}

	void UIPopupHotkey::PopupHotkey(const char* a_title, const char* a_desc, uint32_t* a_hotkey, uint32_t a_default, uint32_t* a_modifier, std::function<void()> onConfirmHotkeyCallback)
	{
		m_pendingHotkeyTitle = a_title;
		m_pendingHotkeyDesc = a_desc;
		m_hotkeyCurrent = a_hotkey;
		m_hotkeyDefault = a_default;
		m_modifierCurrent = a_modifier;
		m_onConfirmCallback = onConfirmHotkeyCallback;
		m_captureInput = true;

		ModexGUIMenu::RegisterListener([this](uint32_t a_key, uint32_t a_modifier) { AcceptHotkey(a_key, a_modifier); });
	}

	void UIPopupHotkey::AcceptHotkey(uint32_t a_key, uint32_t a_modifier)
	{
		bool success = false;

		// T-key (0x14) resets both key and modifier to defaults.
		if (a_key == 0x14) {
			*m_hotkeyCurrent = m_hotkeyDefault;
			if (m_modifierCurrent) {
				*m_modifierCurrent = 0;
			}
			success = true;
		} else if (KeyCode::IsValidHotkey(a_key) && !KeyCode::IsKeyModifier(a_key)) {
			*m_hotkeyCurrent = a_key;
			if (m_modifierCurrent) {
				*m_modifierCurrent = a_modifier;
			}
			success = true;
		}

		if (success) {
			if (m_onConfirmCallback) {
				m_onConfirmCallback();
			}

			CloseWindow();
		} else {
			UIManager::GetSingleton()->ShowWarning(
				m_pendingHotkeyTitle,
				Translate("SETTINGS_MENU_KEYBIND_INVALID")
			);

			CloseWindow();
		}
	}

	void UIPopupHotkey::DeclineHotkey()
	{
		CloseWindow();
	}

	void UIPopupWarning::Draw()
	{
		static float height;
		auto width = ImGui::GetMainViewport()->Size.x * ThemeConfig::GetWidgetStyle().popup.widthFactor;
		const float center_x = ImGui::GetMainViewport()->Size.x * 0.5f;
		const float center_y = ImGui::GetMainViewport()->Size.y * 0.5f;
		const float pos_x = center_x - (width * 0.5f);
		const float pos_y = center_y - (height * 0.5f);

		DrawPopupBackground(m_alpha);

		ImGui::SetNextWindowSize(ImVec2(width, 0));
		ImGui::SetNextWindowPos(ImVec2(pos_x, pos_y));
		ImGui::SetNextWindowFocus();

		ImGui::PushStyleVar(ImGuiStyleVar_Alpha, m_alpha);
		if (ImGui::Begin("##Modex::WarningPopup", NULL, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoFocusOnAppearing)) {
			if (ImGui::IsWindowAppearing()) {
				ImGui::GetIO().ClearInputKeys();
			}

			if (UICustom::Popup_MenuHeader(m_pendingWarningTitle.c_str())) {
				DeclineWarning();
			}

			if (ImGui::IsKeyPressed(ImGuiKey_LeftArrow) || ImGui::IsKeyPressed(ImGuiKey_RightArrow)) {
				if (!ImGui::GetIO().WantTextInput) {
					m_navAccept = !m_navAccept;
				}
			}
			
			ImGui::TextWrapped("%s", m_pendingWarningMessage.c_str());
			ImGui::NewLine();
			ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal);

			bool _confirm, _cancel;
			if (UICustom::Popup_ConfirmDeclineButtons(_confirm, _cancel, m_navAccept)) {
				if (_confirm) {
					AcceptWarning();
				} else if (_cancel) {
					DeclineWarning();
				}
			}

			if (ImGui::IsKeyPressed(ImGuiKey_Enter) && ImGui::GetIO().WantTextInput == false) {
				if (m_navAccept) {
					AcceptWarning();
				} else {
					DeclineWarning();
				}
			}

			height = ImGui::GetWindowSize().y;
		}

		ImGui::End();
		ImGui::PopStyleVar();
	}

	void UIPopupWarning::PopupWarning(const std::string& a_title, const std::string& a_message, std::function<void()> a_onConfirmCallback)
	{
		m_pendingWarningTitle = a_title;
		m_pendingWarningMessage = a_message;
		m_onConfirmCallback = a_onConfirmCallback;
		m_captureInput = true;
	}

	void UIPopupWarning::AcceptWarning()
	{
		if (m_onConfirmCallback) {
			m_onConfirmCallback();
		}

		CloseWindow();
	}

	void UIPopupWarning::DeclineWarning()
	{
		CloseWindow();
	}

	void UIPopupInputBox::Draw()
	{
		static float height;
		auto width = ImGui::GetMainViewport()->Size.x * ThemeConfig::GetWidgetStyle().popup.widthFactor;
		const float center_x = ImGui::GetMainViewport()->Size.x * 0.5f;
		const float center_y = ImGui::GetMainViewport()->Size.y * 0.5f;
		const float pos_x = center_x - (width * 0.5f);
		const float pos_y = center_y - (height * 0.5f);

		DrawPopupBackground(m_alpha);

		ImGui::SetNextWindowSize(ImVec2(width, 0));
		ImGui::SetNextWindowPos(ImVec2(pos_x, pos_y));
		ImGui::SetNextWindowFocus();

		ImGui::PushStyleVar(ImGuiStyleVar_Alpha, m_alpha);
		if (ImGui::Begin("##Modex::InputBoxPopup", NULL, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar)) {
			if (ImGui::IsWindowAppearing()) {
				ImGui::GetIO().ClearInputKeys();
			}

			if (ImGui::IsKeyPressed(ImGuiKey_LeftArrow) || ImGui::IsKeyPressed(ImGuiKey_RightArrow)) {
				if (!ImGui::GetIO().WantTextInput) {
					m_navAccept = !m_navAccept;
				}
			}

			ImGui::SetNextItemWidth(500.0f);
			if (UICustom::Popup_MenuHeader(m_pendingInputTitle.c_str())) {
				DeclineInput();
			}

			ImGui::TextWrapped("%s",m_pendingInputMessage.c_str());
			ImGui::NewLine();

			ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
			if (ImGui::InputText("##Modex::InputBoxPopup::InputText", m_inputBuffer, IM_ARRAYSIZE(m_inputBuffer), ImGuiInputTextFlags_EnterReturnsTrue)) {
				AcceptInput();
			}

			if (ImGui::IsWindowAppearing()) {
				ImGui::SetKeyboardFocusHere(-1);
			}

			ImGui::NewLine();
			ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal, 2.0f);

			bool _confirm, _cancel;
			if (UICustom::Popup_ConfirmDeclineButtons(_confirm, _cancel, m_navAccept)) {
				if (_confirm) {
					AcceptInput();
				} else if (_cancel) {
					DeclineInput();
				}
			}

			if (ImGui::IsKeyPressed(ImGuiKey_Enter) && ImGui::GetIO().WantTextInput == false) {
				if (m_navAccept) {
					AcceptInput();
				} else {
					DeclineInput();
				}
			}

			height = ImGui::GetWindowSize().y;
		}
		ImGui::End();
		ImGui::PopStyleVar();
	}

	void UIPopupInputBox::PopupInputBox(const std::string& a_title, const std::string& a_message, std::string a_hint, std::function<void(const std::string&)> a_onConfirmCallback)
	{
		m_pendingInputTitle = a_title;
		m_pendingInputMessage = a_message;
		m_onConfirmCallback = a_onConfirmCallback;
		m_navAccept = true;
		m_captureInput = true;

		ImFormatString(m_inputBuffer, IM_ARRAYSIZE(m_inputBuffer), "%s", a_hint.c_str());
	}

	void UIPopupInputBox::AcceptInput()
	{
		if (m_onConfirmCallback) {
			m_onConfirmCallback(m_inputBuffer);
		}

		CloseWindow();
	}

	void UIPopupInputBox::DeclineInput()
	{
		CloseWindow();
	}

	void UIPopupInfo::Draw()
	{
		static float height;
		auto width = ImGui::GetMainViewport()->Size.x * ThemeConfig::GetWidgetStyle().popup.widthFactor;
		const float center_x = ImGui::GetMainViewport()->Size.x * 0.5f;
		const float center_y = ImGui::GetMainViewport()->Size.y * 0.5f;
		const float pos_x = center_x - (width * 0.5f);
		const float pos_y = center_y - (height * 0.5f);

		DrawPopupBackground(m_alpha);

		ImGui::SetNextWindowSize(ImVec2(width, 0));
		ImGui::SetNextWindowPos(ImVec2(pos_x, pos_y));
		ImGui::SetNextWindowFocus();

		ImGui::PushStyleVar(ImGuiStyleVar_Alpha, m_alpha);
		if (ImGui::Begin("##Modex::InfoPopup", NULL, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoFocusOnAppearing)) {
			if (ImGui::IsWindowAppearing()) {
				ImGui::GetIO().ClearInputKeys();
			}

			if (UICustom::Popup_MenuHeader(m_pendingInfoTitle.c_str())) {
				CloseInfo();
			}
			
			ImGui::NewLine();
			ImGui::TextWrapped("%s", m_pendingInfoMessage.c_str());
			ImGui::NewLine();
			ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal);

			ImGui::PushStyleColor(ImGuiCol_Button, ThemeConfig::GetColor("CONFIRM"));
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ThemeConfig::GetHover("CONFIRM"));
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, ThemeConfig::GetActive("CONFIRM"));
			if (ImGui::Button(Translate("CONFIRM"), ImVec2(ImGui::GetContentRegionAvail().x, 0.f))) {
				CloseInfo();
			}
			ImGui::PopStyleColor(3);

			if (ImGui::IsKeyPressed(ImGuiKey_Enter) && ImGui::GetIO().WantTextInput == false) {
				CloseInfo();
			}

			height = ImGui::GetWindowSize().y;
		}

		ImGui::End();
		ImGui::PopStyleVar();
	}

	void UIPopupInfo::PopupInfo(const std::string& a_title, const std::string& a_message)
	{
		m_pendingInfoTitle = a_title;
		m_pendingInfoMessage = a_message;
		m_captureInput = true;
	}

	void UIPopupInfo::CloseInfo()
	{
		CloseWindow();
	}

	// Persist edits to disk and notify the opener. No undo — the popup's
	// philosophy is "every action is immediate".
	static void ApplyKitTagMutation(const std::string& a_kitKey, std::function<void(std::vector<std::string>&)> a_mutate)
	{
		auto* kit = EquipmentConfig::KitLookup(a_kitKey);
		if (!kit) return;
		auto tags = kit->GetTags();
		a_mutate(tags);
		// Dedupe + canonical ordering.
		std::sort(tags.begin(), tags.end());
		tags.erase(std::unique(tags.begin(), tags.end()), tags.end());
		kit->SetTags(tags);
		EquipmentConfig::SaveKit(*kit);
	}

	void UIPopupKitTags::Draw()
	{
		const float  font     = ImGui::GetFontSize();
		const ImVec2 viewport = ImGui::GetMainViewport()->Size;
		const ImVec2 size(font * 32.0f, font * 28.0f);
		const ImVec2 pos((viewport.x - size.x) * 0.5f, (viewport.y - size.y) * 0.5f);

		DrawPopupBackground(m_alpha);

		ImGui::SetNextWindowSize(size);
		ImGui::SetNextWindowPos(pos);
		ImGui::SetNextWindowFocus();

		ImGui::PushStyleVar(ImGuiStyleVar_Alpha, m_alpha);
		if (ImGui::Begin("##Modex::KitTagsPopup", nullptr,
				ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
				ImGuiWindowFlags_NoMove     | ImGuiWindowFlags_NoScrollbar |
				ImGuiWindowFlags_NoFocusOnAppearing)) {

			if (ImGui::IsWindowAppearing()) {
				ImGui::GetIO().ClearInputKeys();
			}

			// Fresh lookup each frame — SaveKit replaces entries in the cache.
			auto* kit = EquipmentConfig::KitLookup(m_kitKey);

			if (UICustom::Popup_MenuHeader(Translate("KIT_TAGS_TITLE"))) {
				CloseWindow();
			}

			ImGui::TextDisabled("%s", kit ? kit->GetNameTail().c_str() : m_kitKey.c_str());
			ImGui::Separator();

			const float footer_h =
				ImGui::GetFrameHeightWithSpacing() * 2.0f +
				ImGui::GetStyle().ItemSpacing.y * 2.0f;
			const float list_h = (std::max)(ImGui::GetContentRegionAvail().y - footer_h, font * 4.0f);

			// Current tags (which checkboxes are checked) + known tags (which
			// checkboxes exist) — rendering reads both, writes hit disk immediately.
			const auto kit_tags_vec = kit ? kit->GetTags() : std::vector<std::string>{};
			const std::unordered_set<std::string> kit_tags(kit_tags_vec.begin(), kit_tags_vec.end());
			const auto known_vec = EquipmentConfig::GetKnownTags();
			const std::set<std::string> visible(known_vec.begin(), known_vec.end());

			if (visible.empty()) {
				if (ImGui::BeginChild("##KitTags::Empty", ImVec2(0.0f, list_h), true)) {
					ImGui::TextDisabled("%s", Translate("KIT_TAGS_NONE_KNOWN"));
				}
				ImGui::EndChild();
			} else {
				if (ImGui::BeginChild("##KitTags::List", ImVec2(0.0f, list_h), true)) {
					const ImU32 disabled_col = ImGui::GetColorU32(ImGuiCol_TextDisabled);
					const float trash_w = ImGui::GetFrameHeight();

					for (const auto& tag : visible) {
						ImGui::PushID(tag.c_str());

						bool checked = kit_tags.contains(tag);
						if (ImGui::Checkbox(tag.c_str(), &checked)) {
							ApplyKitTagMutation(m_kitKey, [&](std::vector<std::string>& a_tags) {
								if (checked) {
									a_tags.push_back(tag);
								} else {
									a_tags.erase(std::remove(a_tags.begin(), a_tags.end(), tag), a_tags.end());
								}
							});
							NotifyChanged();
						}

						ImGui::SameLine(ImGui::GetContentRegionAvail().x - trash_w + ImGui::GetStyle().ItemSpacing.x);
						ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
						ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ThemeConfig::GetHover("DECLINE"));
						ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ThemeConfig::GetActive("DECLINE"));
						ImGui::PushStyleColor(ImGuiCol_Text,          disabled_col);
						if (ImGui::Button(ICON_LC_TRASH_2, ImVec2(trash_w, 0.0f))) {
							EquipmentConfig::DeleteTagFromAllKits(tag);
							NotifyChanged();
						}
						ImGui::PopStyleColor(4);

						ImGui::PopID();
					}
				}
				ImGui::EndChild();
			}

			ImGui::Separator();

			// Add-new-tag row. Enter or + saves the typed tag onto this kit
			// immediately and the new checkbox appears on the next frame.
			const float add_w = font * 4.0f;
			ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - add_w - ImGui::GetStyle().ItemSpacing.x);
			const bool submitted = ImGui::InputTextWithHint(
				"##KitTags::NewTag",
				Translate("KIT_TAGS_NEW_HINT"),
				m_pendingTagInput, sizeof(m_pendingTagInput),
				ImGuiInputTextFlags_EnterReturnsTrue);
			ImGui::SameLine();
			const bool clicked_add = ImGui::Button(ICON_LC_PLUS, ImVec2(add_w, 0.0f));
			if (submitted || clicked_add) {
				std::string candidate = m_pendingTagInput;
				size_t s = 0, e = candidate.size();
				while (s < e && std::isspace(static_cast<unsigned char>(candidate[s]))) s++;
				while (e > s && std::isspace(static_cast<unsigned char>(candidate[e - 1]))) e--;
				candidate = candidate.substr(s, e - s);
				if (!candidate.empty() && candidate.find(',') == std::string::npos) {
					ApplyKitTagMutation(m_kitKey, [&](std::vector<std::string>& a_tags) {
						a_tags.push_back(candidate);
					});
					NotifyChanged();
					m_pendingTagInput[0] = '\0';
				}
				if (submitted) {
					ImGui::SetKeyboardFocusHere(-1);
				}
			}

			ImGui::Separator();

			// Confirm is a pseudo-close — all edits already persisted.
			ImGui::PushStyleColor(ImGuiCol_Button,        ThemeConfig::GetColor("CONFIRM"));
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ThemeConfig::GetHover("CONFIRM"));
			ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ThemeConfig::GetActive("CONFIRM"));
			if (ImGui::Button(Translate("CONFIRM"), ImVec2(ImGui::GetContentRegionAvail().x, 0.0f))) {
				CloseWindow();
			}
			ImGui::PopStyleColor(3);
		}
		ImGui::End();
		ImGui::PopStyleVar();
	}

	void UIPopupKitTags::PopupKitTags(const std::string& a_kitKey, std::function<void()> a_onChanged)
	{
		m_kitKey = a_kitKey;
		m_onChanged = std::move(a_onChanged);
		m_pendingTagInput[0] = '\0';
		m_captureInput = true;
	}

	void UIPopupKitTags::NotifyChanged()
	{
		if (m_onChanged) m_onChanged();
	}
}
