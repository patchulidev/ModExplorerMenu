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

	private:
		ImGuiMultiSelectFlags MULTI_SELECT_FLAGS = 
		// ImGuiMultiSelectFlags_ClearOnClickVoid  | ImGuiMultiSelectFlags_SelectOnClick | 
		ImGuiMultiSelectFlags_ClearOnClickVoid  | ImGuiMultiSelectFlags_SelectOnClickRelease |
		ImGuiMultiSelectFlags_NoAutoSelect      | ImGuiMultiSelectFlags_BoxSelect1d | 
		ImGuiMultiSelectFlags_ClearOnEscape     | ImGuiMultiSelectFlags_NoAutoClearOnReselect;
			// ImGuiMultiSelectFlags_ClearOnClickVoid  | ImGuiMultiSelectFlags_SelectOnClickRelease | 
			// ImGuiMultiSelectFlags_NoAutoSelect      | ImGuiMultiSelectFlags_BoxSelect1d;

		TableList               tableList;
		TableList               recentList;

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
		bool                    useSharedTarget;
		
		//                      lazy event callbacks
		bool                    updateKeyboardNav;
		bool                    updateRecentList;
		
		//                      user settings
		float                   styleHeight;
		float                   styleWidth;
		float                   styleSpacing;
		float                   styleFontSize;
		bool                    showAltRowBG;
		bool                    showItemIcon;
		bool                    showEditorID;
		bool                    useQuickSearch;

		//                      table layout parameters
		float                   LayoutRowSpacing;
		float                   LayoutOuterPadding;
		float                   LayoutHitSpacing;
		float                   LayoutColumnWidth;
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
			ModexTableFlag_EnableFilterTree = 1 << 4,
			ModexTableFlag_EnableHeader = 1 << 5,
			ModexTableFlag_EnableDebugToolkit = 1 << 6,
			ModexTableFlag_EnableItemPreviewOnHover = 1 << 7
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

		// ~UITable() = default;
		// UITable(const UITable&) = delete;
		// UITable(UITable&&) = delete;
		// UITable& operator=(const UITable&) = delete;
		// UITable& operator=(UITable&&) = delete;

		//                      core behaviors
		void                    Draw(const TableList& a_tableList);
		void                    DrawSearchBar();
		void                    DrawStatusBar();
		void                    Refresh();
		void                    Unload();
		void                    Load();
		void                    Init();
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
		void                    SetUseSharedTarget(bool a_use) { useSharedTarget = a_use; }
		void                    SetShowEditorID(bool a_show) { showEditorID = a_show; }

		//                      target reference accessors
		RE::TESObjectREFR*      GetTableTargetRef() const { return tableTargetRef; }
		void                    SetTargetByReference(RE::TESObjectREFR* a_reference);

		int*                    GetClickAmount() { return &clickAmount; }
		
		//                      drag n drop behaviors
		DragDropHandle          GetDragDropHandle() const { return dragDropHandle; }
		void                    SetDragDropTarget(DragDropHandle a_handle, UITable* a_view);
		void                    AddDragDropTarget(DragDropHandle a_handle, UITable* a_view);
		void                    AddPayloadToKit(const std::unique_ptr<BaseObject>& a_item);
		void                    AddPayloadToInventory(const std::unique_ptr<BaseObject>& a_item);
		void                    RemoveDragDropTarget(DragDropHandle a_handle);
		void                    RemovePayloadItemFromKit(const std::unique_ptr<BaseObject>& a_item);
		void                    RemovePayloadFromInventory(const std::unique_ptr<BaseObject>& a_item);
		const char*             GetDragDropHandleText(DragDropHandle a_handle) const { return magic_enum::enum_name(a_handle).data(); }
		
		//                      table action methods
		void                    UpdateActiveInventoryTables();
		void                    PlaceSelectionOnGround(int a_count);
		void                    RemoveSelectionFromTargetInventory();  
		void                    AddSelectionToTargetInventory(int a_count);
		void                    AddKitToTargetInventory(const Kit& a_kit);
		void                    RemoveSelectionFromKit();
		void                    AddSelectionToActiveKit();
		void                    EquipSelectionToTarget();
		void                    PlaceAll();
		void                    AddAll();

		std::vector<BaseObject> GetReferenceInventory();
		std::vector<std::unique_ptr<BaseObject>> GetSelection();

		uint32_t                GetSelectionCount() const;
		TableItem&              GetItemPreview() { return itemPreview; }


	private:
		//                      search and filter impl
		void                    Filter(const std::vector<BaseObject>& a_data);
		void                    UpdateImGuiTableIDs();
		
		//                      sorting
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
		void                    DrawItem(const std::unique_ptr<BaseObject>& a_item, const ImVec2& a_pos);
		void                    DrawKitItem(const std::unique_ptr<BaseObject>& a_item, const ImVec2& a_pos);
		void                    DrawKit(const Kit& a_kit, const ImVec2& a_pos, const bool& a_selected);
		void                    DrawDebugToolkit();
		void                    DrawFormSearchBar(const ImVec2& a_size);
		void                    DrawPluginSearchBar(const ImVec2& a_size);
		void                    DrawTableSettingsPopup();
		void                    DrawHeader();
		void                    CustomSortColumn(std::string a_header, float a_valueWidth, bool a_sorted);
		void                    DrawFormFilterTree();

		DragDropHandle                      dragDropHandle;
		std::map<DragDropHandle, UITable*>     dragDropSourceList;
		ImGuiSelectionBasicStorage          selectionStorage;
	};
}
