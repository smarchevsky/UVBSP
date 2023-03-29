#ifndef WINDOW_H
#define WINDOW_H

#include <SFML/Graphics.hpp>
#include <functional>
#include <unordered_map>
#include <vec2.h>

enum class ModifierKey : uint8_t {
    None = 0,
    Alt = 1,
    Control = 1 << 1,
    Shift = 1 << 2,
    System = 1 << 3
};

static ModifierKey operator|(ModifierKey a, ModifierKey b) { return ModifierKey((int)a | (int)b); }
static ModifierKey makeModifier(bool alt = false, bool ctrl = false, bool shift = false, bool system = false)
{
    return ModifierKey(alt | ctrl << 1 | shift << 2 | system << 3);
}
struct KeyWithModifier {
    KeyWithModifier(sf::Keyboard::Key key, ModifierKey mod, bool down)
        : key(key)
        , mod(mod)
        , down(down)
    {
    }
    bool operator==(const KeyWithModifier& rhs) const { return key == rhs.key && mod == rhs.mod && down == rhs.down; }

    sf::Keyboard::Key key = sf::Keyboard::Key::Unknown;
    ModifierKey mod = ModifierKey::None;
    bool down = true;
};

namespace std {
template <>
struct hash<KeyWithModifier> {
    std::size_t operator()(const KeyWithModifier& k) const
    {
        uint64_t keyAction64
            = (uint64_t)k.key << 32
            | (uint64_t)((uint64_t)k.mod << 16)
            | (uint64_t)((uint64_t)k.down);
        return std::hash<uint64_t>()(static_cast<uint64_t>(keyAction64));
    }
};
}

enum class DragState {
    MouseUp,
    StartDrag,
    ContinueDrag
};

typedef std::function<void(ivec2 currentPos, ivec2 delta)> MouseMoveEvent;
typedef std::function<void(ivec2 startPos, ivec2 currentPos, ivec2 delta, DragState)> MouseDragEvent;
typedef std::function<void(float scrollDelta, ivec2 mousePos)> MouseScrollEvent;
typedef std::function<void(ivec2 mousePos, bool mouseDown)> MouseDownEvent;
typedef std::function<void()> KeyEvent;
typedef std::function<void(KeyWithModifier)> AnyKeyEvent;

class MouseEventData {
    MouseMoveEvent m_mouseMoveEvent {};
    MouseDragEvent m_mouseDragEvent {};
    MouseDownEvent m_mouseDownEvent {};
    ivec2 m_startMouseDragPos {};
    DragState m_dragState = DragState::MouseUp;
    // int m_dragIteration = -1;

public:
    void runMouseMoveEvents(ivec2 currentPos, ivec2 delta)
    {
        if (m_mouseMoveEvent) {
            m_mouseMoveEvent(currentPos, delta);
        }
        if (m_mouseDragEvent) {
            m_mouseDragEvent(m_startMouseDragPos, currentPos, delta, m_dragState);
        }
        if (m_dragState == DragState::StartDrag)
            m_dragState = DragState::ContinueDrag;
    }

    void setMouseMoveEvent(const MouseMoveEvent& event) { m_mouseMoveEvent = event; }
    void setMouseDragEvent(const MouseDragEvent& event) { m_mouseDragEvent = event; }
    void setMouseDownEvent(const MouseDownEvent& event) { m_mouseDownEvent = event; }

    void mouseDown(ivec2 mousePos, bool down)
    {
        if (down) {
            m_startMouseDragPos = mousePos;
            m_dragState = DragState::StartDrag;
        } else {
            m_dragState = DragState::MouseUp;
        }
        if (m_mouseDownEvent)
            m_mouseDownEvent(mousePos, down);
    }

    // operator bool() { return !!m_event; }
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
                    makeModifier(
                        event.key.alt,
                        event.key.control,
                        event.key.shift,
                        event.key.system),
                    true);

                if (m_anyKeyDownEvent)
                    m_anyKeyDownEvent(currentKey);

                auto keyEventIter = m_keyMap.find(currentKey);
                if (keyEventIter != m_keyMap.end())
                    keyEventIter->second();

            } break;
            case sf::Event::KeyReleased: {
                KeyWithModifier currentKey(event.key.code,
                    makeModifier(
                        event.key.alt,
                        event.key.control,
                        event.key.shift,
                        event.key.system),
                    false);

                if (m_anyKeyUpEvent)
                    m_anyKeyUpEvent(currentKey);

                auto keyEventIter = m_keyMap.find(currentKey);
                if (keyEventIter != m_keyMap.end())
                    keyEventIter->second();

            } break;
            case sf::Event::MouseWheelScrolled:
                if (m_mouseScrollEvent) {
                    m_mouseScrollEvent(event.mouseWheelScroll.delta, m_mousePos);
                }
                break;
            case sf::Event::MouseButtonPressed: {
                auto mouseEventData = getMouseEventData(event.mouseButton.button);
                if (mouseEventData) {
                    mouseEventData->mouseDown(m_mousePos, true);
                }
            } break;
            case sf::Event::MouseButtonReleased: {
                auto mouseEventData = getMouseEventData(event.mouseButton.button);
                if (mouseEventData) {
                    mouseEventData->mouseDown(m_mousePos, false);
                }
            } break;
            case sf::Event::MouseMoved: {
                ivec2 prevPos = m_mousePos;
                m_mousePos = toInt(event.mouseMove);
                m_mouseEventLMB.runMouseMoveEvents(m_mousePos, m_mousePos - prevPos);
                m_mouseEventMMB.runMouseMoveEvents(m_mousePos, m_mousePos - prevPos);
                m_mouseEventRMB.runMouseMoveEvents(m_mousePos, m_mousePos - prevPos);
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
        auto mouseEventData = getMouseEventData(button);
        if (mouseEventData) {
            mouseEventData->setMouseDragEvent(event);
        }
    }

    void setMouseMoveEvent(sf::Mouse::Button button, MouseMoveEvent event)
    {
        auto mouseEventData = getMouseEventData(button);
        if (mouseEventData) {
            mouseEventData->setMouseMoveEvent(event);
        }
    }

    void setMouseDownEvent(sf::Mouse::Button button, MouseDownEvent event)
    {
        auto mouseEventData = getMouseEventData(button);
        if (mouseEventData) {
            mouseEventData->setMouseDownEvent(event);
        }
    }

    void setScrollEvent(MouseScrollEvent event) { m_mouseScrollEvent = event; }

    void addKeyDownEvent(sf::Keyboard::Key key, ModifierKey modifier, KeyEvent event) { m_keyMap.insert({ KeyWithModifier(key, modifier, true), event }); }
    void addKeyUpEvent(sf::Keyboard::Key key, ModifierKey modifier, KeyEvent event) { m_keyMap.insert({ KeyWithModifier(key, modifier, false), event }); }

    void setAnyKeyDownEvent(AnyKeyEvent event) { m_anyKeyDownEvent = event; }
    void setAnyKeyUpEvent(AnyKeyEvent event) { m_anyKeyUpEvent = event; }

    void exit()
    {
        if (m_preCloseEvent)
            m_preCloseEvent();
        close();
    }

private:
    MouseEventData* getMouseEventData(sf::Mouse::Button button)
    {
        switch (button) {
        case sf::Mouse::Left:
            return &m_mouseEventLMB;
        case sf::Mouse::Middle:
            return &m_mouseEventMMB;
        case sf::Mouse::Right:
            return &m_mouseEventRMB;
        default: {
            return nullptr;
        }
        }
    }

    std::function<void()> m_preCloseEvent {};

    MouseEventData m_mouseEventLMB {}, m_mouseEventMMB {}, m_mouseEventRMB {};
    MouseScrollEvent m_mouseScrollEvent {};
    std::unordered_map<KeyWithModifier, KeyEvent> m_keyMap;
    AnyKeyEvent m_anyKeyDownEvent, m_anyKeyUpEvent;

    ivec2 m_mousePos {};
    uvec2 m_windowSize {};
    float m_scale = 1.f;
    vec2 m_viewOffset {};
};

#endif // WINDOW_H
