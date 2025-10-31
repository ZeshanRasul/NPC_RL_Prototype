#include "Ground.h"
#include "GameManager.h"

void Ground::SetUpAABB()
{
	for (auto& aabbMeshVerts : aabbMeshVertices)
	{
		AABB* aabb = new AABB();
		aabb->CalculateAABB(aabbMeshVerts);
		aabb->SetShader(m_aabbShader);
		aabb->SetUpMesh();
		aabb->SetOwner(this);
		aabb->SetIsEnemy(false);
		m_gameManager->GetPhysicsWorld()->AddCollider(aabb);
		m_aabbs.push_back(aabb);
	}
}

void Ground::SetupGLTFMeshes(tinygltf::Model* model)
{
	meshData.resize(model->meshes.size());
	int planeCount = 1;
	for (size_t meshIndex = 0; meshIndex < model->meshes.size(); ++meshIndex) {
		const tinygltf::Mesh& mesh = model->meshes[meshIndex];
			const tinygltf::Value& val = mesh.extras.Get("isBox");
			const tinygltf::Value& val2 = mesh.extras.Get("isCollider");
			if (val.IsInt() && val.Get<int>() == 1 && val2.IsInt() && val2.Get<int>() == 1) {
				Logger::Log(1, "Mesh is a box collider, setting up AABB\n");
				//aabbMeshVertices.push_back(meshVerts);
				continue;
			}

			const tinygltf::Value& planeVal = mesh.extras.Get("isPlane");
			const tinygltf::Value& planeVal2 = mesh.extras.Get("isCollider");
			if (planeVal.IsInt() && planeVal.Get<int>() == 1 && planeVal2.IsInt() && planeVal2.Get<int>() == 1) {
				Logger::Log(1, "Mesh is a plane collider, setting up plane collider\n");
				//planeData.resize(++planeCount);
				//planeData.push_back(gltfMesh);
				continue;
			}
		GLTFMesh gltfMesh;


		std::vector<glm::vec3> meshVerts;

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
						gltfPrim.verts.push_back(glm::vec3(positions[i * 3 + 0], positions[i * 3 + 1], positions[i * 3 + 2]));
						meshVerts.push_back(glm::vec3(positions[i * 3 + 0], positions[i * 3 + 1], positions[i * 3 + 2]));
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
			}
			else {
				gltfPrim.indexBuffer = 0;
				gltfPrim.indexCount = 0;
			}

			glBindVertexArray(0);


			gltfMesh.primitives.push_back(gltfPrim);
		}

		meshData[meshIndex] = gltfMesh;
	}
}

std::vector<GLuint> Ground::LoadGLTFTextures(tinygltf::Model* model) {
	std::vector<GLuint> textureIDs(model->textures.size(), 0);

	for (size_t i = 0; i < model->textures.size(); ++i) {
		const tinygltf::Texture& tex = model->textures[i];
		if (tex.source < 0 || tex.source >= model->images.size()) {
			continue; // Invalid texture
		}

		const tinygltf::Image& image = model->images[tex.source];

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

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		textureIDs[i] = texID;
	}

	glBindTexture(GL_TEXTURE_2D, 0);
	return textureIDs;
}

float GetEmissiveStrength(const tinygltf::Material& mat)
{
	// Default is 1.0 if extension not present
	float emissiveStrength = 1.0f;

	// Check if extension exists
	auto extIt = mat.extensions.find("KHR_materials_emissive_strength");
	if (extIt != mat.extensions.end())
	{
		const tinygltf::Value& ext = extIt->second;

		// Check if "emissiveStrength" exists in the extension object
		if (ext.Has("emissiveStrength"))
		{
			emissiveStrength = static_cast<float>(ext.Get("emissiveStrength").GetNumberAsDouble());
		}
	}

	return emissiveStrength;
}

void Ground::DrawGLTFModel(glm::mat4 viewMat, glm::mat4 projMat, glm::vec3 camPos) {
	glDisable(GL_CULL_FACE);
	int texIndex = 0;
	for (size_t meshIndex = 0; meshIndex < meshData.size(); ++meshIndex) {
	
		for (size_t primIndex = 0; primIndex < meshData[meshIndex].primitives.size(); ++primIndex) {
			const GLTFPrimitive& prim = meshData[meshIndex].primitives[primIndex];
		

			m_shader->Use();
			glm::mat4 modelMat = glm::mat4(1.0f);
			modelMat = glm::translate(modelMat, m_position);
			//modelMat = glm::rotate(modelMat, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
			modelMat = glm::scale(modelMat, m_scale);
			std::vector<glm::mat4> matrixData;
			matrixData.push_back(viewMat);
			matrixData.push_back(projMat);
			matrixData.push_back(modelMat);
			m_uniformBuffer.UploadUboData(matrixData, 0);

			m_shader->SetVec3("cameraPos", camPos);

			bool hasTexture = false;
			glBindVertexArray(prim.vao);
			int matIndex = prim.material;
			if (matIndex >= 0 && matIndex < mapModel->materials.size()) {
				const tinygltf::Material& mat = mapModel->materials[matIndex];
				if (mat.pbrMetallicRoughness.baseColorTexture.index >= 0) {
					hasTexture = true;
					texIndex = mat.pbrMetallicRoughness.baseColorTexture.index;
				}
				// You can add more checks for other texture types if needed

				if (mat.pbrMetallicRoughness.baseColorTexture.index >= 0) {
					texIndex = mat.pbrMetallicRoughness.baseColorTexture.index;
					glActiveTexture(GL_TEXTURE0);
					glBindTexture(GL_TEXTURE_2D, glTextures[texIndex]);
					m_shader->SetInt("albedoMap", 0);
					m_shader->SetBool("useAlbedo", mat.pbrMetallicRoughness.baseColorTexture.index >= 0);
					m_shader->SetVec3("color", 1.0f, 1.0f, 1.0f);
				}
				else {
					glm::vec3 baseColor = glm::vec3(mat.pbrMetallicRoughness.baseColorFactor[0], mat.pbrMetallicRoughness.baseColorFactor[1], mat.pbrMetallicRoughness.baseColorFactor[2]);
					m_shader->SetBool("useAlbedo", mat.pbrMetallicRoughness.baseColorTexture.index >= 0);
					m_shader->SetVec3("baseColour", baseColor);
				}

				if (mat.pbrMetallicRoughness.metallicRoughnessTexture.index >= 0) {
					glActiveTexture(GL_TEXTURE1);
					glBindTexture(GL_TEXTURE_2D, glTextures[mat.pbrMetallicRoughness.metallicRoughnessTexture.index]);
					m_shader->SetInt("metallicRoughnessMap", 1);
					m_shader->SetBool("useMetallicRoughness", mat.pbrMetallicRoughness.baseColorTexture.index >= 0);

				}
				else {
					m_shader->SetBool("useMetallicRoughness", mat.pbrMetallicRoughness.baseColorTexture.index >= 0);
					m_shader->SetFloat("metallicFactor", mat.pbrMetallicRoughness.metallicFactor);
					m_shader->SetFloat("roughnessFactor", mat.pbrMetallicRoughness.roughnessFactor);
				}

				if (mat.normalTexture.index >= 0) {
					glActiveTexture(GL_TEXTURE2);
					glBindTexture(GL_TEXTURE_2D, glTextures[mat.normalTexture.index]);
					m_shader->SetInt("normalMap", 2);
					m_shader->SetBool("useNormalMap", mat.normalTexture.index >= 0);
				}
				else {
					m_shader->SetBool("useNormalMap", false);
				}

				if (mat.occlusionTexture.index >= 0) {
					glActiveTexture(GL_TEXTURE3);
					glBindTexture(GL_TEXTURE_2D, glTextures[mat.occlusionTexture.index]);
					m_shader->SetInt("aoMap", 3);
					m_shader->SetBool("useOcclusionMap", mat.occlusionTexture.index >= 0);
				}
				else {
					m_shader->SetBool("useOcclusionMap", false);
				}

				if (!mat.emissiveFactor.empty()) {
					glm::vec3 emissiveFactor = glm::vec3(mat.emissiveFactor[0], mat.emissiveFactor[1], mat.emissiveFactor[2]);
					m_shader->SetBool("useEmissiveFactor", true);
					m_shader->SetVec3("emissiveFactor", emissiveFactor);
					float emissiveStrength = GetEmissiveStrength(mat);
					m_shader->SetFloat("emissiveStrength", emissiveStrength);
				}
				else {
					m_shader->SetBool("useEmissiveFactor", false);
					m_shader->SetVec3("emissiveFactor", 0.0f, 0.0f, 0.0f);
					m_shader->SetFloat("emissiveStrength", 0.0f);
				}
			}

			if (prim.indexBuffer) {
				glDrawElements(GL_TRIANGLES, prim.indexCount, GL_UNSIGNED_INT, 0);
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




void Ground::CreatePlaneColliders()
{
	glm::mat4 modelMat = glm::mat4(1.0f);
	//modelMat = glm::translate(modelMat, m_position);
	modelMat = glm::scale(modelMat, m_scale);
	for (const GLTFMesh& plane : planeData) {
		std::vector<glm::vec3> planeVerts;
		for (const glm::vec3& vertex : plane.primitives[0].verts) {
			glm::vec3 wsVertex = modelMat * glm::vec4(vertex, 1.0f);
			planeVerts.push_back(wsVertex);
		}
		PlaneCollider* planeCollider = new PlaneCollider;
		planeCollider = BuildPlaneFromVerts(planeVerts);
		planeColliders.push_back(*planeCollider);
		m_gameManager->GetPhysicsWorld()->AddPlaneCollider(planeCollider);
	}
}

void Ground::LoadPlaneCollider(tinygltf::Model* model)
{
	for (size_t meshIndex = 0; meshIndex < model->meshes.size(); ++meshIndex) {
		const tinygltf::Mesh& mesh = model->meshes[meshIndex];
		GLTFMesh gltfMesh;


		std::vector<glm::vec3> meshVerts;

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
						gltfPrim.verts.push_back(glm::vec3(positions[i * 3 + 0], positions[i * 3 + 1], positions[i * 3 + 2]));
						meshVerts.push_back(glm::vec3(positions[i * 3 + 0], positions[i * 3 + 1], positions[i * 3 + 2]));
						gltfPrim.vertexCount++;
					}
				}

				const void* dataPtr = &buffer.data[accessor.byteOffset + bufferView.byteOffset];
				size_t dataSize = accessor.count * tinygltf::GetNumComponentsInType(accessor.type) * tinygltf::GetComponentSizeInBytes(accessor.componentType);

				glBufferData(GL_ARRAY_BUFFER, dataSize, dataPtr, GL_STATIC_DRAW);

				// Determine attribute layout location (you must match your shader locations)
				GLint location = -1;
				if (attribName == "POSITION") location = 0;

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
			}
			else {
				gltfPrim.indexBuffer = 0;
				gltfPrim.indexCount = 0;
			}

			glBindVertexArray(0);

			gltfMesh.primitives.push_back(gltfPrim);
		}

		planeData[meshIndex] = gltfMesh;

	}
}

Ground::Ground(glm::vec3 pos, glm::vec3 scale, Shader* shdr, Shader* shadowMapShader, bool applySkinning, GameManager* gameMgr, float yaw)
	: GameObject(pos, scale, yaw, shdr, shadowMapShader, applySkinning, gameMgr) 
{
	mapModel = new tinygltf::Model;

	std::string modelFilename = "Assets/Models/Game_Scene/Final/Env4.glb";

	tinygltf::TinyGLTF gltfLoader;
	std::string loaderErrors;
	std::string loaderWarnings;
	bool result = false;

	result = gltfLoader.LoadBinaryFromFile(mapModel, &loaderErrors, &loaderWarnings,
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

	//plane01Model = new tinygltf::Model;

	//std::string planeModelFilename = "src/Assets/Models/Game_Scene/V2/CollisionMeshTest/SlopePlanes/Plane01.gltf";

	//result = gltfLoader.LoadASCIIFromFile(plane01Model, &loaderErrors, &loaderWarnings,
	//	planeModelFilename);

	CreatePlaneColliders();

	for (auto& planeCol : planeColliders)
	{		
		debugPlanes.push_back(MakeDebugPlane(planeCol));
	}

	//model->loadModelNoAnim(modelFilename);
	//model->uploadVertexBuffersNoAnimations();

	//ComputeAudioWorldTransform();

}

void Ground::DrawObject(glm::mat4 viewMat, glm::mat4 proj, bool shadowMap, glm::mat4 lightSpaceMat, GLuint shadowMapTexture, glm::vec3 camPos)
{
	DrawGLTFModel(viewMat, proj, camPos);
	for (auto& debugPlane : debugPlanes)
	{
		glDisable(GL_DEPTH_TEST);
		planeShader->Use();
		planeShader->SetMat4("view", viewMat);
		planeShader->SetMat4("projection", proj);
		planeShader->SetMat4("lightSpaceMatrix", lightSpaceMat);
		planeShader->SetVec3("lineColor", 1.0f, 0.0f, 1.0f);
		glLineWidth(2.0f);
		glBindVertexArray(debugPlane.vao);
		glDrawArrays(GL_LINE_STRIP, 0, debugPlane.countNo);
		glBindVertexArray(0);
		glUseProgram(0);
		glEnable(GL_DEPTH_TEST);
	}

	for (AABB* aabb : m_aabbs)
	{
		//m_aabbShader->Use();
		glm::mat4 modelMat = glm::mat4(1.0f);
		//modelMat = glm::translate(modelMat, glm::vec3(-9.0f, 354.6f, 163.0f));
		//modelMat = glm::rotate(modelMat, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		modelMat = glm::scale(modelMat, m_scale);
		glm::mat4 modelMatrix = glm::scale(glm::mat4(1.0f), m_scale);
		aabb->Update(modelMatrix);
		glDisable(GL_DEPTH_TEST);
		aabb->Render(viewMat, proj, modelMat, glm::vec3(0.0f, 1.0f, 0.0f));
		glEnable(GL_DEPTH_TEST);
	}
}	

void Ground::ComputeAudioWorldTransform()
{
	if (m_recomputeWorldTransform)
	{
		m_recomputeWorldTransform = false;
		auto worldTransform = glm::mat4(1.0f);
		// Scale, then rotate, then translate
		m_audioWorldTransform = glm::scale(worldTransform, m_scale);
		m_audioWorldTransform = rotate(worldTransform, glm::radians(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		m_audioWorldTransform = translate(worldTransform, m_position);

		// Inform components world transform updated
		for (auto comp : m_components)
		{
			comp->OnUpdateWorldTransform();
		}
	}
};
