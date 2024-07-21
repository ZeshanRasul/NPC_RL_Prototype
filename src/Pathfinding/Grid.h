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

// Custom hash function for glm::ivec2
struct ivec2_hash {
    std::size_t operator()(const glm::ivec2& v) const {
        return std::hash<int>()(v.x) ^ (std::hash<int>()(v.y) << 1);
    }
};
std::vector<glm::ivec2> findPath(const glm::ivec2& start, const glm::ivec2& goal, const std::vector<std::vector<Cell>>& grid);

void moveEnemy(Enemy& enemy, const std::vector<glm::ivec2>& path, float deltaTime);
