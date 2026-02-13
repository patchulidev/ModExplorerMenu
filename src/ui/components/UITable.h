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
		struct Colors
		{
			float alpha;
			ImU32 background;
			ImU32 backgroundAlt;
			ImU32 selected;
			ImU32 hover;
			ImU32 outline;
			ImU32 text;
			ImU32 textEnchanted;
			ImU32 error;
		};

		ImGuiMultiSelectFlags MULTI_SELECT_FLAGS = 
		ImGuiMultiSelectFlags_ClearOnClickVoid  | ImGuiMultiSelectFlags_SelectOnClickRelease |
		ImGuiMultiSelectFlags_NoAutoSelect      | ImGuiMultiSelectFlags_BoxSelect1d | 
		ImGuiMultiSelectFlags_ClearOnEscape     | ImGuiMultiSelectFlags_NoAutoClearOnReselect;

		TableList               tableList;
		TableList               recentList;

		std::unique_ptr<FilterSystem>       filterSystem;
		std::unique_ptr<SortSystem>         sortSystem;
		std::unique_ptr<SearchSystem>       searchSystem;

		PluginList                              pluginList;
		std::unordered_set<const RE::TESFile*>  pluginSet;

		std::string             data_id;
		uint32_t                pluginType;
		uint32_t                flags = 0;
		uint32_t                tableID;
		uint32_t                tableMode;
		Colors                  colors;
		
		//                      core
		TableItem               itemPreview;
		RE::TESObjectREFR*      tableTargetRef;
		char                    pluginSearchBuffer[256];
		std::string             selectedPlugin;
		Kit*                    selectedKitPtr;
		
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
		bool                    showFormID;
		bool                    useQuickSearch;
		bool                    useSharedTarget;

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

		enum TableMode : uint32_t {
			SHOWALL = 0,
			SHOWRECENT,
			SHOWFAVORITE,
		};

		UITable(const std::string& a_dataID, bool a_shared, uint8_t a_type, uint32_t a_flags);
		~UITable();
		UITable(const UITable&) = delete;
		UITable& operator=(const UITable&) = delete;
		UITable(UITable&&) noexcept = default;
		UITable& operator=(UITable&&) noexcept = default;

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

		//                      core behaviors
		void                    Draw(const TableList& a_tableList);
		void                    Refresh();

		TableList*              GetTableListPtr() { return &tableList; }
		TableList&              GetTableListRef() { return tableList; }
		const PluginList&       GetPluginList() const { return pluginList; };
		const TableList&        GetTableList() const { return tableList; }
		const TableList         GetSelection() const;
		const TableItem&        GetItemPreview() { return itemPreview; }

		//                      class builder methods
		void                    SetKitPointer(Kit* a_kit) { selectedKitPtr = a_kit; }
		void                    SetDragDropHandle(DragDropHandle a_handle);

		//                      target reference accessors
		RE::TESObjectREFR*      GetTableTargetRef() const { return tableTargetRef; }
		void                    SetTargetByReference(RE::TESObjectREFR* a_reference);
		bool                    IsActionAllowed();
		bool                    IsValidTargetReference(RE::TESObjectREFR* a_reference = nullptr);
		
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
		void                    ResolvePayloadDrop(UITable* origin, UITable* destination, std::vector<std::unique_ptr<BaseObject>>& payload_items);
		
		//                      table action methods
		uint32_t                GetSelectionCount() const;
		void                    AddSelectionToActiveKit();
		void                    AddSelectionToTargetInventory(uint32_t a_count);
		void                    AddKitToTargetInventory(const Kit& a_kit);
		void                    RemoveSelectionFromKit();
		void                    RemoveSelectionFromTargetInventory();  
		void                    PlaceSelectionOnGround(uint32_t a_count);
		void                    EquipSelectionToTarget();
		void                    PlaceAll();
		void                    AddAll();

	private:
		//                      core
		void                    Setup();
		void                    InitializeSystems();
		void                    LoadSystemState();
		void                    SaveSystemState();
		void                    CleanupResources();
		void                    UpdateLayout();

		//                      search and filter impl
		void                    Filter(const std::vector<BaseObject>& a_data);
		void                    FilterRecentImpl();
		void                    FilterFavoriteImpl();
		void                    FilterKitImpl();
		void                    FilterInventoryImpl();
		void                    UpdateActiveInventoryTables();
		void                    UpdateImGuiTableIDs();
		void                    SyncChangesToKit();
		void                    BuildPluginList();

		std::vector<BaseObject> GetReferenceInventory();
		
		//                      sorting
		bool                    SortFn(const std::unique_ptr<BaseObject>& a, const std::unique_ptr<BaseObject>& b);
		void                    SortListBySpecs();

		//                      input and interaction
		void                    HandleDragDropBehavior();
		void                    HandleKeyboardNavigation(const TableList& a_tableList);
		void                    HandleItemHoverPreview(const std::unique_ptr<BaseObject>& a_item);
		void                    HandleLeftClickBehavior(const std::unique_ptr<BaseObject>& a_item);
		void                    HandleRightClickBehavior(const std::unique_ptr<BaseObject>& a_item);
		bool                    IsMouseHoveringRect(const ImVec2& a_min, const ImVec2& a_max);
		
		//                      debug
		void                    Test_TableSelection();
		void                    Test_TableFilters();
		
		//                      layout and drawing
		void                    DrawDragDropPayload(const std::string& a_icon);
		void                    DrawItem(const std::unique_ptr<BaseObject>& a_item, const ImVec2& a_pos, bool a_selected);
		void                    DrawKitItem(const std::unique_ptr<BaseObject>& a_item, const ImVec2& a_pos, bool a_selected);
		void                    DrawKit(const Kit& a_kit, const ImVec2& a_pos);
		void                    DrawPluginSearchBar(const ImVec2& a_size);
		void                    DrawFormSearchBar(const ImVec2& a_size);
		void                    DrawModeDropdown(const ImVec2& a_size);
		void                    CustomSortColumn();

		//                      widget groups
		void                    DrawTableSettingsPopup();
		void                    DrawDebugToolkit();
		void                    DrawFormFilterTree();
		void                    DrawSearchBar();
		void                    DrawStatusBar();
		void                    DrawHeader();

		DragDropHandle                      dragDropHandle;
		std::map<DragDropHandle, UITable*>     dragDropSourceList;
		ImGuiSelectionBasicStorage          selectionStorage;
	};
}
