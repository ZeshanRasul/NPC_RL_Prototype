#pragma once
#include "GameObject.h"
#include "OpenGL/Texture.h"

class Line : public GameObject
{
public:
	Line(glm::vec3 pos, glm::vec3 scale, Shader* shdr, Shader* shadowMapShader, bool applySkinning,
	     GameManager* gameMgr, float yaw = 0.0f)
		: GameObject(pos, scale, yaw, shdr, shadowMapShader, applySkinning, gameMgr)
	{
		ComputeAudioWorldTransform();
	}

	void LoadMesh();

	void DrawObject(glm::mat4 viewMat, glm::mat4 proj, bool shadowMap, glm::mat4 lightSpaceMat, GLuint shadowMapTexture,
	                glm::vec3 camPos) override
	{
	};

	void DrawLine(glm::mat4 viewMat, glm::mat4 proj, glm::vec3 lineColor, glm::mat4 lightSpaceMat,
	              GLuint shadowMapTexture, bool shadowMap, float lifetime);

	void ComputeAudioWorldTransform() override;

	void CreateAndUploadVertexBuffer();

	void UpdateVertexBuffer(glm::vec3 rayOrigin, glm::vec3 rayEnd);

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

private:
	float m_vertices[6] = {
		0.0f, 0.0f, 0.0f, // Origin
		1.0f, 1.0f, 1.0f // End
	};

	GLuint m_vao;
	GLuint m_vbo;
	GLuint m_ebo;
};
