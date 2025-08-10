#pragma once

#include "GameObject.h"
#include "Physics/AABB.h"

class Ground : public GameObject
{
public:
	Ground(glm::vec3 pos, glm::vec3 scale, Shader* shdr, Shader* shadowMapShader, bool applySkinning,
		GameManager* gameMgr, float yaw = 0.0f);

	void SetPosition(const glm::vec3& pos) { m_position = pos; }
	void SetScale(const glm::vec3& newScale) { m_scale = newScale; }
	glm::vec3 GetPosition() { return m_position; }
	glm::vec3 GetScale() { return m_scale; }


	void DrawObject(glm::mat4 viewMat, glm::mat4 proj, bool shadowMap, glm::mat4 lightSpaceMat, GLuint shadowMapTexture, glm::vec3 camPos) override;

	void ComputeAudioWorldTransform() override;

	void HasKilledPlayer() override
	{
	};

	void HasDealtDamage() override {};

	void SetAABBShader(Shader* aabbShdr) { m_aabbShader = aabbShdr; }

	void OnHit() override
	{
	};

	void OnMiss() override
	{
	};

	void DrawGLTFModel(glm::mat4 viewMat, glm::mat4 projMat, glm::vec3 camPos);


	Texture mTex;
	tinygltf::Model* mapModel;

	std::vector<AABB*> m_aabbs;
	Shader* m_aabbShader;;



	struct GLTFPrimitive {
		GLuint vao;
		GLuint indexBuffer;
		GLsizei indexCount;
		GLsizei vertexCount;
		GLenum indexType;
		GLenum mode;
		int material;
		std::vector<glm::vec3> verts;
		std::vector<unsigned int> indices;

	};

	struct GLTFMesh {
		std::vector<GLTFPrimitive> primitives;
	};

	std::vector<GLTFMesh> meshData;
	std::vector<std::vector<glm::vec3>> aabbMeshVertices;

	void SetupGLTFMeshes(tinygltf::Model* model);

	void SetUpAABB();

	std::vector<GLuint> glTextures;


	std::vector<GLuint> LoadGLTFTextures(tinygltf::Model* model);

	static const void* getDataPointer(tinygltf::Model* model, const tinygltf::Accessor& accessor) {
		const tinygltf::BufferView& bufferView = model->bufferViews[accessor.bufferView];
		const tinygltf::Buffer& buffer = model->buffers[bufferView.buffer];
		return &buffer.data[accessor.byteOffset + bufferView.byteOffset];
	}

};
