#include <uvsplit.h>
#include <window.h>

#include <assert.h>

#include <chrono>
#include <iostream>
#include <math.h>
#include <optional>

#define LOG(x) std::cout << x << std::endl
const vec2 defaultWindowSize(1024, 768);
const std::string projectDir(PROJECT_DIR);

/*
struct Line {
    Line(vec2 p0, vec2 p1, sf::Color color = sf::Color(255, 0, 0))
    {
        m_line[0] = { p0, color };
        m_line[1] = { p1, color };
    }
    void setPositions(vec2 p0, vec2 p1)
    {
        m_line[0].position = p0;
        m_line[1].position = p1;
    }
    void draw(sf::RenderWindow& window) { window.draw(m_line, 2, sf::Lines); }
    sf::Vertex m_line[2];
};

std::optional<vec2> getLineIntersection(vec2 p0, vec2 direction, vec2 segment0, vec2 segment1)
{
    std::optional<vec2> result;
    vec2 P = segment0;
    vec2 R = segment1 - segment0;
    vec2 Q = p0;
    vec2 S = direction;

    vec2 N = vec2(S.y, -S.x);
    float t = dot(Q - P, N) / dot(R, N);

    if (t >= 0.0 && t <= 1.0)
        result = P + R * t;

    return result;
}

bool convexLineIntersection(const sf::ConvexShape& shape, vec2 linePos, vec2 lineDir)
{
    std::vector<vec2> shapePoints0, shapePoints1;

    bool isSecondShape {}, hasSplit {};
    for (int i = 0; i < shape.getPointCount() - 1; ++i) {
        const vec2& p0 = shape.getPoint(i);
        const vec2& p1 = shape.getPoint(i + 1);

        shapePoints0.push_back(p0);
        auto intersection = getLineIntersection(linePos, lineDir, p0, p1);
        if (intersection) {
            hasSplit = true;
            isSecondShape = !isSecondShape;
            // do something, but why?
        }
    }
    return hasSplit;
}
*/
//////////////////////////////////////////////////

int main()
{

    Window window(sf::VideoMode(defaultWindowSize.x, defaultWindowSize.y), "UVBSP");

    window.addKeyEvent(sf::Keyboard::Escape, [&window]() { window.exit(); });
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

    bool isMouseDragging = false;
    window.setMouseClickEvent(sf::Mouse::Left, [&](ivec2 pos, bool mouseDown) {
        if (!mouseDown) {
            isMouseDragging = false;
        }
    });

    sf::Shader textureShader;
    textureShader.loadFromFile(projectDir + "/shaders/BSPshader.glsl", sf::Shader::Type::Fragment);
    textureShader.setUniform("texture", texture);

    int colorIndex = 0;
    window.setMouseDragEvent(sf::Mouse::Left,
        [&](ivec2 startPos, ivec2 currentPos, ivec2 currentDelta) {
            vec2 uvCurrentDelta = window.mapPixelToCoords(currentDelta) / textureSize;
            vec2 uvStartPos = window.mapPixelToCoords(startPos) / textureSize;
            vec2 uvCurrentPos = window.mapPixelToCoords(currentPos) / textureSize;
            vec2 uvCurrentDir = normalized(uvCurrentPos - uvStartPos);
            vec2 uvCurrentPerp = perp(uvCurrentDir);

            if (!isMouseDragging) { // create new split

                uvSplit.addSplit(uvStartPos, uvCurrentPerp, colorIndex, colorIndex + 1);
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
