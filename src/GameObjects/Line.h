#pragma once
#include "GameObject.h"
#include "OpenGL/Texture.h"

class Line : public GameObject {
public:
    Line(glm::vec3 pos, glm::vec3 scale, Shader* shdr, bool applySkinning, GameManager* gameMgr, float yaw = 0.0f)
        : GameObject(pos, scale, yaw, shdr, applySkinning, gameMgr)
    {

        ComputeAudioWorldTransform();
    }

    void LoadMesh();

    void drawObject(glm::mat4 viewMat, glm::mat4 proj) override {};

    void DrawLine(glm::mat4 viewMat, glm::mat4 proj, glm::vec3 rayOrigin, glm::vec3 rayEnd, glm::vec3 lineColor);

    void ComputeAudioWorldTransform() override;

    void CreateAndUploadVertexBuffer();

	void UpdateVertexBuffer(glm::vec3 rayOrigin, glm::vec3 rayEnd);
private:

    float vertices[6] = {
        0.0f, 0.0f, 0.0f, // Origin
		1.0f, 1.0f, 1.0f  // End
    };

    GLuint mVAO;
    GLuint mVBO;
    GLuint mEBO;

};