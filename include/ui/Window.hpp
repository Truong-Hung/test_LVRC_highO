#ifndef WINDOW_HPP
#define WINDOW_HPP

#include "events/KeyboardEvent.hpp"
#include "ui/events/MouseButtonEvent.hpp"
#include "ui/events/MouseMoveEvent.hpp"
#include "ui/events/MouseScrollEvent.hpp"

#include "event_manager.h"

DECLARE_DELEGATE_MULTICAST(WindowResizeEvent, int, int);
DECLARE_DELEGATE_MULTICAST(PressMouseMovedEvent, const MouseMoveEvent&);
DECLARE_DELEGATE_MULTICAST(PressMouseButtonEvent, const MouseButtonEvent&);
DECLARE_DELEGATE_MULTICAST(MouseScrolledEvent, const MouseScrollEvent&);
DECLARE_DELEGATE_MULTICAST(KeyboardPressedEvent, const KeyboardEvent&);
DECLARE_DELEGATE_MULTICAST(CharacterEvent, unsigned);
DECLARE_DELEGATE_MULTICAST(EventNewFrame, double);
DECLARE_DELEGATE_MULTICAST(EventEndFrame, double);
DECLARE_DELEGATE_MULTICAST(EmptyEvent);

class Window
{
public:
	explicit Window(uint32_t width, uint32_t height);
	~Window();

	static Window& singleton();
	static void vSync(bool enable);

	// Return false when shutdown event have been triggered (stop the update loop)
	bool beginFrame();
	void endFrame();
	void stop() const;
	[[nodiscard]] double getDeltaSecond() const { return deltaTime; }

	WindowResizeEvent onWindowResize;
	PressMouseMovedEvent onMouseMove;
	PressMouseButtonEvent onMouseButtonPressed;
	MouseScrolledEvent onMouseScroll;
	KeyboardPressedEvent onKeyboardEvent;
	CharacterEvent onCharacterPressed;

	EventNewFrame onStartFrame;
	EventEndFrame onEndFrame;
	EmptyEvent onInputsProcessed;

private:
	GLFWwindow* glfwWindow;
	double lastMouseXPos;
	double lastMouseYPos;
	double lastTime = 0.0;
	double deltaTime = 0.0;
};

#endif
