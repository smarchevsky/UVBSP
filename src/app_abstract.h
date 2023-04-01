#ifndef APP_ABSTRACT_H
#define APP_ABSTRACT_H
#include "window.h"

class Application {
protected:
    Window m_window;

public:
    Application()
        : m_window(sf::VideoMode(1024, 768), "")
    {
    }
    virtual ~Application() {};
    virtual void drawContext() = 0;

    void mainLoop()
    {
        while (m_window.isOpen()) {
            m_window.processEvents();
            // m_window.setTitle(std::to_string(m_window.windowMayBeDirty()));

            if (m_window.windowMayBeDirty()) {
                drawContext();
            }

            m_window.display();
        }
    }
};

#endif // APP_ABSTRACT_H
