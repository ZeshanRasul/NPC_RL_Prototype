#pragma once

#include "GameObject.h"

class Ground : public GameObject {
public:
	Ground(glm::vec3 pos, glm::vec3 scale, Shader* shdr, Shader* shadowMapShader, bool applySkinning, GameManager* gameMgr, float yaw = 0.0f)
		: GameObject(pos, scale, yaw, shdr, shadowMapShader, applySkinning, gameMgr)
	{
		mapModel = new tinygltf::Model;

		std::string modelFilename = "C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Assets/Models/Turret_Base/Final/Final/turret_fixed.gltf";


		tinygltf::TinyGLTF gltfLoader;
		std::string loaderErrors;
		std::string loaderWarnings;
		bool result = false;

		result = gltfLoader.LoadASCIIFromFile(mapModel, &loaderErrors, &loaderWarnings,
			modelFilename);

		if (!loaderWarnings.empty()) {
			Logger::log(1, "%s: warnings while loading glTF model:\n%s\n", __FUNCTION__,
				loaderWarnings.c_str());
		}

		if (!loaderErrors.empty()) {
			Logger::log(1, "%s: errors while loading glTF model:\n%s\n", __FUNCTION__,
				loaderErrors.c_str());
		}

		if (!result) {
			Logger::log(1, "%s error: could not load file '%s'\n", __FUNCTION__,
				modelFilename.c_str());
		}

		setupGLTFMeshes(mapModel);

		for (int texID : loadGLTFTextures(mapModel))
			glTextures.push_back(texID);

		mTex.loadTexture("C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Assets/Models/Turret_Base/Final/Final/Atlas_00001.png", false);

		//model->loadModelNoAnim(modelFilename);
		//model->uploadVertexBuffersNoAnimations();

		ComputeAudioWorldTransform();

	}

	void SetPosition(const glm::vec3& pos) { position = pos; }
	void SetScale(const glm::vec3& newScale) { scale = newScale; }
	glm::vec3 GetPosition() { return position; }
	glm::vec3 GetScale() { return scale; }


	void drawObject(glm::mat4 viewMat, glm::mat4 proj, bool shadowMap, glm::mat4 lightSpaceMat, GLuint shadowMapTexture, glm::vec3 camPos) override;

	void ComputeAudioWorldTransform() override;

	void OnHit() override {};
	void OnMiss() override {};

	void HasDealtDamage() override {};
	void HasKilledPlayer() override {};
	Texture mTex;
	tinygltf::Model* mapModel;

	struct GLTFPrimitive {
		GLuint vao;
		GLuint indexBuffer;
		GLsizei indexCount;
		GLsizei vertexCount;
		GLenum indexType;
		GLenum mode;
		int material;
	};

	struct GLTFMesh {
		std::vector<GLTFPrimitive> primitives;
	};

	std::vector<GLTFMesh> meshData;

	void setupGLTFMeshes(tinygltf::Model* model) {
		meshData.resize(model->meshes.size());

		for (size_t meshIndex = 0; meshIndex < model->meshes.size(); ++meshIndex) {
			const tinygltf::Mesh& mesh = model->meshes[meshIndex];
			GLTFMesh gltfMesh;

			for (size_t primIndex = 0; primIndex < mesh.primitives.size(); ++primIndex) {
				const tinygltf::Primitive& primitive = mesh.primitives[primIndex];
				GLTFPrimitive gltfPrim = {};
				gltfPrim.mode = primitive.mode; // usually GL_TRIANGLES

				gltfPrim.material = primitive.material;

				// --- Create VAO ---
				glGenVertexArrays(1, &gltfPrim.vao);
				glBindVertexArray(gltfPrim.vao);

				// --- Upload vertex attributes ---
				for (const auto& attrib : primitive.attributes) {
					const std::string& attribName = attrib.first; // "POSITION", "NORMAL", "TEXCOORD_0", etc.
					int accessorIndex = attrib.second;
					const tinygltf::Accessor& accessor = model->accessors[accessorIndex];
					const tinygltf::BufferView& bufferView = model->bufferViews[accessor.bufferView];
					const tinygltf::Buffer& buffer = model->buffers[bufferView.buffer];

					GLuint vbo;
					glGenBuffers(1, &vbo);
					glBindBuffer(GL_ARRAY_BUFFER, vbo);

					const void* dataPtr = &buffer.data[accessor.byteOffset + bufferView.byteOffset];
					size_t dataSize = accessor.count * tinygltf::GetNumComponentsInType(accessor.type) * tinygltf::GetComponentSizeInBytes(accessor.componentType);

					glBufferData(GL_ARRAY_BUFFER, dataSize, dataPtr, GL_STATIC_DRAW);

					// Determine attribute layout location (you must match your shader locations)
					GLint location = -1;
					if (attribName == "POSITION") location = 0;
					else if (attribName == "NORMAL") location = 1;
					else if (attribName == "TEXCOORD_0") location = 2;
					// Add JOINTS_0, WEIGHTS_0 etc. if needed

					if (location >= 0) {
						GLint numComponents = tinygltf::GetNumComponentsInType(accessor.type); // e.g. VEC3 -> 3
						GLenum glType = accessor.componentType; // GL_FLOAT, GL_UNSIGNED_SHORT, etc.

						glEnableVertexAttribArray(location);
						glVertexAttribPointer(location, numComponents, glType,
							accessor.normalized ? GL_TRUE : GL_FALSE,
							bufferView.byteStride ? bufferView.byteStride : 0,
							(const void*)0);
					}

					glBindBuffer(GL_ARRAY_BUFFER, 0);
				}

				// --- Index buffer (if present) ---
				if (primitive.indices >= 0) {
					const tinygltf::Accessor& indexAccessor = model->accessors[primitive.indices];
					const tinygltf::BufferView& bufferView = model->bufferViews[indexAccessor.bufferView];
					const tinygltf::Buffer& buffer = model->buffers[bufferView.buffer];

					glGenBuffers(1, &gltfPrim.indexBuffer);
					glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gltfPrim.indexBuffer);

					const void* dataPtr = &buffer.data[indexAccessor.byteOffset + bufferView.byteOffset];
					size_t dataSize = indexAccessor.count * tinygltf::GetComponentSizeInBytes(indexAccessor.componentType);

					glBufferData(GL_ELEMENT_ARRAY_BUFFER, dataSize, dataPtr, GL_STATIC_DRAW);

					gltfPrim.indexCount = static_cast<GLsizei>(indexAccessor.count);
					gltfPrim.indexType = indexAccessor.componentType;
				}
				else {
					gltfPrim.indexBuffer = 0;
					// Need POSITION accessor to know vertex count
					int posAccessorIndex = primitive.attributes.at("POSITION");
					gltfPrim.vertexCount = static_cast<GLsizei>(model->accessors[posAccessorIndex].count);
				}

				glBindVertexArray(0);

				gltfMesh.primitives.push_back(gltfPrim);
			}

			meshData[meshIndex] = gltfMesh;
		}
	}

	std::vector<GLuint> loadGLTFTextures(tinygltf::Model* model) {
		std::vector<GLuint> textureIDs(model->images.size(), 0);

		for (size_t i = 0; i < model->images.size(); ++i) {
			const tinygltf::Image& image = model->images[i];

			GLuint texID;
			glGenTextures(1, &texID);
			glBindTexture(GL_TEXTURE_2D, texID);

			GLenum format = GL_RGBA;
			if (image.component == 1) format = GL_RED;
			else if (image.component == 2) format = GL_RG;
			else if (image.component == 3) format = GL_RGB;
			else if (image.component == 4) format = GL_RGBA;

			GLenum type = (image.bits == 16) ? GL_UNSIGNED_SHORT : GL_UNSIGNED_BYTE;

			glTexImage2D(GL_TEXTURE_2D,
				0,
				format,
				image.width,
				image.height,
				0,
				format,
				type,
				image.image.data());

			glGenerateMipmap(GL_TEXTURE_2D);

			// Set default sampler parameters
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

			textureIDs[i] = texID;
		}

		glBindTexture(GL_TEXTURE_2D, 0);
		return textureIDs;
	}

	std::vector<GLuint> glTextures;

	void drawGLTFModel() {
		int texIndex = 0;
		for (size_t meshIndex = 0; meshIndex < meshData.size(); ++meshIndex) {
			for (size_t primIndex = 0; primIndex < meshData[meshIndex].primitives.size(); ++primIndex) {
				const GLTFPrimitive& prim = meshData[meshIndex].primitives[primIndex];

				

				int matIndex = prim.material;
				const tinygltf::Material& mat = mapModel->materials[texIndex];

				if (mat.pbrMetallicRoughness.baseColorTexture.index >= 0)
				{
					texIndex = mat.pbrMetallicRoughness.baseColorTexture.index;
					
				}
			
				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, mTex.getTexID());
				shader->setInt("tex", 0);

				glBindVertexArray(prim.vao);

				if (prim.indexBuffer) {
					glDrawElements(prim.mode, prim.indexCount, prim.indexType, 0);
				}
				else {
					glDrawArrays(prim.mode, 0, prim.vertexCount);
				}

				glBindVertexArray(0);

			}
			texIndex += 1;
		}
	}

	static const void* getDataPointer(tinygltf::Model* model, const tinygltf::Accessor& accessor) {
		const tinygltf::BufferView& bufferView = model->bufferViews[accessor.bufferView];
		const tinygltf::Buffer& buffer = model->buffers[bufferView.buffer];
		return &buffer.data[accessor.byteOffset + bufferView.byteOffset];
	}


};