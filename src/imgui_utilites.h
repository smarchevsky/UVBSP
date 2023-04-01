#ifndef IMGUI_UTILITES_H
#define IMGUI_UTILITES_H

#include <filesystem>
#include <functional>
#include <map>
#include <vector>

typedef uint32_t FileVisualColor; // uint8: A G R B
static constexpr FileVisualColor s_defaultFileVisualColor = -1;
class FileSystemNavigator;

class ImGui_utilites {
public:
    static void runTreeNavigator(FileSystemNavigator& fsNavigator);
};

class FileSystemNavigator {
    typedef std::function<void(const std::filesystem::directory_entry&)> FileOpenFunction;

    struct EntryNamePair {
        const std::filesystem::directory_entry entry;
        const std::string visibleName;
        const FileVisualColor color = s_defaultFileVisualColor;
    };

    struct SupportedFileInfo {
        const FileOpenFunction function;
        const FileVisualColor color = s_defaultFileVisualColor;
    };

public:
    FileSystemNavigator(const std::string& name = "File navigator");
    void addSupportedExtension(const std::string& ext, FileOpenFunction func, FileVisualColor color = s_defaultFileVisualColor);
    void retrievePathList() { retrievePathList(m_currentEntry); }
    void retrievePathList(const std::filesystem::path& newPath);

    const EntryNamePair* getEntryByIndex(int index) const { return (index >= 0 && index < m_entryList.size()) ? &m_entryList[index] : nullptr; }
    const auto& getEntryList() const { return m_entryList; }
    const std::string& getName() { return m_name; }

private:
    std::string m_name;
    std::filesystem::path m_currentEntry;
    std::vector<EntryNamePair> m_entryList;
    std::map<std::string, SupportedFileInfo> m_extensionFileInfoMap;

public:
    mutable int m_selectedItemIdxImGui = 0;
    mutable bool m_isOpenInImgui = false;
};

#endif // IMGUI_UTILITES_H
