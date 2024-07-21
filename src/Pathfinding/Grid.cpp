#include "Grid.h"
#include <queue>
#include <unordered_map>

#define GLM_ENABLE_EXPERIMENTAL

std::vector<std::vector<Cell>> grid(GRID_SIZE, std::vector<Cell>(GRID_SIZE, { false, glm::vec3(0.0f, 1.0f, 0.0f) }));

// Add some obstacles
void initializeGrid() {
    for (int i = 5; i < 10; ++i) {
        for (int j = 0; j < 10; ++j) {
            grid[i][j].isObstacle = true;
            grid[i][j].color = glm::vec3(1.0f, 0.0f, 0.0f);
        }
    }
}

void drawGrid(Shader& gridShader) {
    for (int i = 0; i < GRID_SIZE; ++i) {
        for (int j = 0; j < GRID_SIZE; ++j) {
            glm::vec3 position = glm::vec3(i * CELL_SIZE, 0.0f, j * CELL_SIZE);
            glm::mat4 model = glm::translate(glm::mat4(1.0f), position);
            model = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.0f));
            model = glm::scale(model, glm::vec3(CELL_SIZE, 1.0f, CELL_SIZE));
            gridShader.setMat4("model", model);
            gridShader.setVec3("color", grid[i][j].color);
            // Render cell
            glDrawArrays(GL_TRIANGLES, 0, 6);
        }
    }
}

#include <unordered_set>
#include <glm/gtx/hash.hpp>

struct Node {
    glm::ivec2 position;
    float gCost;
    float hCost;
    Node* parent;

    float fCost() const { return gCost + hCost; }
    bool operator<(const Node& other) const { return fCost() > other.fCost(); }
};

float heuristic(const glm::ivec2& a, const glm::ivec2& b) {
    return (std::abs(a.x - b.x) + std::abs(a.y - b.y)) * glm::min((a.x - b.x), (a.y - b.y));
}

std::vector<glm::ivec2> findPath(const glm::ivec2& start, const glm::ivec2& goal, const std::vector<std::vector<Cell>>& grid) {
    std::priority_queue<Node> openSet;
    std::unordered_map<glm::ivec2, Node, ivec2_hash> allNodes;
    std::unordered_set<glm::ivec2, ivec2_hash> closedSet;

    Node startNode = { start, 0.0f, heuristic(start, goal), nullptr };
    openSet.push(startNode);
    allNodes[start] = startNode;

    while (!openSet.empty()) {
        Node current = openSet.top();
        openSet.pop();
        closedSet.insert(current.position);

        if (current.position == goal) {
            std::vector<glm::ivec2> path;
            for (Node* node = &current; node != nullptr; node = node->parent) {
                path.push_back(node->position);
            }
            std::reverse(path.begin(), path.end());
            return path;
        }

        std::vector<glm::ivec2> neighbors = {
            {current.position.x + 1, current.position.y},
            {current.position.x - 1, current.position.y},
            {current.position.x, current.position.y + 1},
            {current.position.x, current.position.y - 1}
        };

        for (const glm::ivec2& neighbor : neighbors) {
            if (neighbor.x < 0 || neighbor.y < 0 || neighbor.x >= GRID_SIZE || neighbor.y >= GRID_SIZE)
                continue;
            if (grid[neighbor.x][neighbor.y].isObstacle || closedSet.count(neighbor))
                continue;

            float tentativeGCost = current.gCost + 1.0f;
            if (allNodes.find(neighbor) == allNodes.end() || tentativeGCost < allNodes[neighbor].gCost) {
                Node neighborNode = { neighbor, tentativeGCost, heuristic(neighbor, goal), &allNodes[current.position] };
                openSet.push(neighborNode);
                allNodes[neighbor] = neighborNode;
            }
        }
    }

    return {};
}

