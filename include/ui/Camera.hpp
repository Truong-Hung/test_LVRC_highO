#ifndef LVRC_ORBITCAMERA_HPP
#define LVRC_ORBITCAMERA_HPP

#include <glm/detail/type_quat.hpp>
#include "glm/glm.hpp"

#include "event_manager.h"

DECLARE_DELEGATE_MULTICAST(OnCameraMoved)

class Camera
{
public:
	Camera(float fov = 45.f);
	Camera(Camera&);

	[[nodiscard]] float getFov() const { return fieldOfView; }
	[[nodiscard]] float getZNear() const { return zNear; }
	[[nodiscard]] float getZFar() const { return zFar; }
	/**
	 * \brief Note : the projection matrix is handled by the CameraController
	 */
	[[nodiscard]] const glm::mat4& getViewMatrix();

	void setPosition(const glm::vec3& inPosition);
	void setRotation(const glm::quat& inRotation);
	void setFov(float fov);

	[[nodiscard]] glm::vec3 forwardVector() const;
	[[nodiscard]] glm::vec3 rightVector() const;
	[[nodiscard]] glm::vec3 upVector() const;
	[[nodiscard]] glm::vec3 getPosition() const { return position; }
	[[nodiscard]] glm::quat getRotation() const { return rotation; }
	[[nodiscard]] float getDistance() const { return distance; }
	void setDistance(float newDistance);

	OnCameraMoved onCameraMoved;

private:
	glm::vec3 position;
	glm::quat rotation;
	glm::mat4 viewMatrix; // Updated with position and rotation when transformDirty is set to true
	float fieldOfView;
	bool transformDirty; // True after the update of the camera transform (will update the view matrix)
	float zNear = 0.1f;
	float zFar = 10000.f;
	float distance = 0.0f;
};

#endif // LVRC_ORBITCAMERA_HPP
