#include "UIPopup.h"

#include "UICustom.h"
#include "UIContainers.h"
#include "localization/Locale.h"

namespace Modex 
{
	struct BrowserNode {
		std::vector<std::string> subdirectories;
		std::vector<std::string> files;
	};

	void BuildFileHierarchy(
		const std::vector<std::string>& itemList,
		std::map<std::string, BrowserNode>& outTree,
		std::vector<std::string>& outRootFiles)
	{
		outTree.clear();
		outRootFiles.clear();

		// Build tree structure from file path vector
		for (const auto& item : itemList) {
			std::filesystem::path itemPath(item);
			std::filesystem::path parentPath = itemPath.parent_path();
			
			if (parentPath.empty()) {
				outRootFiles.push_back(item);
				continue;
			}
			
			std::string parentStr = parentPath.string();
			outTree[parentStr].files.push_back(item);
			
			// Build subdirectory hierarchy
			std::filesystem::path currentPath;
			for (const auto& component : parentPath) {
				std::string parent = currentPath.string();
				currentPath = currentPath.empty() ? component : currentPath / component;
				std::string current = currentPath.string();
				
				auto& subdirs = outTree[parent].subdirectories;
				if (std::find(subdirs.begin(), subdirs.end(), current) == subdirs.end()) {
					subdirs.push_back(current);
				}
			}
		}
	}

	bool RenderFileItem(const std::string& filePath, ImGuiTreeNodeFlags baseFlags = 0)
	{
		std::string displayName = std::filesystem::path(filePath).filename().string();
		ImGuiTreeNodeFlags flags = baseFlags | ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
		
		ImGui::TreeNodeEx(filePath.c_str(), flags, "%s %s", ICON_LC_FILE_CODE, displayName.c_str());
		return ImGui::IsItemClicked();
	}

	bool RenderFolderItem(const std::string& folderPath, bool isOpen, bool asTreeNode = true)
	{
		std::string folder_name = std::filesystem::path(folderPath).filename().string();
		const char* folder_icon = isOpen ? ICON_LC_FOLDER_OPEN : ICON_LC_FOLDER_CLOSED;
		
		if (asTreeNode) {
			ImGui::SameLine();
			ImGui::TextUnformatted(folder_icon);
			ImGui::SameLine();
			ImGui::TextUnformatted(folder_name.c_str());
			return false; // Click handling done by TreeNodeEx
		} else {
			return ImGui::Selectable((std::string(folder_icon) + " " + folder_name).c_str(), false);
		}
	}

	void GetDirectoryContents(
		const std::string& dirPath,
		const std::map<std::string, BrowserNode>& tree,
		std::vector<std::string>& outFolders,
		std::vector<std::string>& outFiles)
	{
		outFolders.clear();
		outFiles.clear();

		// root -> nested
		if (dirPath.empty()) {
			auto it = tree.find("");
			if (it != tree.end()) {
				outFolders = it->second.subdirectories;
			}
		} else {
			auto it = tree.find(dirPath);
			if (it != tree.end()) {
				outFolders = it->second.subdirectories;
				outFiles = it->second.files;
			}
		}
	}

	void UIPopupBrowser::Draw()
	{
		auto width = ImGui::GetMainViewport()->Size.x * 0.50f;
		auto height = ImGui::GetMainViewport()->Size.y * 0.45f;
		const float center_x = ImGui::GetMainViewport()->Size.x * 0.5f;
		const float center_y = ImGui::GetMainViewport()->Size.y * 0.5f;
		const float pos_x = center_x - (width * 0.5f);
		const float pos_y = center_y - (height * 0.5f);
		
		DrawPopupBackground(m_alpha);
		
		ImGui::SetNextWindowSize(ImVec2(width, height));
		ImGui::SetNextWindowPos(ImVec2(pos_x, pos_y));
		ImGui::SetNextWindowFocus();

		ImGui::PushStyleVar(ImGuiStyleVar_Alpha, m_alpha);
		if (ImGui::Begin("Modex::Browser", NULL, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize)) {
			if (ImGui::IsWindowAppearing()) {
				ImGui::GetIO().ClearInputKeys();
				m_filteredBrowserList = m_pendingBrowserList;
			}

			static bool searchAllFiles = false;
			static char currentDirectory[256] = "";
			static std::map<std::string, BrowserNode> fileTree;
			static std::vector<std::string> rootFiles;

			BuildFileHierarchy(m_pendingBrowserList, fileTree, rootFiles);

			if (UICustom::Popup_MenuHeader(m_pendingBrowserTitle.c_str())) {
				ImGui::End();
				ImGui::PopStyleVar();
				DeclineSelection();
				return;
			}

			if (ImGui::BeginChild("Modex::Browser::Tags", ImVec2(ImGui::GetContentRegionAvail().x * 0.35f, 0), false)) {
				UICustom::SubCategoryHeader(Translate("HEADER_BROWSER"));
				
				// Recursive tree rendering
				std::function<void(const std::string&)> DrawTreeNode = [&](const std::string& path) {
					auto it = fileTree.find(path);
					if (it == fileTree.end()) return;

					const auto& node = it->second;
					
					// Draw subdirectories
					for (const auto& subdir : node.subdirectories) {
						ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_AllowOverlap;
						
						bool hasContent = fileTree.find(subdir) != fileTree.end() &&
										(!fileTree[subdir].subdirectories.empty() || !fileTree[subdir].files.empty());
						
						bool node_open = ImGui::TreeNodeEx(subdir.c_str(), flags, "");
						
						if (ImGui::IsItemClicked()) {
							ImFormatString(currentDirectory, IM_ARRAYSIZE(currentDirectory), "%s", subdir.c_str());
						}

						RenderFolderItem(subdir, node_open, true);
						
						if (node_open) {
							if (hasContent) {
								DrawTreeNode(subdir);
							}
							ImGui::TreePop();
						}
					}
					
					// Draw files in this directory
					for (const auto& file : node.files) {
						if (RenderFileItem(file)) {
							// File clicked - select it
							for (size_t i = 0; i < m_pendingBrowserList.size(); i++) {
								if (m_pendingBrowserList[i] == file) {
									m_currentSelection = file;
									AcceptSelection();
									break;
								}
							}
						}
					}
				};
				
				DrawTreeNode("");

				// Draw root-level files last
				if (!rootFiles.empty()) {
					ImGui::Unindent();
					for (const auto& file : rootFiles) {
						if (RenderFileItem(file)) {
							for (size_t i = 0; i < m_pendingBrowserList.size(); i++) {
								if (m_pendingBrowserList[i] == file) {
									m_currentSelection = file;
									AcceptSelection();
									break;
								}
							}
						}
					}
					ImGui::Indent();
				}
			}
			ImGui::EndChild();

			ImGui::SameLine();
			ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
			ImGui::SameLine();

			if (ImGui::BeginChild("Modex::Browser::List", ImVec2(0, 0), false)) {
				ImGui::AlignTextToFramePadding();
				ImGui::Text("%s", Translate("DIRECTORY"));
				ImGui::SameLine();
				ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
				
				if (ImGui::InputText("##Modex::Browser::SearchBar", currentDirectory, IM_ARRAYSIZE(currentDirectory))) {
					searchAllFiles = true;
				}

				if (ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_F, ImGuiInputFlags_RouteFromRootWindow)) {
					ImGui::SetKeyboardFocusHere(-1);
				}

				ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal);

				std::vector<std::string> folders;
				std::vector<std::string> files;
				std::string currentDir(currentDirectory);

				if (currentDir.empty()) {
					searchAllFiles = false;
				}

				if (searchAllFiles) {
					std::transform(currentDir.begin(), currentDir.end(), currentDir.begin(), ::tolower);
					for (const auto& item : m_pendingBrowserList) {
						std::string lower = item;
						std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
						if (lower.find(currentDir) != std::string::npos) {
							files.push_back(item);
						}
					}
				} else {
					GetDirectoryContents(currentDir, fileTree, folders, files);
				}
				
				ImGui::PushStyleVar(ImGuiStyleVar_SelectableTextAlign, ImVec2(0.0f, 0.5f));
				
				// Parent directory navigation
				if (!currentDir.empty()) {
					if (ImGui::Selectable(ICON_LC_FOLDER_UP " ...", false)) {
						std::filesystem::path parentPath = std::filesystem::path(currentDir).parent_path();
						ImFormatString(currentDirectory, IM_ARRAYSIZE(currentDirectory), "%s", parentPath.string().c_str());
					}
				}

				// Display folders
				if (!searchAllFiles) {
					for (const auto& folder : folders) {
						if (RenderFolderItem(folder, false, false)) {
							ImFormatString(currentDirectory, IM_ARRAYSIZE(currentDirectory), "%s", folder.c_str());
						}
					}
				}

				// Display files
				if (currentDir.empty()) {
					for (const auto& file : rootFiles) {
						std::string displayName = std::filesystem::path(file).filename().string();
						if (ImGui::Selectable((std::string(ICON_LC_FILE_CODE) + " " + displayName).c_str(), false)) {
							for (size_t i = 0; i < m_pendingBrowserList.size(); i++) {
								if (m_pendingBrowserList[i] == file) {
									m_currentSelection = file;
									AcceptSelection();
									break;
								}
							}
						}
					}
				} else {
					for (const auto& file : files) {
						std::string displayName = std::filesystem::path(file).filename().string();
						if (ImGui::Selectable((std::string(ICON_LC_FILE_CODE) + " " + displayName).c_str(), false)) {
							for (size_t i = 0; i < m_pendingBrowserList.size(); i++) {
								if (m_pendingBrowserList[i] == file) {
									m_currentSelection = file;
									AcceptSelection();
									break;
								}
							}
						}
					}
				}

				ImGui::PopStyleVar();
			}
			ImGui::EndChild();
		}
		ImGui::End();
		ImGui::PopStyleVar();

	}

	void UIPopupBrowser::PopupBrowser(const std::string& a_title, const std::vector<std::string>& a_list, std::function<void(const std::string&)> a_onSelectCallback)
	{
		m_pendingBrowserTitle = a_title;
		m_pendingBrowserList = a_list;
		m_onSelectCallback = a_onSelectCallback;
		m_currentSelection.clear();
		m_captureInput = true;
	}

	void UIPopupBrowser::AcceptSelection()
	{
		if (m_onSelectCallback && !m_currentSelection.empty()) {
			m_onSelectCallback(m_currentSelection);
		}

		this->CloseWindow();
	}

	void UIPopupBrowser::DeclineSelection()
	{
		this->CloseWindow();
	}
}
