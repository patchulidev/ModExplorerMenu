#pragma once

#include "ui/components/UIWindow.h"
#include "ui/components/UIModule.h"
#include "ui/modules/formselector/FormSelectorOptions.h"

namespace Modex
{
	class Menu : public UIWindow
	{
	public:

		Menu(bool a_apiMode);
		~Menu();

		Menu(const Menu&) = delete;
		Menu(Menu&&) = delete;
		Menu& operator=(const Menu&) = delete;
		Menu& operator=(Menu&&) = delete;

		void Draw() override;
		void OnOpening() override;
		void OnClosing() override;
		void OnClosed() override;
		void OnOpened() override;

		void LoadModule(uint8_t a_module, uint8_t a_layoutIndex);

		void NextWindow();

		static constexpr ImGuiWindowFlags WINDOW_FLAGS =
			ImGuiWindowFlags_NoCollapse         |
			ImGuiWindowFlags_NoScrollbar        |
			ImGuiWindowFlags_NoScrollWithMouse  | 
			ImGuiWindowFlags_NoCollapse         |
			ImGuiWindowFlags_NoTitleBar         |
			ImGuiWindowFlags_NoResize           |
			ImGuiWindowFlags_NoBringToFrontOnFocus;

		static constexpr ImGuiWindowFlags BACKGROUND_FLAGS =
			ImGuiWindowFlags_NoTitleBar         |
			ImGuiWindowFlags_NoResize           |
			ImGuiWindowFlags_NoMove             |
			ImGuiWindowFlags_NoScrollbar        |
			ImGuiWindowFlags_NoInputs           |
			ImGuiWindowFlags_NoFocusOnAppearing |
			ImGuiWindowFlags_NoNav              |
			ImGuiWindowFlags_NoBringToFrontOnFocus;

		using FormSelectorCallback = std::function<void(const std::vector<RE::FormID>&)>;
		void SetFormSelectorParams(Ownership a_ownership, const FormSelectorOptions& a_options, FormSelectorCallback a_callback);

	private:
		void DrawSidebar();
		void DrawModule();
		void DrawBackground(const ImVec2& a_displaySize);

		bool m_apiMode;
		Ownership m_formSelectorOwnership = Ownership::Item;
		FormSelectorOptions m_formSelectorOptions;
		FormSelectorCallback m_formSelectorCallback;
		bool expand_sidebar;
		bool sidebar_initialized = false;

		float sidebar_w;
		float sidebar_h;
		float min_sidebar_w;
		float max_sidebar_w;

		float exit_w = 0;

		enum class ModuleType : uint8_t
		{
			Home = 0,
			AddItem,
			Equipment,
			Inventory,
			Actor,
			Object,
			Teleport,
			Outfit,
			Settings,
			FormSelector,
			Count
		};

		struct ModuleInfo
		{
			std::string name;
			std::string icon;
			float       width;
			ModuleType  type;
		};

		std::vector<ModuleInfo>   m_moduleInfo;
		std::unique_ptr<UIModule> m_activeModule;
		uint8_t                   m_activeModuleIndex; 

		// Helpers
		std::unique_ptr<UIModule> CreateModule(ModuleType a_type, Ownership a_owner = Ownership::None);
		std::unique_ptr<UIModule> CreateModule(uint8_t a_index);
		std::unique_ptr<UIModule> CreateFormSelectorModule(Ownership a_owner, const FormSelectorOptions& a_options);
	};
}
