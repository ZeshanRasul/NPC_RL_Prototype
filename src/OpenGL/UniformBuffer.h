#pragma once

#include <glm/glm.hpp>
#include <glad/glad.h>

class UniformBuffer {
public:
    void init(size_t bufferSize);
    void uploadUboData(std::vector<glm::mat4> bufferData, int bindingPoint);
    void uploadColorUboData(std::vector<glm::vec3> bufferData, int bindingPoint);
    void cleanup();

private:
    size_t mBufferSize;
    GLuint mUboBuffer = 0;
};