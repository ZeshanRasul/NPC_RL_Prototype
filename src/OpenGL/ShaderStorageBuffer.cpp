#include "ShaderStorageBuffer.h"
#include "Logger.h"

void ShaderStorageBuffer::Init(size_t bufferSize)
{
	m_bufferSize = bufferSize;

	glGenBuffers(1, &m_shaderStorageBuffer);

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_shaderStorageBuffer);
	glBufferData(GL_SHADER_STORAGE_BUFFER, m_bufferSize, nullptr, GL_DYNAMIC_COPY);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

void ShaderStorageBuffer::UploadSsboData(std::vector<glm::mat4> bufferData, int bindingPoint) const
{
	if (bufferData.size() == 0)
	{
		return;
	}
	size_t bufferSize = bufferData.size() * sizeof(glm::mat4);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_shaderStorageBuffer);
	glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, bufferSize, bufferData.data());
	glBindBufferRange(GL_SHADER_STORAGE_BUFFER, bindingPoint, m_shaderStorageBuffer, 0,
	                  bufferSize);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

void ShaderStorageBuffer::UploadSsboData(std::vector<glm::mat2x4> bufferData, int bindingPoint) const
{
	if (bufferData.size() == 0)
	{
		return;
	}
	size_t bufferSize = bufferData.size() * sizeof(glm::mat2x4);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_shaderStorageBuffer);
	glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, bufferSize, bufferData.data());
	glBindBufferRange(GL_SHADER_STORAGE_BUFFER, bindingPoint, m_shaderStorageBuffer, 0,
	                  bufferSize);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

void ShaderStorageBuffer::Cleanup()
{
	glDeleteBuffers(1, &m_shaderStorageBuffer);
}
