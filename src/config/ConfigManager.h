#pragma once

#include <filesystem>
#include <fstream>

#include "external/json_serializers.cpp"

namespace Modex
{
	struct SerializedObject {
		std::string plugin;
		std::string editorid;
		uint32_t refid = 0;
		Ownership owner = Ownership::None;

		bool operator==(const SerializedObject& other) const {
			if (this->owner != other.owner) {
				return false;
			} else if (this->refid != 0 && other.refid != 0) {
				return this->refid == other.refid;
			} else if (this->refid == 0 && other.refid != 0) {
				return false;
			} else if (this->refid != 0 && other.refid == 0) {
				return false;
			} else {
				return (this->plugin == other.plugin) && (this->editorid == other.editorid);
			}
		}
	};

	NLOHMANN_JSON_SERIALIZE_ENUM(Ownership, {
		{Ownership::None, 0},
		{Ownership::Item, 1},
		{Ownership::Actor, 2},
		{Ownership::Kit, 3},
		{Ownership::Object, 4},
		{Ownership::Cell, 5},
		{Ownership::Outfit, 6},
		{Ownership::All, 7}
	});

	NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(SerializedObject, plugin, editorid, refid, owner)

	static_assert(static_cast<uint32_t>(Ownership::All) == 7, "Ownership enum changed! Update NLOHMANN_JSON_SERIALIZE_ENUM");

	class ConfigManager
	{
	protected:
		nlohmann::json            m_data;
		std::filesystem::path     m_file_path;
		bool                      m_initialized;

	public:
		inline static const std::filesystem::path FILTER_DIRECTORY = std::filesystem::path("data") / "interface" / "modex" / "user" / "filters";
		inline static const std::filesystem::path EQUIPMENT_DIRECTORY = std::filesystem::path("data") / "interface" / "modex" / "user" / "equipment";
		inline static const std::filesystem::path THEME_DIRECTORY = std::filesystem::path("data") / "interface" / "modex" / "user" / "themes";

		virtual ~ConfigManager() = default;

		virtual bool Load(bool a_create);
		virtual bool Save();
		virtual void Clear();

		void LoadState(const std::string& a_key);
		void SaveState(const std::string& a_key);
		virtual nlohmann::json SerializeState() const;
		virtual void DeserializeState(const nlohmann::json& a_state);
		
		void Add(const std::string& a_key);
		bool Has(const std::string& a_key) const;
		void Remove(const std::string& a_key);

		void GetAsList(std::vector<std::string>& outList) const;
		static void CreateAndDumpJson(const std::filesystem::path& a_path, const nlohmann::json& a_data);

		template<typename T>
		T Get(const std::string& key, const T& defaultValue = T()) const;
		
		template<typename T>
		void Set(const std::string& key, const T& value);

		void SetFilePath(const std::filesystem::path& a_path) { m_file_path = a_path; }
		const std::filesystem::path& GetFilePath() const { return m_file_path; }

		nlohmann::json& GetData() { return m_data; }
		// const nlohmann::json& GetData() const { return m_data; }
	};
}

template<typename T>
T Modex::ConfigManager::Get(const std::string& key, const T& defaultValue) const {
	auto it = m_data.find(key);
	if (it != m_data.end()) {
		try {
		   return it->get<T>();
		} catch (const nlohmann::json::exception& e) {
			   ASSERT_MSG(true, "ConfigManager -> Get: Failed to get key '{}' with error: {}", key, e.what());
				return defaultValue;
			}
		}
		return defaultValue;
	}

template<typename T>
void Modex::ConfigManager::Set(const std::string& key, const T& value) {
	m_data[key] = value;
}
