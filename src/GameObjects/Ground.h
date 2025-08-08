#pragma once

#include "GameObject.h"

class Ground : public GameObject
{
public:
	Ground(glm::vec3 pos, glm::vec3 scale, Shader* shdr, Shader* shadowMapShader, bool applySkinning,
	       GameManager* gameMgr, float yaw = 0.0f)
		: GameObject(pos, scale, yaw, shdr, shadowMapShader, applySkinning, gameMgr)
	{
		mapModel = new tinygltf::Model;

		std::string modelFilename = "C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Assets/Models/New/Updated/Aviary final Model4.gltf";


		tinygltf::TinyGLTF gltfLoader;
		std::string loaderErrors;
		std::string loaderWarnings;
		bool result = false;

		result = gltfLoader.LoadASCIIFromFile(mapModel, &loaderErrors, &loaderWarnings,
			modelFilename);

		if (!loaderWarnings.empty()) {
			Logger::Log(1, "%s: warnings while loading glTF model:\n%s\n", __FUNCTION__,
				loaderWarnings.c_str());
		}

		if (!loaderErrors.empty()) {
			Logger::Log(1, "%s: errors while loading glTF model:\n%s\n", __FUNCTION__,
				loaderErrors.c_str());
		}

		if (!result) {
			Logger::Log(1, "%s error: could not load file '%s'\n", __FUNCTION__,
				modelFilename.c_str());
		}

		SetupGLTFMeshes(mapModel);

		for (int texID : LoadGLTFTextures(mapModel))
			glTextures.push_back(texID);

		mTex.LoadTexture("C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Assets/Models/New/Updated/Atlas_00001.png", false);

		//model->loadModelNoAnim(modelFilename);
		//model->uploadVertexBuffersNoAnimations();

		//ComputeAudioWorldTransform();

	}

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




	void OnHit() override
	{
	};

	void OnMiss() override
	{
	};

	void DrawGLTFModel(glm::mat4 viewMat, glm::mat4 projMat);


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

	void SetupGLTFMeshes(tinygltf::Model* model) {
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
						int numPositionEntries = static_cast<int>(accessor.count);
						Logger::Log(1, "%s: loaded %i vertices from glTF file\n", __FUNCTION__,
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
							Logger::Log(1, " << indexAccessor.componentType, %zu", indexAccessor.componentType);
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

	std::vector<GLuint> glTextures;


	std::vector<GLuint> LoadGLTFTextures(tinygltf::Model* model);

	static const void* getDataPointer(tinygltf::Model* model, const tinygltf::Accessor& accessor) {
		const tinygltf::BufferView& bufferView = model->bufferViews[accessor.bufferView];
		const tinygltf::Buffer& buffer = model->buffers[bufferView.buffer];
		return &buffer.data[accessor.byteOffset + bufferView.byteOffset];
	}


};
