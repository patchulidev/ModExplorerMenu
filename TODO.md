# Modex - A Mod Explorer Menu - v2.0

![](https://capsule-render.vercel.app/api?type=waving&height=300&color=gradient&text=Modex&desc=A%20Mod%20Explorer%20Menu&descSize=20&section=header)

### :bug: Open Reports & Issues:

- [ ] (Bug) SSE Gameplay Tweaks ImpurePotionCostMultiplier set to true in GameplayTweak.config.txt results in NetScript error  
- [ ] (Bug) Potential incompatiblity with esps flagged as header version 1.71 for BBES compatibility. Excluding forms from those range  
- [x] (Bug) Crash to Desktop on linux due to windows font path resolving incorrectly.  
- [x] (Bug) Licentia Next users experiencing issues with menu clipping.
- [x] (Bug) Linux users experiencing issues with loading windows fonts.

### :sparkles: Upcoming Features:

- [ ] (New) A welcome banner on startup.  
- [ ] (New) Window system to handle pop up warnings and messages, increasing user experience.  
- [ ] (New) Outfit module allowing users to act on pre-defined outfits defined by the game. Assign, change, etc.  
- [ ] (WIP) Streamline plugin indexing and filtering. It's not very intuitive for users right now.
- [ ] (WIP) Reduce usage of favicons in unecessary menu panes. Item Preview for example.
- [ ] (WIP) Do something with the home module already...

### :white_check_mark: Completed Column ✓

#### :memo: Table Layout & Widgets
- [x] (New) Status bar indicating context, warning messages, and search parameters on the fly.
- [x] (New) Column headers that replicate legacy table behaviors to help improve user
- [x] (New) Inline search key, field, and plugin combo box as a part of the table widget.
- [x] (New) Exposed more table specific layout settings for users to customize.
- [x] (New) Functional up/down arrow navigation with proper wrapping and selection heuristics.
- [x] (New) More robust Drag'n'Drop behavior implementation with new graphics and tooltips.
- [x] (New) Sort column containing JSON configurable properties to sort over.
- [x] (New) JSON configurable sorting, search, and filter systems for Table objects. 
- [x] (New) Support for basic Regex queries and basic comparators in table search fields.
- [x] (New) Tree node style filter system for form types with multi-selection filtering.

#### :memo: Modules & Menu
- [x] (New) Module actions now leverage console reference to add, remove, and modify inventories.
- [x] (New) Module and Layout are restored upon re-entering the game or re-opening the menu.
- [x] (New) Stylized window popup framework for user prompts, warnings, and other information.
- [x] (New) Optional foreground image banner for potential wabbajack authors to customize.
- [x] (New) Alt now cycles through modules, similarly to in-game behavior. Toggleable in settings.
- [x] (New) (AddItem) Support for keyword lists on items in the preview pane.
- [x] (New) (AddItem) Container View action allowing a quick legacy view of table items in-game.
- [x] (New) (AddItem) Now enables targeted interactions with NPC's, Containers, and player.
- [x] (New) (Actor) Renamed NPC module to Actor module.
- [x] (New) (Actor) Support for keyword lists on actors in the preview pane.
- [x] (New) (Actor) Support for outfit lists on actors in the preview pane.

#### :memo: Backend Systems & Behaviors
- [x] (New) Upgraded to ImGui 1.92.5 from 1.90.5. Updates codebase to support newer API uses.
- [x] (New) Swapped to IMenu interface from DXD11 Present hook. Modex is now a native Game Menu!
- [x] (New) Dynamic font and glyph loading. Re-implemented from the ground up.
- [x] (New) Fully integrated SimpleIME for non-english input method editors.
- [x] (New) Optional smooth scrolling animations across widgets.
- [x] (New) Packaged font and license for out-of-box Modex installations.
- [x] (New) Dynamic tooltip lookups, new tooltip window, and help markers for UX.
- [x] (New) Hundreds of instances of logging, utilizing various log levels for debugging user
  issues.
- [x] (Modify) Tab and Escape now incrementally pop/close windows.
- [x] (Modify) All configuration and plugin data files are now converted to JSON storage.
- [x] (Modify) Migrated command processing to direct SKSE functions from console usage. 

#### :bug: Bug Fixes & Improvements
- [x] (Bug) Hotkey popup and assignment being fragile and inconsistent to user expectations.
- [x] (Bug) Menu not opening when expected, unresponsive show menu hotkey.
- [x] (Bug) Font loading crashes related to filesystem and path handling.
- [x] (Bug) Easy Console Command & Other console mod compatibility issues.
- [x] (Bug) Table initialization failing, causing CTD unexpectedly.
- [x] (Bug) Unexpected behavior with dynamic table sizing resulting in visual clipping.
- [x] (Bug) "Unknown type ToString" runtime error causing CTD.
- [x] (Bug) Unexpected CTD occurence related to Language file loading.
- [x] (Bug) Some features not respecting confirurable compileIndex (load order) setting.
- [x] (Bug) Theme configuration, and ini configuration loading error causing CTD.
- [x] (Bug) Menu clipping outside screen bounds. Menu Size is now clamped to bounds.
- [x] (Bug) Undefined behavior with improper ImGuiIconLibrary installations.
- [x] (Bug) Weapon types not properly attributed in weapon item previews.
- [x] (Bug) Inconsistent behavior when using an optional modifier hotkey to show menu.
- [x] (Imp) Refactored layout and table style to reduce clipping and improve design.
- [x] (Imp) Optimized module memory allocation and management for performance improvements.
- [x] (Imp) Refactored InputTextComboBox behavior to new standards and improve safety.
- [x] (Imp) Refactored BaseObject class and methods to improve nullptr safety.
- [x] (Imp) Refactored Module class and implementations to improve scalability.

### :heavy_minus_sign: Chopping Block:
- [x] (Removed) Windows Font fallbacks and indexing. Too complicated and unecessary.
- [x] (Removed) SimpleINI user settings, and dependencies. Converted fully to JSON systems.
- [x] (Removed) Explicit usage of Glyph and GlyphRanges in settings. (ImGui 1.92.5 upgrade)
- [x] (Removed) Module toggle and visibility settings. May re-introduce later on.
- [x] (Removed) PluginKitView in Table layout. Need to reconsider this design choice.
- [x] (Removed) Some niche keyboard shortcuts and behaviors to streamline new input.
- [x] (Removed) Custom Sidebar Icons in favor of font icons provided by Lucide.


***Note: A great deal of CTD user reports in v.1.2.3 were a result of an old implementation of
saving and loading configuration files. Some occurences may return. I've installed much more verbose
logging with the expectation issues may arise. In which case I'll be more readily equipped to debug
and resolve them.***

