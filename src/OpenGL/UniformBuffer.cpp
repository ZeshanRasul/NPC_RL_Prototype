#include <vector>
#include <glm/gtc/type_ptr.hpp>

#include "UniformBuffer.h"
#include "src/Tools/Logger.h"

void UniformBuffer::Init(size_t bufferSize)
{
	m_bufferSize = bufferSize;

	glGenBuffers(1, &m_uboBuffer);

	glBindBuffer(GL_UNIFORM_BUFFER, m_uboBuffer);
	glBufferData(GL_UNIFORM_BUFFER, m_bufferSize, nullptr, GL_STATIC_DRAW);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

void UniformBuffer::UploadUboData(std::vector<glm::mat4> bufferData, int bindingPoint)
{
	if (bufferData.size() == 0)
	{
		return;
	}
	size_t bufferSize = bufferData.size() * sizeof(glm::mat4);
	glBindBuffer(GL_UNIFORM_BUFFER, m_uboBuffer);
	glBufferSubData(GL_UNIFORM_BUFFER, 0, bufferSize, bufferData.data());
	glBindBufferRange(GL_UNIFORM_BUFFER, bindingPoint, m_uboBuffer, 0, bufferSize);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

void UniformBuffer::UploadColorUboData(std::vector<glm::vec3> bufferData, int bindingPoint)
{
	if (bufferData.size() == 0)
	{
		return;
	}
	size_t bufferSize = bufferData.size() * sizeof(glm::vec3);
	glBindBuffer(GL_UNIFORM_BUFFER, m_uboBuffer);
	glBufferSubData(GL_UNIFORM_BUFFER, 0, bufferSize, bufferData.data());
	glBindBufferRange(GL_UNIFORM_BUFFER, bindingPoint, m_uboBuffer, 0, bufferSize);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

void UniformBuffer::Cleanup()
{
	glDeleteBuffers(1, &m_uboBuffer);
}


