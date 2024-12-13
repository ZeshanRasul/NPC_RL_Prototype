#include "Grid.h"
#include <queue>
#include <unordered_map>
#include "Logger.h"

void Grid::initializeGrid() {

	std::vector<Cell> row(GRID_SIZE, Cell(false, glm::vec3(0.0f, 1.0f, 0.0f)));
	grid = std::vector<std::vector<Cell>>(GRID_SIZE, row);

	std::vector<glm::vec3> cellVerts = grid[0][0].GetVertices();

	for (int i = 0; i < GRID_SIZE; ++i) {
		for (int j = 0; j < GRID_SIZE; ++j) {
			glm::vec3 position = glm::vec3(i * CELL_SIZE, 0.0f, j * CELL_SIZE);
			glm::mat4 model = glm::translate(glm::mat4(1.0f), position);
			model = glm::scale(model, glm::vec3(CELL_SIZE, 1.0f, CELL_SIZE));
			

			for (glm::vec3 posVerts : cellVerts) 
			{
				wsVertices.push_back(glm::vec3(model * glm::vec4(posVerts, 1.0f)));
				indices.push_back(0);
				indices.push_back(1);
				indices.push_back(2);
				indices.push_back(3);
				indices.push_back(4);
				indices.push_back(5);
			}
		}
	}

	for (glm::vec3 coverPos : snapCoverPositionsToGrid())
	{
		int gridX = static_cast<int>(coverPos.x / CELL_SIZE);
		int gridZ = static_cast<int>(coverPos.z / CELL_SIZE);
		grid[gridX][gridZ].SetObstacle(true);
		grid[gridX][gridZ].SetColor(glm::vec3(1.0f, 0.0f, 0.0f));
		glm::vec3 neighborPos = glm::vec3(1.0f);
		if (gridX + 1 <= GRID_SIZE) {
			neighborPos = snapToGrid(glm::vec3((gridX + 1) * CELL_SIZE, coverPos.y, gridZ * CELL_SIZE));
			grid[gridX + 1][gridZ].SetCover(true);
			grid[gridX + 1][gridZ].SetColor(glm::vec3(1.0f, 0.4f, 0.0f));
			Cover* newCover1 = new Cover{ neighborPos, &grid[gridX + 1][gridZ] };
			newCover1->gridX = gridX + 1;
			newCover1->gridZ = gridZ;
			coverLocations.push_back(newCover1);
		}
		if (gridZ + 1 <= GRID_SIZE) {
			neighborPos = snapToGrid(glm::vec3(gridX * CELL_SIZE, coverPos.y, (gridZ + 1) * CELL_SIZE));
			grid[gridX][gridZ + 1].SetCover(true);
			grid[gridX][gridZ + 1].SetColor(glm::vec3(1.0f, 0.4f, 0.0f));
			Cover* newCover2 = new Cover{ neighborPos, &grid[gridX][gridZ + 1] };
			newCover2->gridX = gridX;
			newCover2->gridZ = gridZ + 1;
			coverLocations.push_back(newCover2);

		}
		if (gridX - 1 >= 0) {
			neighborPos = snapToGrid(glm::vec3((gridX - 1) * CELL_SIZE, coverPos.y, gridZ * CELL_SIZE));
			grid[gridX - 1][gridZ].SetCover(true);
			grid[gridX - 1][gridZ].SetColor(glm::vec3(1.0f, 0.4f, 0.0f));
			Cover* newCover3 = new Cover{ neighborPos, &grid[gridX - 1][gridZ] };
			newCover3->gridX = gridX - 1;
			newCover3->gridZ = gridZ;
			coverLocations.push_back(newCover3);
		}
		if (gridZ - 1 >= 0) {
			neighborPos = snapToGrid(glm::vec3(gridX * CELL_SIZE, coverPos.y, (gridZ - 1) * CELL_SIZE));
			grid[gridX][gridZ - 1].SetCover(true);
			grid[gridX][gridZ - 1].SetColor(glm::vec3(1.0f, 0.4f, 0.0f));
			Cover* newCover4 = new Cover{ neighborPos, &grid[gridX][gridZ - 1] };
			newCover4->gridX = gridX;
			newCover4->gridZ = gridZ - 1;
			coverLocations.push_back(newCover4);
		}
	}

	size_t uniformMatrixBufferSize = 4 * sizeof(glm::mat4);
	mGridUniformBuffer.init(uniformMatrixBufferSize);
	Logger::log(1, "%s: matrix uniform buffer (size %i bytes) successfully created\n", __FUNCTION__, uniformMatrixBufferSize);
	uniformMatrixBufferSize = 1 * sizeof(glm::vec3);
	mGridColorUniformBuffer.init(uniformMatrixBufferSize);
	Logger::log(1, "%s: matrix uniform buffer (size %i bytes) successfully created\n", __FUNCTION__, uniformMatrixBufferSize);

}

void Grid::drawGrid(Shader& gridShader, glm::mat4 viewMat, glm::mat4 projMat, glm::vec3 camPos, bool shadowMap, glm::mat4 lightSpaceMat, GLuint shadowMapTexture)
{
	//    glEnable(GL_BLEND);
	//    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	if (!shadowMap)
	{
		gridShader.use();
		if (firstLoad)
		{
			grid[0][0].LoadTexture("C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Assets/Textures/Floor/TCom_Scifi_Floor2_4K_albedo.png", &grid[0][0].mTex);
			grid[0][0].LoadTexture("C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Assets/Textures/Floor/TCom_Scifi_Floor2_4K_normal.png", &grid[0][0].mNormal);
			grid[0][0].LoadTexture("C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Assets/Textures/Floor/TCom_Scifi_Floor2_4K_metallic.png", &grid[0][0].mMetallic);
			grid[0][0].LoadTexture("C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Assets/Textures/Floor/TCom_Scifi_Floor2_4K_roughness.png", &grid[0][0].mRoughness);
			grid[0][0].LoadTexture("C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Assets/Textures/Floor/TCom_Scifi_Floor2_4K_ao.png", &grid[0][0].mAO);
			grid[0][0].LoadTexture("C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Assets/Textures/Floor/TCom_Scifi_Floor2_4K_emissive.png", &grid[0][0].mEmissive);
#ifdef DEBUG
#endif
			grid[0][0].LoadTexture("C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Assets/Textures/Cell.png", &grid[0][0].mDebugOutline);

			firstLoad = false;
		}

		grid[0][0].mTex.bind();
		gridShader.setInt("albedoMap", 0);
		grid[0][0].mNormal.bind(1);
		gridShader.setInt("normalMap", 1);
		grid[0][0].mMetallic.bind(2);
		gridShader.setInt("metallicMap", 2);
		grid[0][0].mRoughness.bind(3);
		gridShader.setInt("roughnessMap", 3);
		grid[0][0].mAO.bind(4);
		gridShader.setInt("aoMap", 4);
		grid[0][0].mEmissive.bind(5);
		gridShader.setInt("emissiveMap", 5);
		glActiveTexture(GL_TEXTURE6);
		glBindTexture(GL_TEXTURE_2D, shadowMapTexture);
		gridShader.setInt("shadowMap", 6);

#ifdef DEBUG
#endif
		grid[0][0].mDebugOutline.bind(7);
		gridShader.setInt("debugOutline", 7);
	}
	for (int i = 0; i < GRID_SIZE; ++i) {
		for (int j = 0; j < GRID_SIZE; ++j) {
			glm::vec3 position = glm::vec3(i * CELL_SIZE, 0.0f, j * CELL_SIZE);
			glm::mat4 model = glm::translate(glm::mat4(1.0f), position);
			model = glm::scale(model, glm::vec3(CELL_SIZE, 1.0f, CELL_SIZE));
			std::vector<glm::mat4> matrixData;
			matrixData.push_back(viewMat);
			matrixData.push_back(projMat);
			matrixData.push_back(model);
			matrixData.push_back(lightSpaceMat);
			mGridUniformBuffer.uploadUboData(matrixData, 0);
			glm::vec3 cellColor = grid[i][j].GetColor();
#ifdef DEBUG
#endif
			gridShader.setVec3("debugColor", cellColor);
			gridShader.setVec3("cameraPos", camPos);
			// Render cell
			grid[i][j].BindVAO();
			glDrawArrays(GL_TRIANGLES, 0, 6);
		}
	}

	if (!shadowMap)
	{
		grid[0][0].mTex.unbind();
		grid[0][0].mNormal.unbind();
		grid[0][0].mMetallic.unbind();
		grid[0][0].mRoughness.unbind();
		grid[0][0].mAO.unbind();
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
	return std::abs(a.x - b.x) + std::abs(a.y - b.y);
}

std::vector<glm::ivec2> Grid::findPath(const glm::ivec2& start, const glm::ivec2& goal, const std::vector<std::vector<Cell>>& grid, int npcId) {
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
			if (grid[neighbor.x][neighbor.y].IsObstacle() || (grid[neighbor.x][neighbor.y].IsOccupied() && !grid[neighbor.x][neighbor.y].IsOccupiedBy(npcId)) || closedSet.count(neighbor))
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

void Grid::OccupyCell(int x, int y, int npcId)
{
	if (grid[x][y].IsObstacle() || grid[x][y].IsOccupied()) return;
	grid[x][y].SetOccupied(true);
	grid[x][y].SetOccupantId(npcId);
	grid[x][y].SetColor(glm::vec3(0.0f, 0.0f, 1.0f));

	if (grid[x][y].IsCover()) {
		grid[x][y].SetColor(glm::vec3(1.0f, 0.0f, 1.0f));
	}
}

void Grid::VacateCell(int x, int y, int npcId)
{
	if (grid[x][y].IsObstacle() || !grid[x][y].IsOccupiedBy(npcId)) return;
	grid[x][y].SetOccupied(false);
	grid[x][y].SetOccupantId(-1);
	grid[x][y].SetColor(glm::vec3(0.0f, 1.0f, 0.0f));

	if (grid[x][y].IsCover()) {
		grid[x][y].SetColor(glm::vec3(1.0f, 0.4f, 0.0f));
	}
}



