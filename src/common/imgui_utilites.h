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

    typedef std::function<void(const fs::path&)> FileInteractionFunction;
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
    const std::string m_thisPtrHashStr;
    const std::string m_ImGuiWidgetName;
    const std::string m_ImGuiExtSensitiveCheckboxName;
    const std::string m_ImGuiFileListBoxName;
    const std::string m_ImGuiTextBoxName;

    // supported file extensions
    std::unordered_map<fs::path, SupportedFileInfo> m_extensionFileInfoMap;

    fs::path m_currentEntry;
    std::vector<EntryNameColor> m_entryList;
    std::string m_selectedFilename;
    std::string m_currentPathText;

    int m_selectedItemIdx = 0;
    bool m_isOpenInImgui = true;
    bool m_mustFocusListBox = false;

    bool m_extensionSensitive = false;
    bool m_extensionSensitivePrev = false;

    const FileAction m_fileAction;
    const uint16_t m_width = 400, m_height = 500;

public:
    void shouldClose() { m_isOpenInImgui = false; }
    FileSystemNavigator(FileAction action, const std::string& name, const fs::path& path);

    void addSupportedExtension(const fs::path& ext, FileInteractionFunction func,
        FileVisualColor color = s_defaultFileVisualColor);

    bool showInImGUI();

protected:
    FileVisualColor getColorByExt(const fs::path& ext)
    {
        auto foundExt = m_extensionFileInfoMap.find(ext);
        if (foundExt != m_extensionFileInfoMap.end())
            return foundExt->second.color;
        return s_defaultFileVisualColor;
    }
    bool runOpenFileFunction(const fs::path& filePath);
    void retrievePathList(const fs::path& newPath);
    const EntryNameColor* getEntryByIndex(int index) const { return (index >= 0 && index < m_entryList.size()) ? &m_entryList[index] : nullptr; }
    const auto& getEntryList() const { return m_entryList; }
};

///////////// FILE WRITER

//

} // ImguiUtils
#endif // IMGUI_UTILITES_H
