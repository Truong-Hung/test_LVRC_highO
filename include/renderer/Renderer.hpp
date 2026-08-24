#ifndef LVRC_RENDERER_HPP
#define LVRC_RENDERER_HPP

#include <string>
#include <glm/gtc/type_ptr.hpp>

#include "opengl/utils/Texture.hpp"
#include "ui/Camera.hpp"

class CameraController;



/**
 * \brief This is the base class fo any renderer.
 * The rendering is done into a texture (renderedImageTexture)
 * The image is regenerated once per frame or less, only if markRenderDirty() have been called (or if forceRefresh is set to true)
 *
 * NOTE FOR DEBUGGING WITH RENDER DOC : Set forceRefresh to true to make the full pass visible in the frame.
 */
class Renderer
{
public:
	Renderer(std::string name, const std::shared_ptr<Camera>& camera);
	virtual ~Renderer();

	// Execute renderer from outside. Return true if the vsync is enabled
	bool renderInternal();

	// Return true if the window have been resized
	virtual bool onResize(int width, int height);

	// When renderer is loaded
	virtual void resetRenderer() = 0;

	[[nodiscard]] const std::string& getName() const { return rendererName; }

	/**
	 * \brief The render target of this renderer. It can be used for example in ImGui
	 */
	[[nodiscard]] const std::shared_ptr<Texture<TextureBase::Texture2D>>& getRenderTexture() const
	{
		return renderedImageTexture;
	}

	/**
	 * \brief Each renderer contains a camera controller which will move the global camera when this controller is active.
	 */
	[[nodiscard]] const std::shared_ptr<CameraController>& getController() const { return cameraController; }

	/**
	 * \brief This is the resolution the renderedImageTexture have been generated with for the last frame.
	 */
	[[nodiscard]] const glm::ivec2& getRenderResolution() const { return renderResolution; }

protected:
	/**
	 * \brief Implement to reload renderer's specific shaders. (called when "reload shaders" button is clicked)
	 */
	virtual void onShadersReloadRequest();

	/**
	 * \brief Called every frame.Could be used to draw custom UI
	 */
	virtual void update();
	/**
	 * \brief Draw content. Called when render is dirty or every frame if forceRefresh is set to true
	 */
	virtual void render();

	/**
	 * \brief  Draw Toolbox UI content
	 */
	virtual void renderUI();

	/**
	 * \brief Notify the renderer that some parameter have been changed and we should call render()
	 */
	void markRenderDirty();
    
    uint32_t currentAttribute = 0;
    uint32_t currentTimestep = 0;

	std::string rendererName;
	glm::ivec2 renderResolution;
	std::shared_ptr<Camera> camera;
	std::shared_ptr<CameraController> cameraController;
	glm::vec3 skyColor = {0, 0, 0};

private:
	std::shared_ptr<Texture<TextureBase::Texture2D>> renderedImageTexture;
	bool forceRefresh = false;
	bool renderDirty = true;

	/**
	 * \brief Should we enable VSync ? //@TODO : move this into LVRC as a global parameter (and not a renderer-specific setting)
	 */
	bool vSync;
	void resize_internal(const int x, const int y) { onResize(x, y); }
};

#endif // LVRC_RENDERER_HPP
