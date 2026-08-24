#ifndef LVRC_MOUSEMOVEEVENT_HPP
#define LVRC_MOUSEMOVEEVENT_HPP

#include <glm/glm.hpp>

struct MouseMoveEvent
{
    MouseMoveEvent(const glm::vec2& position, const glm::vec2& lastPosition):
        position(position),
        lastPosition(lastPosition)
    {}

    MouseMoveEvent(double glfwXPos,
                   double glfwYPos,
                   double glfwLastXPos,
                   double glfwLastYPos):
        position(glfwXPos, glfwYPos),
        lastPosition(glfwLastXPos, glfwLastYPos)
    {}

    glm::vec2 position;
    glm::vec2 lastPosition;
};

#endif //LVRC_MOUSEMOVEEVENT_HPP
