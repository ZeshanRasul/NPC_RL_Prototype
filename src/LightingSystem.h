#pragma once
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "src/OpenGL/RenderData.h"
#include "SceneLights.h"

struct LightingSystem {
    DirLight dirLight = {
        glm::vec3(-0.5f, -1.0f, -0.3f),
        glm::vec3(1.0f),
        glm::vec3(0.8f),
        glm::vec3(0.8f, 0.9f, 1.0f)
    };
    glm::vec3 pbrColor = glm::vec3(300.0f, 300.0f, 300.0f);

    // Shadow map orthographic frustum (tunable via ImGui)
    float orthoLeft   = -200.0f;
    float orthoRight  =  200.0f;
    float orthoBottom = -200.0f;
    float orthoTop    =  200.0f;
    float nearPlane   =   1.0f;
    float farPlane    =  800.0f;

    // Shadow scene framing — where the light looks and how far back it sits
    glm::vec3 sceneCenter  = glm::vec3(27.0f, -20.0f, 130.0f);
    float     lightDistance = 400.0f;

    // Dynamic scene lights (point + spot) — defaults match the old hardcoded shader values
    std::vector<ScenePointLight> pointLights = {
        { glm::vec3(  10.0f, 503.0f, -184.0f), glm::vec3(301.0f, 10.1f, 20.2f), 5.0f, "Point Light 1" },
        { glm::vec3(  16.0f, 339.0f, -155.0f), glm::vec3(301.0f, 10.6f, 20.2f), 5.0f, "Point Light 2" },
        { glm::vec3(-382.0f, 332.0f, -153.0f), glm::vec3(301.0f, 10.6f, 20.2f), 5.0f, "Point Light 3" },
        { glm::vec3( -83.0f, 334.0f,   -8.0f), glm::vec3(300.0f, 10.6f, 20.2f), 5.0f, "Point Light 4" },
    };
    std::vector<SceneSpotLight> spotLights = {
        { glm::vec3(  10.0f, 503.0f, -184.0f), glm::vec3(0,-1,0), glm::vec3(301.0f, 10.1f, 20.2f), 75.0f, 75.5f, 88.0f, "Spot Light 1" },
        { glm::vec3(  16.0f, 339.0f, -155.0f), glm::vec3(0,-1,0), glm::vec3(301.0f, 10.6f, 20.2f), 75.0f, 75.5f, 88.0f, "Spot Light 2" },
        { glm::vec3(-382.0f, 332.0f, -153.0f), glm::vec3(0,-1,0), glm::vec3(301.0f, 10.6f, 20.2f), 75.0f, 75.5f, 88.0f, "Spot Light 3" },
        { glm::vec3( -83.0f, 334.0f,   -8.0f), glm::vec3(0,-1,0), glm::vec3(300.0f, 10.6f, 20.2f), 75.0f, 75.5f, 88.0f, "Spot Light 4" },
    };

    // Derived — call UpdateLightSpaceMatrix() after changing any of the above
    glm::mat4 view       = glm::mat4(1.0f);
    glm::mat4 projection = glm::mat4(1.0f);
    glm::mat4 matrix     = glm::mat4(1.0f);

    void UpdateLightSpaceMatrix()
    {
        glm::vec3 lightDir = glm::normalize(dirLight.m_direction);
        glm::vec3 lightPos = sceneCenter - lightDir * lightDistance;

        // Choose an up vector that isn't parallel to the light direction
        glm::vec3 up = (glm::abs(glm::dot(lightDir, glm::vec3(0.0f, 1.0f, 0.0f))) < 0.99f)
                       ? glm::vec3(0.0f, 1.0f, 0.0f)
                       : glm::vec3(0.0f, 0.0f, 1.0f);

        projection = glm::ortho(orthoLeft, orthoRight, orthoBottom, orthoTop, nearPlane, farPlane);
        view   = glm::lookAt(lightPos, sceneCenter, up);
        matrix = projection * view;
    }
};
