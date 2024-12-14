#pragma once
#include "src/OpenGL/Renderer.h"
#include "src/OpenGL/Texture.h"
#include "src/OpenGL/Shader.h"

class Quad
{
public:
	void SetUpVAO(bool hasZ)
	{
		glGenVertexArrays(1, &quadVAO);
		glGenBuffers(1, &quadVBO);
		glGenBuffers(1, &quadEBO);

		glBindVertexArray(quadVAO);

		glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
		if (hasZ)
		{
			glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices3D), quadVertices3D, GL_STATIC_DRAW);

			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, quadEBO);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(quadIndices3D), quadIndices3D, GL_STATIC_DRAW);

			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), static_cast<void*>(nullptr));
			glEnableVertexAttribArray(0);
			glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
			glEnableVertexAttribArray(1);
		}
		else
		{
			glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);

			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, quadEBO);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

			glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), static_cast<void*>(nullptr));
			glEnableVertexAttribArray(0);
			glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
			glEnableVertexAttribArray(1);
		}

		glBindVertexArray(0);
	}

	bool LoadTexture(std::string textureFilename)
	{
		if (!tex.loadTexture(textureFilename, false))
		{
			Logger::log(1, "%s: texture loading failed\n", __FUNCTION__);
			return false;
		}
		Logger::log(1, "%s: Quad texture successfully loaded\n", __FUNCTION__, textureFilename);
		return true;
	}

	void SetShader(Shader* shdr) { shader = shdr; }

	void Draw()
	{
		glBindVertexArray(quadVAO);
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
		glBindVertexArray(0);
	}

	void Draw3D(glm::vec3 tint, float alpha, glm::mat4 proj, glm::mat4 view, glm::mat4 model)
	{
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		shader->use();
		tex.bind(0);
		shader->setInt("muzzleTexture", 0);
		shader->setMat4("projection", proj);
		shader->setMat4("view", view);
		shader->setMat4("m_model", model);

		shader->setVec3("tint", tint);
		shader->setFloat("alpha", alpha);
		glBindVertexArray(quadVAO);
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
		glBindVertexArray(0);
		tex.unbind();
		glDisable(GL_BLEND);
	}

	Texture* GetTexture() { return &tex; }

private:
	GLuint quadVAO;
	GLuint quadVBO;
	GLuint quadEBO;

	Texture tex{};
	Shader* shader{};

	float quadVertices[16] = {
		// Positions   // Texture Coords
		0.5f, -0.5f, 1.0f, 0.0f, // Bottom-right
		0.5f, -1.0f, 1.0f, 1.0f, // Top-right
		1.0f, -1.0f, 0.0f, 1.0f, // Top-left
		1.0f, -0.5f, 0.0f, 0.0f, // Bottom-left
	};

	float quadVertices3D[20] = {
		// Positions        // Texture Coords
		-0.5f, 0.5f, 0.0f, 0.0f, 1.0f, // Top-left     (0)
		-0.5f, -0.5f, 0.0f, 0.0f, 0.0f, // Bottom-left  (1)
		0.5f, -0.5f, 0.0f, 1.0f, 0.0f, // Bottom-right (2)
		0.5f, 0.5f, 0.0f, 1.0f, 1.0f // Top-right    (3)
	};

	unsigned int indices[6] = {
		0, 1, 3,
		1, 2, 3
	};

	unsigned int quadIndices3D[6] = {
		0, 1, 2,
		0, 2, 3
	};
};
