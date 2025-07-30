#pragma once
#include "GameObject.h"
#include "OpenGL/Texture.h"
#include "Physics/AABB.h"
#include "Components/AudioComponent.h"

class Cube : public GameObject {
public:
	Cube(glm::vec3 pos, glm::vec3 scale, Shader* shdr, Shader* shadowMapShader, bool applySkinning, GameManager* gameMgr, std::string texFilename, float yaw = 0.0f)
		: GameObject(pos, scale, yaw, shdr, shadowMapShader, applySkinning, gameMgr)
	{
		LoadTexture("C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Assets/Textures/Wall/TCom_SciFiPanels09_4K_albedo.png", &mTex);
		LoadTexture("C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Assets/Textures/Wall/TCom_SciFiPanels09_4K_normal.png", &mNormal);
		LoadTexture("C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Assets/Textures/Wall/TCom_SciFiPanels09_4K_metallic.png", &mMetallic);
		LoadTexture("C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Assets/Textures/Wall/TCom_SciFiPanels09_4K_roughness.png", &mRoughness);
		LoadTexture("C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Assets/Textures/Wall/TCom_SciFiPanels09_4K_ao.png", &mAO);
		LoadTexture("C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Assets/Textures/Wall/TCom_SciFiPanels09_4K_emissive.png", &mEmissive);

		bulletHitAC = new AudioComponent(this);

		ComputeAudioWorldTransform();
	}

	void LoadMesh();
	bool LoadTexture(std::string textureFilename, Texture* tex);

	void drawObject(glm::mat4 viewMat, glm::mat4 proj, bool shadowMap, glm::mat4 lightSpaceMat, GLuint shadowMapTexture, glm::vec3 camPos) override;

	void ComputeAudioWorldTransform() override {

		if (mRecomputeWorldTransform)
		{
			mRecomputeWorldTransform = false;
			glm::mat4 worldTransform = glm::mat4(1.0f);
			// Scale, then rotate, then translate
			audioWorldTransform = glm::translate(worldTransform, position);
			//	audioWorldTransform = glm::rotate(worldTransform, glm::radians(-yaw + 180.0f), glm::vec3(0.0f, 1.0f, 0.0f));
			audioWorldTransform = glm::scale(worldTransform, scale);

			// Inform components world transform updated
			for (auto comp : mComponents)
			{
				comp->OnUpdateWorldTransform();
			}
		}

	};

	void CreateAndUploadVertexBuffer();

	void OnHit() override {
		Logger::log(1, "Cover was hit!\n", __FUNCTION__);
		setAABBColor(glm::vec3(1.0f, 0.0f, 1.0f));
		/*takeDamageAC->PlayEvent("event:/PlayerTakeDamage");*/
		bulletHitAC->PlayEvent("event:/Metal_hit");
	};
	void OnMiss() override {
		setAABBColor(glm::vec3(1.0f, 1.0f, 1.0f));
	};

	void updateAABB() {
		glm::mat4 modelMatrix = glm::translate(glm::mat4(1.0f), position) *
			glm::rotate(glm::mat4(1.0f), glm::radians(yaw), glm::vec3(0.0f, 1.0f, 0.0f)) *
			glm::scale(glm::mat4(1.0f), scale);
		aabb->update(modelMatrix);
	};

	void SetUpAABB();

	AABB* GetAABB() const { return aabb; }

	void setAABBColor(glm::vec3 color) { aabbColor = color; }

	void SetAABBShader(Shader* shdr) { aabbShader = shdr; }

	void HasDealtDamage() override {};
	void HasKilledPlayer() override {};

	std::vector<glm::vec3> GetPositionVertices()
	{
		for (int i = 0; i < 192; i = i + 8)
		{
			glm::mat4 modelMatrix = glm::translate(glm::mat4(1.0f), position) *
				glm::rotate(glm::mat4(1.0f), glm::radians(yaw), glm::vec3(0.0f, 1.0f, 0.0f)) *
				glm::scale(glm::mat4(1.0f), scale);

			posVertices.push_back(glm::vec3(modelMatrix * glm::vec4(vertices[i], vertices[i + 1], vertices[i + 2], 1.0f)));
		}
		return posVertices;
	};

	GLuint indices[36] = {
		// Back face
		0, 1, 2,  2, 3, 0,

		// Front face
		4, 5, 6,  6, 7, 4,

		// Left face
		8, 9, 10, 10, 11, 8,

		// Right face
		12, 13, 14, 14, 15, 12,

		// Bottom face
		16, 17, 18, 18, 19, 16,

		// Top face
		20, 21, 22, 22, 23, 20
	};


private:
	std::vector<glm::vec3> posVertices;


	float vertices[192] = {
		// Position          // Normal           // UV
		// Back face (z = -0.5)
		-0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 0.0f,  // 0
		 0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 0.0f,  // 1
		 0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 1.0f,  // 2
		-0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 1.0f,  // 3

		// Front face (z = 0.5)
		-0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f, 0.0f,  // 4
		 0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f, 0.0f,  // 5
		 0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f, 1.0f,  // 6
		-0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f, 1.0f,  // 7

		// Left face (x = -0.5)
		-0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 0.0f,  // 8
		-0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 0.0f,  // 9
		-0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 1.0f,  // 10
		-0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 1.0f,  // 11

		// Right face (x = 0.5)
		 0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 0.0f,  // 12
		 0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f,  // 13
		 0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f,  // 14
		 0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 1.0f,  // 15

		 // Bottom face (y = -0.5)
		 -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 1.0f,  // 16
		  0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 1.0f,  // 17
		  0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 0.0f,  // 18
		 -0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 0.0f,  // 19

		 // Top face (y = 0.5)
		 -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 1.0f,  // 20
		  0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 1.0f,  // 21
		  0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 0.0f,  // 22
		 -0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 0.0f   // 23
	};


	GLuint mVAO;
	GLuint mVBO;
	GLuint mEBO;

	Texture mNormal{};
	Texture mMetallic{};
	Texture mRoughness{};
	Texture mAO{};
	Texture mEmissive{};

	AABB* aabb;
	Shader* aabbShader;
	glm::vec3 aabbColor = glm::vec3(0.0f, 0.0f, 1.0f);

	AudioComponent* bulletHitAC;
};