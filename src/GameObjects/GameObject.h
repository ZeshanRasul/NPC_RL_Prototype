#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "../Shader.h"
#include "../Primitives.h"
#include "../Model.h"

class GameObject {
public:
    GameObject(glm::vec3 pos, glm::vec3 scale, glm::vec3 color)
        : position(pos), scale(scale), color(color) {}

    virtual void Draw(Shader& shader) = 0;

protected:
    glm::vec3 position;
    glm::vec3 scale;
    glm::vec3 color;
    Model model;
};
