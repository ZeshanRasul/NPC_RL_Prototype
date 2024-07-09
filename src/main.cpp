#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>

#include "Shader.h"
#include "Camera.h"

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow *window);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xOffset, double yOffset);

const unsigned int SCREEN_WIDTH = 800;
const unsigned int SCREEN_HEIGHT = 600;

float deltaTime = 0.0f;
float lastFrame = 0.0f;
float currentFrame = 0.0f;


Camera camera(glm::vec3(2.0f, 3.0f, 25.0f));
float lastX = SCREEN_WIDTH / 2.0f;
float lastY = SCREEN_HEIGHT / 2.0f;
bool firstMouse = true;

float vertices[] = {
    -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
     0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
     0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
     0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
    -0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
    -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,

    -0.5f, -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,
     0.5f, -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,
     0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,
     0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,
    -0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,
    -0.5f, -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,

    -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,
    -0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
    -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
    -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
    -0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,
    -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,

     0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,
     0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
     0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
     0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
     0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,
     0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,

    -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,
     0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,
     0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
     0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
    -0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
    -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,

    -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,
     0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,
     0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
     0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
    -0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
    -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f
};

float planeVertices[] = {
    // Positions          // Normals           // Texture Coords
    -0.5f, 0.0f, -0.5f,  0.0f, 1.0f, 0.0f,  0.0f, 0.0f,  // Bottom-left
     0.5f, 0.0f, -0.5f,  0.0f, 1.0f, 0.0f,  1.0f, 0.0f,  // Bottom-right
     0.5f, 0.0f,  0.5f,  0.0f, 1.0f, 0.0f,  1.0f, 1.0f,  // Top-right
    -0.5f, 0.0f,  0.5f,  0.0f, 1.0f, 0.0f,  0.0f, 1.0f   // Top-left
};

GLuint planeIndices[] = {
    0, 1, 2,  // First Triangle
    2, 3, 0   // Second Triangle
};


struct Material {
    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;
    float shininess;
};

struct DirLight {
    glm::vec3 direction;

    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;
};

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
    GLFWwindow* window = glfwCreateWindow(800, 600, "OpenGL Window", NULL, NULL);
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

    unsigned int VAO;
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    unsigned int VBO;
    glGenBuffers(1, &VBO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), &vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);

    unsigned int lightVAO;
    glGenVertexArrays(1, &lightVAO);
    glBindVertexArray(lightVAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);

    unsigned int planeVAO;
    unsigned int planeVBO;
    unsigned int EBO;

    // Generate and bind the VAO
    glGenVertexArrays(1, &planeVAO);
    glBindVertexArray(planeVAO);

    // Generate and bind the VBO
    glGenBuffers(1, &planeVBO);
    glBindBuffer(GL_ARRAY_BUFFER, planeVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(planeVertices), planeVertices, GL_STATIC_DRAW);

    glGenBuffers(1, &EBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(planeIndices), planeIndices, GL_STATIC_DRAW);


    // Define the vertex attributes
    // Position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*)0);
    glEnableVertexAttribArray(0);

    // Texture coordinate attribute
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
    glEnableVertexAttribArray(1);

    // Unbind the VAO
    glBindVertexArray(0);


    Shader shader("C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Shaders/vertex.glsl", "C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Shaders/fragment.glsl");

    Shader lightShader("C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Shaders/lightVertex.glsl", "C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Shaders/lightFragment.glsl");


    shader.use();

    glm::mat4 view;
    glm::mat4 projection;

    Material cubeMaterial = {
        glm::vec3(1.0f, 0.5f, 0.31f),
        glm::vec3(1.0f, 0.5f, 0.31f),
        glm::vec3(0.5f, 0.5f, 0.5f),
        32.0f
    };

    DirLight dirLight = {
            glm::vec3(-0.2f, -1.0f, -0.3f),

            glm::vec3(0.05f, 0.05f, 0.05f),
            glm::vec3(0.5f),
            glm::vec3(0.5f, 0.5f, 0.5f)
    };

    glm::vec3 pointLightPositions[] = {
        glm::vec3(0.7f,  0.2f,  2.0f),
        glm::vec3(2.3f, -3.3f, -4.0f),
        glm::vec3(-4.0f,  2.0f, -12.0f),
        glm::vec3(0.0f,  0.0f, -3.0f)
    };

    // Render loop
    while (!glfwWindowShouldClose(window))
    {
        currentFrame = (float)glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // Input
        processInput(window);

        view = camera.GetViewMatrix();

        projection = glm::perspective(glm::radians(camera.Zoom), (float)SCREEN_WIDTH / (float)SCREEN_HEIGHT, 0.1f, 100.0f);

        shader.setMat4("view", view);
        shader.setMat4("proj", projection);

        // Render
        glClearColor(0.2f, 0.3f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        shader.setVec3("material.ambient", cubeMaterial.ambient);
        shader.setVec3("material.diffuse", cubeMaterial.diffuse);
        shader.setVec3("material.specular", cubeMaterial.specular);
        shader.setFloat("material.shininess", cubeMaterial.shininess);

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


        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(-10.0f, 3.0f, 0.0f));
        //model = glm::rotate(model, glm::radians(15.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::scale(model, glm::vec3(1.0f, 3.0f, 1.0f));
        shader.setMat4("model", model);


        glBindVertexArray(VAO);


        glDrawArrays(GL_TRIANGLES, 0, 36);

        glBindVertexArray(0);

        shader.setVec3("material.ambient", cubeMaterial.ambient);
        shader.setVec3("material.diffuse", cubeMaterial.diffuse);
        shader.setVec3("material.specular", cubeMaterial.specular);
        shader.setFloat("material.shininess", cubeMaterial.shininess);


        glm::mat4 model2 = glm::mat4(1.0f);
        model2 = glm::translate(model2, glm::vec3(5.0f, 3.0f, 0.0f));
        //model = glm::rotate(model, glm::radians(15.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        model2 = glm::scale(model2, glm::vec3(1.0f, 3.0f, 1.0f));
        shader.setMat4("model", model2);

        glBindVertexArray(VAO);


        glDrawArrays(GL_TRIANGLES, 0, 36);

        glBindVertexArray(0);

        shader.setVec3("material.ambient", cubeMaterial.ambient);
        shader.setVec3("material.diffuse", cubeMaterial.diffuse);
        shader.setVec3("material.specular", cubeMaterial.specular);
        shader.setFloat("material.shininess", cubeMaterial.shininess);


        glm::mat4 model3 = glm::mat4(1.0f);
        model3 = glm::translate(model2, glm::vec3(-7.5f, -0.5f, -7.5f));
        //model = glm::rotate(model, glm::radians(15.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        model3 = glm::scale(model3, glm::vec3(50.0f, 1.0f, 50.0f));
        shader.setMat4("model", model3);

        glBindVertexArray(planeVAO);

        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

        glBindVertexArray(0);

        //lightShader.use();

        //lightShader.setMat4("view", view);
        //lightShader.setMat4("proj", projection);

        //for (unsigned int i = 0; i < 4; i++)
        //{
        //    glm::mat4 lightModel = glm::mat4(1.0f);
        //    lightModel = glm::translate(lightModel, pointLightPositions[i]);
        //    lightModel = glm::scale(lightModel, glm::vec3(0.2f));
        //    lightShader.setMat4("model", lightModel);

        //    lightShader.setVec3("lightColor", 0.8f, 0.8f, 0.8f);
        //    glBindVertexArray(lightVAO);

        //    glDrawArrays(GL_TRIANGLES, 0, 36);
        //}

        //glBindVertexArray(0);

        // Swap buffers and poll IO events
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Terminate GLFW
    glfwTerminate();
    return 0;
}
void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.ProcessKeyboard(RIGHT, deltaTime);
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}

void mouse_callback(GLFWwindow* window, double xPosIn, double yPosIn)
{
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

    camera.ProcessMouseMovement(xOffset, yOffset);
}

void scroll_callback(GLFWwindow* window, double xOffset, double yOffset)
{
    camera.ProcessMouseScroll(static_cast<float>(yOffset));
}