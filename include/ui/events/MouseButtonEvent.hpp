#ifndef LVRC_MOUSEBUTTONEVENT_HPP
#define LVRC_MOUSEBUTTONEVENT_HPP

extern "C" {
#include <glad/glad.h>
#include <GLFW/glfw3.h>
}

struct MouseButtonEvent {
public:
    enum Action {
        Press = GLFW_PRESS,
        Release = GLFW_RELEASE
    };

    enum Button {
        One = GLFW_MOUSE_BUTTON_1,
        Two = GLFW_MOUSE_BUTTON_2,
        Three = GLFW_MOUSE_BUTTON_3,
        Four = GLFW_MOUSE_BUTTON_4,
        Five = GLFW_MOUSE_BUTTON_5,
        Six = GLFW_MOUSE_BUTTON_6,
        Seven = GLFW_MOUSE_BUTTON_7,
        Height = GLFW_MOUSE_BUTTON_8,
        Left = One,
        Right = Two,
        Middle = Three
    };

    enum Modifier {
        Shift = GLFW_MOD_SHIFT,
        Control = GLFW_MOD_CONTROL,
        Alt = GLFW_MOD_ALT,
        Super = GLFW_MOD_SUPER,
        CapsLock = GLFW_MOD_CAPS_LOCK,
        NumLock = GLFW_MOD_NUM_LOCK
    };

public:
    MouseButtonEvent(Action action, Button button, Modifier modifier) :
            action(action), button(button), modifiers(modifier) {}

    MouseButtonEvent(int glfwButton, int glfwAction, int glfwMods) {
        action = static_cast<Action>(glfwAction);
        button = static_cast<Button>(glfwButton);
        modifiers = static_cast<Modifier>(glfwMods);
    }

public:
    Action action;
    Button button;
    Modifier modifiers;
};

#endif //LVRC_MOUSEBUTTONEVENT_HPP
