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
	void SetPlaneShader(Shader* planeShdr) { planeShader = planeShdr; }

	void OnHit() override
	{
	};

	void OnMiss() override
	{
	};

	struct PlaneCollider
	{
		glm::vec3 point;
		glm::vec3 normal;
		float     d;         // plane equation: dot(n, X) + d = 0   (d = -dot(n, point))

		glm::vec3 center;    // centroid
		glm::vec3 tangent;   // unit vector on plane
		glm::vec3 bitangent; // unit vector on plane, orthonormal to tangent
		glm::vec2 halfSize;  // half-extent along tangent/bitangent
	};

	std::vector<PlaneCollider> planeColliders;

	struct DebugPlaneGL
	{
		GLuint vao = 0, vbo = 0;
		GLsizei countNo = 0;
	};

	std::vector<DebugPlaneGL> debugPlanes;

	void DrawGLTFModel(glm::mat4 viewMat, glm::mat4 projMat, glm::vec3 camPos);

	void LoadPlaneCollider(tinygltf::Model* model);
	void CreatePlaneColliders();

	static PlaneCollider BuildPlaneFromVerts(const std::vector<glm::vec3>& vertsWS)
	{
		PlaneCollider pc{};

		if (vertsWS.size() < 3) {
			pc.point = glm::vec3(0);
			pc.normal = glm::vec3(0, 1, 0);
			pc.d = 0.0f;
			pc.center = pc.point;
			pc.tangent = glm::vec3(1, 0, 0);
			pc.bitangent = glm::vec3(0, 0, 1);
			pc.halfSize = glm::vec2(0.5f);
			return pc;
		}

		const glm::vec3& p0 = vertsWS[0];
		const glm::vec3& p1 = vertsWS[1];
		const glm::vec3& p2 = vertsWS[2];

		glm::vec3 n = glm::normalize(glm::cross(p1 - p0, p2 - p0));

		// Centroid
		glm::vec3 c(0.0f);
		for (auto& v : vertsWS) c += v;
		c /= float(vertsWS.size());

		pc.point = c;
		pc.normal = n;
		pc.d = -glm::dot(n, c);
		pc.center = c;

		// Build an on-plane orthonormal basis (tangent/bitangent)
		glm::vec3 arbitrary = (std::abs(n.y) < 0.9f) ? glm::vec3(0, 1, 0) : glm::vec3(1, 0, 0);
		glm::vec3 t = glm::normalize(glm::cross(arbitrary, n));
		glm::vec3 b = glm::normalize(glm::cross(n, t));
		pc.tangent = t;
		pc.bitangent = b;

		// Project verts into this basis to get extents
		float minU = FLT_MAX, maxU = -FLT_MAX, minV = FLT_MAX, maxV = -FLT_MAX;
		for (auto& v : vertsWS) {
			glm::vec3 rel = v - c;
			float u = glm::dot(rel, t);
			float v2 = glm::dot(rel, b);
			minU = std::min(minU, u); maxU = std::max(maxU, u);
			minV = std::min(minV, v2); maxV = std::max(maxV, v2);
		}
		pc.halfSize = 0.5f * glm::vec2(maxU - minU, maxV - minV);


		return pc;
	}

	static DebugPlaneGL MakeDebugPlane(const PlaneCollider& pl)
	{
		DebugPlaneGL debugPlane{};
		glm::vec3 c = pl.center;
		glm::vec3 t = pl.tangent * pl.halfSize.x;
		glm::vec3 b = pl.bitangent * pl.halfSize.y;

		glm::vec3 p0 = c + t + b;
		glm::vec3 p1 = c - t + b;
		glm::vec3 p2 = c - t - b;
		glm::vec3 p3 = c + t - b;

		glm::vec3 verts[5] = { p0, p1, p2, p3, p0 };

		glGenVertexArrays(1, &debugPlane.vao);
		glGenBuffers(1, &debugPlane.vbo);
		glBindVertexArray(debugPlane.vao);
		glBindBuffer(GL_ARRAY_BUFFER, debugPlane.vbo);
		glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);

		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);

		glBindVertexArray(0);

		debugPlane.countNo = 5;

		return debugPlane;	
	}


	Texture mTex;
	tinygltf::Model* mapModel;
	tinygltf::Model* plane01Model;

	std::vector<AABB*> m_aabbs;
	Shader* m_aabbShader;
	Shader* planeShader;



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
	std::vector<GLTFMesh> planeData;
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
