#include "imgui_utilites.h"
#include "imgui/imgui.h"

namespace fs = std::filesystem;

FileSystemNavigator::FileSystemNavigator()
    : m_currentEntry(PROJECT_DIR)
{
    addSupportedExtension(".uvbsp", nullptr, IM_COL32(255, 255, 25, 255));
}

void FileSystemNavigator::addSupportedExtension(const std::string& ext, FileOpenFunction func, FileVisualColor color)
{
    m_extensionFileInfoMap.insert({ ext, { func, color } });
}

void FileSystemNavigator::retrievePathList(const fs::path& newPath)
{
    fs::directory_entry newEntry(newPath);

    if (newEntry.exists() && newEntry.is_directory()) {

        decltype(m_entryList) newEntryList;
        fs::path parentPath(newPath.parent_path());
        fs::directory_entry parentEntry(parentPath);

        if (parentEntry.exists())
            newEntryList.push_back({ parentEntry, std::string("..") });

        for (const fs::directory_entry& entry : fs::directory_iterator(newEntry,
                 fs::directory_options::skip_permission_denied)) {

            std::string newFileName = entry.path().filename();
            auto fileColorIterator = m_extensionFileInfoMap.end();

            if (!entry.is_directory()) {
                fileColorIterator = m_extensionFileInfoMap.find(entry.path().extension());
                newFileName = "  " + newFileName;
            }

            FileVisualColor fileColor = (fileColorIterator == m_extensionFileInfoMap.end())
                ? IM_COL32(255, 255, 255, 255)
                : fileColorIterator->second.color;

            newEntryList.push_back({ entry, newFileName, fileColor });
        }
        m_entryList = std::move(newEntryList);
        m_currentEntry = newEntry.path();
    }
}

void ImGui_utilites::runTreeNavigator(FileSystemNavigator& fsNavigator)
{
    static bool fileNavigatorListOpen = false;
    static int item_current_idx = 0;

    if (ImGui::TreeNode("File navigator")) {
        if (!fileNavigatorListOpen) {
            fsNavigator.retrievePathList();
            fileNavigatorListOpen = true;
        }

        if (ImGui::BeginListBox("###File navigator list", ImVec2(0, 500))) {
            const auto& entryList = fsNavigator.getEntryList();
            // ImGui::SetColorEditOptions();
            for (int i = 0; i < entryList.size(); i++) {
                const bool is_selected = (item_current_idx == i);
                const std::string& filename = entryList[i].visibleName;
                const auto& currentEntry = entryList[i].entry;

                ImGui::PushStyleColor(ImGuiCol_Text, entryList[i].color);

                if (ImGui::Selectable(filename.c_str(), is_selected)) {
                    item_current_idx = i;
                    const auto* newEntry = fsNavigator.getEntryByIndex(item_current_idx);
                    if (newEntry) {
                        if (newEntry->entry.is_directory())
                            fsNavigator.retrievePathList(newEntry->entry.path());
                    }
                }

                ImGui::PopStyleColor();

                if (is_selected)
                    ImGui::SetItemDefaultFocus();
            }
            ImGui::EndListBox();
        }

        // Custom size: use all width, 5 items tall
        ImGui::Text("Full-width:");

        ImGui::TreePop();
    } else {
        fileNavigatorListOpen = false;
    }
}
