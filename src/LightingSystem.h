#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "src/OpenGL/RenderData.h"

struct LightingSystem {
    DirLight dirLight = {
        glm::vec3(-0.5f, -1.0f, -0.3f),
        glm::vec3(1.0f),
        glm::vec3(0.8f),
        glm::vec3(0.8f, 0.9f, 1.0f)
    };
    glm::vec3 pbrColor = glm::vec3(300.0f, 300.0f, 300.0f);

    // Shadow map orthographic frustum (tunable via ImGui)
    float orthoLeft   = -90.0f;
    float orthoRight  =  90.0f;
    float orthoBottom = -90.0f;
    float orthoTop    =  90.0f;
    float nearPlane   =   1.0f;
    float farPlane    = 300.0f;

    // Derived — call UpdateLightSpaceMatrix() after changing direction or ortho params
    glm::mat4 view       = glm::mat4(1.0f);
    glm::mat4 projection = glm::mat4(1.0f);
    glm::mat4 matrix     = glm::mat4(1.0f);

    void UpdateLightSpaceMatrix()
    {
        glm::vec3 sceneCenter  = glm::vec3(500.0f / 2.0f, 0.0f, 500.0f / 2.0f);
        glm::vec3 lightDir     = glm::normalize(dirLight.m_direction);
        float     sceneDiag    = glm::sqrt(500.0f * 500.0f + 500.0f * 500.0f);

        projection = glm::ortho(orthoLeft, orthoRight, orthoBottom, orthoTop, nearPlane, farPlane);
        glm::vec3 lightPos = sceneCenter - lightDir * sceneDiag;
        view   = glm::lookAt(lightPos, sceneCenter, glm::vec3(0.0f, -1.0f, 0.0f));
        matrix = projection * view;
    }
};
