#pragma once

// NOLINT

#include "nlohmann/json_fwd.hpp"
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-W"
#pragma clang diagnostic ignored "-Werror"
#include <RE/Skyrim.h>
#include <SKSE/SKSE.h>

// #define MODEX_DEBUG

#define IMGUI_DISABLE_DEMO_WINDOWS
#define IMGUI_DISABLE_DEBUG_TOOLS
#define IMGUI_DISABLE_OBSOLETE_FUNCTIONS
#define IMGUI_ENABLE_FREETYPE
#define IMGUI_DEFINE_MATH_OPERATORS

#include <imgui.h>
#include <imgui_internal.h>
#include <imgui_freetype.h>

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
	enum class Ownership : uint32_t 
	{
		None = 0,
		Item,
		Actor,
		Kit,
		Object,
		Cell,
		Outfit,
		All
	};

	enum class PluginSort : uint32_t 
	{
		Alphabetical = 0,
		Load_Order_Ascending,
		Load_Order_Descending,
		kTotal,
	};
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
