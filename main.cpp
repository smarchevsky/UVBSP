#include <window.h>

#include <iostream>
#include <math.h>

#define LOG(x) std::cout << x << std::endl
const vec2 defaultWindowSize(1024, 768);

int main()
{
    Window window(sf::VideoMode(defaultWindowSize.x, defaultWindowSize.y), "TextureCoordinateBSP");
    window.addKeyEvent(sf::Keyboard::Escape, [&window]() { window.exit(); });

    sf::Texture texture;
    texture.loadFromFile("/home/staseg/Projects/blender-uv.png");
    texture.setSmooth(1);
    texture.generateMipmap();
    vec2 textureSize = toFloat(texture.getSize());

    float scale = 1.f;
    window.setScrollEvent([&](float diff, ivec2 mousePos) {
        float scaleFactor = pow(1.1f, -diff);
        window.addScale(scaleFactor);
        vec2 mouseWorld = window.mapPixelToCoords(mousePos);
        vec2 offset = (mouseWorld - window.getOffset()) * log(scaleFactor);
        window.addOffset(-offset);
    });

    window.setMouseDragEvent(sf::Mouse::Middle,
        [&](ivec2 startPos, ivec2 currentPos, ivec2 currentDelta) {
            window.addOffset(toFloat(-currentDelta) * window.getScale());
        });

    bool isMouseDragging = false;

    window.setMouseClickEvent(sf::Mouse::Left, [&](ivec2 pos, bool mouseDown) {
        if (!mouseDown) {
            isMouseDragging = false;
        }
    });

    window.setMouseDragEvent(sf::Mouse::Left,
        [&](ivec2 startPos, ivec2 currentPos, ivec2 currentDelta) {
            vec2 uvCurrentDelta = window.mapPixelToCoords(currentDelta) / textureSize;
            vec2 uvStartPos = window.mapPixelToCoords(startPos) / textureSize;
            vec2 uvCurrentPos = window.mapPixelToCoords(currentPos) / textureSize;
            vec2 uvCurrentDir = normalized(uvCurrentPos - uvStartPos);

            if (!isMouseDragging) { // create new split

                isMouseDragging = true;
            } else { // rotate new split
            }
        });

    sf::Sprite background(texture);
    const auto& rect = background.getTextureRect();

    while (window.isOpen()) {

        window.processEvents();
        window.clear();
        window.draw(background);

        // for (auto& line : lines) {
        //     line.draw(window);
        // }

        window.display();
    }

    return 0;
}
