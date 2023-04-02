#include "imgui_utilites.h"
#include "imgui/imgui.h"

#include <iostream>
namespace ImguiUtils {
namespace fs = std::filesystem;
template <class T> // make hex string, because we can
std::string toStringCustom(T t, std::ios_base& (*f)(std::ios_base&))
{
    std::ostringstream oss;
    oss << f << t;
    return oss.str();
}

FileSystemNavigator::FileSystemNavigator(const std::string& name)
    : m_ImGuiName(name + "###" + toStringCustom((size_t)this, std::hex))
    , m_currentEntry(DOCUMENTS_DIR)
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

void FileSystemNavigator::showInImGUI()
{
    if (ImGui::TreeNode(m_ImGuiName.c_str())) {
        if (!m_isOpenInImgui) {
            retrievePathList();
            m_isOpenInImgui = true;
        }

        if (ImGui::BeginListBox("###File navigator list", ImVec2(0, 500))) {
            const auto& entryList = getEntryList();

            for (int i = 0; i < entryList.size(); i++) {
                const bool is_selected = (m_selectedItemIdxImGui == i);
                const std::string& filename = entryList[i].visibleName;
                const auto& currentEntry = entryList[i].entry;

                ImGui::PushStyleColor(ImGuiCol_Text, entryList[i].color);

                if (ImGui::Selectable(filename.c_str(), is_selected)) {
                    m_selectedItemIdxImGui = i;
                    const auto* newEntry = getEntryByIndex(m_selectedItemIdxImGui);
                    if (newEntry) {
                        if (newEntry->entry.is_directory()) {
                            retrievePathList(newEntry->entry.path());
                            m_selectedItemIdxImGui = 0;
                        }
                    }
                }

                ImGui::PopStyleColor();

                if (is_selected)
                    ImGui::SetItemDefaultFocus();
            }
            ImGui::EndListBox();
        }

        ImGui::Text("AdditionalText");

        ImGui::TreePop();
    } else {
        m_isOpenInImgui = false;
    }
}

} // ImguiUtils
