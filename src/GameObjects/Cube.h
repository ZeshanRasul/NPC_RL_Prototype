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
        LoadTexture("C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Assets/Textures/Wall/TCom_Scifi_Pattern_4K_albedo.png", &mTex);
		LoadTexture("C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Assets/Textures/Wall/TCom_Scifi_Pattern_4K_normal.png", &mNormal);
		LoadTexture("C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Assets/Textures/Wall/TCom_Scifi_Pattern_4K_metallic.png", &mMetallic);
		LoadTexture("C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Assets/Textures/Wall/TCom_Scifi_Pattern_4K_roughness.png", &mRoughness);
		LoadTexture("C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Assets/Textures/Wall/TCom_Scifi_Pattern_4K_ao.png", &mAO);

		bulletHitAC = new AudioComponent(this);

		ComputeAudioWorldTransform();
    }

    void LoadMesh();
    bool LoadTexture(std::string textureFilename, Texture* tex);

    void drawObject(glm::mat4 viewMat, glm::mat4 proj, bool shadowMap,glm::mat4 lightSpaceMat, GLuint shadowMapTexture, glm::vec3 camPos) override;

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

private:

    float vertices[192] = {
        // Positions           // Normals          // Texture Coords
        // Back face (z = -0.5)
        -0.5f, -0.5f, -0.5f,   0.0f,  0.0f, -1.0f,  0.0f, 0.0f,  // 0 Back-bottom-left
         0.5f, -0.5f, -0.5f,   0.0f,  0.0f, -1.0f,  1.0f, 0.0f,  // 1 Back-bottom-right
         0.5f,  0.5f, -0.5f,   0.0f,  0.0f, -1.0f,  1.0f, 1.0f,  // 2 Back-top-right
        -0.5f,  0.5f, -0.5f,   0.0f,  0.0f, -1.0f,  0.0f, 1.0f,  // 3 Back-top-left

        // Front face (z = 0.5)
        -0.5f, -0.5f,  0.5f,   0.0f,  0.0f,  1.0f,  0.0f, 0.0f,  // 4 Front-bottom-left
         0.5f, -0.5f,  0.5f,   0.0f,  0.0f,  1.0f,  1.0f, 0.0f,  // 5 Front-bottom-right
         0.5f,  0.5f,  0.5f,   0.0f,  0.0f,  1.0f,  1.0f, 1.0f,  // 6 Front-top-right
        -0.5f,  0.5f,  0.5f,   0.0f,  0.0f,  1.0f,  0.0f, 1.0f,  // 7 Front-top-left

        // Left face (x = -0.5)
        -0.5f,  0.5f,  0.5f,  -1.0f,  0.0f,  0.0f,  1.0f, 0.0f,  // 8 Left-top-front
        -0.5f,  0.5f, -0.5f,  -1.0f,  0.0f,  0.0f,  1.0f, 1.0f,  // 9 Left-top-back
        -0.5f, -0.5f, -0.5f,  -1.0f,  0.0f,  0.0f,  0.0f, 1.0f,  // 10 Left-bottom-back
        -0.5f, -0.5f,  0.5f,  -1.0f,  0.0f,  0.0f,  0.0f, 0.0f,  // 11 Left-bottom-front

        // Right face (x = 0.5)
         0.5f,  0.5f,  0.5f,   1.0f,  0.0f,  0.0f,  1.0f, 0.0f,  // 12 Right-top-front
         0.5f,  0.5f, -0.5f,   1.0f,  0.0f,  0.0f,  1.0f, 1.0f,  // 13 Right-top-back
         0.5f, -0.5f, -0.5f,   1.0f,  0.0f,  0.0f,  0.0f, 1.0f,  // 14 Right-bottom-back
         0.5f, -0.5f,  0.5f,   1.0f,  0.0f,  0.0f,  0.0f, 0.0f,  // 15 Right-bottom-front

         // Bottom face (y = -0.5)
         -0.5f, -0.5f, -0.5f,   0.0f, -1.0f,  0.0f,  0.0f, 1.0f,  // 16 Bottom-back-left
          0.5f, -0.5f, -0.5f,   0.0f, -1.0f,  0.0f,  1.0f, 1.0f,  // 17 Bottom-back-right
          0.5f, -0.5f,  0.5f,   0.0f, -1.0f,  0.0f,  1.0f, 0.0f,  // 18 Bottom-front-right
         -0.5f, -0.5f,  0.5f,   0.0f, -1.0f,  0.0f,  0.0f, 0.0f,  // 19 Bottom-front-left

         // Top face (y = 0.5)
         -0.5f,  0.5f, -0.5f,   0.0f,  1.0f,  0.0f,  0.0f, 1.0f,  // 20 Top-back-left
          0.5f,  0.5f, -0.5f,   0.0f,  1.0f,  0.0f,  1.0f, 1.0f,  // 21 Top-back-right
          0.5f,  0.5f,  0.5f,   0.0f,  1.0f,  0.0f,  1.0f, 0.0f,  // 22 Top-front-right
         -0.5f,  0.5f,  0.5f,   0.0f,  1.0f,  0.0f,  0.0f, 0.0f   // 23 Top-front-left
    };

    GLuint indices[36] = {
        // Back face
        0, 1, 2,    2, 3, 0,    

        // Front face
        4, 5, 6,    6, 7, 4,    

        // Left face
        8, 9, 10,   10, 11, 8,  

        // Right face
        12, 13, 14, 14, 15, 12, 

        // Bottom face
        16, 17, 18, 18, 19, 16, 

        // Top face
        20, 21, 22, 22, 23, 20  
    };

    GLuint mVAO;
    GLuint mVBO;
    GLuint mEBO;
    
    Texture mNormal{};
    Texture mMetallic{};
    Texture mRoughness{};
    Texture mAO{};

    AABB* aabb;
    Shader* aabbShader;
    glm::vec3 aabbColor = glm::vec3(0.0f, 0.0f, 1.0f);

    AudioComponent* bulletHitAC;
};