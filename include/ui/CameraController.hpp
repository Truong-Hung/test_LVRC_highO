#ifndef CAMERA_CONTROLLER_H
#define CAMERA_CONTROLLER_H

#include <glm/glm.hpp>
#include <memory>

struct KeyboardEvent;
struct MouseScrollEvent;
struct MouseMoveEvent;
struct MouseButtonEvent;
class Mesh;
class Camera;

/*
 * Extend this class to implement custom camera movement (See OrbitCameraController)
 * There can be multiple controller per camera (for example one per renderer if multiple renderer are running at the same time)
 */

class CameraController
{
public:
	CameraController(std::shared_ptr<Camera> controlled_camera);
	virtual ~CameraController();

	/**
	 * \brief Refresh the projection viewport resolution.
	 * This method should be called before we get the projection matrix.
	 */
	void setViewportSize(const glm::ivec2& size);
	[[nodiscard]] glm::mat4 getProjectionMatrix() const;

	/**
	 * \brief Helper method to focus on a mesh. (focusing on other objects is done manually)
	 */
	virtual void focus_mesh(Mesh& mesh) = 0;

	/**
	 * \brief Mark this controller as active for the current frame.
	 * calling this function will enable inputs for the next frame.
	 */
	void trigger_inputs()
	{
		enableInputsNextFrame = true;
	}
	[[nodiscard]] bool isActive() const { return enableInputsNextFrame; }
	
	/**
	 * \brief This is the camera currently controlled by this controller.
	 */
	[[nodiscard]] const std::shared_ptr<Camera>& get_camera() const { return camera; }

	void setCamera(const std::shared_ptr<Camera>& inCamera) { camera = inCamera; }

protected:
	std::shared_ptr<Camera> camera;
	glm::ivec2 viewportSize;
	glm::mat4 projectionMatrix;

	virtual void inputEvent(const MouseButtonEvent&)
	{
	}
	virtual void inputEvent(const MouseMoveEvent&)
	{
	}
	virtual void inputEvent(const MouseScrollEvent&)
	{
	}
	virtual void inputEvent(const KeyboardEvent&)
	{
	}

	/**
	 * \brief Called when this controller is not active anymore.
	 */
	virtual void lostFocus()
	{
	}

private:
	virtual void inputEvent_internal(const MouseButtonEvent& event);
	virtual void inputEvent_internal(const MouseMoveEvent& event);
	virtual void inputEvent_internal(const MouseScrollEvent& event);
	virtual void inputEvent_internal(const KeyboardEvent& event);

	bool enableInputsNextFrame;

	/**
	 * \brief Called when all inputs have been processed for this frame. Reset the active state of this controller.
	 */
	void reset_inputs_for_next_frame();
};

#endif // CAMERA_CONTROLLER_H
