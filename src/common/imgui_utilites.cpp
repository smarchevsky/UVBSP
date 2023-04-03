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
    , m_ImGuiExtSensitiveCheckboxName("Extension sensitive###" + m_thisPtrHashStr)
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
        if (e.entry.path().extension() == newExt)
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

            const bool isDir = entry.is_directory();
            fs::path fileName = entry.path().filename();
            fs::path fileExt = fileName.extension();

            std::string fileNameStr = entry.path().filename();

            if (isDir) {
                newEntryList.push_back({ entry, fileNameStr, s_defaultFileVisualColor });
            } else {
                if (m_extensionSensitive) {
                    if (m_extensionFileInfoMap.find(fileExt) != m_extensionFileInfoMap.end()) {
                        newEntryList.push_back({ entry, "  " + fileNameStr, getColorByExt(fileExt) });
                    }
                } else {
                    newEntryList.push_back({ entry, "  " + fileNameStr, getColorByExt(fileExt) });
                }
            }
        }
        m_entryList = std::move(newEntryList);
        m_currentEntry = newEntry.path();
        m_currentPathText = m_currentEntry;

        m_mustFocusListBox = true;
        m_selectedItemIdx = 0;
    }
    static int retrievePathListCounter = 0;
    LOG("Path retrieved num: " << ++retrievePathListCounter);
}

bool FileSystemNavigator::showInImGUI()
{
    // ImGui::SetNextWindowCollapsed(true, ImGuiCond_Appearing);
    ImGui::SetNextWindowSize(ImVec2(m_width, m_height));
    if (ImGui::Begin(m_ImGuiWidgetName.c_str(), &m_isOpenInImgui, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar)) {
        if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Escape))) {
            shouldClose();
        }

        ImGui::Text("%s", m_currentPathText.c_str());
        ImGui::Checkbox("Extension sensitive##65536", &m_extensionSensitive);
        if (m_extensionSensitive != m_extensionSensitivePrev) {
            retrievePathList(m_currentEntry);
            m_extensionSensitivePrev = m_extensionSensitive;
        }

        if (ImGui::BeginListBox(m_ImGuiFileListBoxName.c_str(), ImVec2(m_width, m_height - 126))) {
            const auto& entryList = getEntryList();
            if (m_mustFocusListBox) {
                ImGui::SetKeyboardFocusHere(0);
                m_mustFocusListBox = false;
            }

            for (int i = 0; i < entryList.size(); i++) {
                const bool is_selected = (m_selectedItemIdx == i);
                const std::string& filename = entryList[i].visibleName;

                ImGui::PushStyleColor(ImGuiCol_Text, entryList[i].visibleNameColor);
                if (ImGui::Selectable(filename.c_str(), is_selected)) { // on item selected

                    m_selectedItemIdx = i;

                    if (const auto* selectedEntryPtr = getEntryByIndex(m_selectedItemIdx)) {

                        if (selectedEntryPtr->entry.is_directory()) {
                            retrievePathList(selectedEntryPtr->entry.path());

                        } else if (selectedEntryPtr->entry.is_regular_file()) {
                            const fs::path& filePath = selectedEntryPtr->entry.path();
                            if (runOpenFileFunction(filePath))
                                shouldClose();
                        }
                    }
                }
                ImGui::PopStyleColor();

                if (is_selected)
                    ImGui::SetItemDefaultFocus();
            }
            ImGui::EndListBox();

            ImGui::SetNextItemWidth(m_width);
            if (ImGui::InputText(m_ImGuiTextBoxName.c_str(), &m_selectedFilename)) {
            }
        }
    }
    ImGui::End();

    if (false) {
        if (ImGui::Begin("Save file", &m_isOpenInImgui, ImGuiWindowFlags_NoCollapse)) {
        }
        ImGui::End();
    }

    return m_isOpenInImgui;
}

bool FileSystemNavigator::runOpenFileFunction(const fs::path& filePath)
{
    const fs::path& fileName = filePath.filename();
    m_selectedFilename = fileName;

    const std::string& ext = fileName.extension();
    auto foundExtensionOpenFunctionPair = m_extensionFileInfoMap.find(ext);
    if (foundExtensionOpenFunctionPair != m_extensionFileInfoMap.end()) {
        const auto& function = foundExtensionOpenFunctionPair->second.function;
        if (function) {
            function(filePath);
            return true;
        }
    }
    return false;
}
} // ImguiUtils
