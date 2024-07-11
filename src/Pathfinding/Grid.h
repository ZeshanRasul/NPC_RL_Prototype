#pragma once

#include <vector>
#include <glm/glm.hpp>
#include <glad/glad.h>

#include "../GameObjects/Cell.h"
#include "../Shader.h"

// Create a 100x100 grid
const int GRID_SIZE = 100;
const int CELL_SIZE = 1;
extern std::vector<std::vector<Cell>> grid;

void initializeGrid();
void drawGrid(Shader& gridShader);
