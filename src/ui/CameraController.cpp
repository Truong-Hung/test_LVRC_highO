#include "ui/CameraController.hpp"

#include <glm/ext/matrix_clip_space.hpp>

#include "ui/Camera.hpp"
#include "ui/Window.hpp"

CameraController::CameraController(std::shared_ptr<Camera> controlled_camera) :
	camera(std::move(controlled_camera)),
	viewportSize(0, 0),
	projectionMatrix(1),
	enableInputsNextFrame(false)
{
	Window::singleton().onInputsProcessed.add_object(this, &CameraController::reset_inputs_for_next_frame);
	Window::singleton().onKeyboardEvent.add_object(this, &CameraController::inputEvent_internal);
	Window::singleton().onMouseButtonPressed.add_object(this, &CameraController::inputEvent_internal);
	Window::singleton().onMouseMove.add_object(this, &CameraController::inputEvent_internal);
	Window::singleton().onMouseScroll.add_object(this, &CameraController::inputEvent_internal);
}

CameraController::~CameraController()
{
	Window::singleton().onInputsProcessed.clear_object(this);
	Window::singleton().onKeyboardEvent.clear_object(this);
	Window::singleton().onMouseButtonPressed.clear_object(this);
	Window::singleton().onMouseMove.clear_object(this);
	Window::singleton().onMouseScroll.clear_object(this);
}

glm::mat4 CameraController::getProjectionMatrix() const
{
	const float aspect = static_cast<float>(viewportSize.x) / static_cast<float>(viewportSize.y);
	return glm::perspective(glm::radians(camera->getFov()),
		std::isnan(aspect) || abs(aspect - std::numeric_limits<float>::epsilon()) < 0.f
		                        ? 1
		                        : aspect,
	                        camera->getZNear(),
	                        camera->getZFar());
}

void CameraController::setViewportSize(const glm::ivec2& size)
{
	viewportSize = size;
}

void CameraController::inputEvent_internal(const MouseButtonEvent& event)
{
	if (enableInputsNextFrame)
		inputEvent(event);
}

void CameraController::inputEvent_internal(const MouseMoveEvent& event)
{
	if (enableInputsNextFrame)
		inputEvent(event);
}

void CameraController::inputEvent_internal(const MouseScrollEvent& event)
{
	if (enableInputsNextFrame)
		inputEvent(event);
}

void CameraController::inputEvent_internal(const KeyboardEvent& event)
{
	if (enableInputsNextFrame)
		inputEvent(event);
}

void CameraController::reset_inputs_for_next_frame()
{
	if (!enableInputsNextFrame)
		lostFocus();
	enableInputsNextFrame = false;
}
