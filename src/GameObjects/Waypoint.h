#pragma once

#include "GameObject.h"

class Waypoint : public GameObject
{
public:
	Waypoint(glm::vec3 pos, glm::vec3 scale, Shader* shdr, Shader* shadowMapShader, bool applySkinning,
	         GameManager* gameMgr, float yaw = 0.0f)
		: GameObject(pos, scale, yaw, shdr, shadowMapShader, applySkinning, gameMgr)
	{
		m_model = std::make_shared<GltfModel>();

		std::string modelFilename =
			"C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Assets/Models/GLTF/MilitaryMale/MilitaryMale.gltf";
		std::string modelTextureFilename =
			"C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Assets/Models/GLTF/MilitaryMale/albedo.png";

		if (!m_model->loadModelNoAnim(modelFilename))
		{
			Logger::Log(1, "%s: loading glTF m_model '%s' failed\n", __FUNCTION__, modelFilename.c_str());
		}

		m_model->loadTexture(modelTextureFilename, false);

		m_model->uploadIndexBuffer();
		m_model->uploadVertexBuffersNoAnimations();
		Logger::Log(1, "%s: glTF m_model '%s' succesfully loaded\n", __FUNCTION__, modelFilename.c_str());


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
