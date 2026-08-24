#ifndef MESH_BASIC_RENDERER_H_
#define MESH_BASIC_RENDERER_H_

#include "Renderer.hpp"
#include "opengl/utils/Texture.hpp"
#include "ui/TransferFunction.hpp"
#include "renderer/opengl/utils/ShaderStorageBufferObject.hpp"
#include "mesh/Mesh.hpp"



// This renderer class is dedicated to the rendering of a single 3D Mesh. It also include a transfer function
class MeshBasicRenderer : public Renderer
{
public:
	MeshBasicRenderer(std::shared_ptr<Mesh> mesh, std::string renderer_name, const std::shared_ptr<Camera>& camera);
	virtual ~MeshBasicRenderer();

	[[nodiscard]] const std::shared_ptr<Mesh>& getMesh() const { return mesh; }
	[[nodiscard]] const TransferFunction& getCurrentTransferFunction() const { return transferFunctions[currentAttribute]; }
	[[nodiscard]] uint32_t getPhysicalValueIndex() const { return currentAttribute; }

protected:
	virtual void renderUI() override;
	virtual void resetRenderer() override;

	virtual void updateOrCreateTransferFunctionTexture();
	virtual void updateOrCreatePhysicalValuesSSBO();
	virtual void onCurrentAttributeChanged();

	// Reload all the mesh data (when mesh is updated)
	virtual void update() override;

protected:
	// Mesh
	std::shared_ptr<Mesh> mesh;
	glm::vec3 bbMin{};
	glm::vec3 bbMax{};

	// Transfer Function
	Texture<TextureBase::Texture1D, TextureBase::Immutable> transferFunctionTexture;
	std::vector<std::vector<glm::vec4>> transferFunctionSamples;
	std::vector<TransferFunction> transferFunctions;

	// Physical values
	std::unique_ptr<ShaderStorageBufferObject> physicalValuesSSBO;
	uint32_t currentAttribute = 0;

private:
	bool meshDirty = false;
	void onMeshUpdate() { meshDirty = true; }
};


#endif // MESH_BASIC_RENDERER_H_
