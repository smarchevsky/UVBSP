#include "window.h"

#include "imgui/imgui-SFML.h"
#include "imgui/imgui.h"

void Window::init()
{
    bool imguiSuccessfullyInit = ImGui::SFML::Init(*this);
    imguiSuccessfullyInit = imguiSuccessfullyInit;
}

Window::~Window()
{
    ImGui::SFML::Shutdown();
}

void Window::processEvents()
{
    sf::Event event;

    while (pollEvent(event)) {
        ImGui::SFML::ProcessEvent(event);
        ImGuiIO& io = ImGui::GetIO();

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
            if (!io.WantCaptureMouse) {
                if (m_mouseScrollEvent) {
                    m_mouseScrollEvent(event.mouseWheelScroll.delta, m_mousePos);
                }
            }
            break;
        case sf::Event::MouseButtonPressed: {
            if (!io.WantCaptureMouse) {
                auto mouseEventData = getMouseEventData(event.mouseButton.button);
                if (mouseEventData) {
                    mouseEventData->mouseDown(m_mousePos, true);
                }
            }
        } break;
        case sf::Event::MouseButtonReleased: {
            if (!io.WantCaptureMouse) {
                auto mouseEventData = getMouseEventData(event.mouseButton.button);
                if (mouseEventData) {
                    mouseEventData->mouseDown(m_mousePos, false);
                }
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
    ImGui::SFML::Update(*this, m_deltaClock.restart());
}

void Window::setMouseDragEvent(sf::Mouse::Button button, MouseDragEvent event)
{
    auto mouseEventData = getMouseEventData(button);
    if (mouseEventData) {
        mouseEventData->setMouseDragEvent(event);
    }
}

void Window::setMouseMoveEvent(sf::Mouse::Button button, MouseMoveEvent event)
{
    auto mouseEventData = getMouseEventData(button);
    if (mouseEventData) {
        mouseEventData->setMouseMoveEvent(event);
    }
}

void Window::setMouseDownEvent(sf::Mouse::Button button, MouseDownEvent event)
{
    auto mouseEventData = getMouseEventData(button);
    if (mouseEventData) {
        mouseEventData->setMouseDownEvent(event);
    }
}

void Window::addKeyDownEvent(sf::Keyboard::Key key, ModifierKey modifier, KeyEvent event)
{
    m_keyMap.insert({ KeyWithModifier(key, modifier, true), event });
}

void Window::addKeyUpEvent(sf::Keyboard::Key key, ModifierKey modifier, KeyEvent event)
{
    m_keyMap.insert({ KeyWithModifier(key, modifier, false), event });
}

void Window::display()
{

    ImGui::SFML::Render(*this);
    sf::Window::display();
}

void Window::exit()
{
    if (m_preCloseEvent)
        m_preCloseEvent();
    close();
}
