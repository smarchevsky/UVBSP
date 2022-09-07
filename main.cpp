#include <uvsplit.h>
#include <window.h>

#include <assert.h>

#include <chrono>
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

int main()
{

    Window window(sf::VideoMode(defaultWindowSize.x, defaultWindowSize.y), "UVBSP");

    window.addKeyEvent(sf::Keyboard::Escape, ModifierKey::None, [&window]() { window.exit(); });
    window.setVerticalSyncEnabled(true);

    sf::Texture texture;
    texture.loadFromFile("/home/staseg/Projects/blender-uv.png");
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

    sf::Shader textureShader;
    textureShader.loadFromFile(projectDir + "/shaders/BSPshader.glsl", sf::Shader::Type::Fragment);
    textureShader.setUniform("texture", texture);

    bool isMouseDragging = false;
    window.setMouseClickEvent(sf::Mouse::Left, [&](ivec2 pos, bool mouseDown) {
        if (!mouseDown) {
            isMouseDragging = false;
            const BSPNode* lastNode = uvSplit.getLastNode();
            if (lastNode) {
                UVSplitAction split(lastNode->pos, lastNode->dir, lastNode->left - colorIndexThreshold, lastNode->right - colorIndexThreshold);
                splitActions.add(split);

                uvSplit.printDepth();
                uvSplit.printNodes();
            }
        }
    });

    window.addKeyEvent(sf::Keyboard::Z, ModifierKey::Control, [&]() { // undo
        splitActions.undo();
        uvSplit.updateUniforms(textureShader);
        uvSplit.printDepth();
        uvSplit.printNodes();
    });

    ushort colorIndex = 0;
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
    auto rect = background.getTextureRect();

    int frameCounter = 0;
    double tDiffAccum = 0.;
    std::string windowTitle;
    windowTitle.reserve(256);

    uvSplit.updateUniforms(textureShader);

    srand(time(0));
    while (window.isOpen()) {

        window.processEvents();
        window.clear(sf::Color(50, 50, 50));

        sf::Shader::bind(&textureShader);
        auto t0 = std::chrono::system_clock::now();
        window.draw(background);
        std::chrono::duration<double> tDiff = std::chrono::system_clock::now() - t0;
        tDiffAccum += tDiff.count();

        if (frameCounter++ % 30 == 0) {
            windowTitle = std::to_string(tDiffAccum / 100 * 1000000) + " mks";
            tDiffAccum = 0;
            window.setTitle(windowTitle);
        }

        sf::Shader::bind(nullptr);

        window.display();
    }

    return 0;
}
