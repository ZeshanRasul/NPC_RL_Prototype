#include "imgui/imgui.h"
#include "imgui/backend/imgui_impl_glfw.h"
#include "imgui/backend/imgui_impl_opengl3.h"

#include "Window.h"
#include "src/InputManager.h"
#include "src/Tools/Logger.h"

// TODO: Move these to Renderer
void framebuffer_size_callback(GLFWwindow* window, int width, int height);

bool Window::init(unsigned int width, unsigned int height, std::string title)
{
	mWidth = width;
	mHeight = height;

	if (!glfwInit())
	{
		Logger::log(1, "%s: glfwInit() error\n", __FUNCTION__);;
		return false;
	}

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	mWindow = glfwCreateWindow(width, height, title.c_str(), NULL, NULL);

	if (!mWindow) {
		glfwTerminate();
		Logger::log(1, "%s error: Could not create window\n", __FUNCTION__);
		return false;
	}

	glfwMakeContextCurrent(mWindow);

	mRenderer = new Renderer(mWindow);

	if (!mRenderer->init(width, height)) {
		glfwTerminate();
		Logger::log(1, "%s error: Could not init OpenGL\n", __FUNCTION__);
		return false;
	}

	glfwSetFramebufferSizeCallback(mWindow, framebuffer_size_callback);

	glfwSetInputMode(mWindow, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	glfwSetCursorPosCallback(mWindow, InputManager::mouse_callback);

	glfwSetScrollCallback(mWindow, InputManager::scroll_callback);

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

	// Setup Platform/Renderer backends
	ImGui_ImplGlfw_InitForOpenGL(mWindow, true);          // Second param install_callback=true will install GLFW callbacks and chain to existing ones.
	ImGui_ImplOpenGL3_Init();

	return true;
}

void Window::mainLoop()
{
	glfwSwapBuffers(mWindow);
	glfwPollEvents();
}

void Window::setInputManager(InputManager* inputManager)
{
	glfwSetWindowUserPointer(mWindow, inputManager);
}

void Window::cleanup()
{
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	glfwDestroyWindow(mWindow);
	glfwTerminate();

}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	glViewport(0, 0, width, height);
}



