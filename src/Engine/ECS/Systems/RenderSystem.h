#pragma once

#include "glm/glm.hpp"
#include "EnTT/entt.hpp"

#include "OpenGL/UniformBuffer.h"
#include "Scene/Components/TransformComponent.h"
#include "Scene/Components/StaticMeshComponent.h"
#include "Shader.h"
#include "Logger.h"


struct PerObjectUBO
{
	std::vector<glm::mat4> matrices;
	bool shadowPass
};

inline Shader* PickShader(const StaticMeshComponent& comp, bool shadowPass)
{
	return shadowPass ? (comp.shadowShader ? comp.shadowShader : comp.shader) : comp.shader;
}

inline std::vector<GLuint> LoadTextures(tinygltf::Model* model)
{
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

inline void SetupStaicMesh(entt::registry& r)
{
	auto view = r.view<StaticMeshComponent>();

	for (auto e : view)
	{
		auto& sm = view.get<StaticMeshComponent>(e);
		for (int texID : LoadTextures(sm.model))
			sm.glTextures.push_back(texID);

		meshData.resize(sm.model->meshes.size());
		int planeCount = 1;
		for (size_t meshIndex = 0; meshIndex < sm.model->meshes.size(); ++meshIndex) {
			const tinygltf::Mesh& mesh = sm.model->meshes[meshIndex];
			GLTFMesh gltfMesh;


			std::vector<glm::vec3> meshVerts;

			for (size_t primIndex = 0; primIndex < mesh.primitives.size(); ++primIndex) {
				const tinygltf::Primitive& primitive = mesh.primitives[primIndex];
				GLTFPrimitive gltfPrim = {};
				gltfPrim.mode = primitive.mode; 

				gltfPrim.material = primitive.material;

				glGenVertexArrays(1, &gltfPrim.vao);
				glBindVertexArray(gltfPrim.vao);

				// --- Upload vertex attributes ---
				for (const auto& attrib : primitive.attributes) {
					const std::string& attribName = attrib.first; 
					int accessorIndex = attrib.second;
					const tinygltf::Accessor& accessor = sm.model->accessors[accessorIndex];
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

					GLint location = -1;
					if (attribName == "POSITION") location = 0;
					else if (attribName == "NORMAL") location = 1;
					else if (attribName == "TEXCOORD_0") location = 2;
					else if (attribName == "TEXCOORD_1") location = 3;
					else if (attribName == "TEXCOORD_2") location = 4;

					if (location >= 0) {
						GLint numComponents = tinygltf::GetNumComponentsInType(accessor.type);
						GLenum glType = accessor.componentType; .

						glEnableVertexAttribArray(location);
						glVertexAttribPointer(location, numComponents, glType,
							accessor.normalized ? GL_TRUE : GL_FALSE,
							bufferView.byteStride ? bufferView.byteStride : 0,
							(const void*)0);
					}

					glBindBuffer(GL_ARRAY_BUFFER, 0);
				}


				if (primitive.indices >= 0) {
					const tinygltf::Accessor& indexAccessor = sm.model->accessors[primitive.indices];
					const tinygltf::BufferView& bufferView = sm.model->bufferViews[indexAccessor.bufferView];
					const tinygltf::Buffer& buffer = sm.model->buffers[bufferView.buffer];



					const unsigned char* dataPtr = buffer.data.data() + bufferView.byteOffset + indexAccessor.byteOffset;

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
					int posAccessorIndex = primitive.attributes.at("POSITION");
					const tinygltf::Accessor& posAccessor = sm.model->accessors[posAccessorIndex];
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
}

inline void RenderStaticMeshes(
	entt::registry& r,
	UniformBuffer& perObjectUBO,
	const glm::mat4& viewMatrix,
	const glm::mat4& projMatrix,
	const glm::mat4& lightSpaceMatrix,
	const glm::vec3& cameraPos,
	bool shadowPass)
{
	auto view = r.view<TransformComponent, StaticMeshComponent>();

	for (auto e : view)
	{
		auto& t = view.get<TransformComponent>(e);
		auto& sm = view.get<StaticMeshComponent>(e);

		if (!sm.visible) continue;
		if (shadowPass && !sm.castsShadows) continue;
		if (!sm.model || !sm.shader) continue;

		Shader* shader = PickShader(sm, shadowPass);
		if (!shader) continue;
		shader->Use();

		PerObjectUBO ubo;
		ubo.matrices.push_back(viewMatrix);
		ubo.matrices.push_back(projMatrix);
		ubo.matrices.push_back(t.World);
		size_t uboBufferSize = 4 * sizeof(glm::mat4);
		perObjectUBO.Init(uboBufferSize);
		Logger::Log(1, "%s: matrix uniform buffer (size %i bytes) successfully created\n", __FUNCTION__,
			uboBufferSize);
		perObjectUBO.UploadUboData(ubo.matrices, 0);

		for (size_t meshIndex = 0; meshIndex < sm.meshData.size(); ++meshIndex) {

			for (size_t primIndex = 0; primIndex < sm.meshData[meshIndex].primitives.size(); ++primIndex) {
				const GLTFPrimitive& prim = sm.meshData[meshIndex].primitives[primIndex];
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

		}
	}
}