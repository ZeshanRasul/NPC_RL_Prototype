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
#include "Components/Component.h"
#include "Logger.h"

class GameObject {
public:
    GameObject(glm::vec3 pos, glm::vec3 scale, float yaw, Shader* shdr, bool applySkinning, class GameManager* gameMgr)
        : position(pos), scale(scale), yaw(yaw), shader(shdr), toSkin(applySkinning), mGameManager(gameMgr)
    {
        size_t uniformMatrixBufferSize = 3 * sizeof(glm::mat4);
        mUniformBuffer.init(uniformMatrixBufferSize);
        Logger::log(1, "%s: matrix uniform buffer (size %i bytes) successfully created\n", __FUNCTION__, uniformMatrixBufferSize);
    }

    bool isSkinned() const { return toSkin; }

    virtual void Draw(glm::mat4 viewMat, glm::mat4 proj) {
        shader->use();
        drawObject(viewMat, proj);
    }

    virtual Shader* GetShader() const { return shader; }

    virtual void AddComponent(Component* component) {
        // Find the insertion point in the sorted vector
        // (The first element with a order higher than me)
        int myOrder = component->GetUpdateOrder();
        auto iter = mComponents.begin();
        for (;
            iter != mComponents.end();
            ++iter)
        {
            if (myOrder < (*iter)->GetUpdateOrder())
            {
                break;
            }
        }

        // Inserts element before position of iterator
        mComponents.insert(iter, component);
    }
    ;
    virtual void RemoveComponent(Component* component) {
        auto iter = std::find(mComponents.begin(), mComponents.end(), component);
        if (iter != mComponents.end())
        {
            mComponents.erase(iter);
        }
    };

    virtual void UpdateComponents(float deltaTime)
    {
        for (auto comp : mComponents)
        {
            comp->Update(deltaTime);
        }
    }

    virtual void ComputeAudioWorldTransform() {};

	GameManager* GetGameManager() const { return mGameManager; }

    glm::mat4 GetAudioWorldTransform() const { return audioWorldTransform; }

	virtual void OnHit() {};

    virtual void OnMiss() {};

    std::shared_ptr<GltfModel> model = nullptr;;
    float yaw;

    glm::vec3 position;

    bool isDestroyed = false;

protected:
    virtual void drawObject(glm::mat4 viewMat, glm::mat4 proj) = 0;

    glm::vec3 scale;
    bool mRecomputeWorldTransform = true;
    glm::mat4 audioWorldTransform;

    bool toSkin;
    Shader* shader = nullptr;
    RenderData renderData;

    class GameManager* mGameManager = nullptr;

    UniformBuffer mUniformBuffer{};

    std::vector<Component*> mComponents;
};
