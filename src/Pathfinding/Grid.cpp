#include "Grid.h"
#include <queue>
#include <unordered_map>

std::vector<std::vector<Cell>> grid(GRID_SIZE, std::vector<Cell>(GRID_SIZE, { false, glm::vec3(0.0f, 1.0f, 0.0f) }));

// Add some obstacles
void initializeGrid() {
    for (int i = 5; i < 10; ++i) {
        for (int j = 0; j < 10; ++j) {
            if (j >= 20 && j <= GRID_SIZE)
            {
            }
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

// Node structure for A*
struct Node {
    glm::ivec2 position; // The (x, y) position of the node in the grid
    float gCost;         // Cost from the start node to this node
    float hCost;         // Heuristic cost from this node to the goal
    Node* parent;        // Pointer to the parent node to reconstruct the path

    float fCost() const { return gCost + hCost; } // Total cost

    bool operator<(const Node& other) const {
        return fCost() > other.fCost(); // Priority queue needs the smallest fCost
    }
};

// Heuristic function to calculate Manhattan distance
float heuristic(const glm::ivec2& a, const glm::ivec2& b) {
    return std::abs(a.x - b.x) + std::abs(a.y - b.y);
}

// Function to find the path using A* algorithm
std::vector<glm::ivec2> findPath(const glm::ivec2& start, const glm::ivec2& goal, const std::vector<std::vector<Cell>>& grid) {
    std::priority_queue<Node> openSet;
    std::unordered_map<glm::ivec2, Node, ivec2_hash> allNodes;

    // Initialize the start node
    Node startNode = { start, 0.0f, heuristic(start, goal), nullptr };
    openSet.push(startNode);
    allNodes[start] = startNode;

    while (!openSet.empty()) {
        Node current = openSet.top();
        openSet.pop();

        // If the goal is reached, reconstruct the path
        if (current.position == goal) {
            std::vector<glm::ivec2> path;
            for (Node* node = &current; node != nullptr; node = node->parent) {
                path.push_back(node->position);
            }
            std::reverse(path.begin(), path.end());
            return path;
        }

        // Explore neighbors
        std::vector<glm::ivec2> neighbors = {
            {current.position.x + 1, current.position.y},
            {current.position.x - 1, current.position.y},
            {current.position.x, current.position.y + 1},
            {current.position.x, current.position.y - 1}
        };

        for (const glm::ivec2& neighbor : neighbors) {
            // Check if the neighbor is within the grid bounds
            if (neighbor.x < 0 || neighbor.y < 0 || neighbor.x >= GRID_SIZE || neighbor.y >= GRID_SIZE)
                continue;
            // Check if the neighbor is an obstacle
            if (grid[neighbor.x][neighbor.y].isObstacle)
                continue;

            float tentativeGCost = current.gCost + 1.0f;
            if (allNodes.find(neighbor) == allNodes.end() || tentativeGCost < allNodes[neighbor].gCost) {
                // Update or add the neighbor node with new costs
                Node neighborNode = { neighbor, tentativeGCost, heuristic(neighbor, goal), &allNodes[current.position] };
                openSet.push(neighborNode);
                allNodes[neighbor] = neighborNode;
            }
        }
    }

    return {}; // Return an empty path if no path is found
}

void moveEnemy(Enemy& enemy, const std::vector<glm::ivec2>& path, float deltaTime) {
    if (path.empty()) return;

    glm::vec3 targetPos = glm::vec3(path[0].x * CELL_SIZE, enemy.getPosition().y, path[0].y * CELL_SIZE);
    glm::vec3 direction = glm::normalize(targetPos - enemy.getPosition());
    float speed = 5.0f;
    enemy.setPosition(enemy.getPosition() + direction * speed * deltaTime);
}





