#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "src/OpenGL/Shader.h"
#include "../Primitives.h"
#include "../Model.h"
#include "Model/GltfModel.h"
#include "RenderData.h"

class GameObject {
public:
    GameObject(glm::vec3 pos, glm::vec3 scale, Shader* shdr, bool applySkinning)
        : position(pos), scale(scale), shader(shdr), toSkin(applySkinning) {}

    void ApplySkinning() const { model->applyVertexSkinning(toSkin); }

    bool isSkinned() const { return toSkin; }

    virtual void Draw() {
        shader->use();
        drawObject();
    }

    virtual Shader* GetShader() const { return shader; }

    std::shared_ptr<GltfModel> model = nullptr;;
protected:
    virtual void drawObject() = 0;

    glm::vec3 position;
    glm::vec3 scale;
    bool toSkin;
    Shader* shader = nullptr;
    RenderData renderData;
};
