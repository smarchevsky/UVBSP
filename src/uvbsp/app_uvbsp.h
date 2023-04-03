#ifndef APP_UVBSP_H
#define APP_UVBSP_H
#include "app_abstract.h"
#include "imgui_utilites.h"
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
    std::filesystem::path m_currentFilePath;

public:
    Application_UVBSP();
    ~Application_UVBSP() = default;

    void bindActions();

    bool loadTexture(const std::filesystem::path& path);
    virtual void drawContext() override;
};

#endif // APP_UVBSP_H
