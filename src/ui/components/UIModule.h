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

		float               m_offset;
		bool                m_show;

		std::string         m_name;
		std::string         m_icon;
		float               m_sidebar;
		

	public:
		static inline RE::TESObjectREFR* s_targetReference = nullptr;

		virtual ~UIModule() = default;
		virtual void Draw() {};
		virtual void Load();
		virtual void Unload();

		virtual void DrawTabMenu();

		bool IsLoaded() const { return m_show; };
		float& GetSidebarWidth() { return m_sidebar; };
		const std::string& GetName() const { return m_name; };
		const std::string& GetIcon() const { return m_icon; };

		void UpdateTableTargets(RE::TESObjectREFR* a_ref);

		void SetOffset(float a_offset) { m_offset = a_offset; };
		void SetActiveLayout(uint8_t a_layoutIndex);
		uint8_t GetActiveLayoutIndex() const;

		static RE::TESObjectREFR* GetTargetReference() { return s_targetReference; }
		static void SetTargetReference(RE::TESObjectREFR* a_ref);
		static std::optional<RE::TESObjectREFR*> LookupReferenceBySearch(const std::string& a_search);
		static std::optional<RE::TESObjectREFR*> LookupReferenceByFormID(const RE::FormID& a_id);
	};
}
