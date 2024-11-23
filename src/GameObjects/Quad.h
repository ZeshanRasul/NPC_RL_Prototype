#pragma once
#include "src/OpenGL/Renderer.h"

class Quad {
public:
	void SetUpVAO() {
		glGenVertexArrays(1, &quadVAO);
		glGenBuffers(1, &quadVBO);
		glGenBuffers(1, &quadEBO);

		glBindVertexArray(quadVAO);

		glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, quadEBO);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
		glEnableVertexAttribArray(1);

		glBindVertexArray(0);

	}

	void Draw() {
		glBindVertexArray(quadVAO);
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
		glBindVertexArray(0);
	}

private:
	GLuint quadVAO;
	GLuint quadVBO;
	GLuint quadEBO;

	float quadVertices[16] = {
		// Positions   // Texture Coords
		0.5f, -0.5f,   1.0f, 0.0f, // Bottom-right
		0.5f, -1.0f,   1.0f, 1.0f, // Top-right
		1.0f, -1.0f,   0.0f, 1.0f, // Top-left
		1.0f, -0.5f,   0.0f, 0.0f, // Bottom-left
	};

	unsigned int indices[6] = {
		0, 1, 3, // First triangle
		1, 2, 3  // Second triangle
	};
};