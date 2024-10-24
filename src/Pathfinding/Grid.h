#pragma once

#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glad/glad.h>

#include "../GameObjects/Cell.h"
#include "src/OpenGL/Shader.h"
#include "OpenGL/UniformBuffer.h"

class Grid {
public:
	struct Cover {
		glm::vec3 worldPosition;
		Cell* gridPos;
        int gridX;
		int gridZ;
	};

    std::vector<glm::vec3> coverPositions = {
        glm::vec3(15.0f, 3.5f, 10.0f),
        glm::vec3(45.0f, 3.5f, 10.0f),
        glm::vec3(30.0f, 3.5f, 10.0f),
        glm::vec3(25.0f, 3.5f, 10.0f),
        glm::vec3(30.0f, 3.5f, 50.0f),
        glm::vec3(30.0f, 3.5f, 57.0f),
        glm::vec3(30.0f, 3.5f, 64.0f),
        glm::vec3(30.0f, 3.5f, 71.0f),
    };

    std::vector<glm::vec3> snappedCoverPositions;


    Grid() {};

    void initializeGrid();
    void drawGrid(Shader& gridShader, glm::mat4 viewMat, glm::mat4 projMat);

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
            snappedCoverPositions.push_back(glm::vec3(gridX * CELL_SIZE + CELL_SIZE / 2.0f, 0.0f, gridZ * CELL_SIZE + CELL_SIZE / 2.0f));
	    
        }
		;
		return snappedCoverPositions;
	}

	glm::vec3 ConvertCellToWorldSpace(const int gridX, const int gridZ) const
	{
		return glm::vec3(gridX * CELL_SIZE + CELL_SIZE / 2.0f, 0.0f, gridZ * CELL_SIZE + CELL_SIZE / 2.0f);
	}

    std::vector<glm::ivec2> findPath(const glm::ivec2& start, const glm::ivec2& goal, const std::vector<std::vector<Cell>>& grid, int npcId);

    void OccupyCell(int x, int y, int npcId);
    void VacateCell(int x, int y, int npcId);

    std::vector<std::vector<Cell>> GetGrid() const { return grid; }
    std::vector<Cover*> GetCoverLocations() const { return coverLocations; }

	int GetGridSize() const { return GRID_SIZE; }
    int GetCellSize() const { return CELL_SIZE; }

    std::vector<Cover*> coverLocations;
private:
    UniformBuffer mGridUniformBuffer{};
    UniformBuffer mGridColorUniformBuffer{};

    int GRID_SIZE = 15;
    int CELL_SIZE = 7;
    std::vector<std::vector<Cell>> grid;

    bool firstLoad = true;
}; 


// Custom hash function for glm::ivec2
struct ivec2_hash {
    std::size_t operator()(const glm::ivec2& v) const {
        return std::hash<int>()(v.x) ^ (std::hash<int>()(v.y) << 1);
    }
};
 
