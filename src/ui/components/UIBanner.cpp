#include "UIBanner.h"
#include "core/InputManager.h"

namespace Modex
{
    void UIBanner::Display()
    {
        this->lingerTime = DISPLAY_TIME + 1.0f;
        this->firstFrame = true;
    }

    bool UIBanner::ShouldDisplay() const
    {
		// TODO: REFACTOR
		// if (Settings::GetSingleton()->GetConfig().welcomeBanner == true) {
		// 	return this->lingerTime > 0.0f;
		// }

		return false;
    }

    void UIBanner::Draw() 
    {
		auto width = ImGui::GetIO().DisplaySize.x * 0.5f;
		auto height = ImGui::GetIO().DisplaySize.y * 0.30f;

		ImGui::SetNextWindowPos(ImVec2(width, height), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
        constexpr ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoMove;

		float alpha = 1.0f;
		if (this->lingerTime > DISPLAY_TIME - 1.0f) {
			float t = (DISPLAY_TIME - this->lingerTime);
			t = std::clamp(t, 0.0f, 1.0f);
			alpha = std::lerp(0.0f, 1.0f, t);
		} else if (this->lingerTime < 1.0f) {
			float t = this->lingerTime;
			t = std::clamp(t, 0.0f, 1.0f);
			alpha = std::lerp(0.0f, 1.0f, t);
		}

		if (alpha <= 0.0f && lingerTime <= 0.0f) {
			manager->DeleteWindow(this);
			return;
		}

		ImGui::PushStyleVar(ImGuiStyleVar_Alpha, alpha);
		if (ImGui::Begin("TestPopupForModexWindowGoesHere", nullptr, windowFlags)) {
			const auto titleText = std::format("Mod Explorer Menu {}.{}.{}", SKSE::GetPluginVersion().major(), SKSE::GetPluginVersion().minor(), SKSE::GetPluginVersion().patch());
			constexpr auto textA = "Press"sv;
			const auto keyNameText = InputManager::GetSingleton()->GetShowMenuKeyAsText();
			constexpr auto textB = "to open the in-game UI."sv;
			const auto windowWidth = ImGui::GetWindowSize().x;
			const auto titleTextWidth = ImGui::CalcTextSize(titleText.data()).x;
			ImGui::SetCursorPosX((windowWidth - titleTextWidth) * 0.5f);
			ImGui::TextUnformatted(titleText.data());
			ImGui::Separator();

			ImGui::TextUnformatted(textA.data());
			ImGui::SameLine();
			ImGui::TextColored(ImVec4(0.4f, 0.7f, 1.0f, 1.0f), "%s", keyNameText.data());
			ImGui::SameLine();
			ImGui::TextUnformatted(textB.data());
		}
		ImGui::End();
		ImGui::PopStyleVar();

        if (firstFrame) {
			firstFrame = false;
		} else {
			const ImGuiIO& io = ImGui::GetIO();
			lingerTime -= io.DeltaTime;
		}
    }
}