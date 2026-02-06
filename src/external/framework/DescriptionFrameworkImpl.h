#pragma once

#include "external/framework/DescriptionFrameworkAPI.h"

// Source: Nightfallstorm | License: GPL-3.0
// https://github.com/Nightfallstorm/DescriptionFramework

namespace DescriptionFramework_Impl
{
	static DescriptionFrameworkAPI::IDescriptionFrameworkInterface001* g_DescriptionFrameworkInterface = nullptr;

	inline static void SetDescriptionFrameworkInterface(DescriptionFrameworkAPI::IDescriptionFrameworkInterface001* a_interface)
	{
		g_DescriptionFrameworkInterface = a_interface;
	}

	static void RemoveHTMLTags(std::string& a_string)
	{
		size_t pos = 0;
		while ((pos = a_string.find('<', pos)) != std::string::npos) {
			size_t endpos = a_string.find('>', pos);
			if (endpos != std::string::npos) {
				a_string.erase(pos, endpos - pos + 1);
			}
		}
	}

	static void FormatHTMLTag(std::string& a_string, char open_char, char close_char)
	{
		size_t startpos = 0;
		while ((startpos = a_string.find(open_char, startpos)) != std::string::npos) {
			a_string.erase(startpos, 1);  // Remove the opening bracket
		}

		size_t endpos = 0;
		while ((endpos = a_string.find(close_char, endpos)) != std::string::npos) {
			a_string.erase(endpos, 1);  // Remove the closing bracket
		}
	}

	static std::string FormatSpellTag(std::string& a_input, const std::string& a_tag, int a_value)
	{
		size_t pos = a_input.find(a_tag);
		if (pos != std::string::npos) {
			a_input.replace(pos, a_tag.length(), std::to_string(static_cast<int>(a_value)));
		}
		
		return a_input;
	}

	static void FixDescriptionEncoding(std::string& a_desc)
	{
		size_t pos = 0;

		while ((pos = a_desc.find("%", pos)) != std::string::npos) {
			a_desc.replace(pos, 1, "%%");
			pos += 2;  // Move past the '%%'
		}
	}

	// This entirely removes HTML tags in Descriptions from Description Framework
	// I did not see an instance where a Tag was necessary, so if there is
	// This will need modified to only remove unecessary HTML tags.
	[[nodiscard]] static std::string GetItemDescription(RE::TESForm* form)
	{
		std::string desc;

		// Initial Description Framework lookup.
		if (g_DescriptionFrameworkInterface != nullptr) {
            desc = g_DescriptionFrameworkInterface->GetDescription(form);
			if (!desc.empty()) {
                RemoveHTMLTags(desc);
				return desc;
			}
		}

		// Fallback to Description found in TESForm.
		if (auto tesDescription = form->As<RE::TESDescription>()) {
            RE::BSString buf;
			tesDescription->GetDescription(buf, nullptr);
			desc = buf.c_str();
			FixDescriptionEncoding(desc);
			FormatHTMLTag(desc, '[', ']');
			FormatHTMLTag(desc, '<', '>');
		}

		// Additionally, fallback to the Base MGEF description on enchanted weapons.
		if (const auto enchantable = form->As<RE::TESEnchantableForm>(); enchantable != nullptr) {
			if (enchantable->formEnchanting == nullptr) return desc;

			if (const auto magicItem = enchantable->formEnchanting->As<RE::MagicItem>(); magicItem != nullptr) {
				if (const auto effectItem = magicItem->GetAVEffect(); effectItem != nullptr) {
					auto costliest = magicItem->GetCostliestEffectItem();
					int magnitude = costliest ? static_cast<int>(costliest->GetMagnitude()) : 0;
					int duration = costliest ? costliest->GetDuration() : 0;

					desc = effectItem->magicItemDescription.c_str();
					FormatSpellTag(desc, "<mag>", magnitude);
					FormatSpellTag(desc, "<dur>", duration);

					return desc;
				}
			}
		}

		return desc;
	}
}
