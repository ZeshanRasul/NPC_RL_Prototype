#pragma once

#include "GameObject.h"

class Ground : public GameObject {
public:
	Ground(glm::vec3 pos, glm::vec3 scale, Shader* shdr, Shader* shadowMapShader, bool applySkinning, GameManager* gameMgr, float yaw = 0.0f)
		: GameObject(pos, scale, yaw, shdr, shadowMapShader, applySkinning, gameMgr)
	{
		mapModel = new tinygltf::Model;

		std::string modelFilename = "C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Assets/Models/New/Updated/Aviary final Model1.gltf";


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

		mTex.loadTexture("C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Assets/Models/New/Updated/Atlas_00001.png", false);

		//model->loadModelNoAnim(modelFilename);
		//model->uploadVertexBuffersNoAnimations();

		//ComputeAudioWorldTransform();

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
		std::vector<glm::vec3> verts;
		std::vector<unsigned int> indices;

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

					if (attribName == "POSITION") {
						int numPositionEntries = accessor.count;
						Logger::log(1, "%s: loaded %i vertices from glTF file\n", __FUNCTION__,
							numPositionEntries);

						// Extract vertices
						const float* positions = reinterpret_cast<const float*>(
							buffer.data.data() + bufferView.byteOffset + accessor.byteOffset);

						for (int i = 0; i < numPositionEntries; i++) 
						{
							gltfPrim.verts.push_back(glm::vec3(positions[i * 3 + 0], positions[i * 3 + 1] , positions[i * 3 + 2]));
							gltfPrim.vertexCount++;
						}
					}

					const void* dataPtr = &buffer.data[accessor.byteOffset + bufferView.byteOffset];
					size_t dataSize = accessor.count * tinygltf::GetNumComponentsInType(accessor.type) * tinygltf::GetComponentSizeInBytes(accessor.componentType);

					glBufferData(GL_ARRAY_BUFFER, dataSize, dataPtr, GL_STATIC_DRAW);

					// Determine attribute layout location (you must match your shader locations)
					GLint location = -1;
					if (attribName == "POSITION") location = 0;
					else if (attribName == "NORMAL") location = 1;
					else if (attribName == "TEXCOORD_0") location = 2;
					else if (attribName == "TEXCOORD_1") location = 3;
					else if (attribName == "TEXCOORD_2") location = 4;
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


				if (primitive.indices >= 0) {
					// Get the accessor, bufferview, and buffer for the index data
					const tinygltf::Accessor& indexAccessor = model->accessors[primitive.indices];
					const tinygltf::BufferView& bufferView = model->bufferViews[indexAccessor.bufferView];
					const tinygltf::Buffer& buffer = model->buffers[bufferView.buffer];

					

					// Pointer to the actual index data
					const unsigned char* dataPtr = buffer.data.data() + bufferView.byteOffset + indexAccessor.byteOffset;

					// Loop through and extract indices based on the component type
					for (size_t i = 0; i < indexAccessor.count; ++i) {
						switch (indexAccessor.componentType) {
						case TINYGLTF_PARAMETER_TYPE_UNSIGNED_BYTE: {
							gltfPrim.indices.push_back(static_cast<unsigned int>(reinterpret_cast<const uint8_t*>(dataPtr)[i]));
							break;
						}
						case TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT: {
							gltfPrim.indices.push_back(static_cast<unsigned int>(reinterpret_cast<const uint16_t*>(dataPtr)[i]));
							break;
						}
						case TINYGLTF_PARAMETER_TYPE_UNSIGNED_INT: {
							gltfPrim.indices.push_back(reinterpret_cast<const uint32_t*>(dataPtr)[i]);
							break;
						}
						default:
							Logger::log(1, " << indexAccessor.componentType, %zu", indexAccessor.componentType);
							break;
						}
					}
				}
				else {
					// No index buffer: assume the primitive is non-indexed (each vertex is used once)
					int posAccessorIndex = primitive.attributes.at("POSITION");
					const tinygltf::Accessor& posAccessor = model->accessors[posAccessorIndex];
					for (size_t i = 0; i < posAccessor.count; ++i) {
						gltfPrim.indices.push_back(static_cast<unsigned int>(i));
					}
				}

				if (!gltfPrim.indices.empty()) {
					glGenBuffers(1, &gltfPrim.indexBuffer);
					glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gltfPrim.indexBuffer);
					glBufferData(GL_ELEMENT_ARRAY_BUFFER,
						gltfPrim.indices.size() * sizeof(unsigned int),
						gltfPrim.indices.data(),
						GL_STATIC_DRAW);

					gltfPrim.indexCount = static_cast<GLsizei>(gltfPrim.indices.size());
					gltfPrim.indexType = GL_UNSIGNED_INT;
				} else {
					gltfPrim.indexBuffer = 0;
					gltfPrim.indexCount = 0;
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

	

	void drawGLTFModel(glm::mat4 viewMat, glm::mat4 projMat) {
		glDisable(GL_CULL_FACE);
		
		int texIndex = 0;
		for (size_t meshIndex = 0; meshIndex < meshData.size(); ++meshIndex) {
			for (size_t primIndex = 0; primIndex < meshData[meshIndex].primitives.size(); ++primIndex) {
				const GLTFPrimitive& prim = meshData[meshIndex].primitives[primIndex];


				shader->use();
				glm::mat4 modelMat = glm::mat4(1.0f);
				modelMat = glm::translate(modelMat, position);
				//modelMat = glm::rotate(modelMat, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
				modelMat = glm::scale(modelMat, scale);
				std::vector<glm::mat4> matrixData;
				matrixData.push_back(viewMat);
				matrixData.push_back(projMat);
				matrixData.push_back(modelMat);
				mUniformBuffer.uploadUboData(matrixData, 0);

				glBindVertexArray(prim.vao);
				int matIndex = prim.material;
				if (matIndex >= 0 && matIndex < mapModel->materials.size()) {
					const tinygltf::Material& mat = mapModel->materials[matIndex];
					if (mat.pbrMetallicRoughness.baseColorTexture.index >= 0) {
						texIndex = mat.pbrMetallicRoughness.baseColorTexture.index;
					}
					// You can add more checks for other texture types if needed

					if (texIndex >= 0 && texIndex < glTextures.size() && mat.pbrMetallicRoughness.baseColorTexture.index >= 0) {
						glActiveTexture(GL_TEXTURE0);
						glBindTexture(GL_TEXTURE_2D, glTextures[texIndex]);
						shader->setInt("tex", 0); // Use location 0 for GL_TEXTURE0
					}
					else {

						glActiveTexture(GL_TEXTURE0);
						glBindTexture(GL_TEXTURE_2D, mTex.getTexID());
						shader->setInt("tex", 0);
					}
				}
	


				//int matIndex = prim.material;
				//const tinygltf::Material& mat = mapModel->materials[texIndex];
				//
				//if (mat.pbrMetallicRoughness.baseColorTexture.index >= 0)
				//{
			//		texIndex = mat.pbrMetallicRoughness.baseColorTexture.index;
			//		
				//}
		


				if (prim.indexBuffer) {
					glDrawElements(GL_TRIANGLES, prim.indexCount, GL_UNSIGNED_INT , 0);
				}
				else {
					glDrawArrays(prim.mode, 0, prim.vertexCount);
				}

				glBindVertexArray(0);

			}

			//if (texIndex < glTextures.size())
			//	texIndex += 1;
		}
	}

	static const void* getDataPointer(tinygltf::Model* model, const tinygltf::Accessor& accessor) {
		const tinygltf::BufferView& bufferView = model->bufferViews[accessor.bufferView];
		const tinygltf::Buffer& buffer = model->buffers[bufferView.buffer];
		return &buffer.data[accessor.byteOffset + bufferView.byteOffset];
	}


};