#include "UINotification.h"

#include "config/UserConfig.h"
#include "config/ThemeConfig.h"
#include "external/icons/IconsLucide.h"
#include "imgui_internal.h"
#include "ui/components/UICustom.h"

#include "imgui.h"

namespace Modex 
{
	void UINotification::ShowTooltip(const std::string& a_text, const std::string& a_icon)
	{
		s_tooltip.text = Translate(a_text.c_str());
		s_tooltip.icon = a_icon.empty() ? ICON_LC_INFO : a_icon;
		s_tooltip.type = UIMessageType::Tooltip;
		s_tooltip.active = true;
		s_tooltip.duration = 1.0f; // Linger Time
		s_tooltip.timestamp = std::chrono::steady_clock::now();
	}

	// Conditionally shows an Object's Primary PROPERTY_TOOLTIP.
	void UINotification::ShowObjectTooltip(const std::unique_ptr<BaseObject>& a_item)
	{
		const auto key = magic_enum::enum_name(a_item->GetItemPropertyType());
		const auto icon = a_item->GetItemIcon();
		const auto tooltip = Translate(key.data());
		if (Locale::GetSingleton()->HasTooltip(key.data())) ShowTooltip(tooltip, icon);
	}

	// Conditionally shows "PROPERTY_TOOLTIP" tooltip with Icon appended.
	void UINotification::ShowPropertyTooltip(PropertyType a_property)
	{
		const auto key = FilterProperty::GetPropertyTooltipKey(a_property);
		const auto icon = FilterProperty::GetIcon(a_property);
		const auto tooltip = Translate(key.c_str());
		if (Locale::GetSingleton()->HasTooltip(key.c_str())) ShowTooltip(tooltip, icon);
	}

	void UINotification::ShowInfo(const std::string& a_text, float a_duration)
	{
		PushMessage(a_text, ICON_LC_MESSAGES_SQUARE, UIMessageType::Info, a_duration);
	}

	void UINotification::ShowAction(const std::string& a_text, float a_duration)
	{
		PushMessage(a_text, ICON_LC_ITERATION_CCW, UIMessageType::Info, a_duration);
	}

	void UINotification::ShowWarning(const std::string& a_text, float a_duration)
	{
		PushMessage(a_text, ICON_LC_OCTAGON_ALERT, UIMessageType::Warning, a_duration);
	}

	void UINotification::ShowError(const std::string& a_text, float a_duration)
	{
		PushMessage(a_text, ICON_LC_TRIANGLE_ALERT, UIMessageType::Error, a_duration);
	}

	void UINotification::ShowAdd(const std::unique_ptr<BaseObject>& a_object, float a_duration)
	{
		const std::string text = a_object->GetName();
		PushMessage(text, ICON_LC_PLUS, UIMessageType::Info, a_duration);
	}

	void UINotification::ShowRemove(const std::unique_ptr<BaseObject>& a_object, float a_duration)
	{
		const std::string text = a_object->GetName();
		PushMessage(text, ICON_LC_MINUS, UIMessageType::Warning, a_duration);
	}

	void UINotification::PushMessage(const std::string& a_text, const std::string& a_icon, UIMessageType a_type, float a_duration)
	{
		if (s_messages.size() >= MAX_QUEUE) {
			s_messages.pop_front();
		}

		s_messages.emplace_back(a_text, a_icon, a_type, a_duration);
	}

	void UINotification::RemoveMessage(size_t a_index)
	{
		if (a_index < s_messages.size()) {
			s_messages.erase(s_messages.begin() + a_index);
		}
	}

	void UINotification::Update()
	{
		s_messages.erase(
				std::remove_if(s_messages.begin(), s_messages.end(),
					[](const UIMessage a_msg) {
					return a_msg.IsExpired();
				}),
		s_messages.end());
	}

	void UINotification::DrawProgressBar(const UIMessage& a_msg, float a_width, float a_height, float a_alpha)
	{
		if (a_msg.duration <= 0.0f) return;

		const float progress = a_msg.GetProgress();
		const float remainingWidth = a_width * (1.0f - progress);

		ImVec2 progress_min = ImGui::GetWindowPos();
		progress_min.y += a_height - 2.0f;
		
		ImVec2 progress_max(progress_min.x + remainingWidth, progress_min.y + 2.0f);

		ImGui::GetWindowDrawList()->AddRectFilled(
				progress_min,
				progress_max,
				ThemeConfig::GetColorU32("TEXT", 0.5f * a_alpha)
		);
	}

	UINotification::Container UINotification::SetupContainer(const ImVec2& a_pos, const ImVec2& a_size, int a_padding)
	{
		ImGui::SetNextWindowPos(a_pos);
		ImGui::SetNextWindowSize(a_size);

		if (a_padding == 0)
			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
		if (a_padding == 1)
			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(2.0f, 2.0f));

		return {a_pos, a_size, WINDOW_FLAGS, a_padding};
	}

	void UINotification::DrawMessageContainer(const ImVec2& a_sidebarPos, const ImVec2& a_sidebarSize, bool a_sidebarExtended)
	{
		Update();

		const size_t MAX_COUNT = 10;
		auto messagesToShow = min(s_messages.size(), MAX_COUNT);

		if (messagesToShow == 0 || !a_sidebarExtended) return;

		const float msg_height = ImGui::GetFontSize();
		const float window_width = a_sidebarSize.x;
		const float window_height = (msg_height * MAX_COUNT);

		const ImVec2 pos(a_sidebarPos.x, a_sidebarPos.y + a_sidebarSize.y - window_height - 2.0f);
		auto config = SetupContainer(pos, ImVec2(window_width, window_height), 1);

		if (ImGui::Begin("##Modex::UIMessageBox", nullptr, config.flags)) {
			for (size_t i = 0; i < messagesToShow; i++) {
				float row = window_height - ((i + 1) * msg_height);
				ImGui::SetCursorPosY(row);

				const auto& msg = s_messages[messagesToShow - 1 - i];
				DrawMessageRow(msg, i, msg_height, window_width);
			}
		}

		ImGui::End();
		ImGui::PopStyleVar();
	}

	void UINotification::DrawTooltipContainer(const ImVec2 &a_parentPos, const ImVec2 &a_parentSize)
	{
		if (!s_tooltip.active) return;

		if (s_tooltip.IsExpired()) {
			ClearTooltip();
			return;
		}

		const float msg_height = ImGui::GetFrameHeight() * 1.5f;
		const float window_width = a_parentSize.x;

		ImGui::PushStyleVar(ImGuiStyleVar_Alpha, s_tooltip.GetAlpha());
		const ImVec2 pos(a_parentPos.x, a_parentPos.y + a_parentSize.y - msg_height);
		auto config = SetupContainer(pos, ImVec2(window_width, msg_height), 1);

		if (ImGui::Begin("##Modex::TooltipBox", nullptr, config.flags)) {
			DrawTooltip(s_tooltip, msg_height, window_width);
		}
		
		ImGui::End();
		ImGui::PopStyleVar(2);
	}

	void UINotification::DrawMessageRow(const UIMessage &a_msg, size_t a_index, float a_height, float a_width)
	{
		if (!a_msg.active) return;

		float alpha = a_msg.GetAlpha();
		ImU32 bgColor = GetMessageColor(a_msg.type);
		
		ImGui::PushID(static_cast<int>(a_index));
		ImGui::PushStyleColor(ImGuiCol_ChildBg, bgColor);
		ImGui::PushStyleVar(ImGuiStyleVar_Alpha, alpha);

		if (ImGui::BeginChild("##UIMessage::Message", ImVec2(a_width, a_height), 0, ImGuiWindowFlags_NoScrollbar)) {

			// Center Text Vertically
			ImGui::SetCursorPosY((a_height / 2.0f) - (a_height / 2.0f));
			ImGui::SetCursorPosX(ImGui::GetStyle().WindowPadding.x + 2.0f);

			// Icon
			if (!a_msg.icon.empty()) {
				ImGui::Text("%s", a_msg.icon.c_str());
				ImGui::SameLine(ImGui::GetFrameHeight());
			}

			// Message
			ImGui::Text("%s", TRUNCATE(a_msg.text.c_str(), a_width / 1.5f).c_str());

			// TODO: Add an option to enable/disable this:
			// DrawProgressBar(a_msg, a_width, a_height, alpha);

		}

		ImGui::EndChild();
		ImGui::PopStyleVar();
		ImGui::PopStyleColor();
		ImGui::PopID();
	}

	void UINotification::DrawTooltip(const UIMessage& a_msg, float a_height, float a_width)
	{
		if (!a_msg.active) return;

		ImU32 bgColor = GetMessageColor(a_msg.type);
		ImGui::PushStyleColor(ImGuiCol_ChildBg, bgColor);

		if (ImGui::BeginChild("##UIMessage::Tooltip", ImVec2(a_width, a_height), 0, ImGuiWindowFlags_NoScrollbar)) {
			// ImGui::SetCursorPosY((a_height - ImGui::GetFontSize()) / 2.0f);

			// Enlarge Tooltip Icon
			if (!a_msg.icon.empty()) {
				ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetFrameHeight() / 2.0f);
				ImGui::PushFont(NULL, ImGui::GetFontSize() + 4.0f);
				ImGui::SetCursorPosY((a_height / 2.0f) - (ImGui::CalcTextSize(a_msg.icon.c_str()).y / 2.0f));
				ImGui::Text("%s", a_msg.icon.c_str());
				ImGui::PopFont();
				ImGui::SameLine();
			}

			// Centered Text
			const float center_x = UICustom::GetCenterTextPosX(a_msg.text);
			ImGui::SetCursorPosY((a_height / 2.0f) - (ImGui::GetFontSize() / 2.0f));
			ImGui::SetCursorPosX(center_x);
			ImGui::Text("%s", a_msg.text.c_str());
		}

		ImGui::EndChild();
		ImGui::PopStyleColor();
	}

	ImU32 UINotification::GetMessageColor(UIMessageType a_type)
	{
		switch (a_type) {
		case UIMessageType::Info:
			return ThemeConfig::GetColorU32("MSG_INFO");
		case UIMessageType::Warning:
			return ThemeConfig::GetColorU32("MSG_WARN");
		case UIMessageType::Error:
			return ThemeConfig::GetColorU32("MSG_ERROR");
		case UIMessageType::Tooltip:
			return ThemeConfig::GetColorU32("TOOLTIP");
		default:
			return ImGui::GetColorU32(ImGuiCol_Text);
		}
	}
}
