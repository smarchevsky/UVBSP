#include "imgui_utilites.h"
#include "imgui/imgui.h"
#include "imgui/imgui_stdlib.h"

#include <iostream>
namespace ImguiUtils {

#ifndef LOG
#define LOG(x) std::cout << x << std::endl
#endif

template <class T> // make hex string, because I can (copy from StackOverflow)
static std::string toStringCustom(T t, std::ios_base& (*f)(std::ios_base&))
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
    , m_strImGuiOverwriteMsg(std::string("Overwrite"))
    , m_pathCurrentEntry(DOCUMENTS_DIR)
    , m_fileAction(action)
{
    retrievePathList(m_pathCurrentEntry);
}

void FileSystemNavigator::addSupportedExtension(const fs::path& newExt,
    FileInteractionFunction func, FileVisualColor color)
{
    m_extensionFileInfoMap[newExt] = SupportedFileInfo { func, color };
    m_bVisibleEntryListDirty = true;
}

void FileSystemNavigator::retrievePathList(const fs::path& newPath)
{
    fs::directory_entry newEntry(newPath);
    if (newEntry.exists() && newEntry.is_directory()) {

        decltype(m_allEntryList) newEntryList;
        decltype(m_filenamesInThisFolder) newFileNames;

        fs::path parentPath(newPath.parent_path());
        fs::directory_entry parentEntry(parentPath);

        newEntryList.push_back(parentEntry);

        for (const fs::directory_entry& entry : fs::directory_iterator(newEntry,
                 fs::directory_options::skip_permission_denied)) {
            newEntryList.push_back(entry);
            newFileNames.insert(entry.path().filename());
        }

        m_allEntryList = std::move(newEntryList);
        m_filenamesInThisFolder = std::move(newFileNames);

        m_pathCurrentEntry = newEntry.path();
        m_strCurrentPath = m_pathCurrentEntry;

        m_strSelectedFilename.clear();

        m_bMustFocusListBox = true;
        m_iSelectedItemIndex = 0;
    }

    m_bVisibleEntryListDirty = true;
    static int retrievePathListCounter = 0;
    LOG("Path retrieved num: " << ++retrievePathListCounter);
}

void FileSystemNavigator::updateVisibleEntryListInternal()
{
    m_visibleEntryListInfo.clear();
    std::vector<fs::directory_entry> m_folderEntries;
    std::vector<fs::directory_entry> m_fileEntries;

    for (const auto& e : m_allEntryList) {
        if (e.is_directory())
            m_folderEntries.push_back(e);
        else if (e.is_regular_file())
            m_fileEntries.push_back(e);
    }

    for (int i = 0; i < m_folderEntries.size(); ++i) {
        const auto& e = m_folderEntries[i];
        std::string fileNameStr(e.path().filename());
        m_visibleEntryListInfo.push_back({ e, isParent(i) ? ".." : fileNameStr, s_defaultFileVisualColor });
    }

    for (const auto& e : m_fileEntries) {
        FileVisualColor fileColor = s_defaultFileVisualColor;

        const fs::path& path = e.path();
        const fs::path& ext = path.extension();
        std::string fileNameStr = "  " + std::string(path.filename());

        auto* extensionSupportInfo = getSupportedExtensionInfo(ext);
        if (extensionSupportInfo)
            fileColor = extensionSupportInfo->color;

        if (extensionSupportInfo || !m_bFilterSupportedExtensions)
            m_visibleEntryListInfo.push_back({ e, fileNameStr, fileColor });
    }

    static int refreshPathListCounter = 0;
    LOG("List refreshed: " << ++refreshPathListCounter);
    m_bVisibleEntryListDirty = false;
}

bool FileSystemNavigator::showInImGUI()
{
    auto showOverwriteDialogWindow = [this]() {
        if (m_bFileOverwritePopupOpen) {
            ImGui::OpenPopup(m_strImGuiOverwriteMsg.c_str());
            if (ImGui::BeginPopupModal(m_strImGuiOverwriteMsg.c_str(),
                    nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_Popup)) {
                ImVec2 button_size(ImGui::GetFontSize() * 7.0f, 0.0f);

                bool clickedYes = ImGui::Button("Yes", button_size);
                bool clickedNo = ImGui::Button("No", button_size);

                if (clickedYes || clickedNo) {
                    m_bFileOverwritePopupOpen = false;
                    ImGui::CloseCurrentPopup();

                    if (clickedYes) {
                        if (const auto* selectedEntryPtr = getVisibleEntryByIndex(m_iSelectedItemIndex)) {
                            const fs::path& filePath = selectedEntryPtr->entry.path();
                            if (auto* info = getSupportedExtensionInfo(filePath.extension())) {
                                info->function(filePath);
                                shouldClose();
                            }
                        }
                    }
                }

                ImGui::EndPopup();
            }
        }
    };
    updateVisibleEntryList();
    //////////////// SAVE DIALOG BOX ////////////////////
    ImGui::SetNextWindowSize(ImVec2(m_width, m_height));
    if (ImGui::Begin(m_strImGuiWidgetName.c_str(), &m_bIsOpenInImgui,
            ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar)) {

        bool enterPressed = ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Enter));
        if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Escape)))
            shouldClose();

        ImGui::Text("%s", m_strCurrentPath.c_str());

        if (ImGui::Checkbox("Extension sensitive", &m_bFilterSupportedExtensions)) {
            if (m_bFilterSupportedExtensions != m_bFilterSupportedExtensionsPrev) {
                m_bFilterSupportedExtensionsPrev = m_bFilterSupportedExtensions;
                m_bVisibleEntryListDirty = true;
            }
        }

        //////////////// LIST BOX ////////////////////
        if (ImGui::BeginListBox(m_strImGuiFileListBoxName.c_str(), ImVec2(m_width, m_height - 126))) {

            if (m_bMustFocusListBox) {
                ImGui::SetKeyboardFocusHere(0);
                m_bMustFocusListBox = false;
            }

            for (int i = 0; i < m_visibleEntryListInfo.size(); i++) {
                const bool is_selected = (m_iSelectedItemIndex == i);
                const std::string& filename = m_visibleEntryListInfo[i].visibleName;

                //////////////// SELECTABLE ITEM ////////////////////
                ImGui::PushStyleColor(ImGuiCol_Text, m_visibleEntryListInfo[i].visibleNameColor);
                if (ImGui::Selectable(filename.c_str(), is_selected)) { // on item selected
                    m_iSelectedItemIndex = i;

                    if (const auto* selectedEntryPtr = getVisibleEntryByIndex(m_iSelectedItemIndex)) {

                        if (selectedEntryPtr->entry.is_directory()) {
                            retrievePathList(selectedEntryPtr->entry.path());

                        } else if (selectedEntryPtr->entry.is_regular_file()) {
                            const fs::path& filePath = selectedEntryPtr->entry.path();
                            const fs::path& ext = filePath.extension();

                            ///////////////////// PUSH OVERWRITE POPUP ////////////////////////
                            if (auto* info = getSupportedExtensionInfo(ext)) {
                                if (!m_bFileOverwritePopupOpen) {
                                    m_bFileOverwritePopupOpen = true;
                                }
                            }
                        }
                    }
                }
                ImGui::PopStyleColor();

                if (ImGui::IsItemFocused()) {
                    m_iFocusedItemIndex = i;
                }

                ///////////////////// OVERWRITE POPUP ////////////////////////
                if (is_selected) {
                    showOverwriteDialogWindow();
                }
            }

            if (m_iFocusedItemIndex != m_iFocusedItemIndexPrev) { // on lsit element focus changed
                m_iFocusedItemIndexPrev = m_iFocusedItemIndex;
                if (const auto* selectedEntryPtr = getVisibleEntryByIndex(m_iFocusedItemIndex)) {
                    m_strSelectedFilename = isParent(m_iFocusedItemIndex) ? "" : selectedEntryPtr->entry.path().filename();
                    m_bTextBoxFileWithThisNameAlreadyExists = true;
                }
            }
            ImGui::EndListBox(); // end list
        }

        //////////////// TEXT BOX /////////////////
        ImGui::SetNextItemWidth(m_width);
        const auto& textColor = m_bTextBoxFileWithThisNameAlreadyExists ? IM_COL32(255, 50, 50, 255) : IM_COL32(255, 255, 255, 255);
        ImGui::PushStyleColor(ImGuiCol_Text, textColor);

        if (ImGui::InputText(m_strImGuiTextBoxName.c_str(), &m_strSelectedFilename)) { // text input
            m_bTextBoxFileWithThisNameAlreadyExists
                = (m_filenamesInThisFolder.find(m_strSelectedFilename) != m_filenamesInThisFolder.end()) ? true : false;
        }

        if (enterPressed && ImGui::IsItemFocused()) { // enter pressed in TextBox
            LOG("Pressed Enter in text box");
            m_bFileOverwritePopupOpen = true;
            showOverwriteDialogWindow();
        }
        ImGui::PopStyleColor(); // color of input text

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
