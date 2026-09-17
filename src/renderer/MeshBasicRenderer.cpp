#include <utility>

#include "renderer/MeshBasicRenderer.hpp"
#include "ui/CameraController.hpp"
#include "ui/Window.hpp"

MeshBasicRenderer::MeshBasicRenderer(std::shared_ptr<Mesh> inMesh, std::string in_renderer_name,
                                     const std::shared_ptr<Camera>& inCamera) :
	Renderer(in_renderer_name, inCamera),
	mesh(inMesh)
{
    // Initialize transfer functions
    for(uint32_t attribute = 0; attribute < mesh->number_of_attributes_; attribute++){
        std::vector<glm::vec4> currentSample{128};
        transferFunctionSamples.push_back(currentSample);
        transferFunctions.push_back(
            TransferFunction(
                transferFunctionSamples[attribute].data(),
                transferFunctionSamples[attribute].size(),
                mesh->physical_datas_[attribute]));
    }
	
	bbMin = mesh->AABB_[0];
	bbMax = mesh->AABB_[1];
	cameraController->focus_mesh(*mesh);
	mesh->onMeshUpdated.add_object(this, &MeshBasicRenderer::onMeshUpdate);
}

MeshBasicRenderer::~MeshBasicRenderer()
{
	Window::singleton().onKeyboardEvent.clear_object(this);
	mesh->onMeshUpdated.clear_object(this);
}

void MeshBasicRenderer::renderUI()
{
	Renderer::renderUI();

	if(ImGui::CollapsingHeader("Scalar field", ImGuiTreeNodeFlags_DefaultOpen|ImGuiTreeNodeFlags_AllowItemOverlap)){
		if(transferFunctions[currentAttribute].draw("Transfer function"))
			updateOrCreateTransferFunctionTexture();

		std::string preview_label;
		if(mesh->has_element_scalar_fields() && getIsElementFieldActive() && getCurrentElementFieldIndex() < mesh->get_number_of_element_scalar_fields()){
			const auto& elem_field = mesh->get_element_scalar_field(getCurrentElementFieldIndex());
			preview_label = elem_field.name + " [Element P" + std::to_string(elem_field.field_order) + "]";
		} else if(currentAttribute < mesh->physical_data_names_.size()){
			preview_label = mesh->physical_data_names_[currentAttribute] + " [Nodal]";
		} else {
			preview_label = "None";
		}

		if(ImGui::BeginCombo("Field", preview_label.c_str())){
			// Nodal fields
			for(uint32_t n = 0; n < mesh->number_of_attributes_; n++){
				bool is_selected = !getIsElementFieldActive() && currentAttribute == n;
				std::string label = mesh->physical_data_names_[n] + " [Nodal]";
				if(ImGui::Selectable(label.c_str(), is_selected)){
					selectNodalField(n);
				}
				if(is_selected) ImGui::SetItemDefaultFocus();
			}

			// Element fields
			if(mesh->has_element_scalar_fields()){
				for(size_t e = 0; e < mesh->get_number_of_element_scalar_fields(); e++){
					bool is_selected = getIsElementFieldActive() && getCurrentElementFieldIndex() == e;
					const auto& elem_field = mesh->get_element_scalar_field(e);
					std::string label = elem_field.name + " [Element P" + std::to_string(elem_field.field_order) + "]";
					if(ImGui::Selectable(label.c_str(), is_selected)){
						selectElementField(e);
					}
					if(is_selected) ImGui::SetItemDefaultFocus();
				}
			}

			ImGui::EndCombo();
		}
	}
}

void MeshBasicRenderer::selectNodalField(size_t index)
{
	if(currentAttribute != index){
		currentAttribute = static_cast<uint32_t>(index);
		updateOrCreatePhysicalValuesSSBO();
		updateOrCreateTransferFunctionTexture();
		onCurrentAttributeChanged();
	}
}

void MeshBasicRenderer::selectElementField(size_t index)
{
	(void)index;
}

void MeshBasicRenderer::onCurrentAttributeChanged()
{
}

void MeshBasicRenderer::resetRenderer()
{
	physicalValuesSSBO = nullptr;
	updateOrCreatePhysicalValuesSSBO();
	updateOrCreateTransferFunctionTexture();
}

void MeshBasicRenderer::updateOrCreatePhysicalValuesSSBO()
{
	markRenderDirty();

	if (!physicalValuesSSBO)
	{
		physicalValuesSSBO = std::make_unique<ShaderStorageBufferObject>(10);
	}

	// Update physical value SSBO
	physicalValuesSSBO->allocate(sizeof(float) * mesh->physical_datas_[currentAttribute][0].size());
	physicalValuesSSBO->use();
	physicalValuesSSBO->write(mesh->physical_datas_[currentAttribute][0].data(),
	                          sizeof(float) * mesh->physical_datas_[currentAttribute][0].size());
	physicalValuesSSBO->release();
	markRenderDirty();
}

void MeshBasicRenderer::update()
{
	Renderer::update();

	if(meshDirty){
		meshDirty = false;
		resetRenderer();
		markRenderDirty();
	}
}

void MeshBasicRenderer::updateOrCreateTransferFunctionTexture()
{
	if (!transferFunctionTexture.isGenerated())
	{
		transferFunctionTexture.gen();
		transferFunctionTexture.bind();
		transferFunctionTexture.allocate(TextureBase::RGBA32F,
		                                 static_cast<int>(transferFunctionSamples[currentAttribute].size()));
		transferFunctionTexture.setFilterAndWrapParameters(GL_LINEAR, GL_CLAMP_TO_EDGE);
	}
	else
		transferFunctionTexture.bind();

	auto buffer = std::vector<float>(transferFunctionSamples[currentAttribute].size() * 4);

	for (size_t i = 0; i < transferFunctionSamples[currentAttribute].size(); i++)
		for (int j = 0; j < 4; j++)
			buffer[i * 4 + j] = transferFunctionSamples[currentAttribute][i][j];

	transferFunctionTexture.fill(buffer.data());
	transferFunctionTexture.unbind();

	markRenderDirty();
}
