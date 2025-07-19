#include "include/B/Blacklist.h"
#include "include/D/Data.h"
#include "include/I/ISearch.h"
#include "include/P/Persistent.h"
#include "include/U/Util.h"

namespace Modex
{

	void Blacklist::BuildPluginList()
	{
		this->pluginList.clear();
		this->pluginListVector.clear();

		const auto& config = Settings::GetSingleton()->GetConfig();
		this->pluginList = Data::GetSingleton()->GetModulePluginListSorted(Data::PLUGIN_TYPE::ALL, (Data::SORT_TYPE)config.modListSort);
		this->pluginListVector = Data::GetSingleton()->GetFilteredListOfPluginNames(Data::PLUGIN_TYPE::ALL, (Data::SORT_TYPE)config.modListSort, RE::FormType::None);
		pluginListVector.insert(pluginListVector.begin(), Translate("Show All Plugins"));
	}

	void Blacklist::Draw(float a_offset)
	{
		(void)a_offset;

		const auto& blacklist = PersistentData::GetBlacklist();

		totalPlugins = static_cast<int>(this->pluginList.size());
		blacklistedPlugins = static_cast<int>(blacklist.size());
		nonBlacklistedPlugins = totalPlugins - blacklistedPlugins;

		if (ImGui::BeginChild("##Blacklist::CompareArea", ImVec2(0, 0), true, ImGuiWindowFlags_NoFocusOnAppearing)) {
			ImGui::SubCategoryHeader(Translate("SETTING_BLACKLIST"));
			ImGui::NewLine();
			ImGui::BeginColumns("##Blacklist::Column", 2, ImGuiOldColumnFlags_NoBorder);
			ImGui::Indent();

			// FormType Filter Box.
			ImGui::Text(TranslateFormat("GENERAL_FILTER_FORMTYPE", ":"));
			const auto primary_filter_text = RE::FormTypeToString(primaryFilter);
			if (ImGui::BeginCombo("##Blacklist::PluginType", primary_filter_text.data())) {
				if (ImGui::Selectable(Translate("None"), primaryFilter == RE::FormType::None)) {
					updateHidden = true;
					hiddenPlugins = 0;
					primaryFilter = RE::FormType::None;
					ImGui::SetItemDefaultFocus();
				}

				for (auto& filter : filterList) {
					bool isSelected = (filter == primaryFilter);

					if (ImGui::Selectable(Translate(RE::FormTypeToString(filter).data()), isSelected)) {
						primaryFilter = filter;
						updateHidden = true;
						hiddenPlugins = 0;
					}

					if (isSelected) {
						ImGui::SetItemDefaultFocus();
					}
				}

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
			ImGui::Text(TranslateFormat("GENERAL_FILTER_FUZZY", ":"));
			// ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x / 2);
			if (ImGui::InputTextWithHint("##Blacklist::ModSearch", Translate("GENERAL_CLICK_TO_TYPE"), modSearchBuffer, IM_ARRAYSIZE(modSearchBuffer))) {
				updateHidden = true;
				hiddenPlugins = 0;
			}

			std::string pluginFilter = modSearchBuffer;
			std::transform(pluginFilter.begin(), pluginFilter.end(), pluginFilter.begin(),
				[](unsigned char c) { return static_cast<char>(std::tolower(c)); });

			ImGui::NewLine();

			ImGui::Unindent();

			ImGui::NextColumn();

			ImGui::Indent();

			// Plugin Count
			ImGui::Text(TranslateFormat("GENERAL_TOTAL_PLUGINS", ": %d"), totalPlugins);
			ImGui::Text(TranslateFormat("GENERAL_TOTAL_BLACKLIST", ": %d"), blacklistedPlugins);
			ImGui::Text(TranslateFormat("GENERAL_TOTAL_NOT_BLACKLIST", ": %d"), nonBlacklistedPlugins);

			if (hiddenPlugins > 0) {
				ImGui::TextColored(ImVec4(0.9f, 0.1f, 0.1f, 1.0f), TranslateFormat("GENERAL_TOTAL_HIDDEN", ": %d"), hiddenPlugins);
				ImGui::TextColored(ImVec4(0.9f, 0.1f, 0.1f, 1.0f), Translate("GENERAL_TOTAL_HIDDEN_MESSAGE"));
			} else {
				ImGui::Text(TranslateFormat("GENERAL_TOTAL_HIDDEN", ": %d"), hiddenPlugins);
			}

			ImGui::Unindent();

			ImGui::EndColumns();

			// Left and Right Sections
			ImGui::BeginChild("##Blacklist::LeftBox", ImVec2(ImGui::GetContentRegionAvail().x / 2, 0), true, ImGuiWindowFlags_NoFocusOnAppearing);
			{
				ImGui::SubCategoryHeader(Translate("Whitelist"));

				ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal);
				ImGui::NewLine();

				for (const auto& plugin : pluginList) {
					auto pluginName = ValidateTESFileName(plugin);
					auto pluginNameLower = pluginName;

					std::transform(pluginNameLower.begin(), pluginNameLower.end(), pluginNameLower.begin(),
						[](unsigned char c) { return static_cast<char>(std::tolower(c)); });

					if (pluginNameLower.find(pluginFilter) == std::string::npos) {
						if (updateHidden) {
							hiddenPlugins++;
						}
						continue;
					}

					if (!FilterModByRecord(plugin, primaryFilter)) {
						if (updateHidden) {
							hiddenPlugins++;
						}
						continue;
					}

					if (blacklist.find(plugin) != blacklist.end()) {
						continue;
					}

					if (ImGui::Selectable(pluginName.c_str())) {
						PersistentData::GetSingleton()->AddPluginToBlacklist(plugin);
					}
				}
			}

			ImGui::EndChild();

			ImGui::SameLine(0.0f, -1.0f);

			ImGui::BeginChild("##Blacklist::RightBox", ImVec2(ImGui::GetContentRegionAvail().x, 0.0f), true, ImGuiWindowFlags_NoFocusOnAppearing);
			{
				ImGui::SubCategoryHeader(Translate("Blacklist"));
				ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal);
				ImGui::NewLine();

				for (const auto& plugin : pluginList) {
					auto pluginName = ValidateTESFileName(plugin);
					auto pluginNameLower = pluginName;

					std::transform(pluginNameLower.begin(), pluginNameLower.end(), pluginNameLower.begin(),
						[](unsigned char c) { return static_cast<char>(std::tolower(c)); });

					if (pluginNameLower.find(pluginFilter) == std::string::npos) {
						continue;
					}

					if (!FilterModByRecord(plugin, primaryFilter)) {
						continue;
					}

					if (blacklist.find(plugin) == blacklist.end()) {
						continue;
					}

					if (ImGui::Selectable(pluginName.c_str())) {
						PersistentData::GetSingleton()->RemovePluginFromBlacklist(plugin);
					}
				}
			}

			ImGui::EndChild();
		}

		ImGui::EndChild();

		updateHidden = false;
	}

	void Blacklist::Init()
	{
		hiddenPlugins = 0;
		updateHidden = true;
		selectedMod = Translate("Show All Plugins");
	}
}