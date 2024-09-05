#pragma once
#include "GameObject.h"
#include "OpenGL/Texture.h"

class Cube : public GameObject {
public:
    Cube(glm::vec3 pos, glm::vec3 scale, Shader* shdr, bool applySkinning, GameManager* gameMgr, float yaw = 0.0f)
        : GameObject(pos, scale, yaw, shdr, applySkinning, gameMgr)
    {

        ComputeAudioWorldTransform();
    }

    void LoadMesh();

    void drawObject(glm::mat4 viewMat, glm::mat4 proj) override;

    void ComputeAudioWorldTransform() override {};

    void CreateAndUploadVertexBuffer();

    void OnHit() override {};
    void OnMiss() override {};

private:

    float vertices[64] = {
        // Positions          // Normals           // Texture Coords
        -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f,  0.0f, 0.0f,  // 0 Back-bottom-left
         1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f,  1.0f, 0.0f,  // 1 Back-bottom-right
         1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f,  1.0f, 1.0f,  // 2 Back-top-right
        -1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f,  0.0f, 1.0f,  // 3 Back-top-left

        -1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f,  0.0f, 0.0f,  // 4 Front-bottom-left
         1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f,  1.0f, 0.0f,  // 5 Front-bottom-right
         1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f,  1.0f, 1.0f,  // 6 Front-top-right
        -1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f,  0.0f, 1.0f,  // 7 Front-top-left
    };

    GLuint indices[36] = {
        // Back face
        0, 1, 2,   2, 3, 0, 

        // Front face
        4, 5, 6,   6, 7, 4, 

        // Left face
        4, 7, 3,   3, 0, 4, 

        // Right face
        1, 5, 6,   6, 2, 1, 

        // Bottom face
        0, 1, 5,   5, 4, 0, 

        // Top face
        3, 2, 6,   6, 7, 3  
    };

    GLuint mVAO;
    GLuint mVBO;
    GLuint mEBO;

};