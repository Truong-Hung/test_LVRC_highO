#ifndef ORBIT_CAMERA_CONTROLLER_H_
#define ORBIT_CAMERA_CONTROLLER_H_

#include "CameraController.hpp"



class Mesh;
class Camera;
struct KeyboardEvent;
struct MouseScrollEvent;
struct MouseButtonEvent;
struct MouseMoveEvent;

class OrbitCameraController : public CameraController
{
public:
	OrbitCameraController(std::shared_ptr<Camera> controlled_camera);
	virtual ~OrbitCameraController();
	void focus_mesh(Mesh& mesh) override;

protected:
	void inputEvent(const MouseMoveEvent& event) override;
	void inputEvent(const MouseButtonEvent& event) override;
	void inputEvent(const MouseScrollEvent& event) override;
	void inputEvent(const KeyboardEvent& event) override;
	void lostFocus() override;

private:
	void processTickEvent(double deltaTime);

	enum class DraggingState
	{
		None,
		Rotating,
		Translating,
	};
	
	DraggingState draggingState;
	float moveSensitivity;
	float scrollZoomSensitivity;
	float keyboard_sensitivity;
	glm::vec3 movementVector;
};

#endif // ORBIT_CAMERA_CONTROLLER_H_
