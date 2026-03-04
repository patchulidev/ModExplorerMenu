#pragma once

#include "ui/components/UIModule.h"

namespace Modex
{
	class HomeModule : public UIModule
	{
	public:
		HomeModule();
		~HomeModule() = default;
		HomeModule(const HomeModule&) = delete;
		HomeModule(HomeModule&&) = delete;
		HomeModule& operator=(const HomeModule&) = delete;
		HomeModule& operator=(HomeModule&&) = delete;

		void Draw() override;

	private:
		static void DrawWelcomeLayout(std::vector<std::unique_ptr<UITable>>& a_tables);
		static void DrawShortcutsLayout(std::vector<std::unique_ptr<UITable>>& a_tables);
		static void DrawFeaturesLayout(std::vector<std::unique_ptr<UITable>>& a_tables);

		static void DrawFeatureCard(const char* a_icon, const char* a_titleKey, const char* a_descKey, uint8_t a_moduleIndex, float a_cardWidth);
		static void DrawShortcutEntry(const char* a_keyLabel, const char* a_descKey);
		static void DrawFeatureSection(const char* a_titleKey, const char* a_descKey);

		static void DrawStatsRow();
		static void DrawTargetStatus(float a_width, float a_height);
		static void DrawQuickActions(float a_width, float a_height);
		static void DrawTipCard(float a_width, float a_height);
	};
}
