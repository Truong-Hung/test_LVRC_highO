#include "ui/OrbitCameraController.hpp"

#if _WIN32
#include <corecrt_math_defines.h>
#endif

#include "ui/Camera.hpp"
#include "ui/events/KeyboardEvent.hpp"
#include "ui/events/MouseButtonEvent.hpp"
#include <glm/gtc/quaternion.hpp>

#include "mesh/Mesh.hpp"
#include "ui/Window.hpp"
#include "ui/events/MouseMoveEvent.hpp"
#include "ui/events/MouseScrollEvent.hpp"

OrbitCameraController::OrbitCameraController(std::shared_ptr<Camera> controlled_camera) :
	CameraController(std::move(controlled_camera)),
	draggingState(DraggingState::None),
	moveSensitivity(1.f),
	scrollZoomSensitivity(1.0f),
	keyboard_sensitivity(0.1f),
	movementVector(0)
{
	Window::singleton().onStartFrame.add_object(this, &OrbitCameraController::processTickEvent);
}

OrbitCameraController::~OrbitCameraController()
{
	Window::singleton().onStartFrame.clear_object(this);
}

void OrbitCameraController::focus_mesh(Mesh& mesh)
{
	const auto objectCenter = (mesh.AABB_[0] + mesh.AABB_[1]) * 0.5f;
	const auto objectSize = (mesh.AABB_[1] - mesh.AABB_[0]);

	camera->setPosition(-objectCenter);
	camera->setDistance(length(objectSize) * 2);
	camera->setRotation(lookAt(glm::vec3{-0.5, 0, 1}, glm::vec3{0}, glm::vec3{0, 1, 0}));
}

void OrbitCameraController::inputEvent(const MouseMoveEvent& event)
{
	if (draggingState == DraggingState::Rotating)
	{
		const float deltaAngleX = 2 * static_cast<float>(M_PI) / static_cast<float>(viewportSize.x);
		// a movement from left to right = 2 * PI = 360 deg
		const float deltaAngleY = static_cast<float>(M_PI) / static_cast<float>(viewportSize.y);
		// a movement from top to bottom = PI = 180 deg
		const float xAngle = (event.position.x - event.lastPosition.x) * deltaAngleX;
		const float yAngle = (event.position.y - event.lastPosition.y) * deltaAngleY;
		const auto camera_rotation = camera->getRotation();
		const auto iv = inverse(camera_rotation);
		camera->setRotation(
			camera->getRotation() * angleAxis(xAngle, iv * glm::vec3{0, 1, 0}) * angleAxis(
				yAngle, iv * glm::vec3{1, 0, 0}));
	}
	else if (draggingState == DraggingState::Translating)
	{
		const auto delta = glm::vec2(event.lastPosition - event.position) * moveSensitivity * camera->getDistance() *
			0.002f;
		camera->setPosition(
			camera->getPosition() + inverse(camera->getRotation()) * glm::vec3(glm::vec2(-delta.x, delta.y), 0));
	}
}


void OrbitCameraController::inputEvent(const MouseButtonEvent& event)
{
	if (event.button == GLFW_MOUSE_BUTTON_LEFT && event.action == GLFW_PRESS)
		draggingState = DraggingState::Rotating;
	else if (event.button == GLFW_MOUSE_BUTTON_RIGHT && event.action == GLFW_PRESS)
		draggingState = DraggingState::Translating;
	else
		draggingState = DraggingState::None;
}

void OrbitCameraController::inputEvent(const MouseScrollEvent& event)
{
	camera->setDistance(std::max(
		0.000001f, camera->getDistance() + -event.offset.y * scrollZoomSensitivity * camera->getDistance() * .10f));
}

void OrbitCameraController::inputEvent(const KeyboardEvent& event)
{
	if (event.action == KeyboardEvent::Press)
	{
		switch (event.key)
		{
		case GLFW_KEY_W:
			movementVector.z = 1;
			break;
		case GLFW_KEY_S:
			movementVector.z = -1;
			break;

		case GLFW_KEY_D:
			movementVector.x = -1;
			break;
		case GLFW_KEY_A:
			movementVector.x = 1;
			break;

		case GLFW_KEY_SPACE:
			movementVector.y = -1;
			break;
		case GLFW_KEY_LEFT_SHIFT:
			movementVector.y = 1;
			break;
		default: ;
		}
	}
	else if (event.action == KeyboardEvent::Release)
	{
		switch (event.key)
		{
		case GLFW_KEY_W:
		case GLFW_KEY_S:
			movementVector.z = 0;
			break;

		case GLFW_KEY_D:
		case GLFW_KEY_A:
			movementVector.x = 0;
			break;

		case GLFW_KEY_SPACE:
		case GLFW_KEY_LEFT_SHIFT:
			movementVector.y = 0;
			break;
		default: ;
		}
	}
}

void OrbitCameraController::lostFocus()
{
	CameraController::lostFocus();
	draggingState = DraggingState::None;
}

void OrbitCameraController::processTickEvent(double deltaTime)
{
	if (isActive())
	{
		camera->setPosition(
			camera->getPosition() + (camera->getRotation() * movementVector) *
                                          moveSensitivity * 20.f * camera->
			getDistance() *
			keyboard_sensitivity * static_cast<float>(deltaTime));
	}
}
