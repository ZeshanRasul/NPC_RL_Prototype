#pragma once
#include <string>
#include <glm/glm.hpp>

static constexpr int MAX_POINT_LIGHTS = 16;
static constexpr int MAX_SPOT_LIGHTS  = 8;

struct ScenePointLight {
    glm::vec3   position  = glm::vec3(0.0f);
    glm::vec3   color     = glm::vec3(300.0f);
    float       intensity = 5.0f;
    std::string name      = "Point Light";
};

struct SceneSpotLight {
    glm::vec3   position   = glm::vec3(0.0f);
    glm::vec3   direction  = glm::vec3(0.0f, -1.0f, 0.0f);
    glm::vec3   color      = glm::vec3(300.0f);
    float       intensity  = 75.0f;
    float       innerAngle = 75.5f; // degrees, pre-cosined on upload
    float       outerAngle = 88.0f; // degrees, pre-cosined on upload
    std::string name       = "Spot Light";
};
