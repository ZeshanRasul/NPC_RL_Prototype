#pragma once
#include <glad/glad.h>
#include <vector>
#include <glm/glm.hpp>
#include <GLFW/glfw3.h>

#include "src/OpenGL/RenderData.h"

class VertexBuffer
{
public:
	void Init();
	void UploadData(Mesh vertexData);
	void Bind();
	void Unbind();
	void Draw(GLuint mode, unsigned int start, unsigned int num);
	void BindAndDraw(GLuint mode, unsigned int start, unsigned int num);
	void Cleanup();

private:
	GLuint m_vao = 0;
	GLuint m_vertexVbo = 0;
};
