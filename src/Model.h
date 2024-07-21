#pragma once

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "Mesh.h"

unsigned int TextureFromFile(const char* path, const std::string& directory, bool gamma = false);

class Model {
public:
	std::vector<Texture> texturesLoaded;

	Model();

	void LoadModel(std::string path);
	void Draw(ShaderOld& shader);

private:
	std::vector<Mesh> meshes;
	std::string directory;

	void processNode(aiNode* node, const aiScene* scene);
	Mesh processMesh(aiMesh* mesh, const aiScene* scene);
	std::vector<Texture> loadMaterialTextures(aiMaterial* mat, aiTextureType type, std::string typeName);
};