
#include <SFML/Window/Clipboard.hpp>
#include <uvsplit.h>
#include <window.h>

#include <assert.h>
#include <iostream>
#include <math.h>

#define LOG(x) std::cout << x << std::endl
const vec2 defaultWindowSize(1024, 768);
const std::string projectDir(PROJECT_DIR);

//////////////////////////////////////////////////

class UVSplitActionHistory {
public:
    UVSplitActionHistory(UVSplit& uvSplit)
        : m_uvSplit(uvSplit)
    {
    }

    void add(const UVSplitAction& action)
    {
        if (m_drawHistory.size() > m_currentIndex)
            m_drawHistory[m_currentIndex] = action;
        else
            m_drawHistory.push_back(action);

        // assert(false && "m_currentIndex is beyond drawHistory size");

        m_currentIndex++;
    }

    bool undo()
    {
        if (m_currentIndex > 0) {
            m_uvSplit.reset();
            for (int i = 0; i < m_currentIndex - 1; ++i) {
                const auto& action = m_drawHistory[i];
                m_uvSplit.addSplit(action);
            }
            m_currentIndex--;
            return true;
        }
        return false;
    }

private:
    std::vector<UVSplitAction> m_drawHistory;
    int m_currentIndex {};
    UVSplit& m_uvSplit;
};

int main(int argc, char** argv)
{
    Window window(sf::VideoMode(defaultWindowSize.x, defaultWindowSize.y), "UVBSP");

    window.addKeyEvent(sf::Keyboard::Escape, ModifierKey::None, [&window]() { window.exit(); });
    window.setVerticalSyncEnabled(true);

    sf::Texture texture;

    bool validTexture {};
    if (argc <= 1) {
        window.setTitle("No input file!");
    } else {
        validTexture = texture.loadFromFile(argv[1]);
        if (!validTexture)
            window.setTitle(std::string("Unable to find file: ") + argv[1]);
    }

    if (!validTexture) {
        sf::Image img;
        img.create(1024, 1024, sf::Color(33, 33, 33));
        texture.loadFromImage(img);
    }

    texture.setSmooth(1);
    texture.generateMipmap();
    vec2 textureSize = toFloat(texture.getSize());

    // zoom canvas on scroll
    window.setScrollEvent([&](float diff, ivec2 mousePos) {
        float scaleFactor = pow(1.1f, -diff);
        window.addScale(scaleFactor);
        vec2 mouseWorld = window.mapPixelToCoords(mousePos);
        vec2 offset = (mouseWorld - window.getOffset()) * log(scaleFactor);
        window.addOffset(-offset);
    });

    // drag canvas on MMB
    window.setMouseDragEvent(sf::Mouse::Middle,
        [&](ivec2 startPos, ivec2 currentPos, ivec2 currentDelta) {
            window.addOffset(toFloat(-currentDelta) * window.getScale());
        });

    UVSplit uvSplit(textureSize);
    UVSplitActionHistory splitActions(uvSplit);

    ushort colorIndex = 0;

    sf::Shader textureShader;
    textureShader.loadFromFile(projectDir + "/shaders/BSPshader.glsl", sf::Shader::Type::Fragment);
    textureShader.setUniform("texture", texture);

    auto updateWindowTitle = [&]() {
        window.setTitle("Node count: " + std::to_string(uvSplit.getNumNodes())
            + "   Tree depth: " + std::to_string(uvSplit.getMaxDepth()));
        LOG(uvSplit.printNodes());
    };

    bool isMouseDragging = false;
    window.setMouseClickEvent(sf::Mouse::Left, [&](ivec2 pos, bool mouseDown) {
        if (!mouseDown) {
            isMouseDragging = false;
            const BSPNode* lastNode = uvSplit.getLastNode();
            if (lastNode) {
                UVSplitAction split(lastNode->pos, lastNode->dir, lastNode->left, lastNode->right);
                splitActions.add(split);
                updateWindowTitle();
            }
        }
    });

    // projectDir

    window.addKeyEvent(sf::Keyboard::S, ModifierKey::Control, [&]() { // undo
        std::string fileDir = projectDir + "/test.uvbsp";
        uvSplit.writeToFile(fileDir);
        window.setTitle("Saved to: " + fileDir);
    });

    window.addKeyEvent(sf::Keyboard::O, ModifierKey::Control, [&]() { // undo
        std::string fileDir = projectDir + "/test.uvbsp";
        uvSplit.readFromFile(fileDir);
        uvSplit.updateUniforms(textureShader);
    });

    window.addKeyEvent(sf::Keyboard::Z, ModifierKey::Control, [&]() { // undo
        if (splitActions.undo())
            colorIndex -= 2;
        uvSplit.updateUniforms(textureShader);
        updateWindowTitle();

    });
    bool isReadyToExport {};
    window.addKeyEvent(sf::Keyboard::E, ModifierKey::Control | ModifierKey::Shift, [&]() {
        isReadyToExport = true;
        window.setTitle("Export shader to: G-glsl, H-hlsl, U-unreal");
    });

    window.setAnyKeyEvent([&](KeyWithModifier key) {
        if (isReadyToExport) {
            std::string shaderText;
            switch (key.key) {
            case sf::Keyboard::G:
                shaderText = uvSplit.generateShader(UVSplit::ShaderType::GLSL);
                sf::Clipboard::setString(shaderText);
                std::cout << shaderText << std::endl;
                break;
            case sf::Keyboard::H:
                shaderText = uvSplit.generateShader(UVSplit::ShaderType::HLSL);
                sf::Clipboard::setString(shaderText);
                std::cout << shaderText << std::endl;
                break;
            case sf::Keyboard::U:
                shaderText = uvSplit.generateShader(UVSplit::ShaderType::UnrealCustomNode);
                sf::Clipboard::setString(shaderText);
                std::cout << shaderText << std::endl;
                break;
            default: {
            }
            }
            window.setTitle("Shader code copied to clipboard, you're welcome :)");
        }
        isReadyToExport = false;
    });

    window.setMouseDragEvent(sf::Mouse::Left,
        [&](ivec2 startPos, ivec2 currentPos, ivec2 currentDelta) {
            vec2 uvCurrentDelta = window.mapPixelToCoords(currentDelta) / textureSize;
            vec2 uvStartPos = window.mapPixelToCoords(startPos) / textureSize;
            vec2 uvCurrentPos = window.mapPixelToCoords(currentPos) / textureSize;
            vec2 uvCurrentDir = normalized(uvCurrentPos - uvStartPos);
            vec2 uvCurrentPerp = perp(uvCurrentDir);

            if (!isMouseDragging) { // create new split

                UVSplitAction split = { uvStartPos, uvCurrentPerp, colorIndex, ushort(colorIndex + 1) };
                uvSplit.addSplit(split);
                uvSplit.updateUniforms(textureShader);

                colorIndex += 2;
                isMouseDragging = true;
            } else { // rotate new split
                uvSplit.adjustSplit(uvCurrentPerp);
                uvSplit.updateUniforms(textureShader);
            }
        });

    sf::Sprite background(texture);
    uvSplit.updateUniforms(textureShader);
    while (window.isOpen()) {

        window.processEvents();
        window.clear(sf::Color(50, 50, 50));

        sf::Shader::bind(&textureShader);
        window.draw(background);
        sf::Shader::bind(nullptr);

        window.display();
    }

    return 0;
}
