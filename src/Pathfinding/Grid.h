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
    Grid() {};

    void initializeGrid();
    void drawGrid(Shader& gridShader, glm::mat4 viewMat, glm::mat4 projMat);

    glm::vec3 snapToGrid(const glm::vec3& position) const
    {
        int gridX = static_cast<int>(position.x / CELL_SIZE);
        int gridZ = static_cast<int>(position.z / CELL_SIZE);
        return glm::vec3(gridX * CELL_SIZE + CELL_SIZE / 2.0f, position.y, gridZ * CELL_SIZE + CELL_SIZE / 2.0f);
    }

    std::vector<glm::ivec2> findPath(const glm::ivec2& start, const glm::ivec2& goal, const std::vector<std::vector<Cell>>& grid);

    std::vector<std::vector<Cell>> GetGrid() const { return grid; }

	int GetGridSize() const { return GRID_SIZE; }
    int GetCellSize() const { return CELL_SIZE; }

private:
    UniformBuffer mGridUniformBuffer{};
    UniformBuffer mGridColorUniformBuffer{};

    int GRID_SIZE = 100;
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
 
