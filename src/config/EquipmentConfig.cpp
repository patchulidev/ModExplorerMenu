#include "EquipmentConfig.h"

// TODO: Cache all kits at startup.

namespace Modex
{
	// Helper to open and parse JSON file.
	nlohmann::json OpenJSON(const std::filesystem::path& a_path)
	{
		nlohmann::json data;
		if (!std::filesystem::exists(a_path)) {
			Warn("JSON file does not exist: '{}'", a_path.stem().string());
			return nlohmann::json::object();
		}

		try {
			std::ifstream file(a_path);

			if (!file.is_open()) {
				Error("Could not open JSON file: '{}'", a_path.stem().string());
				return nlohmann::json::object();
			}

			file >> data;
			Trace("Successfully opened JSON file: '{}'", a_path.stem().string());
			return data;
		} catch (...) {
			ASSERT_MSG(true, "Failed to open JSON file: '{}'", a_path.stem().string());
			return nlohmann::json::object();
		}
	}

	// Validator for name assignement using invalid characters.
	bool EquipmentConfig::ValidateKeyName(const std::string& a_keyName)
	{
		if (a_keyName.empty()) {
			return Warn("Kit key validation failed: empty key");
		}

		// invalid character check
		const std::string invalid_chars = R"("')";
		for (const char c : invalid_chars) {
			if (a_keyName.find(c) != std::string::npos) {
				return Warn("Kit key validation failed: invalid character ' {} ' in key '{}'", c, a_keyName);
			}
		}

		// whitespace-only check
		if (std::all_of(a_keyName.begin(), a_keyName.end(), isspace)) {
			return Warn("Kit key validation failed: whitespace-only key");
		}

		return true;
	}


	// Initialize equipment
	bool EquipmentConfig::Load()
	{
		Trace("Loading all equipment kits from JSON directory: '{}'", EQUIPMENT_JSON_PATH.string());

		if (!std::filesystem::is_directory(EQUIPMENT_JSON_PATH)) {
			Trace("Equipment JSON directory does not exist, creating: '{}'", EQUIPMENT_JSON_PATH.string());
			std::filesystem::create_directory(EQUIPMENT_JSON_PATH);
		}

		auto& cache = GetSingleton()->m_cache;
		cache.clear();

		int loaded_count = 0;
		for (auto& entry : std::filesystem::recursive_directory_iterator(EQUIPMENT_JSON_PATH)) {
			if (!entry.is_regular_file() || entry.path().extension() != ".json") {
				continue;
			}

			auto relativePath = std::filesystem::relative(entry.path(), EQUIPMENT_JSON_PATH);
			auto parentPath = relativePath.parent_path();

			// Validate the relative path doesn't escape. Otherwise our m_key will be invalid
			// annd result in weird behavior when renaming, copying, and saving.

			if (relativePath.string().starts_with("..")) {
				Trace("  Skipping file outside base path: '{}'", entry.path().string());
				continue;
			}

			KitData metadata;
			metadata.m_filepath = entry.path().string();
			metadata.m_key = relativePath.string();
			
			if (parentPath.empty()) {
				metadata.m_collection = "";
			} else {
				metadata.m_collection = parentPath.string();
			}
			
			// Cache metadata as KitData
			cache[metadata.m_key] = metadata;
			loaded_count++;
			
			Trace("  [{}] Key: '{}' | Collection: '{}'", loaded_count, metadata.m_key, metadata.m_collection);
		}
		
		Debug("Loaded {} kits metadata from '{}'", cache.size(), EQUIPMENT_JSON_PATH.string());
		return true;
	}

	// Create a new kit with a given relative path (key). Returns a Kit object on success.
	std::optional<Kit> EquipmentConfig::CreateKit(const std::filesystem::path& a_relativePath)
	{
		Debug("Creating new kit at relative path: '{}'", a_relativePath.string());

		Kit data;

		if (!ValidateKeyName(a_relativePath.string())) {
			return std::nullopt;
		}

		std::filesystem::path full_relative_path = a_relativePath;
		if (full_relative_path.extension() != ".json") {
			full_relative_path += ".json";
		}

		auto parentPath = full_relative_path.parent_path();
		data.m_collection = parentPath.empty() ? "" : parentPath.string();
		data.m_filepath = (EQUIPMENT_JSON_PATH / full_relative_path).string();
		data.m_key = full_relative_path.string();

		Debug("  Key: '{}' | Collection: '{}' | Filepath: '{}'", data.m_key, data.m_collection, data.m_filepath.string());

		if (!SaveKit(data)) {
			return std::nullopt;
		}

		auto& cache = GetSingleton()->m_cache;
		cache[data.m_key] = data;

		Info("Created new kit: '{}'", data.m_key);
		return data;
	}

	// Query runtime cache and load kit if metadata is found already.
	std::optional<Kit> EquipmentConfig::LoadKit(const KitData& a_metadata)
	{
		Debug("Loading kit from cache: '{}'", a_metadata.m_key);

		auto& cache = GetSingleton()->m_cache;
		if (cache.find(a_metadata.m_key) != cache.end()) {
			return LoadKit(a_metadata.m_filepath);
		}

		Error("  Kit filepath not found in cache: '{}'", a_metadata.m_filepath.string());
		return std::nullopt;
	}

	// Open and load a kit directly from JSON file path.
	std::optional<Kit> EquipmentConfig::LoadKit(const std::filesystem::path& a_fullPath)
	{
		Debug("Loading kit from JSON file: '{}'", a_fullPath.string());

		auto JSON = OpenJSON(a_fullPath);
		if (JSON.is_null() || JSON.empty()) {
			ASSERT_MSG(true, "Failed to read or parse JSON kit: {}", a_fullPath.string());
			return std::nullopt;
		}

		if (JSON.size() != 1) {
			ASSERT_MSG(true, "Expected exactly 1 kit in file, found {}:  {}", JSON.size(), a_fullPath.string());
			return std::nullopt;
		}

		auto kit_entry = JSON.items().begin();
		const auto& kit_data = kit_entry.value();

		Kit new_kit;
		new_kit.m_filepath = a_fullPath.string();
		new_kit.m_key = std::filesystem::relative(a_fullPath, EQUIPMENT_JSON_PATH).string();
		new_kit.m_collection = kit_data.value("Collection", "");
		new_kit.m_desc = kit_data.value("Description", "No description.");
		new_kit.m_tableID = 0;

		if (kit_data.contains("Items") && kit_data["Items"].is_object()) {
			for (auto& [editorid, item_data] : kit_data["Items"].items()) {
				auto item = std::make_shared<KitItem>();
				item->m_editorid = editorid;
				item->m_plugin = item_data.value("Plugin", "");
				item->m_name = item_data.value("Name", "");
				item->m_amount = item_data.value("Amount", 1);
				item->m_equipped = item_data.value("Equipped", false);

				new_kit.m_items.emplace_back(item);
			}
		}

		Info("Loaded Kit: '{}' with {} items", new_kit.m_key, new_kit.m_items.size());
		return new_kit;
	}

	// Save a Kit object to its associated JSON file.
	bool EquipmentConfig::SaveKit(const Kit& a_kit)
	{
		ASSERT_MSG(a_kit.m_filepath.empty(), "No filepath associated with kit: {}", a_kit.m_key);
		Debug("Saving kit to JSON file: '{}'", a_kit.m_filepath.string());

		nlohmann::json data;
		std::string json_key = std::filesystem::path(a_kit.m_key).stem().string();
		
		data[json_key] = nlohmann::json::object();
		data[json_key]["Collection"] = a_kit.m_collection;
		data[json_key]["Description"] = a_kit.m_desc;

		if (a_kit.m_items.empty()) {
			data[json_key]["Items"] = nlohmann::json::object();
		} else {
			for (auto& item : a_kit.m_items) {
				data[json_key]["Items"][item->m_editorid] = {
					{ "Plugin", item->m_plugin },
					{ "Name", item->m_name },
					{ "Amount", item->m_amount },
					{ "Equipped", item->m_equipped }
				};
			}
		}

		try {
			std::ofstream file(a_kit.m_filepath);
			if (!file.is_open()) {
				return Error("  Could not open JSON file for writing: '{}'", a_kit.m_filepath.string());
			}

			file << data.dump(4);
			Info("Saved kit '{}' to file", a_kit.m_key);
			return true;
		} catch (const std::exception& e) {
			return Error("  Exception occurred while saving kit: '{}'\n\n{}", a_kit.m_filepath.string(), e.what());
		}
	}

	// Duplicate and Save an existing kit, returns the new Kit object on success.
	std::optional<Kit> EquipmentConfig::CopyKit(const Kit& a_kit)
	{
		Debug("Copying kit: '{}'", a_kit.m_key);

		Kit new_kit = a_kit;
		
		std::filesystem::path original_key_path(a_kit.m_key);
		std::filesystem::path parent = original_key_path.parent_path();
		std::string stem = original_key_path.stem().string();
		std::string extension = original_key_path.extension().string();

		std::filesystem::path new_filename = stem + " (Copy)" + extension;
		std::filesystem::path new_key_path = parent / new_filename;

		new_kit.m_key = new_key_path.string();
		new_kit.m_filepath = (EQUIPMENT_JSON_PATH / new_key_path).string();
		
		if (parent.empty()) {
			new_kit.m_collection = "";
		} else {
			new_kit.m_collection = parent.string();
		}

		if (!SaveKit(new_kit)) {
			return std::nullopt;
		}

		auto& cache = GetSingleton()->m_cache;
		cache[new_kit.m_key] = new_kit;

		Info("Copied kit '{}' to new kit '{}'", a_kit.m_key, new_kit.m_key);
		return new_kit;
	}

	// Rename an existing kit, returns a new Kit object on success.
	std::optional<Kit> EquipmentConfig::RenameKit(Kit& a_kit, std::string a_keyName)
	{
		Debug("Renaming kit '{}' to '{}'", a_kit.m_key, a_keyName);

		if (a_keyName.size() > 5 && a_keyName.substr(a_keyName.size() - 5) == ".json") {
			a_keyName = a_keyName.substr(0, a_keyName.size() - 5);
			Trace("  Removed .json extension from new kit name");
		}

		if (a_keyName == a_kit.GetName()) {
			return a_kit;
		}

		if (!ValidateKeyName(a_keyName)) {
			return a_kit;
		}

		std::string old_key = a_kit.m_key;
		std::string new_key = a_keyName + ".json";
		std::filesystem::path oldPath = a_kit.m_filepath;
		std::filesystem::path newPath = EQUIPMENT_JSON_PATH / new_key;
		
		if (std::filesystem::exists(newPath)) {
			Warn("  Kit with name '{}' already exists in collection", a_keyName);
			return a_kit;
		}
		
		try {
			Kit new_kit = a_kit;
			new_kit.m_filepath = newPath.string();
			new_kit.m_key = new_key;

			Debug("  Old Path: '{}'", oldPath.string());
			Debug("  New Path: '{}'", newPath.string());
			
			if (!SaveKit(new_kit)) {
				Warn("  Failed to save renamed kit to:  {}", newPath.string());
				return a_kit;
			}
			
			std::filesystem::remove(oldPath);
			
			auto& cache = GetSingleton()->m_cache;
			cache.erase(old_key);
			cache[new_key] = new_kit;
			
			Info("Successfully renamed kit '{}' to '{}'", old_key, new_key);
			return std::move(new_kit);
			
		} catch (const std::exception& e) {
			ASSERT_MSG(true,"Exception while renaming kit '{}':\n\n{}", old_key, e.what());
			return a_kit;
		}
	}

	// Delete a kit and its associated JSON file.
	void EquipmentConfig::DeleteKit(const Kit& a_kit)
	{
		Debug("Deleting kit: '{}'", a_kit.m_key);

		auto& cache = GetSingleton()->m_cache;
		if (cache.find(a_kit.m_key) == cache.end()) {
			ASSERT_MSG(true, "DeleteKit Failed: Kit key '{}' not found in cache", a_kit.m_key);
			return;
		}

		auto kit_path = cache.at(a_kit.m_key).m_filepath;
		try {
			if (std::filesystem::exists(kit_path)) {
				std::filesystem::remove(kit_path);
			}
		} catch (const std::exception& e) {
			ASSERT_MSG(true, "Failed to delete kit JSON file: {}\n\n{}", kit_path.string(), e.what());
			return;
		}
		
		cache.erase(a_kit.m_key);
		Info("Deleted kit: {}", a_kit.m_key);
	}

	// Query runtime cache and return Kit object if found.
	std::optional<Kit> EquipmentConfig::KitLookup(const std::string& a_key)
	{
		Debug("Looking up kit by key: '{}'", a_key);

		auto& cache = GetSingleton()->m_cache;
		auto it = cache.find(a_key);
		
		if (it == cache.end()) {
			Warn("Kit Lookup failed: key '{}' not found in cache", a_key);
			return std::nullopt;
		}

		Trace("  Found kit metadata in cache: '{}'", a_key);
				
		if (auto kit = GetSingleton()->LoadKit(it->second.m_filepath); kit.has_value()) {
			return kit;
		}
		
		ASSERT_MSG(true, "Kit Lookup for {} found in cache but failed to load!", a_key);
		return std::nullopt;
	}

	// Returns items in a given kit as BaseObject vector.
	std::vector<BaseObject> EquipmentConfig::GetItems(const Kit& a_kit)
	{
		Debug("Getting items for kit: '{}'", a_kit.m_key);

		std::vector<BaseObject> items;
		
		for (auto& kitItem : a_kit.m_items) {
			RE::TESForm* form = RE::TESForm::LookupByEditorID(kitItem->m_editorid);

			if (form) {
				items.push_back(BaseObject(form, 0));
			} else {
				items.push_back(BaseObject(kitItem->m_name, kitItem->m_editorid, kitItem->m_plugin, 0));
			}
		}
		
		return items;
	}

	// Returns reference to the runtime equipment list cache. vector<std::string, KitData>
	std::unordered_map<std::string, KitData>& EquipmentConfig::GetEquipmentList()
	{
		return GetSingleton()->m_cache;
	}

	// Returns sorted list of equipment keys as vector<string>
	std::vector<std::string> EquipmentConfig::GetEquipmentListSortedKeys()
	{
		auto& cache = GetSingleton()->m_cache;
		std::vector<std::string> keys;
		
		for (const auto& [key, data] : cache) {
			keys.push_back(key);
		}
		
		std::sort(keys.begin(), keys.end());
		return keys;
	}

	// Returns sorted list of equipment tails as vector<string>
	std::vector<std::string> EquipmentConfig::GetEquipmentListSortedTails()
	{
		auto& cache = GetSingleton()->m_cache;
		std::vector<std::string> tails;
		
		for (const auto& [key, data] : cache) {
			tails.push_back(data.GetNameTail());
		}
		
		std::sort(tails.begin(), tails.end());
		return tails;
	}

	// This probably doesn't belong here.
	std::shared_ptr<KitItem> EquipmentConfig::CreateKitItem(const BaseObject& a_item)
	{
		auto new_item = std::make_shared<KitItem>();
		
		new_item->m_plugin 	= a_item.GetPluginName();
		new_item->m_name 		= a_item.GetName();
		new_item->m_editorid 	= a_item.GetEditorID();
		new_item->m_amount 	= a_item.GetQuantity();
		new_item->m_equipped 	= a_item.GetEquipped();
		
		return new_item;
	}
}
