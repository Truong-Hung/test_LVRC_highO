#include "ui/Window.hpp"

#include <filesystem>
#include <iostream>
#include <string>

#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"
#include "ImGuizmo.h"

static Window* window_singleton = nullptr;

Window::Window(uint32_t width, uint32_t height)
{
	window_singleton = this;
	if (!glfwInit())
	{
		throw std::runtime_error("[ERROR] Unable to initialize GLFW3");
	}

	glfwSetErrorCallback([](int error, const char* description)
	{
		throw std::runtime_error(
			"GLFW error (" + std::to_string(error) + ") " + description
		);
	});

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	glfwWindow = glfwCreateWindow(width, height, "LittleVolumeRayCaster", nullptr, nullptr);

	glfwMakeContextCurrent(glfwWindow);

	if (!glfwWindow)
	{
		glfwTerminate();
		throw std::runtime_error("[Error] Unable to create GLFW Window.");
	}

	if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress)))
	{
		throw std::runtime_error("[Error] Unable to load GLAD.");
	}
	std::cout << "[Window]"
		<< std::endl
		<< "-- OpenGL version : "
		<< GLVersion.major << "." << GLVersion.minor
		<< std::endl
		<< "-- Initial size : " << width << "x" << height
		<< std::endl;

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigWindowsMoveFromTitleBarOnly = true;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad; // Enable Gamepad Controls
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

	ImGui::StyleColorsDark();

	// Load default ini file
	if (!std::filesystem::exists("../resources/imgui.ini"))
	{
		ImGui::LoadIniSettingsFromDisk("../resources/default_layout.ini");
	}

	ImGui_ImplGlfw_InitForOpenGL(glfwWindow, false);
	ImGui_ImplOpenGL3_Init("#version 130");

	// Setup and bind window events
	glfwSetWindowUserPointer(glfwWindow, this);


	glfwSetCharCallback(glfwWindow, [](GLFWwindow* window, const unsigned codepoint)
	{
		ImGui_ImplGlfw_CharCallback(window, codepoint);
		static_cast<Window*>(glfwGetWindowUserPointer(window))->onCharacterPressed.execute(codepoint);
	});

	glfwSetFramebufferSizeCallback(glfwWindow, [](GLFWwindow* window, const int width, const int height)
	{
		static_cast<Window*>(glfwGetWindowUserPointer(window))->onWindowResize.execute(
			width, height);
	});

	glfwSetCursorPosCallback(glfwWindow, [](GLFWwindow* window, const double xpos, const double ypos)
	{
		ImGui_ImplGlfw_CursorPosCallback(window, xpos, ypos);
		const auto w = static_cast<Window*>(glfwGetWindowUserPointer(window));
		const MouseMoveEvent event(xpos, ypos, w->lastMouseXPos, w->lastMouseYPos);
		w->onMouseMove.execute(event);
		w->lastMouseXPos = xpos;
		w->lastMouseYPos = ypos;
	});
	glfwSetKeyCallback(glfwWindow, [](GLFWwindow* window, const int key, const int scancode, const int action, const int mods)
	{
		ImGui_ImplGlfw_KeyCallback(window, key, scancode, action, mods);
		static_cast<Window*>(glfwGetWindowUserPointer(window))->onKeyboardEvent.execute(
			KeyboardEvent(key, action, mods));
	});

	glfwSetMouseButtonCallback(glfwWindow, [](GLFWwindow* window, const int button, const int action, const int mods)
	{
		ImGui_ImplGlfw_MouseButtonCallback(window, button, action, mods);
		static_cast<Window*>(glfwGetWindowUserPointer(window))->onMouseButtonPressed.execute(
			MouseButtonEvent(button, action, mods));
	});

	glfwSetScrollCallback(glfwWindow, [](GLFWwindow* window, const double xoffset, const double yoffset)
		{
			ImGui_ImplGlfw_ScrollCallback(window, xoffset, yoffset);			;
			static_cast<Window*>(glfwGetWindowUserPointer(window))->onMouseScroll.execute(MouseScrollEvent(xoffset, yoffset));
		}
	);
	lastTime = glfwGetTime();
}

Window::~Window()
{
	glfwTerminate();
	window_singleton = nullptr;
}

Window& Window::singleton()
{
	assert(window_singleton);
	return *window_singleton;
}

bool Window::beginFrame()
{
	// Update input and window events

	// Refresh delta second
	const double current_time = glfwGetTime();
	deltaTime = current_time - lastTime;
	lastTime = current_time;

	// Clear backbuffer and reset openGL
	glClearColor(0., 0., 0., 0.);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);

	// Prepare new ImGui record
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();
	ImGuizmo::BeginFrame();
	ImGuizmo::AllowAxisFlip(false);

	onStartFrame.execute(deltaTime);
	glfwPollEvents();
	onInputsProcessed.execute();
	return !glfwWindowShouldClose(glfwWindow);
}

void Window::endFrame()
{
	onEndFrame.execute(deltaTime);

	// Render ImGui data
	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
	
	glfwSwapBuffers(glfwWindow);
}

void Window::stop() const
{
	glfwSetWindowShouldClose(glfwWindow, true);
}

void Window::vSync(bool enable)
{
	if(enable){
		glfwSwapInterval(1);
	}else{
		glfwSwapInterval(0);
	}
}
