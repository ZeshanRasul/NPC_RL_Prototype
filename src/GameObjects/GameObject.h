#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "src/OpenGL/Shader.h"
#include "../Primitives.h"
#include "../Model.h"
#include "Model/GltfModel.h"
#include "RenderData.h"
#include "UniformBuffer.h"
#include "Logger.h"

class GameObject {
public:
    GameObject(glm::vec3 pos, glm::vec3 scale, Shader* shdr, bool applySkinning)
        : position(pos), scale(scale), shader(shdr), toSkin(applySkinning) 
    {
        size_t uniformMatrixBufferSize = 3 * sizeof(glm::mat4);
        mUniformBuffer.init(uniformMatrixBufferSize);
        Logger::log(1, "%s: matrix uniform buffer (size %i bytes) successfully created\n", __FUNCTION__, uniformMatrixBufferSize);
    }

    void ApplySkinning() const { model->applyVertexSkinning(toSkin); }

    bool isSkinned() const { return toSkin; }

    virtual void Draw(glm::mat4 viewMat, glm::mat4 proj) {
        shader->use();
        drawObject(viewMat, proj);
    }

    virtual Shader* GetShader() const { return shader; }

    std::shared_ptr<GltfModel> model = nullptr;;
protected:
    virtual void drawObject(glm::mat4 viewMat, glm::mat4 proj) = 0;

    glm::vec3 position;
    glm::vec3 scale;
    bool toSkin;
    Shader* shader = nullptr;
    RenderData renderData;

    UniformBuffer mUniformBuffer{};
};
