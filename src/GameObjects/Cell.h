#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>

#include "Logger.h"
#include "Texture.h"

class Cell
{
	friend class Grid;

public:
	Cell(bool isObs, glm::vec3 col)
		: m_isObstacle(isObs), m_color(col), m_isOccupied(false), m_occupantId(-1), m_isCover(false)
	{
		SetUpVAO();
	}

	Cell() : m_isObstacle(false), m_color(glm::vec3(0.0f, 1.0f, 0.0f))
	{
		SetUpVAO();
	}

	void SetUpVAO()
	{
		glGenVertexArrays(1, &m_cellVao);
		glGenBuffers(1, &m_cellVbo);

		glBindVertexArray(m_cellVao);

		glBindBuffer(GL_ARRAY_BUFFER, m_cellVbo);
		glBufferData(GL_ARRAY_BUFFER, sizeof(m_cellVertices), m_cellVertices, GL_STATIC_DRAW);

		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), static_cast<void*>(nullptr));
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
		glEnableVertexAttribArray(2);

		glBindVertexArray(0);
	}

	bool LoadTexture(std::string textureFilename, Texture* tex)
	{
		if (!tex->LoadTexture(textureFilename, false))
		{
			Logger::Log(1, "%s: texture loading failed\n", __FUNCTION__);
			return false;
		}
		Logger::Log(1, "%s: %s texture successfully loaded\n", __FUNCTION__, textureFilename);
		return true;
	}

	void BindVAO() const
	{
		glBindVertexArray(m_cellVao);
	}

	bool IsObstacle() const
	{
		return m_isObstacle;
	}

	void SetObstacle(bool obs)
	{
		m_isObstacle = obs;
	}

	bool IsCover() const
	{
		return m_isCover;
	}

	void SetCover(bool cover)
	{
		m_isCover = cover;
	}

	glm::vec3 GetColor() const
	{
		return m_color;
	}

	void SetColor(glm::vec3 col)
	{
		m_color = col;
	}

	bool IsOccupied() const
	{
		return m_isOccupied;
	}

	void SetOccupied(bool occ)
	{
		m_isOccupied = occ;
	}

	bool IsOccupiedBy(int id) const
	{
		return m_occupantId == id;
	}

	void SetOccupantId(int id)
	{
		m_occupantId = id;
	}

	int GetOccupantId() const
	{
		return m_occupantId;
	}

	std::vector<glm::vec3> GetVertices() {
		std::vector<glm::vec3> cellPosVerts;
		for (int i = 0; i < 48; i += 8) {
			cellPosVerts.push_back(glm::vec3(cellVertices[i], cellVertices[i + 1], cellVertices[i + 2]));
		}
		return cellPosVerts;
	}

private:
	GLuint m_cellVao = 0;
	GLuint m_cellVbo = 0;

	// Vertex data for a cell
	float m_cellVertices[48] = {
		// positions        //normals        // texture coords
		 0.0f, 0.0f, 0.0f,  0.0f, 1.0f, 0.0f,  0.0f, 0.0f,
		 1.0f, 0.0f, 1.0f,  0.0f, 1.0f, 0.0f,  1.0f, 1.0f,
		 1.0f, 0.0f, 0.0f,  0.0f, 1.0f, 0.0f,  1.0f, 0.0f,

		 0.0f, 0.0f, 0.0f,  0.0f, 1.0f, 0.0f,  0.0f, 0.0f,
		 0.0f, 0.0f, 1.0f,  0.0f, 1.0f, 0.0f,  0.0f, 1.0f,
		 1.0f, 0.0f, 1.0f,  0.0f, 1.0f, 0.0f,  1.0f, 1.0f
	};

	glm::vec3 m_color;
	bool m_isObstacle;
	bool m_isOccupied;
	int m_occupantId;
	bool m_isCover;
};
