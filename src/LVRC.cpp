#include "LVRC.hpp"

#include <imgui.h>
#include "ui/FilePicker.hpp"

#ifdef USING_OPTIX
    #include "renderer/cuda/OptixRenderer.hpp"
#endif



static std::unique_ptr<Renderer> currentRenderer = nullptr;

LVRC::LVRC(
	uint32_t width, 
	uint32_t height
	) :
	window_(width, height), 
	globalCamera(std::make_shared<Camera>(45.f))
{
#ifdef USING_OPTIX
	mesh_ = std::make_shared<Mesh>("../resources/data/010_test.msh");
	currentRenderer = std::make_unique<OptixRenderer>(mesh_, globalCamera);
#endif
}

LVRC::~LVRC()
{
	currentRenderer = nullptr;
}

void LVRC::launch()
{
	while(window_.beginFrame()){
		// Draw ImGui Background dock tab
		ImGui::DockSpaceOverViewport(
			ImGui::GetMainViewport(), 
			ImGuiDockNodeFlags_PassthruCentralNode
		);

		// Draw menu bar
		if(ImGui::BeginMainMenuBar()){
			if(ImGui::BeginMenu("Menu")){
				if(ImGui::BeginMenu("Load mesh")){
					if(const auto file = FilePicker::select("/home/truong/", {".msh", ".lvrc", ".umesh"}))
						reload_with_mesh(file->string());

					ImGui::EndMenu();
				}

				if(ImGui::MenuItem("Quit"))
					window_.stop();

				ImGui::EndMenu();
			}

			ImGui::EndMainMenuBar();
		}

		// Draw renderer
		if(currentRenderer)
			Window::vSync(currentRenderer->renderInternal());
		
		window_.endFrame();
	}
}

void LVRC::reload_with_mesh(const std::string& file_path)
{
#ifdef USING_OPTIX
	mesh_ = std::make_shared<Mesh>(file_path);
	currentRenderer = std::make_unique<OptixRenderer>(mesh_, globalCamera);
#endif
}