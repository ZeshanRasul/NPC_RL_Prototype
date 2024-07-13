#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include "imgui/imgui.h"
#include "imgui/backend/imgui_impl_glfw.h"
#include "imgui/backend/imgui_impl_opengl3.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "Shader.h"
#include "Camera.h"
#include "GameObjects/Player.h"
#include "GameObjects/Enemy.h"
#include "GameObjects/Ground.h"
#include "GameObjects/Cell.h"
#include "Pathfinding/Grid.h"
#include "Primitives.h"

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow *window, bool isTabPressed, Player& player);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xOffset, double yOffset);

void handlePlayerMovement(GLFWwindow* window, Player& player, Camera& camera, float deltaTime);


Player* g_player = nullptr;

const unsigned int SCREEN_WIDTH = 1280;
const unsigned int SCREEN_HEIGHT = 720;

float deltaTime = 0.0f;
float lastFrame = 0.0f;
float currentFrame = 0.0f;


Camera camera(glm::vec3(50.0f, 3.0f, 80.0f));
float lastX = SCREEN_WIDTH / 2.0f;
float lastY = SCREEN_HEIGHT / 2.0f;
bool firstMouse = true;

void ShowCameraControlWindow(Camera& cam);

bool controlCamera = true;
bool spaceKeyPressed = false;

bool tabKeyPressed = false;

float xOfst = 0.0f;

struct Material {
    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;
    float shininess;
};

Material playerMaterial = {
    glm::vec3(0.0f, 0.0f, 1.0f),
    glm::vec3(0.0f, 0.2f, 0.87f),
    glm::vec3(0.2f, 0.2f, 0.2f),
    8.0f
};

Material enemyMaterial = {
    glm::vec3(1.0f, 0.2f, 0.2f),
    glm::vec3(1.0f, 0.2f, 0.2f),
    glm::vec3(0.2f, 0.2f, 0.2f),
    8.0f
};

glm::vec3 snapToGrid(const glm::vec3& position) {
    int gridX = static_cast<int>(position.x / CELL_SIZE);
    int gridZ = static_cast<int>(position.z / CELL_SIZE);
    return glm::vec3(gridX * CELL_SIZE + CELL_SIZE / 2.0f, position.y, gridZ * CELL_SIZE + CELL_SIZE / 2.0f);
}

struct DirLight {
    glm::vec3 direction;

    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;
};

void ShowLightControlWindow(DirLight& light);


struct PointLight {
    glm::vec3 position;

    float constant;
    float linear;
    float quadratic;

    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;
};

struct SpotLight {
    glm::vec3 position;
    glm::vec3 direction;
    float cutoff;
    float outerCutoff;

    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;
};

DirLight dirLight = {
        glm::vec3(-0.2f, -1.0f, -0.3f),

        glm::vec3(0.15f, 0.15f, 0.15f),
        glm::vec3(0.4f),
        glm::vec3(0.1f, 0.1f, 0.1f)
};

int main()
{
    // Initialize and configure GLFW
    if (!glfwInit())
    {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Create a windowed mode window and its OpenGL context
    GLFWwindow* window = glfwCreateWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "OpenGL Window", NULL, NULL);
    if (!window)
    {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    // Load all OpenGL function pointers with GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    glViewport(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);

    glEnable(GL_DEPTH_TEST);

    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    glfwSetCursorPosCallback(window, mouse_callback);

    glfwSetScrollCallback(window, scroll_callback);

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);          // Second param install_callback=true will install GLFW callbacks and chain to existing ones.
    ImGui_ImplOpenGL3_Init();


    unsigned int lightVAO;
    glGenVertexArrays(1, &lightVAO);
    glBindVertexArray(lightVAO);

    unsigned int VBO;
    glGenBuffers(1, &VBO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), &vertices, GL_STATIC_DRAW);


    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);

    Shader shader("C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Shaders/vertex.glsl", "C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Shaders/fragment.glsl");

    Shader lightShader("C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Shaders/lightVertex.glsl", "C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Shaders/lightFragment.glsl");

    Shader gridShader("C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Shaders/gridVertex.glsl", "C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Shaders/gridFragment.glsl");

    glm::mat4 view;
    glm::mat4 projection;

    Material groundMaterial = {
        glm::vec3(0.1f, 0.85f, 0.12f),
        glm::vec3(0.1f, 0.85f, 0.12f),
        glm::vec3(0.2f, 0.2f, 0.2f),
        8.0f
    };

    glm::vec3 pointLightPositions[] = {
        glm::vec3(0.7f,  0.2f,  2.0f),
        glm::vec3(2.3f, -3.3f, -4.0f),
        glm::vec3(-4.0f,  2.0f, -12.0f),
        glm::vec3(0.0f,  0.0f, -3.0f)
    };

    Player player(snapToGrid(glm::vec3(130.0f, 0.0f, 25.0f)), glm::vec3(0.02f, 0.02f, 0.02f), playerMaterial.diffuse);
    float playerCamRearOffset = 15.0f;
    float playerCamHeightOffset = 5.0f;

    g_player = &player;

    Enemy enemy(snapToGrid(glm::vec3(13.0f, 0.0f, 13.0f)), glm::vec3(0.02f, 0.02f, 0.02f), enemyMaterial.diffuse);
    float enemyCamRearOffset = 15.0f;
    float enemyCamHeightOffset = 5.0f;

    Ground ground(glm::vec3(-100.0f, -0.3f, 50.0f), glm::vec3(100.0f, 1.0f, 100.0f), groundMaterial.diffuse);
    Cell cell;
    cell.SetUpVAO();

    initializeGrid();

    // Render loop
    while (!glfwWindowShouldClose(window))
    {
        currentFrame = (float)glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        bool spaceKeyCurrentlyPressed = glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS;

        if (spaceKeyCurrentlyPressed && !spaceKeyPressed)
            controlCamera = !controlCamera;

        spaceKeyPressed = spaceKeyCurrentlyPressed;

        bool tabKeyCurrentlyPressed = glfwGetKey(window, GLFW_KEY_TAB) == GLFW_PRESS;


        // Input
        processInput(window, tabKeyCurrentlyPressed, player);

     

        tabKeyPressed = tabKeyCurrentlyPressed;

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        ShowLightControlWindow(dirLight);
        ShowCameraControlWindow(camera);

        ImGui::Begin("Player");

        ImGui::InputFloat3("Position", &player.getPosition()[0]);
        ImGui::InputFloat("Yaw", &player.PlayerYaw);

        ImGui::End();

        //if (camera.Mode == PLAYER_FOLLOW)
        //    player.PlayerProcessMouseMovement(xOfst);

        handlePlayerMovement(window, player, camera, deltaTime);

        if (camera.Mode == PLAYER_FOLLOW)
        {
            camera.FollowTarget(player.getPosition(), player.PlayerFront, playerCamRearOffset, playerCamHeightOffset);
            view = camera.GetViewMatrixPlayerFollow(player.getPosition(), glm::vec3(0.0f, 1.0f, 0.0f));
        }
        else if (camera.Mode == ENEMY_FOLLOW)
        {
            camera.FollowTarget(enemy.getPosition(), glm::vec3(1.0f, 0.0f, 0.0f), enemyCamRearOffset, enemyCamHeightOffset);
            view = camera.GetViewMatrixPlayerFollow(enemy.getPosition(), glm::vec3(0.0f, 1.0f, 0.0f));
        }
        else if (camera.Mode == FLY)
            view = camera.GetViewMatrix();

        projection = glm::perspective(glm::radians(camera.Zoom), (float)SCREEN_WIDTH / (float)SCREEN_HEIGHT, 0.1f, 100.0f);

        shader.use();

        shader.setMat4("view", view);
        shader.setMat4("proj", projection);

        // Render
        glClearColor(0.2f, 0.3f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        shader.setVec3("material.ambient", playerMaterial.ambient);
        shader.setVec3("material.diffuse", playerMaterial.diffuse);
        shader.setVec3("material.specular", playerMaterial.specular);
        shader.setFloat("material.shininess", playerMaterial.shininess);

        shader.setVec3("dirLight.direction", dirLight.direction);
        shader.setVec3("dirLight.ambient", dirLight.ambient);
        shader.setVec3("dirLight.diffuse", dirLight.diffuse);
        shader.setVec3("dirLight.specular", dirLight.specular);

        shader.setVec3("pointLights[0].position", pointLightPositions[0]);
        shader.setVec3("pointLights[0].ambient", 0.05f, 0.05f, 0.05f);
        shader.setVec3("pointLights[0].diffuse", 0.8f, 0.8f, 0.8f);
        shader.setVec3("pointLights[0].specular", 1.0f, 1.0f, 1.0f);
        shader.setFloat("pointLights[0].constant", 1.0f);
        shader.setFloat("pointLights[0].linear", 0.09f);
        shader.setFloat("pointLights[0].quadratic", 0.032f);

        shader.setVec3("pointLights[1].position", pointLightPositions[1]);
        shader.setVec3("pointLights[1].ambient", 0.05f, 0.05f, 0.05f);
        shader.setVec3("pointLights[1].diffuse", 0.8f, 0.8f, 0.8f);
        shader.setVec3("pointLights[1].specular", 1.0f, 1.0f, 1.0f);
        shader.setFloat("pointLights[1].constant", 1.0f);
        shader.setFloat("pointLights[1].linear", 0.09f);
        shader.setFloat("pointLights[1].quadratic", 0.032f);

        shader.setVec3("pointLights[2].position", pointLightPositions[2]);
        shader.setVec3("pointLights[2].ambient", 0.05f, 0.05f, 0.05f);
        shader.setVec3("pointLights[2].diffuse", 0.8f, 0.8f, 0.8f);
        shader.setVec3("pointLights[2].specular", 1.0f, 1.0f, 1.0f);
        shader.setFloat("pointLights[2].constant", 1.0f);
        shader.setFloat("pointLights[2].linear", 0.09f);
        shader.setFloat("pointLights[2].quadratic", 0.032f);

        shader.setVec3("pointLights[3].position", pointLightPositions[3]);
        shader.setVec3("pointLights[3].ambient", 0.05f, 0.05f, 0.05f);
        shader.setVec3("pointLights[3].diffuse", 0.8f, 0.8f, 0.8f);
        shader.setVec3("pointLights[3].specular", 1.0f, 1.0f, 1.0f);
        shader.setFloat("pointLights[3].constant", 1.0f);
        shader.setFloat("pointLights[3].linear", 0.09f);
        shader.setFloat("pointLights[3].quadratic", 0.032f);

        shader.setVec3("spotLight.position", camera.Position);
        shader.setVec3("spotLight.direction", camera.Front);
        shader.setVec3("spotLight.ambient", 0.0f, 0.0f, 0.0f);
        shader.setVec3("spotLight.diffuse", 1.0f, 1.0f, 1.0f);
        shader.setVec3("spotLight.specular", 1.0f, 1.0f, 1.0f);
        shader.setFloat("spotLight.constant", 1.0f);
        shader.setFloat("spotLight.linear", 0.09f);
        shader.setFloat("spotLight.quadratic", 0.032f);
        shader.setFloat("spotLight.cutOff", glm::cos(glm::radians(1.5f)));
        shader.setFloat("spotLight.outerCutOff", glm::cos(glm::radians(5.0f)));
        shader.setVec3("viewPos", camera.Position);

        player.Draw(shader);

        shader.setVec3("material.ambient", enemyMaterial.ambient);
        shader.setVec3("material.diffuse", enemyMaterial.diffuse);
        shader.setVec3("material.specular", enemyMaterial.specular);
        shader.setFloat("material.shininess", enemyMaterial.shininess);

        std::vector<glm::ivec2> path = findPath(
            glm::ivec2(enemy.getPosition().x / CELL_SIZE, enemy.getPosition().z / CELL_SIZE),
            glm::ivec2(player.getPosition().x / CELL_SIZE, player.getPosition().z / CELL_SIZE),
            grid
        );

        //if (path.empty()) {
        //    std::cerr << "No path found" << std::endl;
        //}
        //else {
        //    std::cout << "Path found: ";
        //    for (const auto& step : path) {
        //        std::cout << "(" << step.x << ", " << step.y << ") ";
        //    }
        //    std::cout << std::endl;
        //}

        moveEnemy(enemy, path, deltaTime);


        enemy.Draw(shader);

        shader.setVec3("material.ambient", groundMaterial.ambient);
        shader.setVec3("material.diffuse", groundMaterial.diffuse);
        shader.setVec3("material.specular", groundMaterial.specular);
        shader.setFloat("material.shininess", groundMaterial.shininess);


        glm::mat4 model3 = glm::mat4(1.0f);
        model3 = glm::translate(model3, glm::vec3(-4.5f, 0.0f, -7.5f));
        //model = glm::rotate(model, glm::radians(15.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        model3 = glm::scale(model3, glm::vec3(100.0f, 1.0f, 100.0f));
        shader.setMat4("model", model3);

        //        ground.Draw(shader);

        gridShader.use();
        gridShader.setMat4("view", view);
        gridShader.setMat4("proj", projection);

        cell.BindVAO();

        drawGrid(gridShader);

        glBindVertexArray(0);

        // Swap buffers and poll IO events

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    // Terminate GLFW
    glfwTerminate();
    return 0;
}
void processInput(GLFWwindow* window, bool isTabPressed, Player& player)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (!tabKeyPressed && isTabPressed)
    {
        if (glfwGetKey(window, GLFW_KEY_TAB) == GLFW_PRESS)
            camera.Mode = static_cast<CameraMode>((camera.Mode + 1) % MODE_COUNT);
    }

    if (controlCamera && camera.Mode == FLY)
    {
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
            camera.ProcessKeyboard(FORWARD, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
            camera.ProcessKeyboard(BACKWARD, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
            camera.ProcessKeyboard(LEFT, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
            camera.ProcessKeyboard(RIGHT, deltaTime);
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

    g_player->PlayerProcessMouseMovement(xOffset);
    if (controlCamera)
    {
        camera.ProcessMouseMovement(xOffset, yOffset);
        g_player->PlayerYaw = camera.Yaw;
        g_player->UpdatePlayerVectors();

    }
    

//    xOfst = xOffset;
}

void scroll_callback(GLFWwindow* window, double xOffset, double yOffset)
{
    camera.ProcessMouseScroll(static_cast<float>(yOffset));
}

void ShowLightControlWindow(DirLight& light)
{
    ImGui::Begin("Directional Light Control");

    // Light direction control
    ImGui::Text("Light Direction");
    ImGui::DragFloat3("Direction", (float*)&light.direction, dirLight.direction.x, dirLight.direction.y, dirLight.direction.z);

    // Ambient color picker
    ImGui::ColorEdit4("Ambient", (float*)&light.ambient);

    // Diffuse color picker
    ImGui::ColorEdit4("Diffuse", (float*)&light.diffuse);

    // Specular color picker
    ImGui::ColorEdit4("Specular", (float*)&light.specular);

    ImGui::End();
}

void ShowCameraControlWindow(Camera& cam)
{
    ImGui::Begin("Camera Control");

    std::string modeText = "";

    if (camera.Mode == FLY)
    {
        modeText = "Flycam";
 

        camera.UpdateCameraVectors();
    }
    else if (camera.Mode == PLAYER_FOLLOW)
        modeText = "Player Follow";
    else if (camera.Mode == ENEMY_FOLLOW)
        modeText = "Enemy Follow";

    ImGui::Text(modeText.c_str());

    ImGui::InputFloat3("Position", (float*)&cam.Position);

    ImGui::InputFloat("Pitch", (float*)&cam.Pitch);
    ImGui::InputFloat("Yaw", (float*)&cam.Yaw);
    ImGui::InputFloat("Zoom", (float*)&cam.Zoom);

    ImGui::End();
}

void handlePlayerMovement(GLFWwindow* window, Player& player, Camera& camera, float deltaTime)
{
    if (camera.Mode == PLAYER_FOLLOW)
    {
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        {
            player.PlayerYaw = camera.Yaw;
            player.UpdatePlayerVectors();
            player.PlayerProcessKeyboard(FORWARD, deltaTime);
        }
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
            player.PlayerProcessKeyboard(BACKWARD, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
            player.PlayerProcessKeyboard(LEFT, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
            player.PlayerProcessKeyboard(RIGHT, deltaTime);

    }
}
