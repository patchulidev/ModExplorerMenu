#include "SettingsModule.h"
#include "data/Data.h"
#include "config/BlacklistConfig.h"
#include "external/icons/IconsLucide.h"
#include "ui/components/UICustom.h"
#include "localization/Locale.h"
#include "localization/FontManager.h"

namespace Modex
{
	void SettingsModule::DrawBlacklistSettings()
	{
		ImGui::NewLine();
		ImGui::Indent();
		ImGui::PushTextWrapPos(ImGui::GetContentRegionAvail().x - ImGui::GetStyle().IndentSpacing);
		ImGui::TextWrapped("%s", Translate("BLACKLIST_DESCRIPTION"));
		ImGui::PopTextWrapPos();
		ImGui::Unindent();
		ImGui::NewLine();

		const float separator_width = ImGui::CalcTextSize(ICON_LC_ARROW_RIGHT).x + ImGui::GetStyle().ItemSpacing.x;
		const float widget_width = ((ImGui::GetContentRegionAvail().x - (2.0f) * separator_width) / 3.0f);
		UICustom::FancyInputText("##Blacklist::ModSearch", Translate("TABLE_SEARCH_HINT"), "PLUGIN_SEARCH_TOOLTIP", m_modSearchBuffer, widget_width);

		if (ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_F, ImGuiInputFlags_RouteAlways)) {
			ImGui::SetKeyboardFocusHere(-1);
		}

		ImGui::SameLine();
		ImGui::AlignTextToFramePadding();
		ImGui::Text(ICON_LC_ARROW_RIGHT);
		ImGui::SameLine();

		std::vector<std::string> pluginOptions = Data::GetTypeString();
		if (UICustom::FancyDropdown("##Blacklist::TypeDropdown", "PLUGIN_TYPE_TOOLTIP", m_type, pluginOptions, widget_width)) {
			BuildBlacklistPlugins();
		}

		ImGui::SameLine();
		ImGui::AlignTextToFramePadding();
		ImGui::Text(ICON_LC_ARROW_RIGHT);
		ImGui::SameLine();

		std::vector<std::string> sortOptions = Data::GetSortStrings();
		if (UICustom::FancyDropdown("##Blacklist::SortDropdown", "SORT_TYPE_TOOLTIP", m_sort, sortOptions, 0.0f)) { 
			BuildBlacklistPlugins();
		}

		std::string pluginFilter = m_modSearchBuffer;
		std::transform(pluginFilter.begin(), pluginFilter.end(), pluginFilter.begin(),
			[](unsigned char c) { return static_cast<char>(std::tolower(c)); });

		ImGui::NewLine();

		// Left and Right Sections
		const auto& blacklist = BlacklistConfig::Get();
		ImGui::BeginChild("##Blacklist::LeftContainer", ImVec2(ImGui::GetContentRegionAvail().x / 2, 0), false, ImGuiWindowFlags_NoFocusOnAppearing);
		{
			ImGui::PushFontBold();
			UICustom::SubCategoryHeader(Translate("WHITELIST"));
			ImGui::PopFont();

			ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal);

			for (const auto& plugin : m_pluginList) {
				const std::string pluginName = plugin->GetFilename().data();
				auto pluginNameLower = pluginName;

				std::transform(pluginNameLower.begin(), pluginNameLower.end(), pluginNameLower.begin(),
					[](unsigned char c) { return static_cast<char>(std::tolower(c)); });

				if (pluginNameLower.find(pluginFilter) == std::string::npos) {
					continue;
				}

				if (BlacklistConfig::GetSingleton()->Has(plugin) == true) {
					continue;
				}

				if (ImGui::Selectable(pluginName.c_str())) {
					BlacklistConfig::GetSingleton()->AddPluginToBlacklist(plugin);
				}
			}
		}
		ImGui::EndChild();

		ImGui::SameLine();
		ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
		ImGui::SameLine();

		ImGui::BeginChild("##Blacklist::RightContainer", ImVec2(ImGui::GetContentRegionAvail().x, 0.0f), false, ImGuiWindowFlags_NoFocusOnAppearing);
		{
			ImGui::PushFontBold();
			UICustom::SubCategoryHeader(Translate("BLACKLIST"));
			ImGui::PopFont();

			ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal);

			for (const auto& plugin : m_pluginList) {
				const std::string pluginName = plugin->GetFilename().data();
				auto pluginNameLower = pluginName;

				std::transform(pluginNameLower.begin(), pluginNameLower.end(), pluginNameLower.begin(),
					[](unsigned char c) { return static_cast<char>(std::tolower(c)); });

				if (pluginNameLower.find(pluginFilter) == std::string::npos) {
					continue;
				}

				if (BlacklistConfig::GetSingleton()->Has(plugin) == false) {
					continue;
				}

				if (ImGui::Selectable(pluginName.c_str())) {
					BlacklistConfig::GetSingleton()->RemovePluginFromBlacklist(plugin);
				}
			}
		}

		ImGui::EndChild();
	}
}
