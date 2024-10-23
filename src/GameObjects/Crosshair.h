#pragma once
#include "GameObject.h"
#include "OpenGL/Texture.h"

class Crosshair : public GameObject {
public:
    Crosshair(glm::vec3 pos, glm::vec3 scale, Shader* shdr, bool applySkinning, GameManager* gameMgr, float yaw = 0.0f)
        : GameObject(pos, scale, yaw, shdr, applySkinning, gameMgr)
    {      
        ComputeAudioWorldTransform();
    }

    void LoadMesh();

	bool LoadTexture(std::string textureFilename);

    void drawObject(glm::mat4 viewMat, glm::mat4 proj) override;

	glm::vec2 CalculateCrosshairPosition(glm::vec3 rayEnd, int screenWidth, int screenHeight,
        glm::mat4 proj, glm::mat4 view);
    void DrawCrosshair(glm::vec2 ndcPos);

    void ComputeAudioWorldTransform() override;

    void OnHit() override {};
    void OnMiss() override {};

	void HasDealtDamage() override {};
	void HasKilledPlayer() override {};

private:
    void CreateAndUploadVertexBuffer();
    void CreateAndUploadIndexBuffer();

    float vertices[20] = {
        // positions          // texture coords
        -0.1f,  0.1f, 0.0f,   0.0f, 1.0f, // top left
         0.1f,  0.1f, 0.0f,   1.0f, 1.0f, // top right
         0.1f, -0.1f, 0.0f,   1.0f, 0.0f, // bottom right
        -0.1f, -0.1f, 0.0f,   0.0f, 0.0f  // bottom left
    };

    unsigned int indices[6] = {
        0, 1, 2, // First triangle
        2, 3, 0  // Second triangle
    };

    GLuint mVAO;
    GLuint mVBO;
    GLuint mEBO;

    Texture mTexture{};
};