#pragma once

#include <pch.h>

namespace Modex
{
	inline std::filesystem::path PathFromUtf8(std::string_view a_utf8)
	{
		auto wide = SKSE::stl::utf8_to_utf16(a_utf8);
		if (!wide) return {};
		return std::filesystem::path(*wide);
	}

	inline std::string PathToUtf8(const std::filesystem::path& a_path)
	{
		auto utf8 = SKSE::stl::utf16_to_utf8(a_path.wstring());
		return utf8.value_or(std::string{});
	}
}
