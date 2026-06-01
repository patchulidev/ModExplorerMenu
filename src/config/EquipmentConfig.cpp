#include "EquipmentConfig.h"
#include "config/UserData.h"
#include "core/Commands.h"
#include "core/PathUtf8.h"
#include "data/BaseObject.h"

// All kits are fully loaded into memory at startup for instant access.

namespace Modex
{
	// Helper to open and parse JSON file.
	nlohmann::json OpenJSON(const std::filesystem::path& a_path)
	{
		nlohmann::json data;
		if (!std::filesystem::exists(a_path)) {
			Warn("JSON file does not exist: '{}'", PathToUtf8(a_path.stem()));
			return nlohmann::json::object();
		}

		try {
			std::ifstream file(a_path);

			if (!file.is_open()) {
				Error("Could not open JSON file: '{}'", PathToUtf8(a_path.stem()));
				return nlohmann::json::object();
			}

			file >> data;
			Trace("Successfully opened JSON file: '{}'", PathToUtf8(a_path.stem()));
			return data;
		} catch (...) {
			ASSERT_MSG(true, "Failed to open JSON file: '{}'", PathToUtf8(a_path.stem()));
			return nlohmann::json::object();
		}
	}

	// Validator for name assignement using invalid characters.
	bool EquipmentConfig::ValidateKeyName(const std::string& a_keyName)
	{
		if (a_keyName.empty()) {
			return Warn("Kit key validation failed: empty key");
		}

		// Kit names are flat filenames — no path separators, no quote characters.
		// Users who want to organize on disk can move files manually.
		const std::string invalid_chars = R"("'/\)";
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

	// Split tags on both `,` (new-style tag CSV) and `/` or `\` (legacy folder
	// paths from the pre-tag schema) so old kits migrate transparently on load.
	static std::string MigrateLegacyCollectionString(const std::string& a_raw)
	{
		if (a_raw.empty()) return {};

		std::vector<std::string> tokens;
		std::string current;
		for (char c : a_raw) {
			if (c == ',' || c == '/' || c == '\\') {
				if (!current.empty()) tokens.push_back(current);
				current.clear();
			} else {
				current.push_back(c);
			}
		}
		if (!current.empty()) tokens.push_back(current);

		// Trim and drop empties.
		std::string out;
		for (auto& tok : tokens) {
			size_t start = 0, end = tok.size();
			while (start < end && std::isspace(static_cast<unsigned char>(tok[start]))) start++;
			while (end > start && std::isspace(static_cast<unsigned char>(tok[end - 1]))) end--;
			if (start == end) continue;
			if (!out.empty()) out += ", ";
			out.append(tok, start, end - start);
		}
		return out;
	}


	// Initialize equipment — fully loads all kits into memory.
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

			auto relativePath = entry.path().lexically_relative(EQUIPMENT_JSON_PATH);

			// Validate the relative path doesn't escape. Otherwise our m_key will be invalid
			// and result in weird behavior when renaming, copying, and saving.
			if (relativePath.wstring().starts_with(L"..")) {
				Trace("  Skipping file outside base path: '{}'", PathToUtf8(entry.path()));
				continue;
			}

			if (auto kit = LoadKit(entry.path()); kit.has_value()) {
				auto key = kit->m_key;
				cache[key] = std::move(kit.value());
				loaded_count++;
				Trace("  [{}] Key: '{}'", loaded_count, key);
			}
		}

		Debug("Loaded {} kits from '{}'", cache.size(), EQUIPMENT_JSON_PATH.string());
		RebuildKnownTags();
		return true;
	}

	Kit EquipmentConfig::CreateKit(const std::string& a_name)
	{
		Debug("Creating new kit '{}'", a_name);

		Kit data;

		if (!ValidateKeyName(a_name)) {
			return Kit{};
		}

		std::string key = a_name;
		if (!key.ends_with(".json")) {
			key += ".json";
		}

		data.m_key      = key;
		data.m_filepath = EQUIPMENT_JSON_PATH / PathFromUtf8(key);
		// m_collection left empty — users assign tags after creation.

		Debug("  Key: '{}' | Filepath: '{}'", data.m_key, PathToUtf8(data.m_filepath));

		if (!SaveKit(data)) {
			return Kit{};
		}

		UserData::SendEvent(ModexActionType::CreateKit, data.m_key, Ownership::Kit);

		Info("Created new kit: '{}'", data.m_key);
		return data;
	}

	// Open and load a kit directly from JSON file path.
	std::optional<Kit> EquipmentConfig::LoadKit(const std::filesystem::path& a_fullPath)
	{
		Debug("Loading kit from JSON file: '{}'", PathToUtf8(a_fullPath));

		auto JSON = OpenJSON(a_fullPath);
		if (JSON.is_null() || JSON.empty()) {
			ASSERT_MSG(true, "Failed to read or parse JSON kit: {}", PathToUtf8(a_fullPath));
			return std::nullopt;
		}

		if (JSON.size() != 1) {
			ASSERT_MSG(true, "Expected exactly 1 kit in file, found {}:  {}", JSON.size(), PathToUtf8(a_fullPath));
			return std::nullopt;
		}

		auto kit_entry = JSON.items().begin();
		const auto& kit_data = kit_entry.value();

		Kit new_kit;
		new_kit.m_filepath = a_fullPath;
		new_kit.m_key = PathToUtf8(std::filesystem::relative(a_fullPath, EQUIPMENT_JSON_PATH));
		new_kit.m_collection = MigrateLegacyCollectionString(kit_data.value("Collection", ""));
		new_kit.m_tableID = 0;

		if (kit_data.contains("Items") && kit_data["Items"].is_object()) {
			for (auto& [editorid, item_data] : kit_data["Items"].items()) {
				auto item = std::make_shared<KitItem>();
				item->m_editorid = editorid;
				item->m_plugin = item_data.value("Plugin", "");
				item->m_name = item_data.value("Name", "");
				item->m_amount = item_data.value("Amount", 1);
				item->m_equipped = item_data.value("Equipped", false);

				// Symptom from swapping from EditorID to FormID framework.
				// Don't really want to store form ids either though.
				if (auto form = RE::TESForm::LookupByEditorID(editorid); form != nullptr) {
					item->m_formID = form->GetFormID();
				}

				new_kit.m_items.emplace_back(item);
			}
		}

		// Optional — older kit files won't have this key.
		if (kit_data.contains("Spells") && kit_data["Spells"].is_object()) {
			for (auto& [editorid, spell_data] : kit_data["Spells"].items()) {
				auto spell = std::make_shared<KitSpell>();
				spell->m_editorid = editorid;
				spell->m_plugin = spell_data.value("Plugin", "");
				spell->m_name = spell_data.value("Name", "");

				if (auto form = RE::TESForm::LookupByEditorID(editorid); form != nullptr) {
					spell->m_formID = form->GetFormID();
				}

				new_kit.m_spells.emplace_back(spell);
			}
		}

		Info("Loaded Kit: '{}' with {} items, {} spells", new_kit.m_key, new_kit.m_items.size(), new_kit.m_spells.size());
		return new_kit;
	}

	// Save a Kit object to its associated JSON file and update the cache.
	bool EquipmentConfig::SaveKit(const Kit& a_kit)
	{
		ASSERT_MSG(a_kit.m_filepath.empty(), "No filepath associated with kit: {}", a_kit.m_key);
		Debug("Saving kit to JSON file: '{}'", PathToUtf8(a_kit.m_filepath));

		nlohmann::json data;
		std::string json_key = PathToUtf8(PathFromUtf8(a_kit.m_key).stem());

		data[json_key] = nlohmann::json::object();
		data[json_key]["Collection"] = a_kit.m_collection;

		data[json_key]["Items"] = nlohmann::json::object();
		for (auto& item : a_kit.m_items) {
			data[json_key]["Items"][item->m_editorid] = {
				{ "Plugin", item->m_plugin },
				{ "Name", item->m_name },
				{ "Amount", item->m_amount },
				{ "Equipped", item->m_equipped }
			};
		}

		data[json_key]["Spells"] = nlohmann::json::object();
		for (auto& spell : a_kit.m_spells) {
			data[json_key]["Spells"][spell->m_editorid] = {
				{ "Plugin", spell->m_plugin },
				{ "Name", spell->m_name }
			};
		}

		try {
			std::ofstream file(a_kit.m_filepath);
			if (!file.is_open()) {
				return Error("  Could not open JSON file for writing: '{}'", PathToUtf8(a_kit.m_filepath));
			}

			file << data.dump(4);

			// Sync the kit back to the in-memory cache.
			auto& cache = GetSingleton()->m_cache;
			cache[a_kit.m_key] = a_kit;

			RebuildKnownTags();
			Info("Saved kit '{}' to file", a_kit.m_key);
			return true;
		} catch (const std::exception& e) {
			return Error("  Exception occurred while saving kit: '{}'\n\n{}", PathToUtf8(a_kit.m_filepath), e.what());
		}
	}

	Kit EquipmentConfig::CopyKit(const Kit& a_kit)
	{
		Debug("Copying kit: '{}'", a_kit.m_key);

		Kit new_kit = a_kit;

		std::filesystem::path original_key_path = PathFromUtf8(a_kit.m_key);
		std::string stem      = PathToUtf8(original_key_path.stem());
		std::string extension = PathToUtf8(original_key_path.extension());

		std::string new_key = stem + " (Copy)" + extension;

		new_kit.m_key      = new_key;
		new_kit.m_filepath = EQUIPMENT_JSON_PATH / PathFromUtf8(new_key);

		if (!SaveKit(new_kit)) {
			return Kit{};
		}

		UserData::SendEvent(ModexActionType::CopyKit, new_kit.m_key, Ownership::Kit);
		Info("Copied kit '{}' to new kit '{}'", a_kit.m_key, new_kit.m_key);
		return new_kit;
	}

	Kit EquipmentConfig::RenameKit(Kit& a_kit, std::string a_keyName)
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
		std::filesystem::path newPath = EQUIPMENT_JSON_PATH / PathFromUtf8(new_key);

		if (std::filesystem::exists(newPath)) {
			Warn("  Kit with name '{}' already exists in collection", a_keyName);
			return a_kit;
		}

		try {
			Kit new_kit = a_kit;
			new_kit.m_filepath = newPath;
			new_kit.m_key = new_key;

			Debug("  Old Path: '{}'", PathToUtf8(oldPath));
			Debug("  New Path: '{}'", PathToUtf8(newPath));

			if (!SaveKit(new_kit)) {
				Warn("  Failed to save renamed kit to:  {}", PathToUtf8(newPath));
				return Kit{};
			}

			std::filesystem::remove(oldPath);

			auto& cache = GetSingleton()->m_cache;
			cache.erase(old_key);

			UserData::SendEvent(ModexActionType::RenameKit, new_key, Ownership::Kit);
			Info("Successfully renamed kit '{}' to '{}'", old_key, new_key);
			return new_kit;

		} catch (const std::exception& e) {
			ASSERT_MSG(true,"Exception while renaming kit '{}':\n\n{}", old_key, e.what());
			return Kit{};
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
			ASSERT_MSG(true, "Failed to delete kit JSON file: {}\n\n{}", PathToUtf8(kit_path), e.what());
			return;
		}
		
		cache.erase(a_kit.m_key);
		RebuildKnownTags();
		UserData::SendEvent(ModexActionType::DeleteKit, a_kit.m_key, Ownership::Kit);
		Info("Deleted kit: {}", a_kit.m_key);
	}

	// Query runtime cache and return pointer to Kit if found.
	Kit* EquipmentConfig::KitLookup(const std::string& a_key)
	{
		Debug("Looking up kit by key: '{}'", a_key);

		auto& cache = GetSingleton()->m_cache;
		auto it = cache.find(a_key);

		if (it == cache.end()) {
			Warn("Kit Lookup failed: key '{}' not found in cache", a_key);
			return nullptr;
		}

		return &it->second;
	}

	// Returns items in a given kit as BaseObject vector.
	std::vector<BaseObject> EquipmentConfig::GetItems(const Kit& a_kit)
	{
		Debug("Getting items for kit: '{}'", a_kit.m_key);

		std::vector<BaseObject> items;
		
		for (auto& kitItem : a_kit.m_items) {
			RE::TESForm* form = RE::TESForm::LookupByEditorID(kitItem->m_editorid);

			if (form) {
				items.push_back(BaseObject(form, Ownership::Kit, 0));
			} else {
				items.push_back(BaseObject(kitItem->m_name, kitItem->m_editorid, kitItem->m_plugin, Ownership::Kit, 0));
			}
		}
		
		return items;
	}

	// Returns reference to the runtime equipment list cache.
	std::unordered_map<std::string, Kit>& EquipmentConfig::GetEquipmentList()
	{
		return GetSingleton()->m_cache;
	}

	// Recompute m_knownTags from the union of all kits' tag sets.
	void EquipmentConfig::RebuildKnownTags()
	{
		auto* self = GetSingleton();
		self->m_knownTags.clear();
		for (const auto& [key, kit] : self->m_cache) {
			for (const auto& tag : kit.GetTags()) {
				self->m_knownTags.insert(tag);
			}
		}
		Trace("Rebuilt known tags: {} unique", self->m_knownTags.size());
	}

	// Sorted list of every tag currently present on any loaded kit.
	std::vector<std::string> EquipmentConfig::GetKnownTags()
	{
		const auto& set = GetSingleton()->m_knownTags;
		return std::vector<std::string>(set.begin(), set.end());
	}

	int EquipmentConfig::DeleteTagFromAllKits(const std::string& a_tag)
	{
		if (a_tag.empty()) return 0;

		int modified = 0;
		auto& cache = GetSingleton()->m_cache;
		for (auto& [key, kit] : cache) {
			auto tags = kit.GetTags();
			auto it = std::find(tags.begin(), tags.end(), a_tag);
			if (it == tags.end()) continue;
			tags.erase(it);
			kit.SetTags(tags);
			SaveKit(kit); // also rebuilds known tags
			modified++;
		}

		Info("Removed tag '{}' from {} kit(s)", a_tag, modified);
		return modified;
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

	// Snapshot a reference's current inventory into a new kit. When a_wornOnly is
	// true, only items flagged as worn by the game engine are included — yields
	// an "outfit" kit. When false, every inventory entry with count > 0 lands in
	// the kit (amount + equipped state preserved per item).
	Kit EquipmentConfig::CreateKitFromReference(const std::string& a_name, RE::TESObjectREFR* a_reference, bool a_wornOnly)
	{
		if (!a_reference) {
			Warn("CreateKitFromReference: null reference");
			return Kit{};
		}

		auto kit = CreateKit(a_name);
		if (!kit) {
			return Kit{};
		}

		auto inventory = a_reference->GetInventory();
		for (auto& [obj, data] : inventory) {
			auto& [count, entry] = data;
			if (count <= 0 || !entry) continue;

			const bool worn = entry->IsWorn();
			if (a_wornOnly && !worn) continue;

			auto baseObject = BaseObject(obj, Ownership::Item, 0, 0, static_cast<int>(count), worn);
			kit.m_items.emplace_back(CreateKitItem(baseObject));
		}

		SaveKit(kit);
		Info("Created kit '{}' from reference ({} items, wornOnly={})",
			kit.m_key, kit.m_items.size(), a_wornOnly);
		return kit;
	}

	// Helper method to create a kit from a pre-existing outfit form.
	bool EquipmentConfig::CreateKitFromOutfit(const std::string& a_name, RE::BGSOutfit* a_outfit, uint16_t a_level)
	{
		if (!a_outfit) return false;
		Debug("Creating Kit {} from outfit {}.", a_name, po3_GetEditorID(a_outfit->GetFormID()));

		if (auto kit = CreateKit(a_name); kit) {
			auto resolved = Commands::ResolveOutfitItems(a_outfit, Commands::GetPlayerReference(), a_level);

			for (auto& entry : resolved) {
				auto baseObject = BaseObject(entry.object, Ownership::Outfit, 0, 0, entry.count);
				Trace(" - Adding {} from Outfit to Kit", baseObject.GetEditorID());
				kit.m_items.emplace_back(CreateKitItem(std::move(baseObject)));
			}

			SaveKit(kit);
			Trace("Created {} with {} items", a_name, std::ssize(kit.m_items));
			return true;
		}

		return false;
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
		new_item->m_formID 	    = a_item.GetBaseFormID();

		return new_item;
	}

	std::shared_ptr<KitSpell> EquipmentConfig::CreateKitSpell(const BaseObject& a_object)
	{
		auto new_spell = std::make_shared<KitSpell>();

		new_spell->m_plugin   = a_object.GetPluginName();
		new_spell->m_name     = a_object.GetName();
		new_spell->m_editorid = a_object.GetEditorID();
		new_spell->m_formID   = a_object.GetBaseFormID();

		return new_spell;
	}
}
