#pragma once

#include "ui/components/UIModule.h"
#include "ui/modules/formselector/FormSelectorOptions.h"

namespace Modex
{
	class FormSelectorModule : public UIModule
	{
	public:
		using SelectionCallback = std::function<void(const std::vector<RE::FormID>&)>;

		FormSelectorModule(Ownership a_ownership, const FormSelectorOptions& a_options = {}, SelectionCallback a_callback = nullptr);
		~FormSelectorModule();
		FormSelectorModule(const FormSelectorModule&) = delete;
		FormSelectorModule(FormSelectorModule&&) = delete;
		FormSelectorModule& operator=(const FormSelectorModule&) = delete;
		FormSelectorModule& operator=(FormSelectorModule&&) = delete;

		void Draw() override;
		void DrawTabMenu() override;

		void AddSelection(RE::FormID a_formID);
		void RemoveSelection(RE::FormID a_formID);
		void ClearSelection();
		void ConfirmSelection();
		void SetCallback(SelectionCallback a_callback) { m_callback = std::move(a_callback); }

		const std::vector<RE::FormID>& GetSelection() const { return m_selected; }

	private:
		void                       DrawActionPane(const ImVec2& a_size);
		int                        GetTotalCost() const;
		bool                       CanConfirm() const;

		Ownership                  m_ownership;
		FormSelectorOptions        m_options;
		SelectionCallback          m_callback;
		std::vector<RE::FormID>    m_selected;
	};
}
