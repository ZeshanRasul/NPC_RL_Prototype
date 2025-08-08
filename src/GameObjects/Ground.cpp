#include "Ground.h"

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
					m_shader->SetInt("tex", 0);
					m_shader->SetBool("useBaseColor", mat.pbrMetallicRoughness.baseColorTexture.index >= 0);
					m_shader->SetVec3("color", 1.0f, 1.0f, 1.0f);
				}
				else {
					glm::vec3 baseColor = glm::vec3(mat.pbrMetallicRoughness.baseColorFactor[0], mat.pbrMetallicRoughness.baseColorFactor[1], mat.pbrMetallicRoughness.baseColorFactor[2]);
					glActiveTexture(GL_TEXTURE0);
					glBindTexture(GL_TEXTURE_2D, mTex.GetTexId());
					m_shader->SetInt("tex", 0);
					m_shader->SetBool("useBaseColor", mat.pbrMetallicRoughness.baseColorTexture.index >= 0);
					m_shader->SetVec3("color", baseColor);
				}

				if (mat.pbrMetallicRoughness.metallicRoughnessTexture.index >= 0) {
					glActiveTexture(GL_TEXTURE1);
					glBindTexture(GL_TEXTURE_2D, glTextures[mat.pbrMetallicRoughness.metallicRoughnessTexture.index]);
					m_shader->SetInt("metallicRoughnessTex", 1);
					m_shader->SetBool("useMetallicRoughness", mat.pbrMetallicRoughness.baseColorTexture.index >= 0);

				}
				else {
					m_shader->SetInt("metallicRoughnessTex", 1);
					m_shader->SetBool("useMetallicRoughness", mat.pbrMetallicRoughness.baseColorTexture.index >= 0);
					m_shader->SetFloat("metallicFactor", mat.pbrMetallicRoughness.metallicFactor);
					m_shader->SetFloat("roughnessFactor", mat.pbrMetallicRoughness.roughnessFactor);
				}

				if (mat.normalTexture.index >= 0) {
					glActiveTexture(GL_TEXTURE2);
					glBindTexture(GL_TEXTURE_2D, glTextures[mat.normalTexture.index]);
					m_shader->SetInt("normalTex", 2);
					m_shader->SetBool("useNormalMap", mat.normalTexture.index >= 0);
				}
				else {
					m_shader->SetInt("normalTex", 2);
					m_shader->SetBool("useNormalMap", false);
				}

				if (mat.occlusionTexture.index >= 0) {
					glActiveTexture(GL_TEXTURE3);
					glBindTexture(GL_TEXTURE_2D, glTextures[mat.occlusionTexture.index]);
					m_shader->SetInt("occlusionTex", 3);
					m_shader->SetBool("useOcclusionMap", mat.occlusionTexture.index >= 0);
				}
				else {
					m_shader->SetInt("occlusionTex", 3);
					m_shader->SetBool("useOcclusionMap", false);
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

void Ground::DrawObject(glm::mat4 viewMat, glm::mat4 proj, bool shadowMap, glm::mat4 lightSpaceMat, GLuint shadowMapTexture, glm::vec3 camPos)
{
	DrawGLTFModel(viewMat, proj, camPos);
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
