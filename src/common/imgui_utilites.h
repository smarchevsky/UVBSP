#ifndef IMGUI_UTILITES_H
#define IMGUI_UTILITES_H

#include <filesystem>
#include <functional>

#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace ImguiUtils {

namespace fs = std::filesystem;

typedef uint32_t FileVisualColor; // uint8: A G R B
static constexpr FileVisualColor s_defaultFileVisualColor = -1;

enum FileAction : uint8_t {
    FileRead,
    FileWrite
};
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
        const FileVisualColor color = s_defaultFileVisualColor;
    };

    struct EntryListInfo {
        const fs::directory_entry entry;
        const std::string visibleName;
        FileVisualColor visibleNameColor = s_defaultFileVisualColor;
    };

protected: // data
    const std::string m_strThisPtrHash;
    const std::string m_strImGuiWidgetName;
    const std::string m_strImGuiExtSensitiveCheckboxName;
    const std::string m_strImGuiFileListBoxName;
    const std::string m_strImGuiTextBoxName;
    const std::string m_strImGuiOverwriteMsg;

    // supported file extensions
    std::unordered_map<fs::path, SupportedFileInfo> m_extensionFileInfoMap;

    fs::path m_pathCurrentEntry;

    std::vector<fs::directory_entry> m_allEntryList;
    std::vector<EntryListInfo> m_visibleEntryListInfo;
    std::unordered_set<std::string> m_filenamesInThisFolder;

    std::string m_strSelectedFilename;
    std::string m_strCurrentPath;

    int m_iSelectedItemIndex = 0;

    int m_iFocusedItemIndex = 0;
    int m_iFocusedItemIndexPrev = 0;

    bool m_bIsOpenInImgui = true;
    bool m_bMustFocusListBox = false;

    bool m_bVisibleEntryListDirty = true;
    bool m_bFileOverwritePopupOpen = false;

    bool m_bFilterSupportedExtensions = false;
    bool m_bFilterSupportedExtensionsPrev = false;

    bool m_bTextBoxFileWithThisNameAlreadyExists = false;

    const FileAction m_fileAction;
    const uint16_t m_width = 400, m_height = 500;

public:
    void shouldClose() { m_bIsOpenInImgui = false; }
    FileSystemNavigator(FileAction action, const std::string& name, const fs::path& path);

    void addSupportedExtension(const fs::path& ext, FileInteractionFunction func,
        FileVisualColor color = s_defaultFileVisualColor);

    bool isParent(int selectedElementIndex) {
        return selectedElementIndex == 0;
    }
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

    const EntryListInfo* getVisibleEntryByIndex(int index) const
    {
        return (index >= 0 && index < m_visibleEntryListInfo.size())
            ? &m_visibleEntryListInfo[index]
            : nullptr;
    }

    SupportedFileInfo* getSupportedExtensionInfo(const fs::path& ext)
    {
        auto it = m_extensionFileInfoMap.find(ext);
        if (it != m_extensionFileInfoMap.end())
            return &it->second;
        return nullptr;
    }
};

///////////// FILE WRITER

//

} // ImguiUtils
#endif // IMGUI_UTILITES_H
