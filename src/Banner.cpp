#include "include/B/Banner.h"
#include "include/I/InputManager.h"
#include "include/U/UserSettings.h"

namespace Modex
{
    void UIBanner::Display()
    {
        this->lingerTime = DISPLAY_TIME;
        this->firstFrame = true;
    }

    bool UIBanner::ShouldDisplay() const
    {
		if (Settings::GetSingleton()->GetConfig().welcomeBanner == true) {
			return this->lingerTime > 0.f;
		}

		return false;
    }

    void UIBanner::Draw() 
    {
		auto width = ImGui::GetIO().DisplaySize.x * 0.5f;
		auto height = ImGui::GetIO().DisplaySize.y * 0.30f;

		ImGui::SetNextWindowPos(ImVec2(width, height), ImGuiCond_Always, ImVec2(0.5f, 0.5f));

        constexpr ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoMove;

        float alpha = 1.0f;
		if (lingerTime < DISPLAY_TIME) {
			alpha = std::lerp(0.f, 1.f, lingerTime / DISPLAY_TIME);
		} else if (lingerTime > 0.0f) {
			alpha = std::lerp(0.f, 1.f, (DISPLAY_TIME - lingerTime) / DISPLAY_TIME);
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