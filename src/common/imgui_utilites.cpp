#include "imgui_utilites.h"
#include "imgui/imgui.h"
#include "imgui/imgui_stdlib.h"

#include <iostream>
namespace ImguiUtils {

#ifndef LOG
#define LOG(x) std::cout << x << std::endl
#endif

template <class T> // make hex string, because I can (copy from StackOverflow)
std::string toStringCustom(T t, std::ios_base& (*f)(std::ios_base&))
{
    std::ostringstream oss;
    oss << f << t;
    return oss.str();
}

FileSystemNavigator::FileSystemNavigator(FileAction action, const std::string& name, const fs::path& path)
    : m_thisPtrHashStr(toStringCustom((size_t)this, std::hex))
    , m_ImGuiWidgetName(name + "###" + m_thisPtrHashStr)
    , m_ImGuiFileListBoxName("###FileList" + m_thisPtrHashStr)
    , m_ImGuiTextBoxName("###InputTextBlock" + m_thisPtrHashStr)
    , m_currentEntry(DOCUMENTS_DIR)
    , m_fileAction(action)
{
    retrievePathList(m_currentEntry);
}

void FileSystemNavigator::addSupportedExtension(const fs::path& newExt,
    FileInteractionFunction func, FileVisualColor color)
{
    m_extensionFileInfoMap[newExt] = SupportedFileInfo { func, color };

    for (auto& e : m_entryList) {
        const auto& ext = e.entry.path().extension();
        if (ext == newExt)
            e.visibleNameColor = color;
    }
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
            FileVisualColor fileColor = s_defaultFileVisualColor;
            const bool isDir = entry.is_directory();
            if (!isDir) {
                newFileName = "  " + newFileName;
                fileColor = getColorByExt(entry.path().extension());
            }

            newEntryList.push_back({ entry, newFileName, fileColor });
        }
        m_entryList = std::move(newEntryList);
        m_currentEntry = newEntry.path();
    }
}

bool FileSystemNavigator::showInImGUI()
{
    if (ImGui::Begin("Save file", &m_isOpenInImgui, ImGuiWindowFlags_NoCollapse)) {
        ImGui::Text("Random text");

        if (ImGui::BeginListBox(m_ImGuiFileListBoxName.c_str(), ImVec2(0, 500))) {
            const auto& entryList = getEntryList();

            for (int i = 0; i < entryList.size(); i++) {
                const bool is_selected = (m_selectedItemIdxImGui == i);
                const std::string& filename = entryList[i].visibleName;

                ImGui::PushStyleColor(ImGuiCol_Text, entryList[i].visibleNameColor);
                if (ImGui::Selectable(filename.c_str(), is_selected)) {
                    m_selectedItemIdxImGui = i;

                    if (const auto* selectedEntryPtr = getEntryByIndex(m_selectedItemIdxImGui)) {

                        if (selectedEntryPtr->entry.is_directory()) {
                            retrievePathList(selectedEntryPtr->entry.path());
                            m_selectedItemIdxImGui = 0;

                        } else if (selectedEntryPtr->entry.is_regular_file()) {
                            const fs::path& filePath = selectedEntryPtr->entry.path();
                            const fs::path& fileName = filePath.filename();
                            m_selectedFilename = fileName;

                            const std::string& ext = fileName.extension();
                            auto foundExtensionOpenFunctionPair = m_extensionFileInfoMap.find(ext);
                            if (foundExtensionOpenFunctionPair != m_extensionFileInfoMap.end()) {
                                const auto& function = foundExtensionOpenFunctionPair->second.function;
                                if (function)
                                    function(filePath);
                                m_isOpenInImgui = false;
                            }
                        }
                    }
                }

                ImGui::PopStyleColor();

                if (is_selected)
                    ImGui::SetItemDefaultFocus();
            }
            ImGui::EndListBox();

            if (ImGui::InputText(m_ImGuiTextBoxName.c_str(), &m_selectedFilename)) {
            }
        }
    }
    ImGui::End();

    return m_isOpenInImgui;
}

} // ImguiUtils