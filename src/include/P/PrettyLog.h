#pragma once

namespace PrettyLog
{
    inline int error_count = 0;
    inline int warn_count = 0;

    template <typename... Args>
    void Error(std::string_view a_msg, Args&&... a_args)
    {
        if constexpr (sizeof...(a_args) > 0) {
            SKSE::log::error("ERROR: {}", std::vformat(a_msg, std::make_format_args(a_args...)));
        } else {
            SKSE::log::error("ERROR: {}", a_msg);
        }

        error_count++;
    }

    template <typename... Args>
    void Warn(std::string_view a_msg, Args&&... a_args)
    {
        if constexpr (sizeof...(a_args) > 0) {
            SKSE::log::warn("WARNING: {}", std::vformat(a_msg, std::make_format_args(a_args...)));
        } else {
            SKSE::log::warn("WARNING: {}", a_msg);
        }

        warn_count++;
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