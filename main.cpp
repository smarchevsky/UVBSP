
#include "imgui/imgui.h"

#include <SFML/Window/Clipboard.hpp>

#include <assert.h>
#include <iostream>
#include <math.h>
#include <sstream>
#include <uvbsp.h>
#include <window.h>

#ifndef LOG
#define LOG(x) std::cout << x << std::endl
#endif
const vec2 defaultWindowSize(1024, 768);
const std::string projectDir(PROJECT_DIR);

//////////////////////////////////////////////////

class UVBSPActionHistory {
public:
    UVBSPActionHistory(UVBSP& uvSplit)
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
    UVBSP& m_uvSplit;
};

int main(int argc, char** argv)
{
    Window window(sf::VideoMode(defaultWindowSize.x, defaultWindowSize.y), "UVBSP");
    // Window window2(sf::VideoMode(defaultWindowSize.x, defaultWindowSize.y), "UVBSP");

    window.addKeyDownEvent(sf::Keyboard::Escape, ModifierKey::None, [&window]() { window.exit(); });
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
    window.setMouseScrollEvent([&](float diff, ivec2 mousePos) {
        float scaleFactor = pow(1.1f, -diff);
        window.addScale(scaleFactor);
        vec2 mouseWorld = window.mapPixelToCoords(mousePos);
        vec2 offset = (mouseWorld - window.getOffset()) * log(scaleFactor);
        window.addOffset(-offset);
    });

    // drag canvas on MMB
    window.setMouseDragEvent(sf::Mouse::Middle,
        [&](ivec2 startPos, ivec2 currentPos, ivec2 currentDelta, DragState dragState) {
            window.addOffset(toFloat(-currentDelta) * window.getScale());
        });

    UVBSP uvSplit;
    UVBSPActionHistory splitActions(uvSplit);

    ushort colorIndex = 0;

    sf::Shader textureShader;
    textureShader.loadFromFile(projectDir + "/shaders/BSPshader.glsl", sf::Shader::Type::Fragment);
    textureShader.setUniform("texture", texture);

    auto updateWindowTitle = [&]() {
        // window.setTitle("Node count: " + std::to_string(uvSplit.getNumNodes())
        //     + "   Tree depth: " + std::to_string(uvSplit.getMaxDepth(0)));
        // LOG(uvSplit.printNodes());
    };

    window.setMouseDownEvent(sf::Mouse::Left, [&](ivec2 pos, bool mouseDown) {
        if (!mouseDown) {
            const BSPNode* lastNode = uvSplit.getLastNode();
            if (lastNode) {
                UVSplitAction split(lastNode->pos, lastNode->dir, lastNode->left, lastNode->right);
                splitActions.add(split);
                updateWindowTitle();
            }
        }
    });

    // projectDir

    window.addKeyDownEvent(sf::Keyboard::S, ModifierKey::Control, [&]() { // save
        std::string fileDir = projectDir + "/test.uvbsp";
        // uvSplit.writeToFile(fileDir);
        window.setTitle("Saved to: " + fileDir);
    });

    window.addKeyDownEvent(sf::Keyboard::O, ModifierKey::Control, [&]() { // open
        std::string fileDir = projectDir + "/test.uvbsp";
        // uvSplit.readFromFile(fileDir);
        uvSplit.updateUniforms(textureShader);
    });

    window.addKeyDownEvent(sf::Keyboard::Z, ModifierKey::Control, [&]() { // undo
        if (splitActions.undo())
            colorIndex -= 2;
        uvSplit.updateUniforms(textureShader);
        updateWindowTitle();

    });

    window.addKeyDownEvent(sf::Keyboard::E, ModifierKey::Control | ModifierKey::Shift, [&]() {
        window.setAnyKeyReason("export file type");
        window.setTitle("Export shader to: G-glsl, H-hlsl, U-unreal");
    });

    window.setAnyKeyDownOnceEvent("export file type", [&](KeyWithModifier key) {
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
            return;
        }
        }
        shaderText = uvSplit.generateShader(shaderExportType).str();
        std::cout << shaderText << std::endl;
        sf::Clipboard::setString(shaderText);

        window.setTitle("Shader code copied to clipboard, you're welcome :)");
    });

    window.setMouseDragEvent(sf::Mouse::Left,
        [&](ivec2 startPos, ivec2 currentPos, ivec2 currentDelta, DragState dragState) {
            vec2 uvCurrentDelta = window.mapPixelToCoords(currentDelta) / textureSize;
            vec2 uvStartPos = window.mapPixelToCoords(startPos) / textureSize;
            vec2 uvCurrentPos = window.mapPixelToCoords(currentPos) / textureSize;
            vec2 uvCurrentDir = normalized(uvCurrentPos - uvStartPos);
            vec2 uvCurrentPerp = perp(uvCurrentDir);

            if (dragState == DragState::StartDrag) { // create new split

                UVSplitAction split = { uvStartPos, uvCurrentPerp, colorIndex, ushort(colorIndex + 1) };
                uvSplit.addSplit(split);
                uvSplit.updateUniforms(textureShader);

                colorIndex += 2;
            } else if (dragState == DragState::ContinueDrag) { // rotate new split
                uvSplit.adjustSplit(uvCurrentPerp);
                uvSplit.updateUniforms(textureShader);
            }
        });

    window.setScreenResizeEvent([&](uvec2 oldSize, uvec2 newSize) {

    });

    sf::Sprite background(texture);
    uvSplit.updateUniforms(textureShader);

    auto imguiFunctions = []() {
        auto& io = ImGui::GetIO();

        ImGuiWindowFlags window_flags = 0;
        int frameIndex = 0;
        // window_flags |= ImGuiWindowFlags_NoTitleBar;
        // window_flags |= ImGuiWindowFlags_NoScrollbar;
        // window_flags |= ImGuiWindowFlags_MenuBar;
        // window_flags |= ImGuiWindowFlags_NoMove;
        // window_flags |= ImGuiWindowFlags_NoResize;
        // window_flags |= ImGuiWindowFlags_NoCollapse;

        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

        static int item_current_idx = 0;
        // ImGui::SFML::;

        // auto windowSize = ImGui::GetWindowSize();
        // ImGui::SetNextWindowPos({ io.DisplaySize.x - windowSize.x, 0 }, 0, { 0.f, 0.f });
        // ImGui::SetNextWindowSizeConstraints({ 30, 30 }, { 300, 300 });

        ImGuiStyle& style = ImGui::GetStyle();
        style.WindowPadding = ImVec2(15, 15);
        style.WindowRounding = 10.0f;
        style.DisplaySafeAreaPadding = ImVec2(300, 0);

        ImGui::Begin("Triangle Position/Color", nullptr, window_flags);

        const bool nav_keyboard_active = (io.ConfigFlags & ImGuiConfigFlags_NavEnableKeyboard) != 0;

        ImGui::Text("io.NavActive: %d, io.NavVisible: %d, item_current_idx  %d",
            io.NavActive, io.NavVisible, item_current_idx);

        if (ImGui::TreeNode("List boxes")) {
            const char* items[] = {
                "AAAA", "BBBB", "CCCC", "DDDD",
                "EEEE", "FFFF", "GGGG", "HHHH",
                "IIII", "JJJJ", "KKKK", "LLLL",
                "MMMM", "OOOO"
            };

            if (ImGui::BeginListBox("listbox 1")) {

                for (int n = 0; n < IM_ARRAYSIZE(items); n++) {
                    const bool is_selected = (item_current_idx == n);
                    if (ImGui::Selectable(items[n], is_selected))
                        item_current_idx = n;

                    if (is_selected) {
                        ImGui::SetItemDefaultFocus();
                    }
                }
                ImGui::EndListBox();
            }

            // Custom size: use all width, 5 items tall
            ImGui::Text("Full-width:");

            ImGui::TreePop();
        }

        ImGui::End();
    };

    window.setFramerateLimit(60);
    while (window.isOpen()) {
        window.processEvents();
        window.setTitle(std::to_string(window.windowMayBeDirty()));

        if (window.windowMayBeDirty()) {
            window.clear(sf::Color(50, 50, 50));
            sf::Shader::bind(&textureShader);
            window.draw(background);
            sf::Shader::bind(nullptr);
            window.drawImGuiContext(imguiFunctions);
        }

        window.display();
    }
    return 0;
}
