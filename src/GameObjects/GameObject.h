#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "src/OpenGL/Shader.h"
#include "Model/GltfModel.h"
#include "RenderData.h"
#include "UniformBuffer.h"
#include "Texture.h"
#include "Components/Component.h"
#include "Logger.h"

class GameObject
{
public:
	GameObject(glm::vec3 pos, glm::vec3 scale, float yaw, Shader* shdr, Shader* shadowMapShader, bool applySkinning,
	           class GameManager* gameMgr)
		: m_yaw(yaw), m_position(pos), m_scale(scale), m_toSkin(applySkinning), m_shader(shdr), m_shadowShader(shadowMapShader),
		  m_gameManager(gameMgr)
	{
		size_t uniformMatrixBufferSize = 4 * sizeof(glm::mat4);
		m_uniformBuffer.Init(uniformMatrixBufferSize);
		Logger::Log(1, "%s: matrix uniform buffer (size %i bytes) successfully created\n", __FUNCTION__,
		            uniformMatrixBufferSize);
	}

	bool IsSkinned() const { return m_toSkin; }
	bool IsDestroyed() const { return m_isDestroyed; }

	float GetYaw() const { return m_yaw; }
	void SetIsDestroyed(bool newValue) { m_isDestroyed = newValue; }

	virtual void Draw(glm::mat4 viewMat, glm::mat4 proj, bool shadowMap, glm::mat4 lightSpaceMat,
	                  GLuint shadowMapTexture)
	{
		if (shadowMap)
		{
			m_shadowShader->Use();
		}
		else
		{
			m_shader->Use();
		}

		DrawObject(viewMat, proj, shadowMap, lightSpaceMat, shadowMapTexture);
	}

	Texture GetTexture() const { return m_tex; }

	virtual Shader* GetShader() const { return m_shader; }

	virtual Shader* GetShadowShader() const { return m_shadowShader; }

	virtual void AddComponent(Component* component)
	{
		// Find the insertion point in the sorted vector
		// (The first element with a order higher than me)
		int myOrder = component->GetUpdateOrder();
		auto iter = m_components.begin();
		for (;
			iter != m_components.end();
			++iter)
		{
			if (myOrder < (*iter)->GetUpdateOrder())
			{
				break;
			}
		}

		// Inserts element before m_position of iterator
		m_components.insert(iter, component);
	}
	;

	virtual void RemoveComponent(Component* component)
	{
		auto iter = std::find(m_components.begin(), m_components.end(), component);
		if (iter != m_components.end())
		{
			m_components.erase(iter);
		}
	};

	virtual void UpdateComponents(float deltaTime)
	{
		for (auto comp : m_components)
		{
			comp->Update(deltaTime);
		}
	}

	virtual void ComputeAudioWorldTransform() {};

	GameManager* GetGameManager() const { return m_gameManager; }

	glm::mat4 GetAudioWorldTransform() const { return m_audioWorldTransform; }

	virtual void OnHit() {};

	virtual void OnMiss() {};

	virtual void HasDealtDamage() = 0;
	virtual void HasKilledPlayer() = 0;

protected:
	virtual void DrawObject(glm::mat4 viewMat, glm::mat4 proj, bool shadowMap, glm::mat4 lightSpaceMatrix,
	                        GLuint shadowMapTexture, glm::vec3 camPos = glm::vec3(0.0f, 0.0f, 0.0f)) = 0;

	std::shared_ptr<GltfModel> m_model = nullptr;;
	Texture m_tex{};

	bool m_isEnemy = false;
	bool m_isDestroyed = false;

	glm::vec3 m_position;
	float m_yaw;

	glm::vec3 m_scale;
	bool m_recomputeWorldTransform = true;
	glm::mat4 m_audioWorldTransform;

	bool m_toSkin;
	Shader* m_shader = nullptr;
	Shader* m_shadowShader = nullptr;
	RenderData m_renderData;

	class GameManager* m_gameManager = nullptr;

	UniformBuffer m_uniformBuffer{};

	std::vector<Component*> m_components;
};
