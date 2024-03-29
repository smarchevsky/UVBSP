#ifndef APP_UVBSP_H
#define APP_UVBSP_H
#include "app_abstract.h"
#include "imgui_filesystem.h"
#include "uvbsp.h"

#include <SFML/Graphics/Sprite.hpp>
#include <memory>
#include <string>

namespace sf {
class Texture;
class Shader;
}

//////////////////// APPLICATION UVBSP ///////////////////

class Application_UVBSP : public Application {

    sf::Shader m_BSPShader;
    sf::Texture m_texture;
    sf::Sprite m_backgroundSprite;

    UVBSP m_uvSplit;
    UVBSPActionHistory m_splitActions;
    ushort m_colorIndex = 0;

    std::unique_ptr<ImguiUtils::FileSystemNavigator> m_fsNavigator;
    std::filesystem::path m_currentDir;
    std::optional<std::filesystem::path> m_currentFileName;
    float m_backgroundTransparency = 0.5f;

public:
    Application_UVBSP();
    ~Application_UVBSP() = default;

    void bindActions();

    virtual void drawContext() override;
};

#endif // APP_UVBSP_H
