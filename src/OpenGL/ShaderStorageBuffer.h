#pragma once
#include <vector>
#include <glm/glm.hpp>
#include <glad/glad.h>

class ShaderStorageBuffer
{
public:
	void Init(size_t bufferSize);
	void UploadSsboData(std::vector<glm::mat4> bufferData, int bindingPoint) const;
	void UploadSsboData(std::vector<glm::mat2x4> bufferData, int bindingPoint) const;
	void Cleanup();

private:
	size_t m_bufferSize;
	GLuint m_shaderStorageBuffer = 0;
};
