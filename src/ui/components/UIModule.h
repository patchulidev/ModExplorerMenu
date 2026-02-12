#pragma once

#include "ui/components/UITable.h"

namespace Modex
{
	struct Layout
	{
		std::string name;
		bool selected;

		std::function<void(std::vector<std::unique_ptr<UITable>>&)> DrawFunc;
	};

	class UIModule
	{
	protected:
		std::vector<std::unique_ptr<UITable>> m_tables;
		std::vector<Layout> m_layouts;

		float               m_offset = 0.0f;
		float               m_sidebar = 0;

	public:
		static inline RE::TESObjectREFR* s_targetReference = nullptr;

		virtual ~UIModule();
		virtual void Draw() {};
		virtual void DrawTabMenu();

		float& GetSidebarWidth() { return m_sidebar; };

		void SetOffset(float a_offset) { m_offset = a_offset; };
		void SetActiveLayout(uint8_t a_layoutIndex);
		uint8_t GetActiveLayoutIndex() const;

		static void LoadSharedReference();
		static void SaveSharedReference();

		static RE::TESObjectREFR* GetTargetReference() { return s_targetReference; }
		static void SetTargetReference(RE::TESObjectREFR* a_ref);
		static RE::TESObjectREFR* LookupReferenceBySearch(const std::string& a_search);
		static RE::TESObjectREFR* LookupReferenceByFormID(const RE::FormID& a_id);
	};
}
