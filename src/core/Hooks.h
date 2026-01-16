#pragma once

#include <pch.h>

namespace Hooks
{
	class IMenuOpenCloseEvent : public RE::BSTEventSink<RE::MenuOpenCloseEvent>
	{
		IMenuOpenCloseEvent() = default;
		IMenuOpenCloseEvent(const IMenuOpenCloseEvent&) = delete;
		IMenuOpenCloseEvent(IMenuOpenCloseEvent&&) = delete;
		IMenuOpenCloseEvent& operator=(const IMenuOpenCloseEvent&) = delete;
		IMenuOpenCloseEvent& operator=(IMenuOpenCloseEvent&&) = delete;

	public:
		static inline IMenuOpenCloseEvent* GetSingleton()
		{
			static IMenuOpenCloseEvent singleton;
			return std::addressof(singleton);
		}

		RE::BSEventNotifyControl ProcessEvent(const RE::MenuOpenCloseEvent* event, RE::BSTEventSource<RE::MenuOpenCloseEvent>*);
	};

	void Install();
}
