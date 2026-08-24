#ifndef LVRC_KEYBOARD_EVENT_HPP
#define LVRC_KEYBOARD_EVENT_HPP

extern "C" {
#include <glad/glad.h>
#include <GLFW/glfw3.h>
}

struct KeyboardEvent {
	enum Action {
        Press = GLFW_PRESS,
        Release = GLFW_RELEASE,
        Repeat = GLFW_REPEAT,
    };
    
    enum Modifier {
        Shift = GLFW_MOD_SHIFT,
        Control = GLFW_MOD_CONTROL,
        Alt = GLFW_MOD_ALT,
        Super = GLFW_MOD_SUPER,
        CapsLock = GLFW_MOD_CAPS_LOCK,
        NumLock = GLFW_MOD_NUM_LOCK
    };

	KeyboardEvent(Action action, int key, Modifier modifier) :
            action(action), key(key), modifiers(modifier) {}

    KeyboardEvent(int glfwButton, int glfwAction, int glfwMods) {
        action = static_cast<Action>(glfwAction);
        key = glfwButton;
        modifiers = static_cast<Modifier>(glfwMods);
    }

	Action action;
    int32_t key;
    Modifier modifiers;
};

#endif //LVRC_KEYBOARD_EVENT_HPP
