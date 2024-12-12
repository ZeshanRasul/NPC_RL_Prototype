#pragma once
#include <unordered_map>
#include <iostream>
#include <fstream>
#include <sstream>

#include "Recast.h"
#include "DetourNavMesh.h"
#include "DetourNavMeshBuilder.h"
#include "DetourNavMeshQuery.h"

#include "src/OpenGL/Renderer.h"
#include "src/OpenGL/RenderData.h"
#include "src/OpenGL/ShaderStorageBuffer.h"
#include "src/OpenGL/Cubemap.h"
#include "src/Window/Window.h"

#include "src/InputManager.h"
#include "Audio/AudioSystem.h"
#include "Audio/SoundEvent.h"
#include "Audio/AudioManager.h"
#include "Physics/PhysicsWorld.h"

#include "src/Camera.h"
#include "GameObjects/Player.h"
#include "GameObjects/Enemy.h"
#include "GameObjects/Waypoint.h"
#include "GameObjects/Crosshair.h"
#include "GameObjects/Line.h"
#include "GameObjects/Cube.h"
#include "GameObjects/Quad.h"
#include "Model/GltfModel.h"
#include "src/Pathfinding/Grid.h"

#include "AI/Event.h"
#include "AI/Events.h"

class GameManager {
private:
	void SaveQTable(const std::unordered_map<std::pair<NashState, NashAction>, float, PairHash>& qTable, const std::string& filename) {
		std::ofstream outFile(filename, std::ios::app);
		if (!outFile) {
			std::cerr << "Error opening file for writing: " << filename << std::endl;
			return;
		}

		for (const auto& entry : qTable) {
			const NashState& state = entry.first.first;
			const NashAction& action = entry.first.second;
			float value = entry.second;

			// Save state, action, and Q-value as comma-separated values
			outFile << state.playerDetected << "," << state.playerVisible << "," << state.distanceToPlayer << ","
				<< state.isSuppressionFire << "," << state.health << ","
				<< action << "," << value << "\n";
		}
		outFile.close();
	}

	void LoadQTable(std::unordered_map<std::pair<NashState, NashAction>, float, PairHash>& qTable, const std::string& filename) {
		std::ifstream inFile(filename);
		if (!inFile) {
			std::cerr << "Error opening file for reading: " << filename << std::endl;
			return;
		}

		std::string line;
		while (getline(inFile, line)) {
			std::istringstream iss(line);
			NashState state;
			NashAction action;
			float value;
			char comma;

			// Parse state values
			iss >> state.playerDetected >> comma
				>> state.playerVisible >> comma
				>> state.distanceToPlayer >> comma
				>> state.isSuppressionFire >> comma
				>> state.health >> comma
				>> (int&)action >> comma
				>> value;

			// Load the Q-value into the Q-table
			qTable[{state, action}] = value;
		}
		inFile.close();
	}

	void InitializeQTable(std::unordered_map<std::pair<NashState, NashAction>, float, PairHash>& qTable) {
		std::random_device rd;
		std::mt19937 gen(rd());
		std::uniform_real_distribution<float> dist(-0.1f, 0.1f);

		std::vector<float> distances = { 0.0f, 10.0f, 20.0f, 30.0f, 40.0f, 50.0f, 60.0f, 70.0f, 80.0f, 90.0f, 100.0f };
		std::vector<float> healthLevels = { 20.0f, 40.0f, 60.0f, 80.0f, 100.0f };

		for (bool playerDetected : {true, false}) {
			for (bool playerVisible : {true, false}) {
				for (float distanceToPlayer : distances) {
					for (float health : healthLevels) {
						for (bool isSuppressionFire : {true, false}) {
							NashState state = { playerDetected, playerVisible, distanceToPlayer, health, isSuppressionFire };
							for (auto action : { NashAction::ATTACK, NashAction::ADVANCE, NashAction::RETREAT, NashAction::PATROL }) {
								qTable[{state, action}] = dist(gen);  // Assign random initial Q-value
							}
						}
					}
				}
			}
		}
	}


public:
	GameManager(Window* window, unsigned int width, unsigned int height);

	~GameManager() {

		if (training)
		{
			for (int enemyID = 0; enemyID < 4; ++enemyID) {
				SaveQTable(mEnemyStateQTable[enemyID], std::to_string(enemyID) + mEnemyStateFilename);
			}
		}

		delete camera;
		for (auto it = gameObjects.begin(); it != gameObjects.end(); ) {
			if (*it) {
				delete* it;
			}
			it = gameObjects.erase(it);
		}
		delete inputManager;
	}

	void setupCamera(unsigned int width, unsigned int height);
	void setSceneData();
	AudioSystem* getAudioSystem() { return audioSystem; }

	void update(float deltaTime);

	void render(bool isMinimapRenderPass, bool isShadowMapRenderPass, bool isMainRenderPass);

	void setUpDebugUI();
	void showDebugUI();
	void renderDebugUI();

	bool camSwitchedToAim = false;

	PhysicsWorld* GetPhysicsWorld() { return physicsWorld; }
	Camera* GetCamera() { return camera; }

	void RemoveDestroyedGameObjects();

	void ResetGame();

	bool ShouldUseEDBT() const { return useEDBT; }
	void CreateLightSpaceMatrices();

	Enemy* GetEnemyByID(int id) {
		for (auto& enemy : enemies) {
			if (enemy->GetID() == id) {
				return enemy;
			}
		}
		return nullptr;
	};

	AudioManager* GetAudioManager() { return mAudioManager; }

private:
	void ShowCameraControlWindow(Camera& cam);
	void ShowLightControlWindow(DirLight& light);
	void ShowAnimationControlWindow();
	void ShowPerformanceWindow();
	void ShowEnemyStateWindow();

	void calculatePerformance(float deltaTime);

	EventManager& GetEventManager() { return eventManager; }

	void SetUpAndRenderNavMesh();
	std::vector<float> renderNavMeshVerts;

	float speedDivider = 1.0f;
	float blendFac = 1.0f;

	bool useEDBT = true;
	bool loadQTable = false;
	bool initializeQTable = false;
	bool training = false;
	std::string mEnemyStateFilename = "EnemyStateQTable.csv";
	std::unordered_map<std::pair<NashState, NashAction>, float, PairHash> mEnemyStateQTable[4];
	std::vector<NashState> enemyStates = {
	{ false, false, 100.0f, 100.0f, false },
	{ false, false, 100.0f, 100.0f, false },
	{ false, false, 100.0f, 100.0f, false },
	{ false, false, 100.0f, 100.0f, false }
	};

	std::vector<NashAction> squadActions =
	{
		NashAction::PATROL,
		NashAction::PATROL,
		NashAction::PATROL,
		NashAction::PATROL,
	};

	float decisionTimer = 0.0f;
	float decisionInterval = 0.5f;

	float fps = 0.0f;
	int numFramesAvg = 100;
	float fpsSum = 0.0f;
	int frameCount = 0;
	float frameTime = 0.0f;
	float elapsedTime = 0.0f;
	float avgFPS = 0.0f;

	int screenWidth;
	int screenHeight;
	const int SHADOW_WIDTH = 4096;
	const int SHADOW_HEIGHT = 4096;

	float orthoLeft = -90.0f;
	float orthoRight = 90.0f;
	float orthoBottom = -90.0f;
	float orthoTop = 90.0f;
	float near_plane = 1.0f;
	float far_plane = 300.0f;

	Renderer* renderer;
	Window* window;
	Camera* camera;
	Camera* minimapCamera;

	EventManager eventManager;

	Player* player;
	Enemy* enemy;
	Enemy* enemy2;
	Enemy* enemy3;
	Enemy* enemy4;
	Crosshair* crosshair;
	Line* line;
	Line* enemyLine;
	Line* enemy2Line;
	Line* enemy3Line;
	Line* enemy4Line;

	std::vector<Cube*> coverSpots;

	Quad* minimapQuad;
	Quad* shadowMapQuad;
	Quad* playerMuzzleFlashQuad;
	Quad* enemyMuzzleFlashQuad;
	Quad* enemy2MuzzleFlashQuad;
	Quad* enemy3MuzzleFlashQuad;
	Quad* enemy4MuzzleFlashQuad;

	InputManager* inputManager;
	AudioSystem* audioSystem;
	PhysicsWorld* physicsWorld;

	std::vector<GameObject*> gameObjects;
	std::vector<Enemy*> enemies;

	Shader playerShader{};
	Shader enemyShader{};
	Shader gridShader{};
	Shader crosshairShader{};
	Shader lineShader{};
	Shader aabbShader{};
	Shader cubeShader{};
	Shader cubemapShader{};
	Shader minimapShader{};
	Shader shadowMapShader{};
	Shader playerShadowMapShader{};
	Shader enemyShadowMapShader{};
	Shader shadowMapQuadShader{};
	Shader playerMuzzleFlashShader{};

	ShaderStorageBuffer mPlayerSSBuffer{};
	ShaderStorageBuffer mEnemySSBuffer{};
	ShaderStorageBuffer mPlayerDualQuatSSBuffer{};
	ShaderStorageBuffer mEnemyDualQuatSSBuffer{};

	size_t mPlayerJointMatrixSize;
	size_t mEnemyJointMatrixSize;
	std::vector<glm::mat2x4> playerJointDualQuatsVec;

	float playerAnimBlendFactor = 1.0f;
	bool playerCrossBlend = false;
	int playerCrossBlendSourceClip = 0;
	int playerCrossBlendDestClip = 8;
	float playerAnimCrossBlendFactor = 0.0f;
	bool playerAdditiveBlend = false;
	int playerSkeletonSplitNode = 0;
	std::string playerSkeletonSplitNodeName = "None";

	float enemyAnimBlendFactor = 1.0f;
	bool enemyCrossBlend = false;
	int enemyCrossBlendSourceClip = 1;
	int enemyCrossBlendDestClip = 7;
	float enemyAnimCrossBlendFactor = 0.0f;
	bool enemyAdditiveBlend = false;
	int enemySkeletonSplitNode = 0;
	std::string enemySkeletonSplitNodeName = "None";


	bool renderPlayerMuzzleFlash = false;
	float playerMuzzleFlashStartTime = 0.0f;
	float playerMuzzleTimeSinceStart = 0.0f;
	float playerMuzzleFlashDuration = 0.1f;
	float playerMuzzleAlpha = 0.0f;
	glm::vec3 playerMuzzleTint = { 1.0f, 1.0f, 1.0f };
	float playerMuzzleFlashScale = 1.0f;
	glm::mat4 playerMuzzleModel = glm::mat4(1.0f);

	bool renderEnemyMuzzleFlash = false;
	float enemyMuzzleFlashStartTime = 0.0f;
	float enemyMuzzleTimeSinceStart = 0.0f;
	float enemyMuzzleFlashDuration = 0.1f;
	float enemyMuzzleAlpha = 0.0f;
	glm::vec3 enemyMuzzleTint = { 1.0f, 1.0f, 1.0f };
	float enemyMuzzleFlashScale = 1.0f;
	glm::mat4 enemyMuzzleModel = glm::mat4(1.0f);

	bool renderEnemy2MuzzleFlash = false;
	float enemy2MuzzleFlashStartTime = 0.0f;
	float enemy2MuzzleTimeSinceStart = 0.0f;
	float enemy2MuzzleFlashDuration = 0.1f;
	float enemy2MuzzleAlpha = 0.0f;
	glm::vec3 enemy2MuzzleTint = { 1.0f, 1.0f, 1.0f };
	float enemy2MuzzleFlashScale = 1.0f;
	glm::mat4 enemy2MuzzleModel = glm::mat4(1.0f);

	bool renderEnemy3MuzzleFlash = false;
	float enemy3MuzzleFlashStartTime = 0.0f;
	float enemy3MuzzleTimeSinceStart = 0.0f;
	float enemy3MuzzleFlashDuration = 0.1f;
	float enemy3MuzzleAlpha = 0.0f;
	glm::vec3 enemy3MuzzleTint = { 1.0f, 1.0f, 1.0f };
	float enemy3MuzzleFlashScale = 1.0f;
	glm::mat4 enemy3MuzzleModel = glm::mat4(1.0f);

	bool renderEnemy4MuzzleFlash = false;
	float enemy4MuzzleFlashStartTime = 0.0f;
	float enemy4MuzzleTimeSinceStart = 0.0f;
	float enemy4MuzzleFlashDuration = 0.1f;
	float enemy4MuzzleAlpha = 0.0f;
	glm::vec3 enemy4MuzzleTint = { 1.0f, 1.0f, 1.0f };
	float enemy4MuzzleFlashScale = 1.0f;
	glm::mat4 enemy4MuzzleModel = glm::mat4(1.0f);


	glm::mat4 view = glm::mat4(1.0f);
	glm::mat4 projection = glm::mat4(1.0f);
	glm::mat4 cubemapView = glm::mat4(1.0f);
	glm::mat4 minimapView = glm::mat4(1.0f);
	glm::mat4 minimapProjection = glm::mat4(1.0f);
	glm::mat4 lightSpaceView = glm::mat4(1.0f);
	glm::mat4 lightSpaceProjection = glm::mat4(1.0f);
	glm::mat4 lightSpaceMatrix = glm::mat4(1.0f);


	Cell* cell;
	Grid* gameGrid;

	std::vector<std::string> cubemapFaces;
	Cubemap* cubemap;

	Waypoint* waypoint1;
	Waypoint* waypoint2;
	Waypoint* waypoint3;
	Waypoint* waypoint4;

	int currentStateIndex = 0;

	SoundEvent mMusicEvent;

	bool firstFlyCamSwitch = true;

	AudioManager* mAudioManager;

	std::vector<float> navMeshVertices;
	std::vector<unsigned int> navMeshIndices;
	int* triIndices;
	unsigned char* triAreas;

	rcContext* ctx;
	rcHeightfield* heightField;
	rcCompactHeightfield* compactHeightField;
	rcContourSet* contourSet;
	rcPolyMesh* polyMesh;
	rcPolyMeshDetail* polyMeshDetail;


	dtNavMesh* navMesh;
	dtNavMeshQuery* navMeshQuery = nullptr;
	unsigned char* navData;
	int navDataSize;

	GLuint vao, vbo, ebo;
	Shader navMeshShader{};
	std::vector<float> navmeshVertices;
	std::vector<unsigned int> navmeshIndices; // Use unsigned int for consistency

};
