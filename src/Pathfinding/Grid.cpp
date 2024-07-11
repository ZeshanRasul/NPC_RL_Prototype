#include "Grid.h"

std::vector<std::vector<Cell>> grid(GRID_SIZE, std::vector<Cell>(GRID_SIZE, { false, glm::vec3(0.0f, 1.0f, 0.0f) }));

// Add some obstacles
void initializeGrid() {
    for (int i = 5; i < 10; ++i) {
        for (int j = 5; j < 10; ++j) {
            grid[i][j].isObstacle = true;
            grid[i][j].color = glm::vec3(1.0f, 0.0f, 0.0f);
        }
    }
}

void drawGrid(Shader& gridShader) {
    for (int i = 0; i < GRID_SIZE; ++i) {
        for (int j = 0; j < GRID_SIZE; ++j) {
            glm::vec3 position = glm::vec3(i * CELL_SIZE, 0.2f, j * CELL_SIZE);
            glm::mat4 model = glm::translate(glm::mat4(1.0f), position);
            model = glm::translate(model, glm::vec3(-10.0f, 0.0f, 0.0f));
            model = glm::scale(model, glm::vec3(CELL_SIZE, 1.0f, CELL_SIZE));
            gridShader.setMat4("model", model);
            gridShader.setVec3("color", grid[i][j].color);
            // Render cell
            glDrawArrays(GL_TRIANGLES, 0, 6);
        }
    }
}