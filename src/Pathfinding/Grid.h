#pragma once

#include <vector>
#include <glm/glm.hpp>
#include <glad/glad.h>

#include "../GameObjects/Cell.h"
#include "src/OpenGL/Shader.h"
#include "../GameObjects/Enemy.h"

// Create a 100x100 grid
const int GRID_SIZE = 100;
const int CELL_SIZE = 7;
extern std::vector<std::vector<Cell>> grid;

void initializeGrid();
void drawGrid(Shader& gridShader);

static glm::vec3 snapToGrid(const glm::vec3& position)
{
    int gridX = static_cast<int>(position.x / CELL_SIZE);
    int gridZ = static_cast<int>(position.z / CELL_SIZE);
    return glm::vec3(gridX * CELL_SIZE + CELL_SIZE / 2.0f, position.y, gridZ * CELL_SIZE + CELL_SIZE / 2.0f);
}

// Custom hash function for glm::ivec2
struct ivec2_hash {
    std::size_t operator()(const glm::ivec2& v) const {
        return std::hash<int>()(v.x) ^ (std::hash<int>()(v.y) << 1);
    }
};
std::vector<glm::ivec2> findPath(const glm::ivec2& start, const glm::ivec2& goal, const std::vector<std::vector<Cell>>& grid);

