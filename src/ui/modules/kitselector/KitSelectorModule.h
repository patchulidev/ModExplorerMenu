#pragma once

#include "ui/components/UIModule.h"
#include "ui/modules/formselector/FormSelectorOptions.h"

namespace Modex
{
	class KitSelectorModule : public UIModule
	{
	public:
		using SelectionCallback = std::function<void(const std::vector<std::string>&)>;

		KitSelectorModule(const FormSelectorOptions& a_options = {}, SelectionCallback a_callback = nullptr);
		~KitSelectorModule();
		KitSelectorModule(const KitSelectorModule&) = delete;
		KitSelectorModule(KitSelectorModule&&) = delete;
		KitSelectorModule& operator=(const KitSelectorModule&) = delete;
		KitSelectorModule& operator=(KitSelectorModule&&) = delete;

		void Draw() override;
		void DrawTabMenu() override;
		float GetContentRatio() const override;

		void AddSelection(const std::string& a_kitKey);
		void RemoveSelection(int a_index);
		void ClearSelection();
		void ConfirmSelection();
		void SetCallback(SelectionCallback a_callback) { m_callback = std::move(a_callback); }

		const std::vector<std::string>& GetSelection() const { return m_selectedKeys; }

		static int GetKitGoldValue(const std::string& a_kitKey);

	private:
		void                       DrawActionPane(const ImVec2& a_size);
		int                        GetTotalCost() const;
		bool                       CanConfirm() const;

		FormSelectorOptions        m_options;
		SelectionCallback          m_callback;
		std::vector<std::string>   m_selectedKeys;
	};
}
