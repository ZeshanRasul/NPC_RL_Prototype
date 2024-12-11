#include "Waypoint.h"

void Waypoint::drawObject(glm::mat4 viewMat, glm::mat4 proj, bool shadowMap, glm::mat4 lightSpaceMat, GLuint shadowMapTexture, glm::vec3 camPos)
{
	glm::mat4 modelMat = glm::mat4(1.0f);
	modelMat = glm::translate(modelMat, position);
	//	modelMat = glm::rotate(modelMat, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	modelMat = glm::scale(modelMat, scale);
	std::vector<glm::mat4> matrixData;
	matrixData.push_back(viewMat);
	matrixData.push_back(proj);
	matrixData.push_back(modelMat);
	matrixData.push_back(lightSpaceMat);
	mUniformBuffer.uploadUboData(matrixData, 0);

	// Draw TODO: Update for GLTF

	//	model->draw();
}

void Waypoint::ComputeAudioWorldTransform()
{
	if (mRecomputeWorldTransform)
	{
		mRecomputeWorldTransform = false;
		glm::mat4 worldTransform = glm::mat4(1.0f);
		// Scale, then rotate, then translate
		audioWorldTransform = glm::scale(worldTransform, scale);
		audioWorldTransform = glm::rotate(worldTransform, glm::radians(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		audioWorldTransform = glm::translate(worldTransform, position);

		// Inform components world transform updated
		for (auto comp : mComponents)
		{
			comp->OnUpdateWorldTransform();
		}
	}
}
