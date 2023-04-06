#ifndef IMGUI_UTILITES_H
#define IMGUI_UTILITES_H

#include <filesystem>
#include <functional>

#include <unordered_map>
#include <vector>

namespace ImguiUtils {

namespace fs = std::filesystem;

typedef uint32_t FileVisualColor; // uint8: A G R B

///////////// FILE SYSTEM NAVIGATOR

class FileSystemNavigator {
protected: // const, structs, typedefs
    static constexpr int s_inputTextBoxSize = 256;

    typedef std::function<bool(const fs::path&)> FileInteractionFunction;
    struct SupportedFileInfo {
        SupportedFileInfo& operator=(const SupportedFileInfo& rhs)
        {
            const_cast<FileInteractionFunction&>(function) = rhs.function;
            const_cast<FileVisualColor&>(color) = rhs.color;
            return *this;
        };
        const FileInteractionFunction function;
        const FileVisualColor color = -1;
    };

    typedef std::unordered_map<fs::path, SupportedFileInfo> ExtensionInfoMap;
    struct EntryListInfo {
        EntryListInfo(const fs::directory_entry& entry,
            const std::string& name,
            FileVisualColor color, bool forceParentFolder = false)
            : entry(entry)
            , fileName(name)
            , ImGuiFileName(forceParentFolder ? ".." : "  " + name)
            , visibleNameColor(color)
        {
        }
        const fs::directory_entry entry;
        const std::string fileName;
        const std::string ImGuiFileName;
        FileVisualColor visibleNameColor;
    };
    struct PopupOverwriteWindowInfo {
        PopupOverwriteWindowInfo(const fs::directory_entry& dir, const FileInteractionFunction& info)
            : dir(dir)
            , function(info)
        {
        }
        const fs::directory_entry dir;
        const FileInteractionFunction function;
    };

protected: // data
    const std::string m_strThisPtrHash;
    const std::string m_strImGuiWidgetName;
    const std::string m_strImGuiExtSensitiveCheckboxName;
    const std::string m_strImGuiFileListBoxName;
    const std::string m_strImGuiTextBoxName;
    const std::string m_strImGuiOverwriteMsg;
    std::string m_strImGuiWarningMessage;
    std::string m_strImGuiCurrentPath;

    // supported file extensions
    ExtensionInfoMap m_extensionFileInfoMap;

    fs::path m_currentDir;

    std::vector<fs::directory_entry> m_allEntryList;
    std::vector<EntryListInfo> m_visibleEntryListInfo;

    std::string m_strSelectedFilename;
    std::unique_ptr<PopupOverwriteWindowInfo> m_popupOverwriteWindowInfo;

    int m_iSelectedItemIndex = 0;

    int m_iFocusedItemIndex = 0;
    int m_iFocusedItemIndexPrev = 0;

    bool m_bIsOpenInImgui = true;
    bool m_bMustFocusListBox = false;

    bool m_bVisibleEntryListDirty = true;

    bool m_bFilterSupportedExtensions = false;
    bool m_bFilterSupportedExtensionsPrev = false;

    bool m_bFileWithThisNameAlreadyExists = false;

    const uint16_t m_width = 400, m_height = 500;

public:
    fs::path getCurrentDir() { return m_currentDir; }
    void showWarningMessage(const std::string& msg) { m_strImGuiWarningMessage = msg; }
    void closeWarningMessage() { m_strImGuiWarningMessage.clear(); }
    bool isWarningMessageExists() { return !m_strImGuiWarningMessage.empty(); }

    void shouldClose() { m_bIsOpenInImgui = false; }
    FileSystemNavigator(const std::string& name, const fs::path& path);

    void addSupportedExtension(const fs::path& ext, FileInteractionFunction func);

    bool isParent(int selectedElementIndex) const { return selectedElementIndex == 0; }
    bool showInImGUI();

protected:
    void retrievePathList(const fs::path& newPath);

    void updateVisibleEntryListInternal();

    inline void updateVisibleEntryList()
    {
        if (m_bVisibleEntryListDirty)
            updateVisibleEntryListInternal();
        m_bVisibleEntryListDirty = false;
    }

    void executeFileWrite(const fs::directory_entry& fileEntry, FileInteractionFunction function)
    {
        if (fileEntry.is_regular_file()) { // double check
            if (function)
                function(fileEntry);
            shouldClose();
        }
    }

    int checkFileWithThisNameAlreadyExists_GetIndex();

    const EntryListInfo* getVisibleEntryByIndex(int index) const
    {
        return (index >= 0 && index < m_visibleEntryListInfo.size())
            ? &m_visibleEntryListInfo[index]
            : nullptr;
    }

    ExtensionInfoMap::const_iterator getSupportedFileInfoByIndex(int index) const
    {
        int i = 0;
        for (auto it = m_extensionFileInfoMap.begin(); it != m_extensionFileInfoMap.end(); ++it) {
            if (i == index)
                return it;
            i++;
        }
        return m_extensionFileInfoMap.end();
    }

    const SupportedFileInfo* getSupportedExtensionInfo(const fs::path& ext) const
    {
        auto it = m_extensionFileInfoMap.find(ext);
        if (it != m_extensionFileInfoMap.end())
            return &it->second;
        return nullptr;
    }
};

///////////// FILE WRITER
class FileReader : public FileSystemNavigator {
};

//

} // ImguiUtils
#endif // IMGUI_UTILITES_H
