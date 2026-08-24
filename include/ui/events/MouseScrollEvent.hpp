#ifndef LVRC_MOUSESCROLLEVENT_HPP
#define LVRC_MOUSESCROLLEVENT_HPP

struct MouseScrollEvent {
    explicit MouseScrollEvent(const glm::vec2& offset) : offset(offset) {}

    MouseScrollEvent(double xOffset, double yOffset) :
            offset(xOffset, yOffset) {}

    glm::vec2 offset;
};

#endif //LVRC_MOUSESCROLLEVENT_HPP
