#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>

#include "Logger.h"
#include "Texture.h"

class Cell
{
public:
	Cell(bool isObs, glm::vec3 col)
		: isObstacle(isObs), color(col), isOccupied(false), occupantId(-1), isCover(false)
	{
		SetUpVAO();
	}

	Cell() : isObstacle(false), color(glm::vec3(0.0f, 1.0f, 0.0f))
	{
		SetUpVAO();
	}

	void SetUpVAO()
	{
		glGenVertexArrays(1, &cellVAO);
		glGenBuffers(1, &cellVBO);

		glBindVertexArray(cellVAO);

		glBindBuffer(GL_ARRAY_BUFFER, cellVBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(cellVertices), cellVertices, GL_STATIC_DRAW);

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
		if (!tex->loadTexture(textureFilename, false))
		{
			Logger::Log(1, "%s: texture loading failed\n", __FUNCTION__);
			return false;
		}
		Logger::Log(1, "%s: %s texture successfully loaded\n", __FUNCTION__, textureFilename);
		return true;
	}

	void BindVAO() const
	{
		glBindVertexArray(cellVAO);
	}

	bool IsObstacle() const
	{
		return isObstacle;
	}

	void SetObstacle(bool obs)
	{
		isObstacle = obs;
	}

	bool IsCover() const
	{
		return isCover;
	}

	void SetCover(bool cover)
	{
		isCover = cover;
	}

	glm::vec3 GetColor() const
	{
		return color;
	}

	void SetColor(glm::vec3 col)
	{
		color = col;
	}

	bool IsOccupied() const
	{
		return isOccupied;
	}

	void SetOccupied(bool occ)
	{
		isOccupied = occ;
	}

	bool IsOccupiedBy(int id) const
	{
		return occupantId == id;
	}

	void SetOccupantId(int id)
	{
		occupantId = id;
	}

	int GetOccupantId() const
	{
		return occupantId;
	}

	std::vector<glm::vec3> GetVertices()
	{
		for (int i = 0; i < 48; i = i + 8)
		{
			cellPosVerts.push_back(glm::vec3(cellVertices[i], cellVertices[i + 1], cellVertices[i + 2]));
		}
		return cellPosVerts;
	}

	Texture mTex{};
	Texture mNormal{};
	Texture mMetallic{};
	Texture mRoughness{};
	Texture mAO{};
	Texture mEmissive{};
	Texture mDebugOutline{};

private:
	GLuint cellVAO = 0;
	GLuint cellVBO = 0;

	std::vector<glm::vec3> cellPosVerts;

	// Vertex data for a cell
	float cellVertices[48] = {
		// positions        //normals        // texture coords
		0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f,
		1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f,
		1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f,

		0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f,
		1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f,
		0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f
	};

	bool isObstacle;
	glm::vec3 color;
	bool isOccupied;
	int occupantId;
	bool isCover;
};
