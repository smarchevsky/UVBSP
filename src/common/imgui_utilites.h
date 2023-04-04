#ifndef IMGUI_UTILITES_H
#define IMGUI_UTILITES_H

#include <filesystem>
#include <functional>

#include <unordered_map>
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

    struct EntryNameColor {
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
    std::vector<EntryNameColor> m_entryList;
    std::string m_strSselectedFilename;
    std::string m_strCurrentPath;

    int m_iSelectedItemIndex = 0;
    int m_iSelectedItemIndexPrev = 0;

    bool m_bIsOpenInImgui = true;
    bool m_bMustFocusListBox = false;

    bool m_bFileOverwritePopupOpen = false;
    bool m_bForceOverwrite = false;

    const FileAction m_fileAction;
    const uint16_t m_width = 400, m_height = 500;

public:
    void shouldClose() { m_bIsOpenInImgui = false; }
    FileSystemNavigator(FileAction action, const std::string& name, const fs::path& path);

    void addSupportedExtension(const fs::path& ext, FileInteractionFunction func,
        FileVisualColor color = s_defaultFileVisualColor);

    bool showInImGUI();

protected:
    void retrievePathList(const fs::path& newPath);

    const EntryNameColor* getEntryByIndex(int index) const { return (index >= 0 && index < m_entryList.size()) ? &m_entryList[index] : nullptr; }
    //    SupportedFileInfo* getSupportedExtensionInfoBySelectedIndex(int index)
    //    {
    //        if (auto* entry = getEntryByIndex(index)) {
    //            const auto& path = entry->entry.path();
    //            const auto& ext = path.extension();

    //            auto it = m_extensionFileInfoMap.find(ext);
    //            if (it != m_extensionFileInfoMap.end())
    //                return &it->second;
    //        }
    //        return nullptr;
    //    }
    SupportedFileInfo* getSupportedExtensionInfo(const fs::path& ext)
    {
        auto it = m_extensionFileInfoMap.find(ext);
        if (it != m_extensionFileInfoMap.end())
            return &it->second;
        return nullptr;
    }

    const auto& getEntryList() const { return m_entryList; }
};

///////////// FILE WRITER

//

} // ImguiUtils
#endif // IMGUI_UTILITES_H
