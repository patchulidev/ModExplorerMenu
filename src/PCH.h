#pragma once

// NOLINT

#include "nlohmann/json_fwd.hpp"
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-W"
#pragma clang diagnostic ignored "-Werror"
#include <RE/Skyrim.h>
#include <SKSE/SKSE.h>

#define IMGUI_USER_CONFIG "external/imgui/imgui_user_config.h"
#define MODEX_DEBUG

#include <imgui.h>
#include <imgui_internal.h>
#include <external/imgui/imgui_freetype.h>

#include <SimpleIni.h>
#include <nlohmann/json.hpp>
#include <shared_mutex>
#include <core/PrettyLog.h>
#include <external/magic_enum.hpp>
#include <unordered_set> // for std::unordered_set

#include <spdlog/sinks/basic_file_sink.h>
#pragma clang diagnostic pop

using namespace Modex::PrettyLog;
using namespace std::literals;

using ExclusiveLock = std::mutex;
using Locker = std::lock_guard<ExclusiveLock>;

using SharedLock = std::shared_mutex;
using ReadLocker = std::shared_lock<SharedLock>;
using WriteLocker = std::unique_lock<SharedLock>;

using namespace std::literals;

namespace Modex
{
	// Helper functions for UTF-8/UTF-16 conversion
    inline std::string WideToUTF8(const std::wstring& wstr)
    {
        if (wstr.empty()) return std::string();
        
        int size_needed = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), (int)wstr.size(), nullptr, 0, nullptr, nullptr);
        std::string result(size_needed, 0);
        WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), (int)wstr.size(), &result[0], size_needed, nullptr, nullptr);
        
        return result;
    }

    inline std::wstring UTF8ToWide(const std::string& str)
    {
        if (str.empty()) return std::wstring();
        
        int size_needed = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.size(), nullptr, 0);
        std::wstring result(size_needed, 0);
        MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.size(), &result[0], size_needed);
        
        return result;
    }
}

namespace stl
{
	using namespace SKSE::stl;

	template <class T>
	void write_thunk_call(std::uintptr_t a_src)
	{
		SKSE::AllocTrampoline(14);
		auto& trampoline = SKSE::GetTrampoline();
		T::func = trampoline.write_call<5>(a_src, T::thunk);
	}

	template <class T>
	void write_thunk_call_6(std::uintptr_t a_src)
	{
		SKSE::AllocTrampoline(14);
		auto& trampoline = SKSE::GetTrampoline();
		T::func = *(uintptr_t*)trampoline.write_call<6>(a_src, T::thunk);
	}

	template <class F, size_t index, class T>
	void write_vfunc()
	{
		REL::Relocation<std::uintptr_t> vtbl{ F::VTABLE[index] };
		T::func = vtbl.write_vfunc(T::size, T::thunk);
	}

	template <std::size_t idx, class T>
	void write_vfunc(REL::VariantID id)
	{
		REL::Relocation<std::uintptr_t> vtbl{ id };
		T::func = vtbl.write_vfunc(idx, T::thunk);
	}

	template <class T>
	void write_thunk_jmp(std::uintptr_t a_src)
	{
		SKSE::AllocTrampoline(14);
		auto& trampoline = SKSE::GetTrampoline();
		T::func = trampoline.write_branch<5>(a_src, T::thunk);
	}

	template <class F, class T>
	void write_vfunc()
	{
		write_vfunc<F, 0, T>();
	}
}
