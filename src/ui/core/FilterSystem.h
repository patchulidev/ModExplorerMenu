#pragma once

#include "data/BaseObject.h"
#include "config/ConfigManager.h"

namespace Modex
{
	enum class FilterBehavior {
		SINGLE_SELECT,    // Only one child can be active
		MULTI_SELECT,     // Multiple children can be active
		AUTOMATIC         // All children always show (no selection needed) (unused)
	};

	enum class FilterLogic : uint32_t {
		OR = 0,
		AND
	};

	static inline std::vector<std::string> GetFilterLogicStrings() {
		std::vector<std::string> out = { "OR", "AND" };
		return out;
	}

	struct FilterRule {
		std::string property;
		std::string op;
		std::string value;
		
		bool IsEmpty() const {
			return property.empty();
		}
		
		static FilterRule FromJson(const nlohmann::json& j) {
			FilterRule rule;

			if (j.contains("rule")) {
				const auto& ruleJson = j["rule"];
				rule.property = ruleJson.value("property", "");
				rule.op = ruleJson.value("operator", "equals");
				
				// Handle different value types from JSON
				if (ruleJson.contains("value")) {
					if (ruleJson["value"].is_boolean()) {
						rule.value = ruleJson["value"].get<bool>() ? "true" : "false";
					} else if (ruleJson["value"].is_string()) {
						rule.value = ruleJson["value"].get<std::string>();
					} else if (ruleJson["value"].is_number_integer()) {
						rule.value = std::to_string(ruleJson["value"].get<int>());
					} else if (ruleJson["value"].is_number_float()) {
						rule.value = std::to_string(ruleJson["value"].get<float>());
					}
				}
			}

			return rule;
		}
		
		// Evaluate this rule against an item
		bool Evaluate(const BaseObject* item) const {
			if (IsEmpty() || ! item) {
				return true;  // No rule = always passes
			}
			
			// Get the property value from the item
			std::string itemValue = item->GetPropertyByString(property);
			std::transform(itemValue.begin(), itemValue.end(), itemValue.begin(), ::tolower);

			std::string valueLower = value;
			std::transform(valueLower.begin(), valueLower.end(), valueLower.begin(), ::tolower);
			
			// Apply the operator
			if (op == "equals") {
				return itemValue == valueLower;
			}
			else if (op == "not_equals") {
				return itemValue != valueLower;
			}
			else if (op == "contains") {
				return itemValue.find(valueLower) != std::string::npos;
			}
			else if (op == "not_contains") {
				return itemValue.find(valueLower) == std::string::npos;
			}
			else if (op == "greater_than") {
				try {
					return std::stof(itemValue) > std::stof(valueLower);
				} catch (...) {
					return false;
				}
			}
			else if (op == "less_than") {
				try {
					return std::stof(itemValue) < std::stof(valueLower);
				} catch (...) {
					return false;
				}
			}
			else if (op == "greater_or_equal") {
				try {
					return std::stof(itemValue) >= std::stof(valueLower);
				} catch (...) {
					return false;
				}
			}
			else if (op == "less_or_equal") {
				try {
					return std::stof(itemValue) <= std::stof(valueLower);
				} catch (...) {
					return false;
				}
			}
			else if (op == "starts_with") {
				return itemValue.find(valueLower) == 0;
			}
			else if (op == "ends_with") {
				if (valueLower.length() > itemValue.length()) return false;
				return itemValue. compare(itemValue.length() - valueLower.length(), valueLower.length(), valueLower) == 0;
			}

			Error("Unknown operator in filter rule: " + op);
			return true;
		}
	};

	struct FilterNode {
		std::string id;
		std::string displayName;
		FilterBehavior behavior;
		FilterRule rule;
		
		std::vector<std::shared_ptr<FilterNode>> children;
		FilterNode* parent = nullptr;
		bool isSelected = false;
		
		int colorIndex = -1;
		
		void AddChild(std::shared_ptr<FilterNode> child) {
			child->parent = this;
			children.push_back(child);
		}
		
		// Factory method to create from JSON
		static std::shared_ptr<FilterNode> FromJson(const nlohmann::json& j) {
			auto node = std::make_shared<FilterNode>();
			
			// If this is the top-level object with "FilterProperty" array, create a virtual root
			if (j.contains("FilterProperty")) {
				const auto& filtersArray = j["FilterProperty"];

				if (filtersArray.is_array()) {
					node->id = "__root__";
					node->displayName = "Root";
					node->behavior = FilterBehavior::SINGLE_SELECT;

					// NOTE: Using MULTI_SELECT on root nodes is kind of weird. Would require
					// restructuring filter logic. E.g. Armor -> Type -> Light && Weapon -> Type ->
					// Sword should show both Light Armor && Sword Weapon Type? Because underlying system
					// would not yield those results.
					
					// Add each filter as a child
					for (const auto& filterJson : filtersArray) {
						auto child = FromJson(filterJson);
						node->AddChild(child);
					}
					
					return node;
				}
			}

			// Required fields
			node->id = j.at("id").get<std::string>();
			node->displayName = j.at("displayName").get<std::string>();
			
			// NOTE: We don't treat displayName as a localeString. Translators will have to manually
			// perform translations over filter JSON files if needed. We could expose this
			// optionally I guess?
			
			// Optional toggle behavior
			if (j.contains("behavior")) {
				std::string behaviorStr = j.at("behavior").get<std::string>();
				node->behavior = StringToBehavior(behaviorStr);
			} else {
				node->behavior = FilterBehavior::MULTI_SELECT;
			}

			node->rule = FilterRule::FromJson(j);
			
			// Recursively load children
			if (j.contains("children")) {
				for (const auto& childJson : j.at("children")) {
					auto child = FromJson(childJson);
					node->AddChild(child);
				}
			}

			return node;
		}
		
		// Convert behavior string to enum
		static FilterBehavior StringToBehavior(const std::string& str) {
			if (str == "single_select") return FilterBehavior::SINGLE_SELECT;
			if (str == "multi_select") return FilterBehavior::MULTI_SELECT;
			if (str == "automatic") return FilterBehavior::AUTOMATIC;
			return FilterBehavior::SINGLE_SELECT; // Default
		}
		
		static std::string BehaviorToString(FilterBehavior behavior) {
			switch (behavior) {
				case FilterBehavior::SINGLE_SELECT: return "single_select";
				case FilterBehavior::MULTI_SELECT: return "multi_select";
				case FilterBehavior::AUTOMATIC:  return "automatic";
				default:  return "single_select";
			}
		}

			// Get all active leaf nodes (endpoints of selections)
		void GetActiveLeaves(std::vector<FilterNode*>& outLeaves) {
			if (isSelected) {
				if (children.empty()) {
					// This is a leaf node
					outLeaves.push_back(this);
				} else {
					// Check children
					bool anyChildSelected = false;
					for (auto& child : children) {
						if (child->isSelected) {
							anyChildSelected = true;
							child->GetActiveLeaves(outLeaves);
						}
					}
					
					// If no children selected, this is effectively a leaf
					if (!anyChildSelected) {
						outLeaves.push_back(this);
					}
				}
			}
		}
	};


	class FilterSystem : public ConfigManager
	{
	private:
		std::shared_ptr<FilterNode>                     m_rootNode;
		std::unordered_map<std::string, FilterNode*>    m_nodeRegistry;
		std::function<void()>                           m_filterChangeCallback;

	public:
		FilterSystem(const std::filesystem::path& a_path) :
			m_rootNode(nullptr)
		{
			ConfigManager::m_file_path = a_path;
		}

		// overrides
		virtual bool Load(bool a_create) override;
		nlohmann::json SerializeState() const override;
		void DeserializeState(const nlohmann::json& a_state) override;
		
		// members
		FilterNode* FindNode(const std::string& a_id);
		void HandleNodeClick(FilterNode* a_parent, FilterNode* a_clicked);
		bool MatchesFilters(const BaseObject& a_item);
		void RenderNodeAndChildren(FilterNode* node, const float& a_width, int a_depth = 0);
		bool ShouldShowItem(const BaseObject* a_item) const;

		void ClearActiveNodes();
		void ActivateNodeByID(const std::string& a_id, bool a_select);
		
		std::list<FilterNode*> GetAllNodes() const;

		void SetSystemCallback(std::function<void()> callback) {
			m_filterChangeCallback = callback;
		}

	private:
		void RegisterNodeRecursive(FilterNode* a_node);
		void ClearChildren(FilterNode* a_node);
		void BuildVisibleCategories(const std::string &a_id, std::vector<std::string> &a_out);
		ImVec4 GetParentColor(FilterNode* a_node);
		FilterNode* GetSelectedRootNode() const;
		
		bool ItemMatchesNode(const BaseObject& a_item, const std::vector<std::string>& a_path);
		bool ItemMatchesPath(const BaseObject* item, const std::vector<std::string>& path) const;
		bool HasAnySelectedDescendant(FilterNode* node) const;
		void CollectActivePaths(FilterNode* node,std::vector<std::string> currentPath, std::vector<std::vector<std::string>>& outPaths) const;  
		void CollectSelectedNodesByParent(FilterNode* node, std::map<FilterNode*, std::vector<FilterNode*>>& outMap) const;
		void CollectSelectedNodesByID(FilterNode* a_node, std::vector<std::string>& a_out) const;

		void AssignColorIndices();
		void AssignColorIndicesToChildren(FilterNode *node);
	};
}
