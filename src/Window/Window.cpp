#include "imgui/imgui.h"
#include "imgui/backend/imgui_impl_glfw.h"
#include "imgui/backend/imgui_impl_opengl3.h"

#include "Window.h"
#include "src/Tools/Logger.h"

// TODO: Move these to Renderer
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow* window, bool isTabPressed, float deltaTime);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xOffset, double yOffset);

bool Window::init(unsigned int width, unsigned int height, std::string title)
{
    if (!glfwInit())
    {
        Logger::log(1, "%s: glfwInit() error\n", __FUNCTION__);;
        return false;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    mWindow = glfwCreateWindow(width, height, title.c_str(), NULL, NULL);

    if (!mWindow) {
        glfwTerminate();
        Logger::log(1, "%s error: Could not create window\n", __FUNCTION__);
        return false;
    }

    glfwMakeContextCurrent(mWindow);

    mRenderer = std::make_unique<Renderer>(mWindow);

    if (!mRenderer->init(width, height)) {
        glfwTerminate();
        Logger::log(1, "%s error: Could not init OpenGL\n", __FUNCTION__);
        return false;
    }

    glfwSetFramebufferSizeCallback(mWindow, framebuffer_size_callback);

    glfwSetInputMode(mWindow, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    glfwSetCursorPosCallback(mWindow, mouse_callback);

    glfwSetScrollCallback(mWindow, scroll_callback);

    lastX = width / 2.0f;
    lastY = height / 2.0f;

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
}

void Window::cleanup()
{
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    // Terminate GLFW
    glfwTerminate();
}

void processInput(GLFWwindow* window, bool isTabPressed, float deltaTime)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (!tabKeyPressed && isTabPressed)
    {
        if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS)
            g_camera->Mode = static_cast<CameraMode>((g_camera->Mode + 1) % MODE_COUNT);
    }

    if (controlCamera && g_camera->Mode == FLY)
    {
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
            g_camera->ProcessKeyboard(FORWARD, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
            g_camera->ProcessKeyboard(BACKWARD, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
            g_camera->ProcessKeyboard(LEFT, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
            g_camera->ProcessKeyboard(RIGHT, deltaTime);
    }
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}

void mouse_callback(GLFWwindow* window, double xPosIn, double yPosIn)
{
    if (ImGui::GetIO().WantCaptureMouse) {
        return;
    }

    float xPos = static_cast<float>(xPosIn);
    float yPos = static_cast<float>(yPosIn);

    if (firstMouse)
    {
        lastX = xPos;
        lastY = yPos;
        firstMouse = false;
    }

    float xOffset = xPos - lastX;
    float yOffset = lastY - yPos;

    lastX = xPos;
    lastY = yPos;

    if (g_camera->Mode == PLAYER_FOLLOW)
        g_player->PlayerProcessMouseMovement(xOffset);
    else if (g_camera->Mode == ENEMY_FOLLOW)
        g_enemy->EnemyProcessMouseMovement(xOffset, yOffset, true);

    g_camera->ProcessMouseMovement(xOffset, yOffset);

    if (g_camera->Mode == PLAYER_FOLLOW)
    {
        g_player->PlayerYaw = g_camera->Yaw;
        g_player->UpdatePlayerVectors();
    }
    else if (g_camera->Mode == ENEMY_FOLLOW)
    {
        g_enemy->UpdateEnemyCameraVectors();
    }
    //    xOfst = xOffset;
}

void scroll_callback(GLFWwindow* window, double xOffset, double yOffset)
{
    g_camera->ProcessMouseScroll(static_cast<float>(yOffset));
}