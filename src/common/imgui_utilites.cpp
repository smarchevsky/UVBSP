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
    : m_strThisPtrHash(toStringCustom((size_t)this, std::hex))
    , m_strImGuiWidgetName(name + "###" + m_strThisPtrHash)
    , m_strImGuiExtSensitiveCheckboxName("Extension sensitive###" + m_strThisPtrHash)
    , m_strImGuiFileListBoxName("###FileList" + m_strThisPtrHash)
    , m_strImGuiTextBoxName("###InputTextBlock" + m_strThisPtrHash)
    , m_strImGuiOverwriteMsg(std::string("Riolli overwrite"))
    , m_pathCurrentEntry(DOCUMENTS_DIR)
    , m_fileAction(action)
{
    retrievePathList(m_pathCurrentEntry);
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
                FileVisualColor fileColor = s_defaultFileVisualColor;
                if (auto* info = getSupportedExtensionInfo(fileExt))
                    fileColor = info->color;
                newEntryList.push_back({ entry, "  " + fileNameStr, fileColor });
            }
        }

        m_entryList = std::move(newEntryList);
        m_pathCurrentEntry = newEntry.path();
        m_strCurrentPath = m_pathCurrentEntry;

        m_bMustFocusListBox = true;
        m_iSelectedItemIndex = 0;
    }
    static int retrievePathListCounter = 0;
    LOG("Path retrieved num: " << ++retrievePathListCounter);
}

bool FileSystemNavigator::showInImGUI()
{
    // ImGui::SetNextWindowCollapsed(true, ImGuiCond_Appearing);
    ImGui::SetNextWindowSize(ImVec2(m_width, m_height));
    if (ImGui::Begin(m_strImGuiWidgetName.c_str(), &m_bIsOpenInImgui,
            ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar)) {

        if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Escape))) {
            shouldClose();
        }

        ImGui::Text("%s", m_strCurrentPath.c_str());

        if (ImGui::BeginListBox(m_strImGuiFileListBoxName.c_str(), ImVec2(m_width, m_height - 126))) {
            const auto& entryList = getEntryList();
            if (m_bMustFocusListBox) {
                ImGui::SetKeyboardFocusHere(0);
                m_bMustFocusListBox = false;
            }

            for (int i = 0; i < entryList.size(); i++) {
                const bool is_selected = (m_iSelectedItemIndex == i);
                const std::string& filename = entryList[i].visibleName;

                ImGui::PushStyleColor(ImGuiCol_Text, entryList[i].visibleNameColor);
                if (ImGui::Selectable(filename.c_str(), is_selected)) { // on item selected

                    m_iSelectedItemIndex = i;

                    if (const auto* selectedEntryPtr = getEntryByIndex(m_iSelectedItemIndex)) {

                        if (selectedEntryPtr->entry.is_directory()) {
                            retrievePathList(selectedEntryPtr->entry.path());

                        } else if (selectedEntryPtr->entry.is_regular_file()) {
                            const fs::path& filePath = selectedEntryPtr->entry.path();
                            const fs::path& ext = filePath.extension();

                            ///////////////////// IF OVERWRITE //////////////////////////
                            if (auto* info = getSupportedExtensionInfo(ext)) {
                                if (!m_bFileOverwritePopupOpen) {
                                    m_bFileOverwritePopupOpen = true;
                                    ImGui::OpenPopup(m_strImGuiOverwriteMsg.c_str());
                                }
                            }
                            ///////////////////// ELSE WRITE DIRECTLY //////////////////////////
                            // if (runOpenFileFunction(filePath))
                            //     shouldClose();
                        }
                    }
                }
                ImGui::PopStyleColor();

                if (is_selected) {
                    if (ImGui::BeginPopupModal(m_strImGuiOverwriteMsg.c_str(),
                            nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_Popup)) {

                        ImVec2 button_size(ImGui::GetFontSize() * 7.0f, 0.0f);

                        if (ImGui::Button("Yes", button_size)) { // Yes
                            ImGui::CloseCurrentPopup();
                            m_bFileOverwritePopupOpen = false;
                            if (const auto* selectedEntryPtr = getEntryByIndex(m_iSelectedItemIndex)) {
                                const fs::path& filePath = selectedEntryPtr->entry.path();
                                if (auto* info = getSupportedExtensionInfo(filePath.extension())) {
                                    info->function(filePath);
                                    shouldClose();
                                }
                            }
                        }
                        if (ImGui::Button("No", button_size)) { // No
                            ImGui::CloseCurrentPopup();
                            m_bFileOverwritePopupOpen = false;
                        }
                        ImGui::EndPopup();
                    }
                    // ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndListBox(); // end list

            if (m_iSelectedItemIndex != m_iSelectedItemIndexPrev) {
                m_iSelectedItemIndexPrev = m_iSelectedItemIndex;
                LOG(m_iSelectedItemIndex);
                if (const auto* selectedEntryPtr = getEntryByIndex(m_iSelectedItemIndex))
                    m_strSselectedFilename = selectedEntryPtr->entry.path().filename();
            }
        }
        ImGui::SetNextItemWidth(m_width);
        if (ImGui::InputText(m_strImGuiTextBoxName.c_str(), &m_strSselectedFilename)) { // text box
        }

        ImGui::End();
    }

    return m_bIsOpenInImgui;
}

// bool FileSystemNavigator::runOpenFileFunction(const fs::path& filePath)
//{
//     const fs::path& fileName = filePath.filename();
//     m_strSselectedFilename = fileName;

//    const std::string& ext = fileName.extension();
//    auto foundExtensionOpenFunctionPair = m_extensionFileInfoMap.find(ext);
//    if (foundExtensionOpenFunctionPair != m_extensionFileInfoMap.end()) {
//        const auto& function = foundExtensionOpenFunctionPair->second.function;
//        if (function) {
//            function(filePath);
//            return true;
//        }
//    }
//    return false;
//}
} // ImguiUtils
