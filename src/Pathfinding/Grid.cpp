#include "Grid.h"
#include <queue>
#include <unordered_map>
#include "Logger.h"

void Grid::InitializeGrid()
{
	std::vector<Cell> row(GRID_SIZE, Cell(false, glm::vec3(0.0f, 1.0f, 0.0f)));
	grid = std::vector<std::vector<Cell>>(GRID_SIZE, row);

	for (glm::vec3 coverPos : SnapCoverPositionsToGrid())
	{
		int gridX = static_cast<int>(coverPos.x / CELL_SIZE);
		int gridZ = static_cast<int>(coverPos.z / CELL_SIZE);
		grid[gridX][gridZ].SetObstacle(true);
		grid[gridX][gridZ].SetColor(glm::vec3(1.0f, 0.0f, 0.0f));
		auto neighborPos = glm::vec3(1.0f);
		if (gridX + 1 <= GRID_SIZE)
		{
			neighborPos = SnapToGrid(glm::vec3((gridX + 1) * CELL_SIZE, coverPos.y, gridZ * CELL_SIZE));
			grid[gridX + 1][gridZ].SetCover(true);
			grid[gridX + 1][gridZ].SetColor(glm::vec3(1.0f, 0.4f, 0.0f));
			auto newCover1 = new Cover{neighborPos, &grid[gridX + 1][gridZ]};
			newCover1->m_gridX = gridX + 1;
			newCover1->m_gridZ = gridZ;
			m_coverLocations.push_back(newCover1);
		}
		if (gridZ + 1 <= GRID_SIZE)
		{
			neighborPos = SnapToGrid(glm::vec3(gridX * CELL_SIZE, coverPos.y, (gridZ + 1) * CELL_SIZE));
			grid[gridX][gridZ + 1].SetCover(true);
			grid[gridX][gridZ + 1].SetColor(glm::vec3(1.0f, 0.4f, 0.0f));
			auto newCover2 = new Cover{neighborPos, &grid[gridX][gridZ + 1]};
			newCover2->m_gridX = gridX;
			newCover2->m_gridZ = gridZ + 1;
			m_coverLocations.push_back(newCover2);
		}
		if (gridX - 1 >= 0)
		{
			neighborPos = SnapToGrid(glm::vec3((gridX - 1) * CELL_SIZE, coverPos.y, gridZ * CELL_SIZE));
			grid[gridX - 1][gridZ].SetCover(true);
			grid[gridX - 1][gridZ].SetColor(glm::vec3(1.0f, 0.4f, 0.0f));
			auto newCover3 = new Cover{neighborPos, &grid[gridX - 1][gridZ]};
			newCover3->m_gridX = gridX - 1;
			newCover3->m_gridZ = gridZ;
			m_coverLocations.push_back(newCover3);
		}
		if (gridZ - 1 >= 0)
		{
			neighborPos = SnapToGrid(glm::vec3(gridX * CELL_SIZE, coverPos.y, (gridZ - 1) * CELL_SIZE));
			grid[gridX][gridZ - 1].SetCover(true);
			grid[gridX][gridZ - 1].SetColor(glm::vec3(1.0f, 0.4f, 0.0f));
			auto newCover4 = new Cover{neighborPos, &grid[gridX][gridZ - 1]};
			newCover4->m_gridX = gridX;
			newCover4->m_gridZ = gridZ - 1;
			m_coverLocations.push_back(newCover4);
		}
	}

	size_t uniformMatrixBufferSize = 4 * sizeof(glm::mat4);
	mGridUniformBuffer.Init(uniformMatrixBufferSize);
	Logger::Log(1, "%s: matrix uniform buffer (size %i bytes) successfully created\n", __FUNCTION__,
	            uniformMatrixBufferSize);
	uniformMatrixBufferSize = 1 * sizeof(glm::vec3);
	mGridColorUniformBuffer.Init(uniformMatrixBufferSize);
	Logger::Log(1, "%s: matrix uniform buffer (size %i bytes) successfully created\n", __FUNCTION__,
	            uniformMatrixBufferSize);
}

void Grid::DrawGrid(Shader& gridShader, glm::mat4 viewMat, glm::mat4 projMat, glm::vec3 camPos, bool shadowMap,
                    glm::mat4 lightSpaceMat, GLuint shadowMapTexture,
                    glm::vec3 lightDir, glm::vec3 lightAmbient, glm::vec3 lightDiff, glm::vec3 lightSpec)
{
	if (!shadowMap)
	{
		gridShader.Use();
		if (firstLoad)
		{
			grid[0][0].LoadTexture(
				"src/Assets/Textures/Floor/TCom_Scifi_Floor2_4K_albedo.png",
				&grid[0][0].m_tex);
			grid[0][0].LoadTexture(
				"src/Assets/Textures/Floor/TCom_Scifi_Floor2_4K_normal.png",
				&grid[0][0].m_normal);
			grid[0][0].LoadTexture(
				"src/Assets/Textures/Floor/TCom_Scifi_Floor2_4K_metallic.png",
				&grid[0][0].m_metallic);
			grid[0][0].LoadTexture(
				"src/Assets/Textures/Floor/TCom_Scifi_Floor2_4K_roughness.png",
				&grid[0][0].m_roughness);
			grid[0][0].LoadTexture(
				"src/Assets/Textures/Floor/TCom_Scifi_Floor2_4K_ao.png",
				&grid[0][0].m_ao);
			grid[0][0].LoadTexture(
				"src/Assets/Textures/Floor/TCom_Scifi_Floor2_4K_emissive.png",
				&grid[0][0].m_emissive);
#ifdef DEBUG
#endif
			grid[0][0].LoadTexture("src/Assets/Textures/Cell.png",
			                       &grid[0][0].m_debugOutline);

			firstLoad = false;
		}

		grid[0][0].m_tex.Bind();
		gridShader.SetInt("albedoMap", 0);
		grid[0][0].m_normal.Bind(1);
		gridShader.SetInt("normalMap", 1);
		grid[0][0].m_metallic.Bind(2);
		gridShader.SetInt("metallicMap", 2);
		grid[0][0].m_roughness.Bind(3);
		gridShader.SetInt("roughnessMap", 3);
		grid[0][0].m_ao.Bind(4);
		gridShader.SetInt("aoMap", 4);
		grid[0][0].m_emissive.Bind(5);
		gridShader.SetInt("emissiveMap", 5);
		glActiveTexture(GL_TEXTURE6);
		glBindTexture(GL_TEXTURE_2D, shadowMapTexture);
		gridShader.SetInt("shadowMap", 6);

#ifdef DEBUG
		grid[0][0].m_debugOutline.Bind(7);
		gridShader.SetInt("debugOutline", 7);
#endif
	}
	gridShader.SetVec3("dirLight.direction", lightDir);
	gridShader.SetVec3("dirLight.ambient", lightAmbient);
	gridShader.SetVec3("dirLight.diffuse", lightDiff);
	gridShader.SetVec3("dirLight.specular", lightSpec);

	for (int i = 0; i < GRID_SIZE; ++i)
	{
		for (int j = 0; j < GRID_SIZE; ++j)
		{
			auto position = glm::vec3(i * CELL_SIZE, 0.0f, j * CELL_SIZE);
			glm::mat4 model = translate(glm::mat4(1.0f), position);
			model = scale(model, glm::vec3(CELL_SIZE, 1.0f, CELL_SIZE));
			std::vector<glm::mat4> matrixData;
			matrixData.push_back(viewMat);
			matrixData.push_back(projMat);
			matrixData.push_back(model);
			matrixData.push_back(lightSpaceMat);
			mGridUniformBuffer.UploadUboData(matrixData, 0);
			glm::vec3 cellColor = grid[i][j].GetColor();
#ifdef DEBUG
#endif
			gridShader.SetVec3("debugColor", cellColor);
			gridShader.SetVec3("cameraPos", camPos);
			// Render cell
			grid[i][j].BindVAO();
			glDrawArrays(GL_TRIANGLES, 0, 6);
		}
	}

	if (!shadowMap)
	{
		grid[0][0].m_tex.Unbind();
		grid[0][0].m_normal.Unbind();
		grid[0][0].m_metallic.Unbind();
		grid[0][0].m_roughness.Unbind();
		grid[0][0].m_ao.Unbind();
	}
}

#include <unordered_set>
#include <glm/gtx/hash.hpp>

struct Node
{
	glm::ivec2 position;
	float gCost;
	float hCost;
	Node* parent;

	float fCost() const { return gCost + hCost; }
	bool operator<(const Node& other) const { return fCost() > other.fCost(); }
};

float heuristic(const glm::ivec2& a, const glm::ivec2& b)
{
	return std::abs(a.x - b.x) + std::abs(a.y - b.y);
}

std::vector<glm::ivec2> Grid::FindPath(const glm::ivec2& start, const glm::ivec2& goal,
                                       const std::vector<std::vector<Cell>>& grid, int npcId)
{
	std::priority_queue<Node> openSet;
	std::unordered_map<glm::ivec2, Node, ivec2_hash> allNodes;
	std::unordered_set<glm::ivec2, ivec2_hash> closedSet;

	Node startNode = {start, 0.0f, heuristic(start, goal), nullptr};
	openSet.push(startNode);
	allNodes[start] = startNode;

	while (!openSet.empty())
	{
		Node current = openSet.top();
		openSet.pop();
		closedSet.insert(current.position);

		if (current.position == goal)
		{
			std::vector<glm::ivec2> path;
			for (Node* node = &current; node != nullptr; node = node->parent)
			{
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

		for (const glm::ivec2& neighbor : neighbors)
		{
			if (neighbor.x < 0 || neighbor.y < 0 || neighbor.x >= GRID_SIZE || neighbor.y >= GRID_SIZE)
				continue;
			if (grid[neighbor.x][neighbor.y].IsObstacle() || (grid[neighbor.x][neighbor.y].IsOccupied() && !grid[
				neighbor.x][neighbor.y].IsOccupiedBy(npcId)) || closedSet.contains(neighbor))
				continue;

			float tentativeGCost = current.gCost + 1.0f;
			if (!allNodes.contains(neighbor) || tentativeGCost < allNodes[neighbor].gCost)
			{
				Node neighborNode = {neighbor, tentativeGCost, heuristic(neighbor, goal), &allNodes[current.position]};
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

	if (grid[x][y].IsCover())
	{
		grid[x][y].SetColor(glm::vec3(1.0f, 0.0f, 1.0f));
	}
}

void Grid::VacateCell(int x, int y, int npcId)
{
	if (grid[x][y].IsObstacle() || !grid[x][y].IsOccupiedBy(npcId)) return;
	grid[x][y].SetOccupied(false);
	grid[x][y].SetOccupantId(-1);
	grid[x][y].SetColor(glm::vec3(0.0f, 1.0f, 0.0f));

	if (grid[x][y].IsCover())
	{
		grid[x][y].SetColor(glm::vec3(1.0f, 0.4f, 0.0f));
	}
}
