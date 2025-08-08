#pragma once

#include <glm/glm.hpp>
#include <glad/glad.h>

class UniformBuffer
{
public:
	void Init(size_t bufferSize);
	void UploadUboData(std::vector<glm::mat4> bufferData, int bindingPoint);
	void UploadColorUboData(std::vector<glm::vec3> bufferData, int bindingPoint);
	void Cleanup();

private:
	size_t m_bufferSize;
	GLuint m_uboBuffer = 0;
};
