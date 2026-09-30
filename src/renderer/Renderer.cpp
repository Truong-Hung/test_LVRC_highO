#include "renderer/Renderer.hpp"
#include <optional>
#include "imgui.h"
#include "ImGuizmo.h"
#include "imgui_internal.h"
#include "renderer/opengl/utils/FrameBufferObject.hpp"
#include "renderer/opengl/utils/ShaderStorageBufferObject.hpp"
#include "ui/OrbitCameraController.hpp"
#include "ui/Window.hpp"

Renderer::Renderer(std::string name, 
				   const std::shared_ptr<Camera>& camera) : 
	rendererName(name),
    renderResolution(1600, 900),
    camera(camera),
    cameraController(std::make_shared<OrbitCameraController>(camera)),
	vSync(true)
{
	std::cout << "Loading renderer '" << rendererName << "'..." << std::endl;
	
	renderedImageTexture = std::make_shared<Texture<TextureBase::Texture2D>>();
	renderedImageTexture->gen();
	renderedImageTexture->bind();
	renderedImageTexture->setFilterAndWrapParameters(GL_LINEAR, GL_CLAMP_TO_EDGE);
	renderedImageTexture->allocateAndFill(TextureBase::RGBA32F); // Whe draw the result into a fp32 texture for better precision (it's not a big issue if it is decreased later)
	renderedImageTexture->unbind();

	cameraController->setViewportSize(renderResolution);
	camera->onCameraMoved.add_object(this, &Renderer::markRenderDirty);
}

Renderer::~Renderer()
{
	camera->onCameraMoved.clear_object(this);
}

bool Renderer::renderInternal()
{
	// Update render texture target
	if(renderDirty || forceRefresh){
		renderDirty = false;
		render();
	}

	update();

	// Reset viewport
	glViewport(0, 0, renderResolution.x, renderResolution.y);

	// Switch to Back-buffer
	FrameBufferObject::unbindAnyFrameBuffer();

	if(ImGui::Begin(("LVRC (" + rendererName + ")").c_str()))
		renderUI();
		
	ImGui::End();

	return vSync;
}

bool Renderer::onResize(int width, int height)
{
	if (width != renderResolution.x || height != renderResolution.y)
	{
		renderResolution.x = width;
		renderResolution.y = height;
		cameraController->setViewportSize(renderResolution);

		renderedImageTexture->bind();
		renderedImageTexture->allocateAndFill(nullptr, renderResolution.x, renderResolution.y);
		renderedImageTexture->unbind();

		markRenderDirty();
		return true;
	}
	return false;
}

void Renderer::onShadersReloadRequest()
{
	markRenderDirty();
}

void Renderer::update()
{
	// Display generated image
	if(ImGui::Begin("Viewport", nullptr, ImGuiWindowFlags_MenuBar)){
		onResize(static_cast<int>(ImGui::GetContentRegionAvail().x),
		         static_cast<int>(ImGui::GetContentRegionAvail().y));

		if(!ImGuizmo::IsUsing() && ImGui::IsWindowHovered())
			cameraController->trigger_inputs(); // This window is currently active

		ImGui::Image(
			reinterpret_cast<ImTextureID>(static_cast<uint64_t>(renderedImageTexture->getName())),
		    ImGui::GetContentRegionAvail(), {0, 1.0}, {1.0, 0});
	}

	ImGui::End();
}

void Renderer::render()
{
}

void Renderer::renderUI()
{
	/* FPS COUNTER */
	constexpr int renderingTimeValuesMaxCount = 240;
	static float renderingTimeValues[renderingTimeValuesMaxCount] = {};
	static int renderingTimeValuesOffset = 0;
	float average = 0;
	float count = 0;
	float currentValue = 1000.0f / ImGui::GetIO().Framerate;

	if (currentValue > 300.0f)
		forceRefresh = false;

	renderingTimeValues[renderingTimeValuesOffset] = currentValue;
	renderingTimeValuesOffset = (renderingTimeValuesOffset + 1) % renderingTimeValuesMaxCount;

	for (float renderingTimeValue : renderingTimeValues)
	{
		if (renderingTimeValue != 0)
		{
			average += renderingTimeValue;
			++count;
		}
	}

	average /= count;

	const float maxY = std::max(std::ceil(average + average * .5f), currentValue + currentValue * .5f);
	char label[128];

	snprintf(
		label,
		sizeof(label),
		"Render Time (ms):\n"
		"Min y: 0\n"
		"Max y: %.f\n"
		"Av: %.3fms %.0ffps\n"
		"Curr: %.3fms %.0ffps\n",
		maxY,
		average, 1000 / average,
		currentValue, 1000 / currentValue);

	ImGui::PlotLines(
		label, renderingTimeValues,
		IM_ARRAYSIZE(renderingTimeValues), renderingTimeValuesOffset,
		nullptr, 0., maxY, ImVec2(0., 50.));

	/* RENDERER OPTIONS */

	if (ImGui::CollapsingHeader("Renderer", ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_AllowItemOverlap))
	{
		ImGui::Checkbox("Force refresh", &forceRefresh);
		ImGui::Checkbox("Enable V-Sync", &vSync);

		if (ImGui::CollapsingHeader("Camera", ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_AllowItemOverlap))
		{
			float fieldOfView = camera->getFov();
			if (ImGui::SliderFloat("FOV", &fieldOfView, 1.f, 180.f))
			{
				camera->setFov(fieldOfView);
				renderDirty = true;
			}

			if (ImGui::TreeNodeEx("Camera View Points & Presets", ImGuiTreeNodeFlags_DefaultOpen))
			{
				static std::vector<CameraPose> cameraPresets = {
					camera->getPose("Default Angle")
				};
				static int selectedPresetIdx = 0;
				static char presetNameBuf[64] = "My View Point";
				static char filenameBuf[128] = "camera_presets.txt";
				static std::string statusMsg = "";

				if (selectedPresetIdx >= static_cast<int>(cameraPresets.size()))
				{
					selectedPresetIdx = std::max(0, static_cast<int>(cameraPresets.size()) - 1);
				}

				// Preset Selector Combo
				if (!cameraPresets.empty())
				{
					const std::string comboLabel = cameraPresets[selectedPresetIdx].name;
					if (ImGui::BeginCombo("View Angle", comboLabel.c_str()))
					{
						for (int i = 0; i < static_cast<int>(cameraPresets.size()); ++i)
						{
							const bool isSelected = (selectedPresetIdx == i);
							if (ImGui::Selectable(cameraPresets[i].name.c_str(), isSelected))
							{
								selectedPresetIdx = i;
							}
							if (isSelected)
							{
								ImGui::SetItemDefaultFocus();
							}
						}
						ImGui::EndCombo();
					}
				}

				// Load & Overwrite buttons
				if (ImGui::Button("Load Angle"))
				{
					if (selectedPresetIdx >= 0 && selectedPresetIdx < static_cast<int>(cameraPresets.size()))
					{
						camera->setPose(cameraPresets[selectedPresetIdx]);
						renderDirty = true;
						statusMsg = "Loaded view: " + cameraPresets[selectedPresetIdx].name;
					}
				}
				ImGui::SameLine();
				if (ImGui::Button("Overwrite"))
				{
					if (selectedPresetIdx >= 0 && selectedPresetIdx < static_cast<int>(cameraPresets.size()))
					{
						std::string curName = cameraPresets[selectedPresetIdx].name;
						cameraPresets[selectedPresetIdx] = camera->getPose(curName);
						statusMsg = "Overwrote view: " + curName;
					}
				}

				// Save new preset
				ImGui::InputText("Name", presetNameBuf, sizeof(presetNameBuf));
				if (ImGui::Button("Save Current Angle"))
				{
					std::string nameStr = (strlen(presetNameBuf) > 0)
						? presetNameBuf
						: ("View " + std::to_string(cameraPresets.size() + 1));
					CameraPose newPose = camera->getPose(nameStr);
					cameraPresets.push_back(newPose);
					selectedPresetIdx = static_cast<int>(cameraPresets.size()) - 1;
					statusMsg = "Saved view: " + nameStr;
				}

				if (cameraPresets.size() > 1)
				{
					ImGui::SameLine();
					if (ImGui::Button("Delete"))
					{
						statusMsg = "Deleted view: " + cameraPresets[selectedPresetIdx].name;
						cameraPresets.erase(cameraPresets.begin() + selectedPresetIdx);
						selectedPresetIdx = std::max(0, selectedPresetIdx - 1);
					}
				}

				ImGui::Separator();
				ImGui::Text("File Import / Export:");
				ImGui::InputText("File", filenameBuf, sizeof(filenameBuf));

				if (ImGui::Button("Save to File"))
				{
					if (Camera::savePresetsToFile(filenameBuf, cameraPresets))
					{
						statusMsg = "Saved " + std::to_string(cameraPresets.size()) + " view(s) to " + filenameBuf;
					}
					else
					{
						statusMsg = "Failed to save file: " + std::string(filenameBuf);
					}
				}
				ImGui::SameLine();
				if (ImGui::Button("Load from File"))
				{
					std::vector<CameraPose> loaded;
					if (Camera::loadPresetsFromFile(filenameBuf, loaded))
					{
						cameraPresets = loaded;
						selectedPresetIdx = 0;
						statusMsg = "Loaded " + std::to_string(loaded.size()) + " view(s) from " + filenameBuf;
					}
					else
					{
						statusMsg = "Failed to load file: " + std::string(filenameBuf);
					}
				}

				ImGui::Separator();
				if (ImGui::Button("Copy to Clipboard"))
				{
					CameraPose curPose = camera->getPose();
					ImGui::SetClipboardText(curPose.toString().c_str());
					statusMsg = "Copied camera pose string to clipboard!";
				}
				ImGui::SameLine();
				if (ImGui::Button("Paste from Clipboard"))
				{
					const char* clipText = ImGui::GetClipboardText();
					if (clipText)
					{
						CameraPose pastedPose;
						if (CameraPose::fromString(clipText, pastedPose))
						{
							camera->setPose(pastedPose);
							renderDirty = true;
							statusMsg = "Applied pose from clipboard!";
						}
						else
						{
							statusMsg = "Invalid pose format in clipboard.";
						}
					}
				}

				if (!statusMsg.empty())
				{
					ImGui::TextColored(ImVec4(0.4f, 0.9f, 0.4f, 1.0f), "%s", statusMsg.c_str());
				}

				if (ImGui::TreeNode("Live Camera Info"))
				{
					glm::vec3 pos = camera->getPosition();
					glm::quat rot = camera->getRotation();
					float dist = camera->getDistance();

					ImGui::Text("Position:  X: %.3f | Y: %.3f | Z: %.3f", pos.x, pos.y, pos.z);
					ImGui::Text("Rotation:  W: %.3f | X: %.3f | Y: %.3f | Z: %.3f", rot.w, rot.x, rot.y, rot.z);
					ImGui::Text("Distance:  %.3f", dist);
					ImGui::Text("FOV:       %.1f deg", camera->getFov());
					ImGui::TreePop();
				}

				ImGui::TreePop();
			}

			if (ImGui::TreeNode("Controls"))
			{
				ImGui::Bullet();
				ImGui::Text("Use");
				ImGui::SameLine();
				ImGui::TextColored(ImVec4(1.f, .5f, .5f, 1.f), "LMB");

				if (ImGui::IsItemHovered())
				{
					ImGui::BeginTooltip();
					ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
					ImGui::TextUnformatted("Left Mouse Button");
					ImGui::PopTextWrapPos();
					ImGui::EndTooltip();
				}

				ImGui::SameLine();
				ImGui::Text("to rotate.");

				ImGui::Bullet();
				ImGui::Text("Use");
				ImGui::SameLine();
				ImGui::TextColored(ImVec4(1.f, .5f, .5f, 1.f), "RMB");

				if (ImGui::IsItemHovered())
				{
					ImGui::BeginTooltip();
					ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.f);
					ImGui::TextUnformatted("Right Mouse Button");
					ImGui::PopTextWrapPos();
					ImGui::EndTooltip();
				}

				ImGui::SameLine();
				ImGui::Text("to translate.");

				ImGui::Bullet();
				ImGui::Text("Use");
				ImGui::SameLine();
				ImGui::TextColored(ImVec4(1.f, .5f, .5f, 1.f), "Scroll Wheel");

				if (ImGui::IsItemHovered())
				{
					ImGui::BeginTooltip();
					ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.f);
					ImGui::TextUnformatted("Mouse's Vertical Scroll Wheel");
					ImGui::PopTextWrapPos();
					ImGui::EndTooltip();
				}

				ImGui::SameLine();
				ImGui::Text("to zoom.");

				ImGui::Bullet();
				ImGui::Text("Use");
				ImGui::SameLine();
				ImGui::TextColored(ImVec4(1.f, .5f, .5f, 1.f), "SHIFT + LMB");

				if (ImGui::IsItemHovered())
				{
					ImGui::BeginTooltip();
					ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.f);
					ImGui::TextUnformatted("Shift button + Left Mouse Button");
					ImGui::PopTextWrapPos();
					ImGui::EndTooltip();
				}

				ImGui::SameLine();
				ImGui::Text("to recenter.");

				ImGui::TreePop();
			}
		}
	}
}

void Renderer::markRenderDirty()
{
	renderDirty = true;
}
