#include "app_uvbsp.h"
#include "imgui/imgui.h"
#include <SFML/Graphics.hpp>
#include <SFML/Window/Clipboard.hpp>
#include <filesystem>
#include <sstream>

#ifndef LOG
#define LOG(x) std::cout << x << std::endl
#endif

namespace fs = std::filesystem;
static const fs::path projectDir(PROJECT_DIR);
static const fs::path shaderFolderPath(projectDir / "shaders");

Application_UVBSP::Application_UVBSP()
    : m_splitActions(m_uvSplit)
{
    m_window.setTitle("UVBSP");
    // Create default grey texture
    sf::Image img;
    img.create(1024, 1024, sf::Color(33, 33, 33));
    m_texture.loadFromImage(img);

    // set texture to background sprite
    m_backgroundSprite.setTexture(m_texture);

    // Create BSP shader from file
    m_BSPShader.loadFromFile(shaderFolderPath / "BSPshader.glsl", sf::Shader::Type::Fragment);
    m_BSPShader.setUniform("texture", m_texture);
    m_uvSplit.updateUniforms(m_BSPShader);

    bindActions();
}

bool Application_UVBSP::loadTexture(const fs::path& path)
{
    bool validTexture = m_texture.loadFromFile(path);
    // if (!validTexture) {
    //     if (m_window)
    //         m_window->setTitle(std::string("Unable to find file: ") + path.c_str());
    // }

    m_texture.setSmooth(1);
    m_texture.generateMipmap();
    return validTexture;
}

void Application_UVBSP::drawContext()
{
    auto imguiFunctions = [&]() {
        auto& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

        ImGuiWindowFlags window_flags = 0;

        ImGuiStyle& style = ImGui::GetStyle();
        style.WindowPadding = ImVec2(15, 15);
        style.WindowRounding = 10.0f;
        style.DisplaySafeAreaPadding = ImVec2(300, 0);

        ImGui::Begin("Triangle Position/Color", nullptr, window_flags);

        ImGui::Text("io.NavActive");

        m_fsNavigator.showInImGUI();

        ImGui::End();
    };

    m_window.clear(sf::Color(50, 50, 50));
    sf::Shader::bind(&m_BSPShader);
    m_window.draw(m_backgroundSprite);
    sf::Shader::bind(nullptr);
    m_window.drawImGuiContext(imguiFunctions);
}

void Application_UVBSP::bindActions()
{
    // reminder capture [this] only
    // or std::bind(&Application_UVBSP::some_function, this);
    ///////////////// KEY EVENTS ///////////////////

    // save
    // todo: add ImGui context menu on load/save
    m_window.addKeyDownEvent(sf::Keyboard::S, ModifierKey::Control,
        [this]() {
            std::string fileDir = projectDir / "test.uvbsp";
            m_uvSplit.writeToFile(fileDir);
            m_window.setTitle("Saved to: " + fileDir);
        });

    // open
    // todo: add ImGui context menu on load/save
    m_window.addKeyDownEvent(sf::Keyboard::O, ModifierKey::Control,
        [this]() {
            std::string fileDir = projectDir / "test.uvbsp";
            m_uvSplit.readFromFile(fileDir);
            m_uvSplit.updateUniforms(m_BSPShader);
        });

    // undo
    m_window.addKeyDownEvent(sf::Keyboard::Z, ModifierKey::Control,
        [this]() {
            if (m_splitActions.undo())
                m_colorIndex -= 2;
            m_uvSplit.updateUniforms(m_BSPShader);

            m_window.setTitle(m_uvSplit.getBasicInfo());
        });

    // suggest export shader text
    m_window.addKeyDownEvent(sf::Keyboard::E, ModifierKey::Control | ModifierKey::Shift,
        [this]() {
            m_window.setAnyKeyReason("export file type");
            m_window.setTitle("Export shader to: G-glsl, H-hlsl, U-unreal");
        });

    // export shader text
    m_window.setAnyKeyDownOnceEvent("export file type",
        [this](KeyWithModifier key) {
            std::string shaderText;
            UVBSP::ShaderType shaderExportType;
            switch (key.key) {
            case sf::Keyboard::G:
                shaderExportType = UVBSP::ShaderType::GLSL;
                break;
            case sf::Keyboard::H:
                shaderExportType = UVBSP::ShaderType::HLSL;
                break;
            case sf::Keyboard::U:
                shaderExportType = UVBSP::ShaderType::UnrealCustomNode;
                break;
            default: {
                m_window.setTitle("Invalid export letter, press 'G', 'H' or 'U' next time.");
                return;
            }
            }
            shaderText = m_uvSplit.generateShader(shaderExportType).str();
            std::cout << shaderText << std::endl;
            sf::Clipboard::setString(shaderText);

            m_window.setTitle("Export shader code copied to clipboard, you're welcome :)");
        });

    ///////////////// MOUSE EVENTS ///////////////////

    // drag canvas on MMB
    m_window.setMouseDragEvent(sf::Mouse::Middle,
        [this](ivec2 startPos, ivec2 currentPos, ivec2 currentDelta, DragState dragState) {
            m_window.addOffset(toFloat(-currentDelta) * m_window.getScale());
        });

    // finish split on mouse up
    m_window.setMouseDownEvent(sf::Mouse::Left,
        [this](ivec2 pos, bool mouseDown) {
            if (!mouseDown) {
                const UVBSPSplit* lastNode = m_uvSplit.getLastNode();
                if (lastNode) {
                    UVBSPSplit split(lastNode->pos, lastNode->dir, lastNode->l, lastNode->r);
                    m_splitActions.add(split);

                    m_window.setTitle(m_uvSplit.getBasicInfo());
                }
            }
        });

    // zoom canvas on scroll
    m_window.setMouseScrollEvent(
        [this](float diff, ivec2 mousePos) {
            float scaleFactor = pow(1.1f, -diff);
            m_window.addScale(scaleFactor);
            vec2 mouseWorld = m_window.mapPixelToCoords(mousePos);
            vec2 offset = (mouseWorld - m_window.getOffset()) * log(scaleFactor);
            m_window.addOffset(-offset);
        });

    // create split
    m_window.setMouseDragEvent(sf::Mouse::Left,
        [this](ivec2 startPos, ivec2 currentPos, ivec2 currentDelta, DragState dragState) {
            vec2 textureSize = toFloat(m_texture.getSize());
            vec2 uvCurrentDelta = m_window.mapPixelToCoords(currentDelta) / textureSize;
            vec2 uvStartPos = m_window.mapPixelToCoords(startPos) / textureSize;
            vec2 uvCurrentPos = m_window.mapPixelToCoords(currentPos) / textureSize;
            vec2 uvCurrentDir = normalized(uvCurrentPos - uvStartPos);
            vec2 uvCurrentPerp = perp(uvCurrentDir);

            if (dragState == DragState::StartDrag) { // create new split
                UVBSPSplit split = { uvStartPos, uvCurrentPerp, m_colorIndex, ushort(m_colorIndex + 1) };
                m_uvSplit.addSplit(split);
                m_uvSplit.updateUniforms(m_BSPShader);

                m_colorIndex += 2;
            } else if (dragState == DragState::ContinueDrag) { // rotate new split
                m_uvSplit.adjustSplit(uvCurrentPerp);
                m_uvSplit.updateUniforms(m_BSPShader);
            }
        });
}
