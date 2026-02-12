#include "HomeModule.h"
#include "config/UserData.h"
#include "localization/FontManager.h"
#include "localization/Locale.h"

namespace Modex
{
	void HomeModule::Draw()
	{
		DrawTabMenu();
	}

	static inline std::vector<RE::TESObjectREFR*> refs;

	void DrawReference()
	{
		// if (ImGui::BeginTable("References", 5, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Sortable)) {
		// 	ImGui::TableSetupColumn("FormID", ImGuiTableColumnFlags_DefaultSort);
		// 	ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_NoSort);
		// 	ImGui::TableSetupColumn("Disabled", ImGuiTableColumnFlags_NoSort);
		// 	ImGui::TableSetupColumn("Z", ImGuiTableColumnFlags_NoSort);
		// 	ImGui::TableSetupColumn("Initially Disabled", ImGuiTableColumnFlags_NoSort);
		// 	ImGui::TableHeadersRow();
		//
		//
		// 	// Sort based on specs
		// 	if (ImGuiTableSortSpecs* sortSpecs = ImGui::TableGetSortSpecs()) {
		// 		if (sortSpecs->SpecsDirty) {
		// 			std::sort(refs.begin(), refs.end(), [&](RE::TESObjectREFR* a, RE::TESObjectREFR* b) {
		// 				return sortSpecs->Specs[0].SortDirection == ImGuiSortDirection_Ascending 
		// 					? a->GetFormID() < b->GetFormID() 
		// 					: a->GetFormID() > b->GetFormID();
		// 			});
		// 			sortSpecs->SpecsDirty = false;
		// 		}
		// 	}
		//
		// 	// Display sorted references
		// 	for (auto* a_refr : refs) {
		// 		ImGui::TableNextRow();
		// 		ImGui::TableNextColumn();
		// 		ImGui::Text("%s", std::format("{:08X}", a_refr->GetFormID()).c_str());
		// 		ImGui::TableNextColumn();
		// 		ImGui::Text("%s", a_refr->GetName());
		// 		ImGui::TableNextColumn();
		// 		ImGui::Text("%s", a_refr->IsDisabled() ? "true" : "false");
		// 		ImGui::TableNextColumn();
		// 		ImGui::Text("%.2f", a_refr->GetPositionZ());
		// 		ImGui::TableNextColumn();
		// 		ImGui::Text("%s", a_refr->IsInitiallyDisabled() ? "true" : "false");
		// 	}
		//
		// 	ImGui::EndTable();
		// }
	}

	void DrawHomeLayout(std::vector<std::unique_ptr<UITable>>& a_tables)
	{
		(void)a_tables;

		if (ImGui::BeginChild("##HomeModule::Cells", ImVec2(0, 300), true)) {
			DrawReference();
		}
		ImGui::EndChild();


	}
	
	HomeModule::HomeModule()
	{
		m_layouts.push_back({"Home", true, DrawHomeLayout});

		// refs.clear();
		//
		// auto TES = RE::TES::GetSingleton();
		// TES->ForEachCell([](RE::TESObjectCELL* a_cell) {
		// 	a_cell->ForEachReference([&](RE::TESObjectREFR* a_refr) {
		// 		if (a_refr->GetPositionZ() > -20000) { return RE::BSContainer::ForEachResult::kContinue; }
		// 		refs.push_back(a_refr);
		// 		return RE::BSContainer::ForEachResult::kContinue;
		// 	});
		// });
	}
}
