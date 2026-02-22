#pragma once

#include "data/BaseObject.h"

// TODO: Expand notification display and configuration.

namespace Modex
{
	enum class UIMessageType : uint8_t
	{
		None = 0,
		Tooltip,
		Info,
		Warning,
		Error
	};

	class UINotification 
	{
	private:
		struct UIMessage
		{
			std::string text;
			std::string icon;
			UIMessageType type;
			std::chrono::steady_clock::time_point timestamp;
			float duration;
			bool active;

			// Default Tooltip Constructor
			UIMessage()
				: text("")
				, icon("")
				, type(UIMessageType::None)
				, timestamp(std::chrono::steady_clock::now())
				, duration(0.0f)
				, active(false)
			{}

			// Notification Message Constructor
			UIMessage(const std::string& a_text, const std::string& a_icon, UIMessageType a_type, float a_duration)
				: text(a_text)
				, icon(a_icon)
				, type(a_type)
				, timestamp(std::chrono::steady_clock::now())
				, duration(a_duration)
				, active(true)
			{}

			float GetElapsed() const {
				if (!active || duration < 0.0f) return 0.0f;

				auto now = std::chrono::steady_clock::now();
				auto time = std::chrono::duration_cast<std::chrono::milliseconds>(now - timestamp);
				auto elapsed = static_cast<float>(time.count()) / 1000.0f;

				return elapsed;
			}

			bool IsExpired() const {
				if (!active) return false;
				if (type == UIMessageType::Tooltip) return false;
				if (duration <= 0.0f) return false;

				return GetElapsed() >= duration;
			}

			float GetProgress() const {
				if (!active || duration <= 0.0f) return 0.0f;
				return min(GetElapsed() / duration, 1.0f);
			}

			// Fade out
			float GetAlpha() const {
				if (!active) return 0.0f;

				// if (type == UIMessageType::Tooltip) return 1.0f;

				float progress = GetProgress();

				if (progress > 0.8f) {
					return 1.0f - ((progress - 0.8f) / 0.2f);
				}

				return 1.0f;
			}

			void Clear()
			{
				text.clear();
				icon.clear();
				type = UIMessageType::None;
				active = false;
			}
		};

		struct Container
		{
			ImVec2 position;
			ImVec2 size;
			ImGuiWindowFlags flags;
			int stylePadding;
		};

		static inline constexpr auto WINDOW_FLAGS =
			ImGuiWindowFlags_NoTitleBar |
			ImGuiWindowFlags_NoResize |
			ImGuiWindowFlags_NoMove |
			ImGuiWindowFlags_NoScrollbar |
			ImGuiWindowFlags_NoBackground |
			ImGuiWindowFlags_NoFocusOnAppearing |
			ImGuiWindowFlags_NoNav;

		static inline std::deque<UIMessage> s_messages;
		static inline UIMessage s_tooltip;
		static inline constexpr size_t MAX_QUEUE = 10;
		static inline constexpr size_t MAX_DISPLAY = 1;

		UINotification() = delete;
		~UINotification() = delete;

	public:
		// Tooltip specific impl
		static void ShowTooltip(const std::string& a_text, const std::string& a_icon = "");
		static void ShowObjectTooltip(const std::unique_ptr<BaseObject>& a_object);
		static void ShowPropertyTooltip(PropertyType a_property);
		static void ClearTooltip() { s_tooltip.Clear(); }
		
		// Helpers to push/set messages based on context.
		static void ShowAction(const std::string& a_action, const std::string& a_target, const char* a_icon = ICON_LC_ITERATION_CCW, UIMessageType a_type = UIMessageType::Info, float a_duration = 3.0f);
		static void ShowError(const std::string& a_text, float a_duration = 3.0f);

		// Container & Message Rendering
		static void DrawTooltip(const UIMessage& a_msg, float a_height, float a_width);
		static void DrawTooltipContainer(const ImVec2& a_parentPos, const ImVec2& a_parentSize);
		static void DrawMessageContainer(const ImVec2& a_startPos, const ImVec2& a_windowSize);
		static ImU32 GetMessageColor(UIMessageType a_type, float a_alpha = 1.0f);

		// Core
		static void Update();
		static void RemoveMessage(size_t a_index);
		static void ClearAll() { s_messages.clear(); }
	
	private:
		static void PushMessage(const std::string& a_text, const std::string& a_icon, UIMessageType a_type, float a_duration);
		static void DrawProgressBar(const UIMessage& a_msg, float a_width, float a_height, float a_alpha);

		static Container SetupContainer(const ImVec2& a_pos, const ImVec2& a_size, int a_padding);
		static void DrawMessageRow(const UIMessage& a_msg, size_t a_index, float a_height, float a_width);

		static const UIMessage GetActiveMessage();
	};
}
