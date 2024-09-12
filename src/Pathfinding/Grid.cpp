#include "Grid.h"
#include <queue>
#include <unordered_map>
#include "Logger.h"

void Grid::initializeGrid() {

    std::vector<Cell> row(GRID_SIZE, Cell(false, glm::vec3(0.0f, 1.0f, 0.0f)));
    grid = std::vector<std::vector<Cell>>(GRID_SIZE, row);

    for (glm::vec3 coverPos : snapCoverPositionsToGrid())
    {
		int gridX = static_cast<int>(coverPos.x / CELL_SIZE);
		int gridZ = static_cast<int>(coverPos.z / CELL_SIZE);
		grid[gridX][gridZ].SetObstacle(true);
		grid[gridX][gridZ].SetColor(glm::vec3(1.0f, 0.0f, 0.0f));
		if (gridX + 1 < GRID_SIZE) {
			grid[gridX + 1][gridZ].SetCover(true);
			grid[gridX + 1][gridZ].SetColor(glm::vec3(1.0f, 0.4f, 0.0f));
			Cover newCover1 = { ConvertCellToWorldSpace(gridX + 1, gridZ), grid[gridX + 1][gridZ] };
            coverLocations.push_back(newCover1);
		}
        if (gridZ + 1 < GRID_SIZE) {
            grid[gridX][gridZ + 1].SetCover(true);
            grid[gridX][gridZ + 1].SetColor(glm::vec3(1.0f, 0.4f, 0.0f));
			Cover newCover2 = { ConvertCellToWorldSpace(gridX, gridZ + 1), grid[gridX][gridZ + 1] };
            coverLocations.push_back(newCover2);

        }
        if (gridX - 1 > 0) {
            grid[gridX - 1][gridZ].SetCover(true);
            grid[gridX - 1][gridZ].SetColor(glm::vec3(1.0f, 0.4f, 0.0f));
			Cover newCover3 = { ConvertCellToWorldSpace(gridX - 1, gridZ), grid[gridX - 1][gridZ] };
            coverLocations.push_back(newCover3);
        }
        if (gridZ - 1 > 0) {
            grid[gridX][gridZ - 1].SetCover(true);
            grid[gridX][gridZ - 1].SetColor(glm::vec3(1.0f, 0.4f, 0.0f));
			Cover newCover4 = { ConvertCellToWorldSpace(gridX, gridZ - 1), grid[gridX][gridZ - 1] };
            coverLocations.push_back(newCover4);
        }
    }

    size_t uniformMatrixBufferSize = 3 * sizeof(glm::mat4);
    mGridUniformBuffer.init(uniformMatrixBufferSize);
    Logger::log(1, "%s: matrix uniform buffer (size %i bytes) successfully created\n", __FUNCTION__, uniformMatrixBufferSize);
    uniformMatrixBufferSize = 1 * sizeof(glm::vec3);
    mGridColorUniformBuffer.init(uniformMatrixBufferSize);
    Logger::log(1, "%s: matrix uniform buffer (size %i bytes) successfully created\n", __FUNCTION__, uniformMatrixBufferSize);

}

void Grid::drawGrid(Shader& gridShader, glm::mat4 viewMat, glm::mat4 projMat) 
{
//    glEnable(GL_BLEND);
//    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    gridShader.use();
    if (firstLoad) 
    {
        grid[0][0].LoadTexture("C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Assets/Textures/Cell.png");
		firstLoad = false;
    }

	grid[0][0].mTex.bind();
    for (int i = 0; i < GRID_SIZE; ++i) {
        for (int j = 0; j < GRID_SIZE; ++j) {
            glm::vec3 position = glm::vec3(i * CELL_SIZE, 0.0f, j * CELL_SIZE);
            glm::mat4 model = glm::translate(glm::mat4(1.0f), position);
            model = glm::scale(model, glm::vec3(CELL_SIZE, 1.0f, CELL_SIZE));
            std::vector<glm::mat4> matrixData;
            matrixData.push_back(viewMat);
            matrixData.push_back(projMat);
            matrixData.push_back(model);
            mGridUniformBuffer.uploadUboData(matrixData, 0);
			glm::vec3 cellColor = grid[i][j].GetColor();
            gridShader.setVec3("color", cellColor);
            // Render cell
			grid[i][j].BindVAO();
            glDrawArrays(GL_TRIANGLES, 0, 6);
        }
    }
    grid[0][0].mTex.unbind();
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

std::vector<glm::ivec2> Grid::findPath(const glm::ivec2& start, const glm::ivec2& goal, const std::vector<std::vector<Cell>>& grid) {
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
            if (grid[neighbor.x][neighbor.y].IsObstacle() || closedSet.count(neighbor))
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

