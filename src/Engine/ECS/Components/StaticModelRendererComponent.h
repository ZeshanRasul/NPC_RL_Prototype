#pragma once
#include "tinygltf/tiny_gltf.h"

class Shader;

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

struct StaticMeshComponent
{
	tinygltf::Model* model{ nullptr };
	std::vector<GLTFMesh> meshData;

	std::vector<GLuint> glTextures;

	Shader* shader{ nullptr };
	Shader* shadowShader{ nullptr };

	bool visible{ true };
	bool castsShadows{ true };
	uint32_t layer{ 0 };
};