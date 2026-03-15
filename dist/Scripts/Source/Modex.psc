ScriptName Modex Hidden

; Modex Papyrus API
; Native function stubs for the Modex SKSE plugin.
; These functions are implemented in C++ and registered at runtime.

; =============================================================================
; Menu Control
; =============================================================================

; Opens the Modex menu. No-op if already open.
Function OpenMenu() Global Native

; Closes the Modex menu. No-op if already closed.
Function CloseMenu() Global Native

; Returns true if the Modex menu is currently open.
bool Function IsMenuOpen() Global Native

; =============================================================================
; Inventory
; =============================================================================

; Add the specified form to the player's inventory.
Function AddItemToPlayer(Form akForm, int aiCount = 1) Global Native

; Remove the specified form from the player's inventory.
Function RemoveItemFromPlayer(Form akForm, int aiCount = 1) Global Native

; =============================================================================
; NPC / Reference Actions
; =============================================================================

; Teleport the player to the specified reference.
Function TeleportPlayerTo(ObjectReference akRef) Global Native

; Teleport the specified reference to the player.
Function TeleportToPlayer(ObjectReference akRef) Global Native

; Kill the specified actor.
Function KillActor(Actor akActor) Global Native

; Resurrect the specified actor.
Function ResurrectActor(Actor akActor) Global Native

; Enable the specified reference.
Function EnableReference(ObjectReference akRef) Global Native

; Disable the specified reference.
Function DisableReference(ObjectReference akRef) Global Native

; =============================================================================
; Spawning
; =============================================================================

; Spawn the specified form at the player's location.
ObjectReference Function PlaceAtPlayer(Form akForm, int aiCount = 1) Global Native

; =============================================================================
; Data / Cache Queries
; =============================================================================

; Returns the number of cached forms for the given cache type.
; Cache types: 0=Item, 1=NPC, 2=Object, 3=Cell, 4=Outfit
int Function GetCachedFormCount(int aiCacheType) Global Native

; Returns true if the specified form exists in any Modex cache.
bool Function IsFormCached(Form akForm) Global Native

; Returns true if Modex has finished initializing its data cache.
bool Function IsDataReady() Global Native

; =============================================================================
; Form Selector Options (Builder Pattern)
;
; Call these BEFORE OpenFormSelector to configure the selector.
; Options are consumed when OpenFormSelector is called and reset to defaults.
;
; Example:
;   Modex.SetFormSelectorSingleSelect(true)
;   Modex.SetFormSelectorShowTotalCost(true)
;   Modex.SetFormSelectorTitle("Choose Your Reward")
;   Modex.OpenFormSelector(0)
; =============================================================================

; When true, only one form can be selected at a time.
; Selecting a new form replaces the previous selection.
Function SetFormSelectorSingleSelect(bool abEnable) Global Native

; When true, displays the total gold cost of selected forms in the action pane.
Function SetFormSelectorShowTotalCost(bool abEnable) Global Native

; When true, the player must have enough gold to afford the total cost.
; Confirm button is disabled if the player cannot afford the selection.
Function SetFormSelectorRequireTotalCost(bool abEnable) Global Native

; Sets the maximum total gold cost allowed. 0 = unlimited.
; Confirm button is disabled if the total exceeds this value.
Function SetFormSelectorMaxCost(int aiMaxCost) Global Native

; Sets the maximum number of forms that can be selected. 0 = unlimited.
Function SetFormSelectorMaxCount(int aiMaxCount) Global Native

; Scales the gold cost of all forms by this multiplier. Default is 1.0.
; Example: 0.5 = half price, 2.0 = double price.
Function SetFormSelectorCostMultiplier(float afMultiplier) Global Native

; Sets a custom title for the selector window.
Function SetFormSelectorTitle(string asTitle) Global Native

; Resets all form selector options to their defaults.
; This is called automatically after OpenFormSelector, but can be called
; manually if you need to cancel a partially-configured selector.
Function ResetFormSelectorOptions() Global Native

; =============================================================================
; Form Selector
; =============================================================================

; Opens the Form Selector UI for the given cache type.
; Cache types: 0=Item, 1=NPC, 2=Object, 3=Cell, 4=Outfit
; Fires "Modex_OnFormSelected" ModEvent when the user confirms or cancels.
;   numArg = count of selected forms (0 = cancelled)
;   sender = first selected form (or None if cancelled)
Function OpenFormSelector(int aiCacheType) Global Native

; Returns the forms from the last selection and clears the buffer.
; Single-consumer: the first script to call this gets the data.
Form[] Function GetSelectedForms() Global Native

; Returns the number of forms in the selection buffer.
int Function GetSelectedFormCount() Global Native

; Returns the total gold cost of forms in the selection buffer.
int Function GetSelectedFormCost() Global Native
