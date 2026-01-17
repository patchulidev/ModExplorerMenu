// #include "include/U/UserSettings.h"
// #include "include/S/Settings.h"
// #include "include/P/Persistent.h"
// #include "include/D/Data.h"
// #include "include/U/Util.h"

#include "SettingsModule.h"

#include "data/Data.h"
#include "config/BlacklistConfig.h"
#include "ui/components/UICustom.h"
#include "localization/Locale.h"

namespace Modex
{
	void SettingsModule::DrawBlacklistSettings()
	{
		const auto& blacklist = BlacklistConfig::Get();

		m_totalBlacklisted = static_cast<int>(blacklist.size());

		UICustom::SubCategoryHeader(Translate("SETTING_BLACKLIST"));
		ImGui::NewLine();
		ImGui::BeginColumns("##Blacklist::Columns", 2, ImGuiOldColumnFlags_NoBorder);
		ImGui::Indent();

		// FormType Filter Box.
		ImGui::Text("%s", TranslateFormat("GENERAL_FILTER_FORMTYPE", ":"));
		const auto primary_filter_text = RE::FormTypeToString(m_primaryFilter);
		if (ImGui::BeginCombo("##Blacklist::PluginType", primary_filter_text.data())) {
			if (ImGui::Selectable(Translate("None"), m_primaryFilter == RE::FormType::None)) {
				m_updateHidden = true;
				m_totalHidden = 0;
				m_primaryFilter = RE::FormType::None;
				ImGui::SetItemDefaultFocus();
			}

			// TODO: REFACTOR post removal of Utils namespace
			// for (auto& filter : Utils::GetHandledFormTypes()) {
			// 	bool isSelected = (filter == m_primaryFilter);

			// 	if (ImGui::Selectable(Translate(RE::FormTypeToString(filter).data()), isSelected)) {
			// 		m_primaryFilter = filter;
			// 		m_updateHidden = true;
			// 		m_totalHidden = 0;
			// 	}

			// 	if (isSelected) {
			// 		ImGui::SetItemDefaultFocus();
			// 	}
			// }

			ImGui::EndCombo();
		}

		const auto FilterModByRecord = [](const RE::TESFile* mod, RE::FormType formType) {
			if (formType == RE::FormType::None) {
				return true;
			}

			return Data::GetSingleton()->IsFormTypeInPlugin(mod, formType);
		};

		ImGui::NewLine();

		// Plugin Name Fuzzy Search
		ImGui::Text("%s", TranslateFormat("GENERAL_FILTER_FUZZY", ":"));
		if (ImGui::InputTextWithHint("##Blacklist::ModSearch", Translate("GENERAL_CLICK_TO_TYPE"), m_modSearchBuffer, IM_ARRAYSIZE(m_modSearchBuffer))) {
			m_updateHidden = true;
			m_totalHidden = 0;
		}

		std::string pluginFilter = m_modSearchBuffer;
		std::transform(pluginFilter.begin(), pluginFilter.end(), pluginFilter.begin(),
			[](unsigned char c) { return static_cast<char>(std::tolower(c)); });

		ImGui::NewLine();
		ImGui::Unindent();
		ImGui::NextColumn();
		ImGui::Indent();

		// Plugin Count
		ImGui::Text(TranslateFormat("GENERAL_TOTAL_PLUGINS", ": %d"), m_totalPlugins);
		ImGui::Text(TranslateFormat("GENERAL_TOTAL_BLACKLIST", ": %d"), m_totalBlacklisted);

		if (m_totalHidden > 0) {
			ImGui::TextColored(ImVec4(0.9f, 0.1f, 0.1f, 1.0f), TranslateFormat("GENERAL_TOTAL_HIDDEN", ": %d"), m_totalHidden);
			ImGui::TextColored(ImVec4(0.9f, 0.1f, 0.1f, 1.0f), "%s", Translate("GENERAL_TOTAL_HIDDEN_MESSAGE"));
		} else {
			ImGui::Text(TranslateFormat("GENERAL_TOTAL_HIDDEN", ": %d"), m_totalHidden);
		}

		ImGui::Unindent();

		ImGui::EndColumns();

		// Left and Right Sections
		ImGui::BeginChild("##Blacklist::LeftBox", ImVec2(ImGui::GetContentRegionAvail().x / 2, 0), true, ImGuiWindowFlags_NoFocusOnAppearing);
		{
			UICustom::SubCategoryHeader(Translate("Whitelist"));

			ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal);
			ImGui::NewLine();

			for (const auto& plugin : m_pluginList) {
				auto pluginName = ValidateTESFileName(plugin);
				auto pluginNameLower = pluginName;

				std::transform(pluginNameLower.begin(), pluginNameLower.end(), pluginNameLower.begin(),
					[](unsigned char c) { return static_cast<char>(std::tolower(c)); });

				if (pluginNameLower.find(pluginFilter) == std::string::npos) {
					if (m_updateHidden) {
						m_totalHidden++;
					}
					continue;
				}

				if (!FilterModByRecord(plugin, m_primaryFilter)) {
					if (m_updateHidden) {
						m_totalHidden++;
					}
					continue;
				}

				if (blacklist.find(plugin) != blacklist.end()) {
					continue;
				}

				if (ImGui::Selectable(pluginName.c_str())) {
					BlacklistConfig::GetSingleton()->AddPluginToBlacklist(plugin);
				}
			}
		}

		ImGui::EndChild();

		ImGui::SameLine(0.0f, -1.0f);

		ImGui::BeginChild("##Blacklist::RightBox", ImVec2(ImGui::GetContentRegionAvail().x, 0.0f), true, ImGuiWindowFlags_NoFocusOnAppearing);
		{
			UICustom::SubCategoryHeader(Translate("Blacklist"));
			ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal);
			ImGui::NewLine();

			for (const auto& plugin : m_pluginList) {
				auto pluginName = ValidateTESFileName(plugin);
				auto pluginNameLower = pluginName;

				std::transform(pluginNameLower.begin(), pluginNameLower.end(), pluginNameLower.begin(),
					[](unsigned char c) { return static_cast<char>(std::tolower(c)); });

				if (pluginNameLower.find(pluginFilter) == std::string::npos) {
					continue;
				}

				if (!FilterModByRecord(plugin, m_primaryFilter)) {
					continue;
				}

				if (blacklist.find(plugin) == blacklist.end()) {
					continue;
				}

				if (ImGui::Selectable(pluginName.c_str())) {
					BlacklistConfig::GetSingleton()->RemovePluginFromBlacklist(plugin);
				}
			}
		}

		ImGui::EndChild();
		

		m_updateHidden = false;
	}
}
