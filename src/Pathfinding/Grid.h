#pragma once

#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glad/glad.h>

#include "../GameObjects/Cell.h"
#include "src/OpenGL/Shader.h"
#include "OpenGL/UniformBuffer.h"

class Grid
{
public:
	struct Cover
	{
		glm::vec3 worldPosition;
		Cell* gridPos;
		int gridX;
		int gridZ;
	};

	std::vector<glm::vec3> coverPositions = {
		glm::vec3(28.0f, 3.5f, 20.0f),
		glm::vec3(50.0f, 3.5f, 35.0f),

		glm::vec3(10.0f, 3.5f, 60.0f),
		glm::vec3(70.0f, 3.5f, 15.0f),

		glm::vec3(75.0f, 3.5f, 65.0f),

		glm::vec3(70.0f, 3.5f, 27.0f),
		glm::vec3(70.0f, 3.5f, 34.0f),
		glm::vec3(70.0f, 3.5f, 41.0f),
	};


	std::vector<glm::vec3> snappedCoverPositions;


	Grid()
	{
	};

	void initializeGrid();
	void drawGrid(Shader& gridShader, glm::mat4 viewMat, glm::mat4 projMat, glm::vec3 camPos, bool shadowMap,
	              glm::mat4 lightSpaceMat, GLuint shadowMapTexture,
	              glm::vec3 lightDir, glm::vec3 lightAmbient, glm::vec3 lightDiff, glm::vec3 lightSpec);

	glm::vec3 snapToGrid(const glm::vec3& position) const
	{
		int gridX = static_cast<int>(position.x / CELL_SIZE);
		int gridZ = static_cast<int>(position.z / CELL_SIZE);
		return glm::vec3(gridX * CELL_SIZE + CELL_SIZE / 2.0f, position.y, gridZ * CELL_SIZE + CELL_SIZE / 2.0f);
	}

	std::vector<glm::vec3> snapCoverPositionsToGrid() const
	{
		std::vector<glm::vec3> snappedCoverPositions;
		for (glm::vec3 coverPos : coverPositions)
		{
			int gridX = static_cast<int>(coverPos.x / CELL_SIZE);
			int gridZ = static_cast<int>(coverPos.z / CELL_SIZE);
			snappedCoverPositions.push_back(glm::vec3(gridX * CELL_SIZE + CELL_SIZE / 2.0f, 0.0f,
			                                          gridZ * CELL_SIZE + CELL_SIZE / 2.0f));
		}
		return snappedCoverPositions;
	}

	glm::vec3 ConvertCellToWorldSpace(const int gridX, const int gridZ) const
	{
		return glm::vec3(gridX * CELL_SIZE + CELL_SIZE / 2.0f, 0.0f, gridZ * CELL_SIZE + CELL_SIZE / 2.0f);
	}

	std::vector<glm::ivec2> findPath(const glm::ivec2& start, const glm::ivec2& goal,
	                                 const std::vector<std::vector<Cell>>& grid, int npcId);

	void OccupyCell(int x, int y, int npcId);
	void VacateCell(int x, int y, int npcId);

	std::vector<std::vector<Cell>> GetGrid() const { return grid; }
	std::vector<Cover*> GetCoverLocations() const { return coverLocations; }

	int GetGridSize() const { return GRID_SIZE; }
	int GetCellSize() const { return CELL_SIZE; }

	std::vector<Cover*> coverLocations;

	std::vector<glm::vec3> GetWSVertices() const { return wsVertices; }

	std::vector<int> GetIndices() const { return indices; }

private:
	std::vector<glm::vec3> wsVertices;
	std::vector<int> indices;

	UniformBuffer mGridUniformBuffer{};
	UniformBuffer mGridColorUniformBuffer{};

	int GRID_SIZE = 15;
	int CELL_SIZE = 7;
	std::vector<std::vector<Cell>> grid;

	bool firstLoad = true;
};


// Custom hash function for glm::ivec2
struct ivec2_hash
{
	std::size_t operator()(const glm::ivec2& v) const
	{
		return std::hash<int>()(v.x) ^ (std::hash<int>()(v.y) << 1);
	}
};
