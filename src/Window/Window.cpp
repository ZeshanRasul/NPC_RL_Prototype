#include "imgui/imgui.h"
#include "imgui/backend/imgui_impl_glfw.h"
#include "imgui/backend/imgui_impl_opengl3.h"

#include "Window.h"
#include "src/InputManager.h"
#include "src/Tools/Logger.h"

void framebuffer_size_callback(GLFWwindow* window, int width, int height);

bool Window::Init(unsigned int width, unsigned int height, std::string title)
{
	m_width = width;
	m_height = height;

	if (!glfwInit())
	{
		Logger::Log(1, "%s: glfwInit() error\n", __FUNCTION__);;
		return false;
	}

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	
	GLFWmonitor* monitor = glfwGetPrimaryMonitor();
	if (!monitor) {
		std::cerr << "No monitor found!\n";
		glfwTerminate();
		return -1;
	}

	const GLFWvidmode* mode = glfwGetVideoMode(monitor);

	glfwWindowHint(GLFW_RED_BITS, mode->redBits);
	glfwWindowHint(GLFW_GREEN_BITS, mode->greenBits);
	glfwWindowHint(GLFW_BLUE_BITS, mode->blueBits);
	glfwWindowHint(GLFW_REFRESH_RATE, mode->refreshRate);

	m_window = glfwCreateWindow(width, height, title.c_str(), monitor, NULL);

	if (!m_window) {
		glfwTerminate();
		Logger::Log(1, "%s error: Could not create window\n", __FUNCTION__);
		return false;
	}

	glfwMakeContextCurrent(m_window);

	m_renderer = new Renderer(m_window);

	if (!m_renderer->Init(width, height)) {
		glfwTerminate();
		Logger::Log(1, "%s error: Could not init OpenGL\n", __FUNCTION__);
		return false;
	}

	glfwSetFramebufferSizeCallback(m_window, framebuffer_size_callback);

	glfwSetInputMode(m_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	glfwSetCursorPosCallback(m_window, InputManager::MouseCallback);

	glfwSetScrollCallback(m_window, InputManager::ScrollCallback);

	int fbWidth, fbHeight;
	glfwGetFramebufferSize(m_window, &fbWidth, &fbHeight);
	framebuffer_size_callback(m_window, fbWidth, fbHeight);

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

	// Setup Platform/Renderer backends
	ImGui_ImplGlfw_InitForOpenGL(m_window, true);          // Second param install_callback=true will install GLFW callbacks and chain to existing ones.
	ImGui_ImplOpenGL3_Init();

	return true;
}

void Window::MainLoop()
{
	glfwSwapBuffers(m_window);
	glfwPollEvents();
}

void Window::SetInputManager(InputManager* inputManager)
{
	glfwSetWindowUserPointer(m_window, inputManager);
}

void Window::Cleanup()
{
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	glfwDestroyWindow(m_window);
	glfwTerminate();

}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	glViewport(0, 0, width, height);
}



