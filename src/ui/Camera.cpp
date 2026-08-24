#include "ui/Camera.hpp"

#include <iostream>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/transform.hpp>

Camera::Camera(float fov) :
	position(0),
	rotation(0, 0, 0, 1),
	viewMatrix(1),
	fieldOfView(fov),
	distance(0.1f)
{
	transformDirty = true;
}

Camera::Camera(Camera& other) :
	position(other.position),
	rotation(other.rotation),
	viewMatrix(other.viewMatrix),
	fieldOfView(other.fieldOfView),
	transformDirty(other.transformDirty),
	zNear(other.zNear),
	zFar(other.zFar),
	distance(other.distance)
{
}

const glm::mat4& Camera::getViewMatrix()
{
	if (transformDirty)
	{
		transformDirty = false;
		viewMatrix = translate(glm::mat4(1), glm::vec3(0, 0, -distance)) * mat4_cast(rotation) * translate(position);
	}
	return viewMatrix;
}

void Camera::setPosition(const glm::vec3& inPosition)
{
	if (position != inPosition)
	{
		position = inPosition;
		transformDirty = true;
		onCameraMoved.execute();
	}
}

void Camera::setRotation(const glm::quat& inRotation)
{
	if (rotation != inRotation)
	{
		rotation = inRotation;
		transformDirty = true;
		onCameraMoved.execute();
	}
}

void Camera::setFov(const float fov)
{
	fieldOfView = fov;
}

glm::vec3 Camera::forwardVector() const
{
	return rotation * glm::vec3(0, 0, 1) ;
}

glm::vec3 Camera::rightVector() const
{
	return rotation * glm::vec3(1, 0, 0);
}

glm::vec3 Camera::upVector() const
{
	return rotation * glm::vec3(0, 1, 0);
}

void Camera::setDistance(float newDistance)
{
	if (std::abs(distance - newDistance) > 0.00000f)
	{
		distance = newDistance;
		transformDirty = true;
		onCameraMoved.execute();
	}
}
