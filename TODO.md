# Modex - A Mod Explorer Menu - v2.0

![](https://capsule-render.vercel.app/api?type=waving&height=300&color=gradient&text=Modex&desc=A%20Mod%20Explorer%20Menu&descSize=20&section=header)
## :bug: Open Reports & Outstanding Issues:

- [ ] :bug: SSE Gameplay Tweaks ImpurePotionCostMultiplier set to true in GameplayTweak.config.txt results in NetScript error  
- [ ] :bug: Potential incompatiblity with esps flagged as header version 1.71 for BBES compatibility. Excluding forms from those range  
- [ ] :bug: Easy Console Command & Other console mod compatibility issues.
- [ ] :bug: Crash to Desktop on linux due to windows font path resolving incorrectly.  
- [x] :bug: fixed: Licentia Next users experiencing issues with menu clipping.
- [x] :bug: fixed: "Unknown type ToString" runtime error causing CTD.
- [x] :bug: fixed: Unexpected CTD occurence related to Language file loading.
- [x] :bug: fixed: Linux users experiencing issues with loading windows fonts.

## :sparkles: Planned Features / :bulb: Ideas

- [ ] :construction: A welcome banner on startup.  
- [ ] :construction: Outfit module allowing users to act on pre-defined outfits defined by the game. Assign, change, etc.  
- [ ] :construction: Streamline plugin indexing and filtering. It's not very intuitive for users right now.
- [ ] :construction: Reduce usage of favicons in unecessary menu panes. Item Preview for example.
- [ ] :bulb: Shout / Dragon Soul module to unify Modex as more of a cheat menu and debug tool.
- [ ] :bulb: Peek / Popup inventory window for NPC references in Actor module.
- [ ] :bulb: Manual Input / Popup for table/console reference selection.
- [ ] :bulb: Inventory Swap / Trade module for quickly swapping between two inventories.
- [ ] :bulb: Do something with the home module already...
- [ ] :bulb: Skyprompt integration for quicker menu interactions, reference selection, and more.

## :white_check_mark: Completed Column ✓

#### :memo: Table Layout & Widgets
- [x] :art: design: Interactable Status bar indicating context, warning messages, and search parameters on the fly.
- [x] :art: desgin: Dynamic Sortable Column headers with JSON configurable sorting types.
- [x] :art: design: Exposed more table specific layout settings for users to customize.
- [x] :art: design: Relocated and redesigned table search fields to save UI space and conform to new style.
- [x] :art: design: Optional smooth scrolling animations.
- [x] :zap: optimize: More robust Drag'n'Drop behavior implementation with new graphics and tooltips.
- [x] :zap: optimize: Huge improvements to memory allocation and management for table and objects.
- [x] :sparkles: feature: Tables now leverage console reference to add, remove, and modify inventories.
- [x] :sparkles: feature: Support for additional search comparators including basic Regex.
- [x] :sparkles: feature: Configurable Tree node style filter system for form types with multi-selection filtering.
- [x] :bug: fixed: Table initialization failing, causing CTD unexpectedly.
- [x] :bug: fixed: Unexpected behavior with table layout sizing resulting in visual clipping.

#### :memo: Modules & Menu

##### - General:
- [x] :art: design: Stylized window popup framework for user prompts, warnings, and other information.
- [x] :art: design: Optional foreground image banner for potential wabbajack authors to customize.
- [x] :recycle: refactor: Re-implemented InputTextComboBox behavior to new standards and improve safety.
- [x] :sparkles: feature: Module and Layout are restored upon re-entering the game or re-opening the menu.
- [x] :zap: optimize: Optimized module memory usage and loading for performance improvements.

##### - Add Item Module:
- [x] :dizzy: add: Support for keyword lists on items in the preview pane.
- [x] :sparkles: feature: Container View action allowing a quick legacy view of table items in-game.
- [x] :bug: fixed: Weapon types not properly attributed in weapon item previews.

##### - Equipment Module:
- [x] :art: design: Renamed from Kit to Equipment module.
- [x] :art: design: Completely overhauled the layout and interactions for Kits.
- [x] :dizzy: add: New kit file browser-like popup to navigate and browse local kits.
- [x] :dizzy: add: Sorting and filtering items within selected Kit table.
- [x] :bug: fixed: Items being re-arranged and re-sorted every refresh.
- [x] :bug: fixed: Items from a previously enabled plugin not being handled properly.

##### - Actor Module:
- [x] :art: design: Renamed from NPCto Actor module.
- [x] :dizzy: add: Support for keyword lists on actors in the preview pane.
- [x] :dizzy: add: Support for outfit lists on actors in the preview pane.

##### - Settings Module:
- [x] :art: design: Redesigned settings layout to be more user-friendly.
- [x] :recycle: change: All configuration and plugin data files are now converted to JSON storage.
- [x] :recycle: change: Blacklist relocated and re-implemented to global settings.
- [x] :bug: fixed: Hotkey popup and assignment being fragile and inconsistent to user expectations.
- [x] :bug: fixed: Some features not respecting confirurable compileIndex (load order) setting.
- [x] :bug: fixed: Inconsistent behavior when using an optional modifier hotkey to show menu.
- [x] :bug: fixed: Theme configuration, and ini configuration loading error causing CTD.
- [x] :bug: fixed: Menu clipping outside screen bounds. Menu Size is now clamped to bounds.

### :memo: Backend Systems & Behaviors
- [x] :construction_worker: refactor: Upgraded to ImGui 1.92.5 from 1.90.5. Updates codebase to support newer API uses.
- [x] :construction_worker: refactor: Swapped to IMenu interface from DXD11 Present hook. Modex is now a native Game Menu!
- [x] :construction_worker: refactor: Dynamic font and glyph loading. Re-implemented from the ground up.
- [x] :sparkles: feature: Fully integrated SimpleIME compatibility for non-english input method editors.
  issues.
- [x] :zap: optimize: Fully migrated command processing to direct SKSE functions from console usage. 
- [x] :bug: fixed: Font loading crashes related to filesystem and path handling.
- [x] :bug: fixed: Undefined behavior with improper ImGuiIconLibrary installations.
- [x] :bug: fixed: Menu not opening when expected, unresponsive show menu hotkey.
- [x] :loud_sound: Hundreds of instances of logging, utilizing various log levels for debugging user

## :wastebasket: Removals & Deletions:
:heavy_minus_sign: Windows Font fallbacks and indexing. Too complicated and unecessary.<br>
:heavy_minus_sign: SimpleINI user settings, and dependencies. Converted fully to JSON systems.<br>
:heavy_minus_sign: Explicit usage of Glyph and GlyphRanges in settings. (ImGui 1.92.5 upgrade).<br>
:heavy_minus_sign: Module toggle and visibility settings. May re-introduce later on.<br>
:heavy_minus_sign: PluginKitView in Table layout. Need to reconsider this design choice.<br>
:heavy_minus_sign: Some niche keyboard shortcuts and behaviors to streamline new input.<br>
:heavy_minus_sign: Custom Sidebar Icons in favor of font icons provided by Lucide.<br>


>***Note: A great deal of CTD user reports in v.1.2.3 were a result of an old implementation of
saving and loading configuration files. Some occurences may return. I've installed much more verbose
logging with the expectation issues may arise. In which case I'll be more readily equipped to debug
and resolve them.***

