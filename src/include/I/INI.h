#pragma once

namespace IniHelper
{
	enum class iHeader
	{
		MasterPresetHeader,
		PresetMainModules,
	};

	enum class iComment
	{
		ConfigShowMenuKey,
		ConfigShowMenuModifier,
		ConfigModListSort,
		ConfigUIScale,
		ConfigUIScaleHorizontal,
		ConfigFullscreen,
		ConfigPauseGame,
		ConfigDisableInMenu,
		ConfigLanguage,
		ConfigGlyphRange,
		ConfigGlobalFont,
		ConfigGlobalFontSize,
		ConfigDefaultShow,
		ConfigShowHomeMenu,
		ConfigShowAddItemMenu,
		ConfigShowObjectMenu,
		ConfigShowNPCMenu,
		ConfigShowTeleportMenu,
		ConfigDataPath,
	};

	std::map<iComment, const char*> comment_map = {
		{ iComment::ConfigShowMenuKey,
			"# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #\n"
			"# ShowMenuKey = <int> (Uses Skyrim Input Codes: https://ck.uesp.net/wiki/Input_Script\n"
			"# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #" },
		{ iComment::ConfigShowMenuModifier,
			"# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #\n"
			"# ShowMenuModifier = <int> (Shift = 42, Alt = 56, Control = 29).\n"
			"# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #" },
		{ iComment::ConfigModListSort,
			"# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #\n"
			"# ModListSort = <int> (0 = Alphabetical, 1 = Load Order (Ascending), 2 = Load Order (Descending) )\n"
			"# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #" },
		{ iComment::ConfigUIScale,
			"# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #\n"
			"# UI Scale = <int>\n"
			"# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #" },
		{ iComment::ConfigUIScaleHorizontal,
			"# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #\n"
			"# UI Scale Horizontal = <int>\n"
			"# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #" },
		{ iComment::ConfigFullscreen,
			"# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #\n"
			"# Fullscreen = <bool> (true, false)\n"
			"# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #" },
		{ iComment::ConfigPauseGame,
			"# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #\n"
			"# PauseGame = <bool> (true, false) (Determines if menu pauses the game or not)\n"
			"# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #" },
		{ iComment::ConfigDisableInMenu,
			"# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #\n"
			"# DisableInMenu = <bool> (true, false) (Determines if Modex is disabled while in-game menus)\n"
			"# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #" },
		{ iComment::ConfigLanguage,
			"# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #\n"
			"# Language = <string>\n"
			"#\n"
			"# - Language translation files are located in the 'Data/Interface/Modex/User/Language' directory.\n"
			"# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #" },
		{ iComment::ConfigGlyphRange,
			"# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #\n"
			"# GlyphRange = <string> (English, Greek, Cyrillic, Korean, Japanese, ChineseFull, ChineseSimplified, Thai, Vietnamese)\n"
			"# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #" },
		{ iComment::ConfigGlobalFont,
			"# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #\n"
			"# GlobalFont = <string> (Font name) [Default]\n"
			"#\n"
			"# - Fonts are registered based on filename. If you have a custom font, it must be placed in the 'Data/Interface/Modex/Fonts' directory.\n"
			"# - If using a custom font, specify the filename without the extension. Example: 'Arial' for 'Arial.ttf'.\n"
			"# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #" },
		{ iComment::ConfigGlobalFontSize,
			"# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #\n"
			"# GlobalFontSize = <float> (Default = 14.0)\n"
			"# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #" },
		{ iComment::ConfigDefaultShow,
			"# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #\n"
			"# DefaultShow = <int> (0 = Home, 1 = Add Item, 2 = Object, 3 = NPC, 4 = Teleport, 5 = Settings)\n"
			"#\n"
			"# - This setting determines which module is loaded by default on startup.\n"
			"# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #" },
		{ iComment::ConfigShowHomeMenu,
			"# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #\n"
			"# ShowHomeMenu = <bool> (true, false)\n"
			"# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #" },
		{ iComment::ConfigShowAddItemMenu,
			"# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #\n"
			"# ShowAddItemMenu = <bool> (true, false)\n"
			"# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #" },
		{ iComment::ConfigShowObjectMenu,
			"# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #\n"
			"# ShowObjectMenu = <bool> (true, false)\n"
			"# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #" },
		{ iComment::ConfigShowNPCMenu,
			"# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #\n"
			"# ShowNPCMenu = <bool> (true, false)\n"
			"# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #" },
		{ iComment::ConfigShowTeleportMenu,
			"# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #\n"
			"# ShowTeleportMenu = <bool> (true, false)\n"
			"# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #" },
		{ iComment::ConfigDataPath,
			"# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #\n"
			"# DataPath = <string> (Path to your Skyrim Data folder)\n"
			"#\n"
			"# - This is used to determine the install date of mods for sorting.\n"
			"# - Regardless of your Mod Manager, this should point to your *actual* data folder.\n"
			"# - This is only necessary if you're using the 'Install Date' sorting option.\n"
			"# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #" }
	};

	std::map<iHeader, const char*> header_map = {
		{ iHeader::MasterPresetHeader,
			"#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~#\n"
			"# # Master Configuration\n"
			"#\n"
			"# This file is used to store variables across sessions\n"
			"# You should not have to change anything in here. Most of it is handled in-game.\n"
			"#\n"
			"#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~#\n" }
	};

	[[nodiscard]] const char* GetComment(iComment a_type)
	{
		return comment_map[a_type];
	}

	[[nodiscard]] const char* GetHeader(iHeader a_type)
	{
		return header_map[a_type];
	}
}