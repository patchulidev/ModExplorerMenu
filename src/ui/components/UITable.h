#pragma once

#include "data/BaseObject.h"
#include "data/Data.h"

#include "ui/core/FilterSystem.h"
#include "ui/core/SortSystem.h"
#include "ui/core/SearchSystem.h"

namespace Modex
{
	class UITable
	{
		using TableList             = std::vector<std::unique_ptr<BaseObject>>;
		using TableItem             = std::unique_ptr<BaseObject>;
		using PluginList            = std::vector<std::string>;
		using PluginKitList         = std::vector<std::unique_ptr<Kit>>;

	private:
		ImGuiMultiSelectFlags MULTI_SELECT_FLAGS = 
		ImGuiMultiSelectFlags_ClearOnClickVoid  | ImGuiMultiSelectFlags_SelectOnClick | 
		ImGuiMultiSelectFlags_NoAutoSelect      | ImGuiMultiSelectFlags_BoxSelect1d | 
		ImGuiMultiSelectFlags_ClearOnEscape     | ImGuiMultiSelectFlags_NoAutoClearOnReselect;
			// ImGuiMultiSelectFlags_ClearOnClickVoid  | ImGuiMultiSelectFlags_SelectOnClickRelease | 
			// ImGuiMultiSelectFlags_NoAutoSelect      | ImGuiMultiSelectFlags_BoxSelect1d;

		TableList               tableList;
		TableList               recentList;
		TableList               searchList;
		TableList               filterList;
		PluginKitList           pluginKitList;

		std::function<std::vector<BaseObject>()> generator;

		std::unique_ptr<FilterSystem>       filterSystem;
		std::unique_ptr<SortSystem>         sortSystem;
		std::unique_ptr<SearchSystem>       searchSystem;

		PluginList                              pluginList;
		std::unordered_set<const RE::TESFile*>  pluginSet;

		uint32_t                flags = 0;
		Data::PLUGIN_TYPE       pluginType;
		std::string             data_id;
		int                     clickAmount;
		
		//                      core
		TableItem               itemPreview;
		RE::TESObjectREFR*      tableTargetRef;
		char                    pluginSearchBuffer[256];
		char                    lastSearchBuffer[256];
		bool                    generalSearchDirty;
		std::string             selectedPlugin;
		Kit*                    selectedKitPtr;
		
		//                      lazy event callbacks
		bool                    updateKeyboardNav;
		bool                    updateRecentList;
		
		//                      internal flags
		bool                    showEditorID;
		bool                    showPluginKitView;
		bool                    showFormType;

		//                      table layout parameters
		float                   LayoutRowSpacing; // FIXME: Does default initializers here fix the bug?
		float                   LayoutOuterPadding;
		float                   LayoutHitSpacing;
		int                     LayoutColumnCount;
		ImVec2                  LayoutItemStep;
		ImVec2                  LayoutItemSize;
		ImVec2                  ItemSize;

		ImGuiID                 navPositionID = 0;


	public:
		enum TableFlag : uint32_t {
			ModexTableFlag_Kit = 1 << 0,
			ModexTableFlag_Inventory = 1 << 1,
			ModexTableFlag_Base = 1 << 2,
			ModexTableFlag_EnableSearch = 1 << 3,
			ModexTableFlag_EnablePluginKitView = 1 << 4,
			ModexTableFlag_EnableCategoryTabs = 1 << 5,
			ModexTableFlag_EnableHeader = 1 << 6,
			ModexTableFlag_EnableDebugToolkit = 1 << 7,
			ModexTableFlag_EnableItemPreviewOnHover = 1 << 8
		};

		enum class DragBehavior {
			None,
			Invalid,
			Add,
			Remove
		};

		enum class DragDropHandle {
			Table,
			Kit,
			Inventory
		};

		void AddFlag(TableFlag flag) { flags |= flag; }
		void RemoveFlag(TableFlag flag) { flags &= ~flag; }
		bool HasFlag(TableFlag flag) const { return (flags & flag) != 0; }

		// TODO: Look at how we're constructing and reloading table objects for memory management before release.
		
		UITable() : 
			filterSystem(nullptr), 
			sortSystem(),

			pluginType(Data::PLUGIN_TYPE::kTotal),
			clickAmount(1),
			updateKeyboardNav(false),
			updateRecentList(false),

			LayoutRowSpacing(5.0f),
			LayoutOuterPadding(0.0f),
			LayoutHitSpacing(0.0f),
			LayoutColumnCount(1),
			LayoutItemStep(ImVec2(0.0f, 0.0f)),
			LayoutItemSize(ImVec2(0.0f, 0.0f)),
			ItemSize(ImVec2(0.0f, 0.0f))
		{}

		~UITable() = default;

		//                      core behaviors
		void                    Draw(const TableList& a_tableList);
		void                    ShowSort();
		void                    DrawWarningBar();
		void                    PluginKitView();
		void                    Refresh();
		void                    Unload();
		void                    Load();
		void                    Init();
		void                    Reset();
		void                    SyncChangesToKit();
		void                    BuildPluginList();
		void                    LoadRecentList();

		TableList*              GetTableListPtr() { return &tableList; }
		TableList&              GetTableListRef() { return tableList; }
		const PluginList&       GetPluginList() const { return pluginList; };
		const TableList&        GetTableList() const { return tableList; }

		//                      class builder methods
		void                    SetGenerator(std::function<std::vector<BaseObject>()> a_generator);
		void                    SetPluginType(Data::PLUGIN_TYPE a_type) { pluginType = a_type; }
		void                    SetKitPointer(Kit* a_kit) { selectedKitPtr = a_kit; }
		void                    SetDragDropHandle(DragDropHandle a_handle);
		void                    SetClickAmount(int a_amount) { clickAmount = a_amount; }
		void                    SetUserDataID(const std::string& a_id) { data_id = a_id; }
		void                    SetShowEditorID(bool a_show) { showEditorID = a_show; }
		void                    SetShowPluginKitView(bool a_show) { showPluginKitView = a_show; }

		void                    SetTableTargetRef(RE::TESObjectREFR* a_ref) { tableTargetRef = a_ref; }
		RE::TESObjectREFR*      GetTableTargetRef() const { return tableTargetRef; }

		int*                    GetClickAmount() { return &clickAmount; }
		
		//                      drag n drop behaviors
		DragDropHandle          GetDragDropHandle() const { return dragDropHandle; }
		void                    SetDragDropTarget(DragDropHandle a_handle, UITable* a_view);
		void                    AddDragDropTarget(DragDropHandle a_handle, UITable* a_view);
		void                    RemoveDragDropTarget(DragDropHandle a_handle);
		void                    AddPayloadItemToKit(const std::unique_ptr<BaseObject>& a_item);
		void                    AddPayloadItemToTable(const std::unique_ptr<BaseObject>& a_item);
		void                    RemovePayloadItemFromKit(const std::unique_ptr<BaseObject>& a_item);
		void                    RemovePayloadItemFromInventory(const std::unique_ptr<BaseObject>& a_item);
		const char*             GetDragDropHandleText(DragDropHandle a_handle) const { return magic_enum::enum_name(a_handle).data(); }
		
		//                      table action methods
		void                    PlaceSelectionOnGround(int a_count);
		void                    RemoveSelectedFromInventory();  
		void                    AddSelectionToInventory(int a_count);
		void                    AddKitItemsToInventory(const Kit& a_kit);
		void                    RemoveSelectedFromKit();
		void                    AddSelectedToKit();
		void                    EquipSelection();
		void                    PlaceAll();
		void                    AddAll();

		std::vector<std::unique_ptr<BaseObject>> GetSelection();
		uint32_t                GetSelectionCount() const;
		TableItem&              GetItemPreview() { return itemPreview; }

		//                      Recently Used
		void                    AddItemToRecent(const std::unique_ptr<BaseObject>& a_item);

	private:
		//                      plugin kit view impl
		void                    LoadKitsFromSelectedPlugin();

		//                      search and filter impl
		void                    Filter(const std::vector<BaseObject>& a_data);
		void                    UpdateImGuiTableIDs();
		void                    UpdateKitItemData();
		
		//                      sorting
		bool                    SortFnKit(const std::unique_ptr<Kit>& a, const std::unique_ptr<Kit>& b);
		bool                    SortFn(const std::unique_ptr<BaseObject>& a, const std::unique_ptr<BaseObject>& b);
		void                    SortListBySpecs();

		//                      input and interaction
		void                    HandleDragDropBehavior();
		void                    HandleKeyboardNavigation(const TableList& a_tableList);
		void                    HandleItemHoverPreview(const std::unique_ptr<BaseObject>& a_item);
		void                    HandleLeftClickBehavior(const std::unique_ptr<BaseObject>& a_item);
		void                    HandleRightClickBehavior(const std::unique_ptr<BaseObject>& a_item);
		bool                    IsMouseHoveringRectDelayed(const ImVec2& a_min, const ImVec2& a_max);
		
		//                      debug
		void                    Test_TableSelection();
		void                    Test_TableFilters();
		
		//                      layout and drawing
		void                    UpdateLayout();
		void                    DrawDragDropPayload(const std::string& a_icon);
		void                    DrawItem(const BaseObject& a_item, const ImVec2& a_pos, const bool& a_selected);
		void                    DrawKitItem(const std::unique_ptr<BaseObject>& a_item, const ImVec2& a_pos, const bool& a_selected);
		void                    DrawKit(const Kit& a_kit, const ImVec2& a_pos, const bool& a_selected);
		void                    DrawDebugToolkit();
		void                    DrawFormSearchBar(const ImVec2& a_size);
		void                    DrawPluginSearchBar(const ImVec2& a_size);
		void                    DrawTableSettingsPopup();
		void                    DrawHeader();
		void                    DrawHeaderSortCombo(std::string a_header, float a_valueWidth, bool a_sorted);
		void                    DrawFormTypeTabs();

		DragDropHandle                      dragDropHandle;
		std::map<DragDropHandle, UITable*>     dragDropSourceList;
		ImGuiSelectionBasicStorage          selectionStorage;
	};
}
