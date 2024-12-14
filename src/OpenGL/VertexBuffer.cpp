#include "VertexBuffer.h"
#include "src/Tools/Logger.h"

void VertexBuffer::Init()
{
	glGenVertexArrays(1, &m_vao);
	glGenBuffers(1, &m_vertexVbo);

	glBindVertexArray(m_vao);

	glBindBuffer(GL_ARRAY_BUFFER, m_vertexVbo);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, m_position));
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, m_color));
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, m_uv));

	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glEnableVertexAttribArray(2);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	Logger::Log(1, "%s: VAO and VBO initialized\n", __FUNCTION__);
}

void VertexBuffer::Cleanup()
{
	glDeleteBuffers(1, &m_vertexVbo);
	glDeleteVertexArrays(1, &m_vao);
}

void VertexBuffer::UploadData(Mesh vertexData)
{
	glBindVertexArray(m_vao);
	glBindBuffer(GL_ARRAY_BUFFER, m_vertexVbo);

	glBufferData(GL_ARRAY_BUFFER, vertexData.m_vertices.size() * sizeof(Vertex), &vertexData.m_vertices.at(0),
	             GL_DYNAMIC_DRAW);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}

void VertexBuffer::Bind()
{
	glBindVertexArray(m_vao);
}

void VertexBuffer::Unbind()
{
	glBindVertexArray(0);
}

void VertexBuffer::Draw(GLuint mode, unsigned int start, unsigned int num)
{
	glDrawArrays(mode, start, num);
}

void VertexBuffer::BindAndDraw(GLuint mode, unsigned int start, unsigned int num)
{
	Bind();
	glDrawArrays(mode, start, num);
	Unbind();
}
