#ifndef WINDOW_H
#define WINDOW_H

#include <SFML/Graphics.hpp>
#include <functional>
#include <unordered_map>
#include <vec2.h>
enum class ModifierKey : uint8_t { None = 0,
    Alt = 1,
    Control = 1 << 1,
    Shift = 1 << 2,
    System = 1 << 3 };
ModifierKey operator|(ModifierKey a, ModifierKey b) { return ModifierKey((int)a | (int)b); }

struct KeyWithModifier {
    KeyWithModifier(sf::Keyboard::Key key, ModifierKey mod)
        : key(key)
        , mod(mod)
    {
    }
    KeyWithModifier(sf::Keyboard::Key key,
        bool alt = false, bool ctrl = false, bool shift = false, bool system = false)
        : key(key)
        , mod(ModifierKey(alt | ctrl << 1 | shift << 2 | system << 3))
    {
    }
    bool operator==(const KeyWithModifier& rhs) const { return key == rhs.key && mod == rhs.mod; }

    sf::Keyboard::Key key = sf::Keyboard::Key::Unknown;
    ModifierKey mod = ModifierKey::None;
};

namespace std {
template <>
struct hash<KeyWithModifier> {
    std::size_t operator()(const KeyWithModifier& k) const
    {
        uint64_t keyAction64 = (uint64_t)k.key | (uint64_t)((uint64_t)k.mod << 32);
        return std::hash<uint64_t>()(static_cast<uint64_t>(keyAction64));
    }
};
}

// ivec2 startPos, ivec2 currentPos, ivec2 currentDelta
typedef std::function<void(ivec2, ivec2, ivec2)> MouseDragEvent;
// float scrollDelta, ivec2 mousePos
typedef std::function<void(float, ivec2)> MouseScrollEvent;
// ivec2 pos, bool mouseDown
typedef std::function<void(ivec2, bool)> MouseClickEvent;

typedef std::function<void()> KeyDownEvent;
typedef std::function<void(KeyWithModifier)> AnyKeyEvent;

struct DragEvent {
    MouseDragEvent event {};
    ivec2 startMousePos {};
    bool pressed {};

public:
    operator bool() { return pressed && event; }
};
// kinda window wrapper, you can wrap SDL window the same way

class Window : public sf::RenderWindow {
public:
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
                applyScaleAndOffset();
            } break;
            case sf::Event::LostFocus:
                break;
            case sf::Event::GainedFocus:
                break;
            case sf::Event::TextEntered:
                break;
            case sf::Event::KeyPressed: {
                KeyWithModifier currentKey(event.key.code,
                    event.key.alt, event.key.control, event.key.shift, event.key.system);

                if (m_anyKeyEvent)
                    m_anyKeyEvent(currentKey);

                auto keyEventIter = m_keyMap.find(currentKey);
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
                    if (m_clickEventLMB)
                        m_clickEventLMB(m_mousePos, true);
                    break;
                case sf::Mouse::Middle:
                    m_dragEventMMB.pressed = true;
                    m_dragEventMMB.startMousePos = m_mousePos;
                    if (m_clickEventMMB)
                        m_clickEventMMB(m_mousePos, true);
                    break;
                case sf::Mouse::Right:
                    m_dragEventRMB.pressed = true;
                    m_dragEventRMB.startMousePos = m_mousePos;
                    if (m_clickEventRMB)
                        m_clickEventRMB(m_mousePos, true);
                    break;
                default: {
                }
                }
                break;
            case sf::Event::MouseButtonReleased:
                switch (event.mouseButton.button) {
                case sf::Mouse::Left:
                    m_dragEventLMB.pressed = false;
                    if (m_clickEventLMB)
                        m_clickEventLMB(m_mousePos, false);
                    break;
                case sf::Mouse::Middle:
                    m_dragEventMMB.pressed = false;
                    if (m_clickEventMMB)
                        m_clickEventMMB(m_mousePos, false);
                    break;
                case sf::Mouse::Right:
                    m_dragEventRMB.pressed = false;
                    if (m_clickEventRMB)
                        m_clickEventRMB(m_mousePos, false);
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
    void setOffset(vec2 offset) { m_viewOffset = offset; }
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

    void setMouseClickEvent(sf::Mouse::Button button, MouseClickEvent event)
    {
        switch (button) {
        case sf::Mouse::Left:
            m_clickEventLMB = event;
            break;
        case sf::Mouse::Middle:
            m_clickEventMMB = event;
            break;
        case sf::Mouse::Right:
            m_clickEventRMB = event;
            break;
        default: {
        }
        }
    }

    void setScrollEvent(MouseScrollEvent event) { m_mouseScrollEvent = event; }

    void addKeyEvent(sf::Keyboard::Key key, ModifierKey modifier, KeyDownEvent event)
    {
        m_keyMap.insert({ KeyWithModifier(key, modifier), event });
    }

    void setAnyKeyEvent(AnyKeyEvent event) { m_anyKeyEvent = event; }
    void exit()
    {
        if (m_preCloseEvent)
            m_preCloseEvent();
        close();
    }

private:
    std::function<void()> m_preCloseEvent {};
    DragEvent m_dragEventLMB {}, m_dragEventMMB {}, m_dragEventRMB {};
    MouseClickEvent m_clickEventLMB {}, m_clickEventMMB {}, m_clickEventRMB {};
    MouseScrollEvent m_mouseScrollEvent {};
    std::unordered_map<KeyWithModifier, KeyDownEvent> m_keyMap;
    AnyKeyEvent m_anyKeyEvent;

    ivec2 m_mousePos {};
    uvec2 m_windowSize {};
    float m_scale = 1.f;
    vec2 m_viewOffset {};
};

#endif // WINDOW_H
