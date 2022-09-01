#include <SFML/Graphics.hpp>
#include <functional>
#include <iostream>
#include <math.h>
#include <unordered_map>

#define LOG(x) std::cout << x << std::endl

typedef sf::Vector2f vec2;
typedef sf::Vector2u uvec2;
typedef sf::Vector2i ivec2;

const uvec2 defaultWindowSize(1024, 768);
vec2 toFloat(const ivec2& v) { return { (float)v.x, (float)v.y }; }
vec2 toFloat(const uvec2& v) { return { (float)v.x, (float)v.y }; }
ivec2 toInt(const vec2& v) { return { (int)v.x, (int)v.y }; }
ivec2 toInt(const sf::Event::MouseMoveEvent& v) { return { v.x, v.y }; }
uvec2 toUInt(const vec2& v) { return { (unsigned)std::max(v.x, 0.f), (unsigned)std::max(v.y, 0.f) }; }
uvec2 toUInt(const sf::Event::SizeEvent& event) { return { event.width, event.height }; }
vec2 toFloat(const sf::Event::SizeEvent& event) { return { (float)event.width, (float)event.height }; }

inline vec2 operator*(const vec2& a, const vec2& b) { return { a.x * b.x, a.y * b.y }; };
inline vec2 operator*(const vec2& a, const uvec2& b) { return { a.x * b.x, a.y * b.y }; };
inline vec2 operator*(const uvec2& a, const vec2& b) { return { a.x * b.x, a.y * b.y }; };
inline vec2 operator*(const vec2& a, const ivec2& b) { return { a.x * b.x, a.y * b.y }; };
inline vec2 operator*(const ivec2& a, const vec2& b) { return { a.x * b.x, a.y * b.y }; };

inline vec2 operator/(const vec2& a, const vec2& b) { return { a.x / b.x, a.y / b.y }; };
inline vec2 operator/(const vec2& a, const uvec2& b) { return { a.x / b.x, a.y / b.y }; };
inline vec2 operator/(const uvec2& a, const vec2& b) { return { a.x / b.x, a.y / b.y }; };
inline vec2 operator/(const vec2& a, const ivec2& b) { return { a.x / b.x, a.y / b.y }; };
inline vec2 operator/(const ivec2& a, const vec2& b) { return { a.x / b.x, a.y / b.y }; };
inline vec2 operator/(const vec2& a, float b) { return { a.x / b, a.y / b }; };

class Window : public sf::RenderWindow {
public:
    // ivec2 startPos, ivec2 currentPos, ivec2 currentDelta
    typedef std::function<void(ivec2, ivec2, ivec2)> MouseDragEvent;
    // float scrollDelta, ivec2 mousePos
    typedef std::function<void(float, ivec2)> MouseScrollEvent;
    typedef std::function<void()> KeyDownEvent;

    struct DragEvent {
        MouseDragEvent event {};
        ivec2 startMousePos {};
        bool pressed {};

    public:
        operator bool() { return pressed && event; }
    };

    template <typename... Args>
    Window(Args&&... args)
        : sf::RenderWindow(std::forward<Args>(args)...)
        , m_windowSize(getSize())
        , m_viewOffset(toFloat(m_windowSize) / 2)
    {
    }
    void processEvents()
    {
        sf::Event event;
        while (pollEvent(event)) {
            // bool escPressed = event.KeyPressed && event.key.code == sf::Keyboard::Escape;
            switch (event.type) {
            case sf::Event::Closed:
                exit();
                break;
            case sf::Event::Resized: {
                m_windowSize = toUInt(event.size);
                vec2 newFrameSize = toFloat(m_windowSize);
                // m_aspectRatio = newFrameSize.x / newFrameSize.y;
                applyScaleAndOffset();

            } break;
            case sf::Event::LostFocus:
                break;
            case sf::Event::GainedFocus:
                break;
            case sf::Event::TextEntered:
                break;
            case sf::Event::KeyPressed: {
                auto keyEventIter = m_keyMap.find(event.key.code);
                if (keyEventIter != m_keyMap.end()) {
                    keyEventIter->second();
                }
            } break;
            case sf::Event::KeyReleased:
                break;
            case sf::Event::MouseWheelScrolled:
                if (m_mouseScrollEvent) {
                    m_mouseScrollEvent(event.mouseWheelScroll.delta, m_mousePos);
                }
                break;
            case sf::Event::MouseButtonPressed:
                switch (event.mouseButton.button) {
                case sf::Mouse::Left:
                    m_dragEventLMB.pressed = true;
                    m_dragEventLMB.startMousePos = m_mousePos;
                    break;
                case sf::Mouse::Middle:
                    m_dragEventMMB.pressed = true;
                    m_dragEventMMB.startMousePos = m_mousePos;
                    break;
                case sf::Mouse::Right:
                    m_dragEventRMB.pressed = true;
                    m_dragEventRMB.startMousePos = m_mousePos;
                    break;
                default: {
                }
                }
                break;
            case sf::Event::MouseButtonReleased:
                switch (event.mouseButton.button) {
                case sf::Mouse::Left:
                    m_dragEventLMB.pressed = false;
                    break;
                case sf::Mouse::Middle:
                    m_dragEventMMB.pressed = false;
                    break;
                case sf::Mouse::Right:
                    m_dragEventRMB.pressed = false;
                    break;
                default: {
                }
                }
                break;
            case sf::Event::MouseMoved: {
                ivec2 prevPos = m_mousePos;
                m_mousePos = toInt(event.mouseMove);
                if (m_dragEventLMB)
                    m_dragEventLMB.event(m_dragEventLMB.startMousePos, m_mousePos, m_mousePos - prevPos);
                if (m_dragEventMMB)
                    m_dragEventMMB.event(m_dragEventMMB.startMousePos, m_mousePos, m_mousePos - prevPos);
                if (m_dragEventRMB)
                    m_dragEventRMB.event(m_dragEventRMB.startMousePos, m_mousePos, m_mousePos - prevPos);

            } break;
            case sf::Event::MouseEntered:
                break;
            case sf::Event::MouseLeft:
                break;
            default: {
            }
            }
        }
    }

    void setScale(float scale) { m_scale = scale, applyScaleAndOffset(); }
    void addScale(float scaleFactor) { m_scale *= scaleFactor, applyScaleAndOffset(); }
    float getScale() const { return m_scale; }

    void addOffset(vec2 offset) { m_viewOffset += offset, applyScaleAndOffset(); }
    vec2 getOffset() const { return m_viewOffset; }
    void applyScaleAndOffset()
    {
        sf::View view(m_viewOffset, toFloat(m_windowSize) * m_scale);
        setView(view);
    }

    void setMouseDragEvent(sf::Mouse::Button button, MouseDragEvent event)
    {
        switch (button) {
        case sf::Mouse::Left:
            m_dragEventLMB.event = event;
            break;
        case sf::Mouse::Middle:
            m_dragEventMMB.event = event;
            break;
        case sf::Mouse::Right:
            m_dragEventRMB.event = event;
            break;
        default: {
        }
        }
    }
    void setScrollEvent(MouseScrollEvent event) { m_mouseScrollEvent = event; }
    void addKeyEvent(sf::Keyboard::Key key, KeyDownEvent event) { m_keyMap.insert({ key, event }); }

    void exit()
    {
        if (m_preCloseEvent)
            m_preCloseEvent();
        close();
    }

private:
    std::function<void()> m_preCloseEvent;
    ivec2 m_mousePos {};
    DragEvent m_dragEventLMB {}, m_dragEventMMB {}, m_dragEventRMB {};
    std::unordered_map<sf::Keyboard::Key, KeyDownEvent> m_keyMap;
    MouseScrollEvent m_mouseScrollEvent;

    uvec2 m_windowSize {};
    float m_scale = 1.f;
    vec2 m_viewOffset {};
};

//////////////////////////////////////////////////////////////

int main()
{
    Window window(sf::VideoMode(defaultWindowSize.x, defaultWindowSize.y), "TextureCoordinateBSP");
    window.addKeyEvent(sf::Keyboard::Escape, [&window]() { window.exit(); });

    sf::Texture texture;
    texture.loadFromFile("/home/staseg/Projects/blender-uv.png");

    float scale = 1.f;
    window.setScrollEvent([&](float diff, ivec2 mousePos) {
        float scaleFactor = pow(1.1f, -diff);
        window.addScale(scaleFactor);
        // TODO: zoom to mouse cursor
        // vec2 mPos = toFloat(mousePos);
    });

    // sf::Shape shape;

    vec2 delta;
    window.setMouseDragEvent(sf::Mouse::Middle, [&](ivec2 startPos, ivec2 currentPos, ivec2 currentDelta) {
        window.addOffset(toFloat(-currentDelta) * window.getScale());
    });

    window.setMouseDragEvent(sf::Mouse::Left, [&](ivec2 startPos, ivec2 currentPos, ivec2 currentDelta) {

    });

    sf::Sprite background(texture);

    while (window.isOpen()) {

        window.processEvents();
        window.clear();
        window.draw(background);

        window.display();
    }

    return 0;
}
