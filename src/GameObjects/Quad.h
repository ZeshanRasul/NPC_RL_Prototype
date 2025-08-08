#pragma once
#include "src/OpenGL/Renderer.h"
#include "src/OpenGL/Texture.h"
#include "src/OpenGL/Shader.h"

class Quad
{
public:
	void SetUpVAO(bool hasZ)
	{
		glGenVertexArrays(1, &m_quadVao);
		glGenBuffers(1, &m_quadVbo);
		glGenBuffers(1, &m_quadEbo);

		glBindVertexArray(m_quadVao);

		glBindBuffer(GL_ARRAY_BUFFER, m_quadVbo);
		if (hasZ)
		{
			glBufferData(GL_ARRAY_BUFFER, sizeof(m_quadVertices3D), m_quadVertices3D, GL_STATIC_DRAW);

			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_quadEbo);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(m_quadIndices3D), m_quadIndices3D, GL_STATIC_DRAW);

			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), static_cast<void*>(nullptr));
			glEnableVertexAttribArray(0);
			glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
			glEnableVertexAttribArray(1);
		}
		else
		{
			glBufferData(GL_ARRAY_BUFFER, sizeof(m_quadVertices), m_quadVertices, GL_STATIC_DRAW);

			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_quadEbo);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(m_indices), m_indices, GL_STATIC_DRAW);

			glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), static_cast<void*>(nullptr));
			glEnableVertexAttribArray(0);
			glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
			glEnableVertexAttribArray(1);
		}

		glBindVertexArray(0);
	}

	bool LoadTexture(std::string textureFilename)
	{
		if (!m_tex.LoadTexture(textureFilename, false))
		{
			Logger::Log(1, "%s: texture loading failed\n", __FUNCTION__);
			return false;
		}
		Logger::Log(1, "%s: Quad texture successfully loaded\n", __FUNCTION__, textureFilename);
		return true;
	}

	void SetShader(Shader* shdr) { m_shader = shdr; }

	void Draw()
	{
		glBindVertexArray(m_quadVao);
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
		glBindVertexArray(0);
	}

	void Draw3D(glm::vec3 tint, float alpha, glm::mat4 proj, glm::mat4 view, glm::mat4 model)
	{
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		m_shader->Use();
		m_tex.Bind(0);
		m_shader->SetInt("muzzleTexture", 0);
		m_shader->SetMat4("projection", proj);
		m_shader->SetMat4("view", view);
		m_shader->SetMat4("model", model);

		m_shader->SetVec3("tint", tint);
		m_shader->SetFloat("alpha", alpha);
		glBindVertexArray(m_quadVao);
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
		glBindVertexArray(0);
		m_tex.Unbind();
		glDisable(GL_BLEND);
	}

	Texture* GetTexture() { return &m_tex; }

private:
	GLuint m_quadVao;
	GLuint m_quadVbo;
	GLuint m_quadEbo;

	Texture m_tex{};
	Shader* m_shader{};

	float m_quadVertices[16] = {
		// Positions   // Texture Coords
		0.5f, -0.5f, 1.0f, 0.0f, // Bottom-right
		0.5f, -1.0f, 1.0f, 1.0f, // Top-right
		1.0f, -1.0f, 0.0f, 1.0f, // Top-left
		1.0f, -0.5f, 0.0f, 0.0f, // Bottom-left
	};

	float m_quadVertices3D[20] = {
		// Positions        // Texture Coords
		-0.5f, 0.5f, 0.0f, 0.0f, 1.0f, // Top-left     (0)
		-0.5f, -0.5f, 0.0f, 0.0f, 0.0f, // Bottom-left  (1)
		0.5f, -0.5f, 0.0f, 1.0f, 0.0f, // Bottom-right (2)
		0.5f, 0.5f, 0.0f, 1.0f, 1.0f // Top-right    (3)
	};

	unsigned int m_indices[6] = {
		0, 1, 3,
		1, 2, 3
	};

	unsigned int m_quadIndices3D[6] = {
		0, 1, 2,
		0, 2, 3
	};
};
