#include "Ground.h"

void Ground::DrawObject(glm::mat4 viewMat, glm::mat4 proj, bool shadowMap, glm::mat4 lightSpaceMat,
                        GLuint shadowMapTexture, glm::vec3 camPos)
{
	auto modelMat = glm::mat4(1.0f);
	modelMat = translate(modelMat, m_position);
	//	modelMat = glm::rotate(modelMat, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	modelMat = glm::scale(modelMat, m_scale);
	std::vector<glm::mat4> matrixData;
	matrixData.push_back(viewMat);
	matrixData.push_back(proj);
	matrixData.push_back(modelMat);
	matrixData.push_back(lightSpaceMat);
	m_uniformBuffer.uploadUboData(matrixData, 0);


	// TODO: Update for GLTF
	//	m_model.Draw(m_shader);
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
