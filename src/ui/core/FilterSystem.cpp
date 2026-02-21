#include "FilterSystem.h"
#include "localization/Locale.h"
#include "config/ThemeConfig.h"
#include "ui/components/UIContainers.h"

namespace Modex
{
	nlohmann::json FilterSystem::SerializeState() const
	{
		nlohmann::json state = nlohmann::json::object();
		std::vector<std::string> selected_nodes;

		if (m_rootNode) {
			CollectSelectedNodesByID(m_rootNode.get(), selected_nodes);
		}

		state["activeNodes"] = selected_nodes;

		return state;
	}

	void FilterSystem::DeserializeState(const nlohmann::json& a_state)
	{
		if (!m_rootNode) return;

		ClearActiveNodes();

		if (a_state.contains("activeNodes") && a_state["activeNodes"].is_array()) {
			for (const auto& id : a_state["activeNodes"]) {
				if (id.is_string()) {
					ActivateNodeByID(id.get<std::string>(), true);
				}
			}
		}
	}

	// Recursively build a vector of selected node IDs for persistent state.
	void FilterSystem::CollectSelectedNodesByID(FilterNode* a_node, std::vector<std::string>& a_out) const
	{
		if (!a_node) return;

		if (a_node->isSelected && a_node->id != "__root__") {
			a_out.push_back(a_node->id);
		}

		for (const auto& child : a_node->children) {
			CollectSelectedNodesByID(child.get(), a_out);
		}
	}

	bool FilterSystem::Load(bool a_create)
	{
		ASSERT_MSG(a_create && m_file_path.empty(), "FilterSystem::Load() called before setting file path!");

		if (!ConfigManager::Load(a_create))
			return m_initialized;

		if (m_data.contains("FilterProperty")) {
			const auto& j = m_data;
			m_rootNode = FilterNode::FromJson(j);

			AssignColorIndices();

			m_nodeRegistry.clear();
			RegisterNodeRecursive(m_rootNode.get());

			return m_initialized = true;
		}

		ASSERT_MSG(a_create, "No FilterProperty found in JSON.\n\nFile: {}", m_file_path.string());

		return m_initialized = !a_create;
	}

	FilterNode* FilterSystem::FindNode(const std::string& id)
	{
		auto it = m_nodeRegistry.find(id);
		return it != m_nodeRegistry.end() ? it->second : nullptr;
	}

	void FilterSystem::ClearActiveNodes()
	{
		if (m_rootNode) {
			m_rootNode->isSelected = false;
			ClearChildren(m_rootNode.get());
		}
	}

	void FilterSystem::ClearChildren(FilterNode* a_node) {
		for (auto& child : a_node->children) {
			child->isSelected = false;
			ClearChildren(child.get());
		}
	}

	// Return a list of nodes in order from JSON
	std::list<FilterNode*> FilterSystem::GetAllNodes() const {
		std::list<FilterNode*> outList;
		if (!m_rootNode) return outList;

		std::function<void(FilterNode*)> traverse = [&](FilterNode* node) {
			outList.push_back(node);
			for (const auto& child : node->children) {
				traverse(child.get());
			}
		};

		traverse(m_rootNode.get());
		return outList;
	}

	void FilterSystem::ActivateNodeByID(const std::string& a_id, bool a_select) {
		FilterNode* node = FindNode(a_id);

		if (node) {
			node->isSelected = a_select;

			if (node->parent) { // Ensure parent nodes are selected to make this node visible
				FilterNode* current = node->parent;
				while (current) {
					current->isSelected = true;
					current = current->parent;
				}
			}
		}
	}

	FilterNode* FilterSystem::GetSelectedRootNode() const {
		if (!m_rootNode) return nullptr;

		for (const auto& child : m_rootNode->children) {
			if (child->isSelected) {
				return child.get();
			}
		}

		return nullptr;
	}

	void FilterSystem::HandleNodeClick(FilterNode* a_parent, FilterNode* a_clicked) {
		bool _change = false;
		bool isModifierDown = ImGui::GetIO().KeyCtrl || ImGui::GetIO().KeyShift;

		if (!a_parent || !a_clicked) return;

		switch (a_parent->behavior) {
			case FilterBehavior::SINGLE_SELECT:
				for (auto& sibling : a_parent->children) {
					if (sibling.get() == a_clicked) {
						sibling->isSelected = !sibling->isSelected;
						_change = true;
					} else {
						sibling->isSelected = false;
						ClearChildren(sibling.get());
						_change = true;
					}
				}
				break;
				
			case FilterBehavior::MULTI_SELECT:
				if (isModifierDown) {
					a_clicked->isSelected = !a_clicked->isSelected;
					_change = true;
					if (!a_clicked->isSelected) {
						ClearChildren(a_clicked);
					}
				} else {
					// Without Ctrl, behave like SINGLE_SELECT
					for (auto& sibling : a_parent->children) {
						if (sibling.get() == a_clicked) {
							sibling->isSelected = ! sibling->isSelected;
							_change = true;
						} else {
							sibling->isSelected = false;
							ClearChildren(sibling.get());
							_change = true;
						}
					}
				}

				break;
				
			case FilterBehavior::AUTOMATIC:
				break;
		}

		if (GetSelectedRootNode() == nullptr) {
			ClearActiveNodes();
		}

		if (_change && m_filterChangeCallback) {
			m_filterChangeCallback();
		}
	}

	void FilterSystem::RegisterNodeRecursive(FilterNode* a_node) {
		if (!a_node) return;

		m_nodeRegistry[a_node->id] = a_node;

		for (auto& child : a_node->children) {
			RegisterNodeRecursive(child.get());
		}
	}

	void FilterSystem::RenderNodeAndChildren(FilterNode* a_node, const float& a_width, int a_depth) {
		if (!a_node->children.empty()) {            
			const static int MAX_BUTTONS_PER_LINE = 10;
			const float total_nodes = min((float)a_node->children.size(), (float)MAX_BUTTONS_PER_LINE);
			const float item_spacing = ImGui::GetStyle().ItemSpacing.x;
			const float total_spacing = item_spacing * (total_nodes - 1.0f);
			const float button_width = (a_width - total_spacing) / total_nodes;
			
			ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal, 1.0f);

			int _count = 0;
			for (auto& child : a_node->children) {
				if (_count >= MAX_BUTTONS_PER_LINE) {
					_count = 0;
				} else {
					if (_count > 0) {
						ImGui::SameLine();
					}
				}
				
				ImVec4 button_color;

				if (child->colorIndex >= 0) {
					button_color = ThemeConfig::GetColor(std::format("FILTER_{}", child->colorIndex));
				} else {
					button_color = ThemeConfig::GetColor("PRIMARY");
				}

				if (UIContainers::TabButton(child->displayName.c_str(), ImVec2(button_width, 0.0f), child->isSelected, button_color)) {
					HandleNodeClick(a_node, child.get());
				}
				
				_count++;
			}
		}
		
		// Recursively render selected children
		for (auto& child : a_node->children) {
			if (child->isSelected) {
				RenderNodeAndChildren(child.get(), a_width, a_depth + 1);
			}
		}
	}

	// Helper function to check if any descendant is selected
	bool FilterSystem::HasAnySelectedDescendant(FilterNode* node) const {
		if (node->isSelected) return true;
		
		for (const auto& child : node->children) {
			if (HasAnySelectedDescendant(child.get())) {
				return true;
			}
		}
		
		return false;
	}

	void FilterSystem::CollectActivePaths(FilterNode* node, std::vector<std::string> currentPath, std::vector<std::vector<std::string>>& outPaths) const {
		// Always add current node to path (build full hierarchy)
		// Skip the root node itself as it doesn't represent a filter
		if (node != m_rootNode.get()) {
			currentPath.push_back(node->id);
		}
		
		// Leaf node
		if (node->children.empty()) {
			if (node->isSelected && ! currentPath.empty()) {
				outPaths.push_back(currentPath);
			}
			return;
		}
		
		// Check children
		bool anyChildSelected = false;
		for (const auto& child : node->children) {
			// Recurse if child is selected OR any of its descendants might be
			if (child->isSelected || HasAnySelectedDescendant(child.get())) {
				anyChildSelected = true;
				CollectActivePaths(child.get(), currentPath, outPaths);
			}
		}
		
		// This node is selected but no children are (effective leaf)
		if (node->isSelected && !anyChildSelected && !currentPath.empty()) {
			outPaths.push_back(currentPath);
		}
		
		// Root with no selections - return empty
		if (node == m_rootNode.get() && !anyChildSelected) {
			outPaths.clear();
		}
	}

	bool FilterSystem::ItemMatchesPath(const BaseObject* item, const std::vector<std::string>& path) const {
		// Item must match ALL rules in the path (AND logic)
		for (const auto& nodeId : path) {
			auto it = m_nodeRegistry.find(nodeId);
			if (it != m_nodeRegistry.end()) {
				FilterNode* node = it->second;
				
				// If this node has a rule, evaluate it
				if (!node->rule.IsEmpty()) {
					if (!node->rule.Evaluate(item)) {
						return false;  // Failed this rule
					}
				}
			}
		}
		return true;  // Passed all rules
	}

	bool FilterSystem::ItemMatchesNode(const BaseObject& a_item, const std::vector<std::string>& a_path) {
		// Item must match ALL rules in the path (AND logic)
		for (const auto& nodeId : a_path) {
			auto it = m_nodeRegistry.find(nodeId);
			if (it != m_nodeRegistry.end()) {
				FilterNode* node = it->second;
				
				// If this node has a rule, evaluate it
				if (!node->rule.IsEmpty()) {
					if (!node->rule.Evaluate(&a_item)) {
						return false;  // Failed this rule
					}
				}
			}
		}
		return true;  // Passed all rules
	}

	// Collect selected nodes grouped by their parent
	void FilterSystem::CollectSelectedNodesByParent(FilterNode* node, std::map<FilterNode*, std::vector<FilterNode*>>& outMap) const {
		if (node->isSelected && node->parent) {
			outMap[node->parent].push_back(node);
		}
		
		for (const auto& child : node->children) {
			CollectSelectedNodesByParent(child.get(), outMap);
		}
	}

	bool FilterSystem::ShouldShowItem(const BaseObject* a_item) const {
		if (!m_rootNode || !a_item) { return true; }
		
		std::map<FilterNode*, std::vector<FilterNode*>> nodesByParent;
		CollectSelectedNodesByParent(m_rootNode.get(), nodesByParent);
		
		// No filters active = show everything
		if (nodesByParent.empty()) {
			return true;
		}

		if (GetSelectedRootNode() == nullptr) {
			return true;
		}
		
		for (const auto& [parent, nodes] : nodesByParent) {
			bool matchedAnyInGroup = false;
			bool matchedAllInGroup = true;
			
			for (const auto* node : nodes) {
				if (!node->rule.IsEmpty()) {
					if (node->rule.Evaluate(a_item)) {
						matchedAnyInGroup = true;
					} else {
						matchedAllInGroup = false;
					}
				} else {
					matchedAnyInGroup = true;
				}
			}
			
			const FilterLogic logic = magic_enum::enum_cast<FilterLogic>(UserConfig::Get().filterLogic).value_or(FilterLogic::OR);

			switch (logic) {
				case FilterLogic::AND: // ALL must match.
					if (!matchedAllInGroup) {
						return false;
					}
					break;
					
				case FilterLogic::OR: // At least ONE match.
					if (!matchedAnyInGroup) {
						return false;
					}
					break;
			}
		}
		
		return true;
	}

	void FilterSystem::AssignColorIndices() {
		if (!m_rootNode) return;
		
		AssignColorIndicesToChildren(m_rootNode.get());
	}

	void FilterSystem::AssignColorIndicesToChildren(FilterNode* a_node) {
		// Calculate this node's depth
		int depth = 0;
		FilterNode* temp = a_node;
		while (temp->parent) {
			depth++;
			temp = temp->parent;
		}
		
		// Depth 1 nodes are:  Armor, Weapon, Alchemy, etc.
		// We want to assign colors to THEIR children (depth 2)
		if (depth == 1) {
			// Assign colors to children (Slots, Type, Playable, Enchanted, etc.)
			for (size_t i = 0; i < a_node->children.size(); i++) {
				a_node->children[i]->colorIndex = i % 10;
			}
		}
		// Depth 2+ nodes inherit from parent
		else if (depth >= 2) {
			if (a_node->parent && a_node->parent->colorIndex >= 0) {
				a_node->colorIndex = a_node->parent->colorIndex;
			}
		}
		
		// Recurse to all children regardless of depth
		for (auto& child : a_node->children) {
			AssignColorIndicesToChildren(child.get());
		}
	}
}
