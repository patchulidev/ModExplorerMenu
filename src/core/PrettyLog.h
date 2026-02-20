#pragma once

#include <pch.h>

#ifdef _WIN32
#include <windows.h>
#include <dbghelp.h>
#pragma comment(lib, "dbghelp.lib")
namespace Modex::PrettyLog
{
	inline std::string GetStackTrace(int skip_frames = 0)
	{
		constexpr int MAX_FRAMES = 32;
		void* stack[MAX_FRAMES];
		
		HANDLE process = GetCurrentProcess();
		SymInitialize(process, NULL, TRUE);
		
		WORD frames = CaptureStackBackTrace(skip_frames, MAX_FRAMES, stack, NULL);
		
		std::string trace;
		char buffer[sizeof(SYMBOL_INFO) + MAX_SYM_NAME * sizeof(TCHAR)];
		PSYMBOL_INFO symbol = (PSYMBOL_INFO)buffer;
		symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
		symbol->MaxNameLen = MAX_SYM_NAME;
		
		for (WORD i = 0; i < frames; i++) {
			if (SymFromAddr(process, (DWORD64)(stack[i]), nullptr, symbol)) {
				trace += std::format("  #{} {}\n", i, symbol->Name);
			}
		}
		
		SymCleanup(process);
		return trace.empty() ? "  (stack trace unavailable)\n" : trace;
	}
}
#endif

namespace Modex::PrettyLog
{
	inline int error_count = 0;
	inline int warn_count = 0;

#ifdef _NDEBUG
	template <typename... Args>
	std::string Assert(bool a_condition, std::string_view a_msg, const char* file, int line, const char* function, Args&&... a_args)
	{
		if (!a_condition) [[likely]] {
			return std::string();
		}
		
		std::string formatted_msg;
		if constexpr (sizeof...(Args) > 0) {
			formatted_msg = std::vformat(a_msg, std::make_format_args(a_args...));
		} else {
			formatted_msg = std::string(a_msg);
		}
		
		std::string stack_info = "\n\nCall Stack:\n" + GetStackTrace(2);
		
		std::string full_error = std:: format(
			"ASSERTION FAILED:\n\n{}\n\n File: {}\n Line: {}\n Function: {}{}",
			formatted_msg, file, line, function, stack_info);
		
		SKSE::log::error("{}", full_error);
		
		error_count++;
		return full_error;
	}
#else
	template <typename... Args>
	std::string Assert(bool a_condition, std::string_view a_msg, Args&&... a_args)
	{
		if (!a_condition) [[likely]] {
			return std::string();
		}
		
		std::string formatted_msg;
		if constexpr (sizeof...(Args) > 0) {
			formatted_msg = std::vformat(a_msg, std::make_format_args(a_args...));
		} else {
			formatted_msg = std::string(a_msg);
		}
		
		std::string full_error = std:: format("ASSERTION FAILED: {}", formatted_msg);
		SKSE::log::error("{}", full_error);
		
		error_count++;
		return full_error;
	}
#endif

	template <typename... Args>
	bool Error(std::string_view a_msg, Args&&... a_args)
	{
		if constexpr (sizeof...(a_args) > 0) {
			SKSE::log::error("ERROR: {}", std::vformat(a_msg, std::make_format_args(a_args...)));
		} else {
			SKSE::log::error("ERROR: {}", a_msg);
		}

		error_count++;

		return false;
	}

	template <typename... Args>
	bool Warn(std::string_view a_msg, Args&&... a_args)
	{
		if constexpr (sizeof...(a_args) > 0) {
			SKSE::log::warn("WARN: {}", std::vformat(a_msg, std::make_format_args(a_args...)));
		} else {
			SKSE::log::warn("WARN: {}", a_msg);
		}

		warn_count++;

		return false;
	}

	template <typename... Args>
	void Info(std::string_view a_msg, Args&&... a_args)
	{
		if constexpr (sizeof...(a_args) > 0) {
			SKSE::log::info("OK: {}", std::vformat(a_msg, std::make_format_args(a_args...)));
		} else {
			SKSE::log::info("OK: {}", a_msg);
		}
	}

	template <typename... Args>
	void Debug(std::string_view a_msg, Args&&... a_args)
	{
		if constexpr (sizeof...(a_args) > 0) {
			SKSE::log::debug("DBG: {}", std::vformat(a_msg, std::make_format_args(a_args...)));
		} else {
			SKSE::log::debug("DBG: {}", a_msg);
		}
	}

	template <typename... Args>
	void Trace(std::string_view a_msg, Args&&... a_args)
	{
		if constexpr (sizeof...(a_args) > 0) {
			SKSE::log::trace("TRC: {}", std::vformat(a_msg, std::make_format_args(a_args...)));
		} else {
			SKSE::log::trace("TRC: {}", a_msg);
		}
	}

	static void ReportSummary()
	{
		if (error_count > 0) {
			SKSE::log::error("=== *** Summary *** ===");
			SKSE::log::error("Total Errors: {}", error_count);
			SKSE::log::error("Total Warnings: {}", warn_count);
			SKSE::log::error("Please check the log for more details.");
		} else if (warn_count > 0) {
			SKSE::log::warn("=== *** Summary *** ===");
			SKSE::log::warn("Total Warnings: {}", warn_count);
			SKSE::log::warn("No critical errors detected.");
		} else {
			SKSE::log::info("=== *** Summary *** ===");
			SKSE::log::info("No errors or warnings detected.");
		}
	}
}

#ifndef _NDEBUG 
#	define ASSERT_MSG(condition, msg, ...)          \
    PrettyLog::Assert(condition, msg, __VA_ARGS__)
#else
#define ASSERT_MSG(condition, msg, ...) \
	if (auto _assert_msg = PrettyLog::Assert((condition), (msg), __FILE__, __LINE__, __FUNCTION__, ##__VA_ARGS__); !_assert_msg.empty()) { \
		stl::report_and_fail(_assert_msg.c_str()); \
	}
#endif
