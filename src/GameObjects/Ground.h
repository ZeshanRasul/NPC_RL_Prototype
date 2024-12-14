#pragma once

#include "GameObject.h"

class Ground : public GameObject
{
public:
	Ground(glm::vec3 pos, glm::vec3 scale, Shader* shdr, Shader* shadowMapShader, bool applySkinning,
	       GameManager* gameMgr, float yaw = 0.0f)
		: GameObject(pos, scale, yaw, shdr, shadowMapShader, applySkinning, gameMgr)
	{
		//        m_model.LoadModel("C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Models/GrassBase/GrassBase.obj");
		ComputeAudioWorldTransform();
	}

	void DrawObject(glm::mat4 viewMat, glm::mat4 proj, bool shadowMap, glm::mat4 lightSpaceMat, GLuint shadowMapTexture,
	                glm::vec3 camPos) override;

	void ComputeAudioWorldTransform() override;

	void OnHit() override
	{
	};

	void OnMiss() override
	{
	};

	void HasDealtDamage() override
	{
	};

	void HasKilledPlayer() override
	{
	};
};
