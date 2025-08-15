#pragma once
#include <unordered_map>
#include <iostream>
#include <fstream>
#include <sstream>

#include "Recast.h"
#include "DetourNavMeshBuilder.h"
#include "DetourNavMeshQuery.h"
#include "DetourCrowd.h"
#include "DetourCommon.h"
#include "DetourNavMesh.h"

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
#include "GameObjects/Crosshair.h"
#include "GameObjects/Line.h"
#include "GameObjects/Quad.h"
#include "GameObjects/Ground.h"
#include "Model/GltfModel.h"

#include "AI/Event.h"
#include "AI/Events.h"

#include "Scene/Scene.h"

class GameManager {
private:
	void SaveQTable(const std::unordered_map<std::pair<State, Action>, float, PairHash>& qTable, const std::string& filename) {
		std::ofstream outFile(filename, std::ios::app);
		if (!outFile) {
			std::cerr << "Error opening file for writing: " << filename << std::endl;
			return;
		}

		for (const auto& entry : qTable) {
			const State& state = entry.first.first;
			const Action& action = entry.first.second;
			float value = entry.second;

			// Save state, action, and Q-value as comma-separated values
			outFile << state.playerDetected << "," << state.playerVisible << "," << state.distanceToPlayer << ","
				<< state.isSuppressionFire << "," << state.health << ","
				<< action << "," << value << "\n";
		}
		outFile.close();
	}

	void LoadQTable(std::unordered_map<std::pair<State, Action>, float, PairHash>& qTable, const std::string& filename) {
		std::ifstream inFile(filename);
		if (!inFile) {
			std::cerr << "Error opening file for reading: " << filename << std::endl;
			return;
		}

		std::string line;
		while (getline(inFile, line)) {
			std::istringstream iss(line);
			State state;
			Action action;
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

	void InitializeQTable(std::unordered_map<std::pair<State, Action>, float, PairHash>& qTable) {
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
							State state = { playerDetected, playerVisible, distanceToPlayer, health, isSuppressionFire };
							for (auto action : { Action::ATTACK, Action::ADVANCE, Action::RETREAT, Action::PATROL }) {
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

		if (m_training)
		{
			for (int enemyID = 0; enemyID < 4; ++enemyID) {
				SaveQTable(m_enemyStateQTable[enemyID], std::to_string(enemyID) + m_enemyStateFilename);
			}
		}

		delete m_camera;
		for (auto it = m_gameObjects.begin(); it != m_gameObjects.end(); ) {
			if (*it) {
				delete* it;
			}
			it = m_gameObjects.erase(it);
		}
		delete m_inputManager;
	}

	void SetupCamera(unsigned int width, unsigned int height, float deltaTime);

	void SetSceneData();
	AudioSystem* GetAudioSystem() { return m_audioSystem; }

	void Update(float deltaTime);

	void Render(bool isMinimapRenderPass, bool isShadowMapRenderPass, bool isMainRenderPass);

	void SetUpDebugUi();
	void ShowDebugUi();
	void ShowCameraControlWindow(Camera& cam);
	void RenderDebugUi();


	PhysicsWorld* GetPhysicsWorld() const { return m_physicsWorld; }
	Camera* GetCamera() const { return m_camera; }
	AudioManager* GetAudioManager() const { return m_audioManager; }

	void CheckGameOver();
	void ResetGame();

	bool ShouldUseEDBT() const { return m_useEdbt; }
	void CreateLightSpaceMatrices();

	Enemy* GetEnemyByID(int id) {
		for (auto& enemy : m_enemies) {
			if (enemy->GetID() == id) {
				return enemy;
			}
		}
		return nullptr;
	};

	bool HasCamSwitchedToAim() const { return m_camSwitchedToAim; }
	void SetCamSwitchedToAim(bool val) { m_camSwitchedToAim = val; }

	Scene* GetActiveScene() { return m_activeScene; }

private:
	void RenderEnemyLineAndMuzzleFlash(bool isMainPass, bool isMinimapPass, bool isShadowPass);
	void RenderPlayerCrosshairAndMuzzleFlash(bool isMainPass);

	void ShowLightControlWindow(DirLight& light);
	void ShowPerformanceWindow();
	void ShowEnemyStateWindow();

	void CalculatePerformance(float deltaTime);

	EventManager& GetEventManager() { return m_eventManager; }

	bool BuildTile(int tx, int ty, float* bmin, float* bmax, rcConfig cfg, unsigned char*& navData, int* navDataSize, dtNavMeshParams parameters);
	void SetUpAndRenderNavMesh();
	std::vector<float> renderNavMeshVerts;

	float speedDivider = 1.0f;
	float blendFac = 1.0f;
	bool m_camSwitchedToAim = false;

	bool m_useEdbt = true;
	bool m_loadQTable = false;
	bool m_initializeQTable = false;
	bool m_training = false;
	std::string m_enemyStateFilename = "EnemyStateQTable.csv";
	std::unordered_map<std::pair<State, Action>, float, PairHash> m_enemyStateQTable[4];
	std::vector<State> m_enemyStates = {
	{ false, false, 100.0f, 100.0f, false },
	{ false, false, 100.0f, 100.0f, false },
	{ false, false, 100.0f, 100.0f, false },
	{ false, false, 100.0f, 100.0f, false }
	};

	std::vector<Action> m_squadActions =
	{
		Action::PATROL,
		Action::PATROL,
		Action::PATROL,
		Action::PATROL,
	};

	float m_decisionTimer = 0.0f;
	float m_decisionInterval = 0.5f;
	float m_dt;

	float m_fps = 0.0f;
	int m_numFramesAvg = 0;
	float m_fpsSum = 0.0f;
	int m_frameCount = 0;
	float m_frameTime = 0.0f;
	float m_elapsedTime = 0.0f;
	float m_avgFps = 0.0f;

	int m_screenWidth;
	int m_screenHeight;
	const int SHADOW_WIDTH = 4096;
	const int SHADOW_HEIGHT = 4096;

	float m_orthoLeft = -90.0f;
	float m_orthoRight = 90.0f;
	float m_orthoBottom = -90.0f;
	float m_orthoTop = 90.0f;
	float m_nearPlane = 1.0f;
	float m_farPlane = 300.0f;

	Renderer* m_renderer;
	Window* m_window;
	Camera* m_camera;
	Camera* m_minimapCamera;

	EventManager m_eventManager;
	InputManager* m_inputManager;
	AudioSystem* m_audioSystem;
	PhysicsWorld* m_physicsWorld;
	AudioManager* m_audioManager;

	std::vector<std::string> m_cubemapFaces;
	Cubemap* m_cubemap;
	Ground* ground;

	SoundEvent m_musicEvent;

	Player* m_player;
	Enemy* m_enemy;
	Enemy* m_enemy2;
	Enemy* m_enemy3;
	Enemy* m_enemy4;
	Crosshair* m_crosshair;
	Line* m_playerLine;
	std::vector<Line*> m_enemyLines;

	std::vector<GameObject*> m_gameObjects;
	std::vector<Enemy*> m_enemies;

	Quad* m_minimapQuad;
	Quad* m_shadowMapQuad;
	Quad* m_playerMuzzleFlashQuad;
	Quad* m_enemyMuzzleFlashQuad;
	Quad* m_enemy2MuzzleFlashQuad;
	Quad* m_enemy3MuzzleFlashQuad;
	Quad* m_enemy4MuzzleFlashQuad;

	Quad* m_playerTracerQuad;
	Quad* m_enemyTracerQuad;
	Quad* m_enemy2TracerQuad;
	Quad* m_enemy3TracerQuad;
	Quad* m_enemy4TracerQuad;
	Shader playerShader{};
	Shader groundShader{};
	Shader groundShadowShader{};
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

	Shader m_playerShader{};
	Shader m_enemyShader{};
	Shader m_gridShader{};
	Shader m_gridDebugShader{};
	Shader m_crosshairShader{};
	Shader m_lineShader{};
	Shader m_cubeShader{};
	Shader m_cubemapShader{};
	Shader m_minimapShader{};
	Shader m_shadowMapShader{};
	Shader m_playerShadowMapShader{};
	Shader m_enemyShadowMapShader{};
	Shader m_shadowMapQuadShader{};
	Shader m_playerMuzzleFlashShader{};
	Shader m_playerTracerShader{};

	ShaderStorageBuffer m_playerSsBuffer{};
	ShaderStorageBuffer m_enemySsBuffer{};
	ShaderStorageBuffer m_playerDualQuatSsBuffer{};
	ShaderStorageBuffer m_enemyDualQuatSsBuffer{};

	size_t m_playerJointMatrixSize;
	size_t m_enemyJointMatrixSize;
	std::vector<glm::mat2x4> m_jointDualQuatsVec;

	bool m_renderPlayerMuzzleFlash = false;
	float m_playerMuzzleFlashStartTime = 0.0f;
	float m_playerMuzzleTimeSinceStart = 0.0f;
	float m_playerMuzzleFlashDuration = 0.05f;
	float m_playerMuzzleAlpha = 0.0f;
	glm::vec3 m_playerMuzzleTint = { 1.0f, 1.0f, 1.0f };
	float m_playerMuzzleFlashScale = 1.0f;
	glm::mat4 m_playerMuzzleModel = glm::mat4(1.0f);

	std::vector<bool> m_renderEnemyMuzzleFlash = { false, false, false, false };
	std::vector<float> m_enemyMuzzleFlashStartTimes = { 0.0f, 0.0f, 0.0f, 0.0f };
	std::vector<float> m_enemyMuzzleTimesSinceStart = { 0.0f, 0.0f, 0.0f, 0.0f };
	std::vector<float> m_enemyMuzzleFlashDurations = { 0.1f, 0.1f, 0.1f, 0.1f };
	std::vector<float> m_enemyMuzzleAlphas = { 0.0f, 0.0f, 0.0f, 0.0f };
	std::vector<glm::vec3> m_enemyMuzzleFlashTints = { {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f, 1.0f} };
	std::vector<float> m_enemyMuzzleFlashScales = { 1.0f, 1.0f, 1.0f, 1.0f };
	std::vector<glm::mat4> m_enemyMuzzleModelMatrices = { glm::mat4(1.0f), glm::mat4(1.0f), glm::mat4(1.0f), glm::mat4(1.0f) };
	glm::vec3 m_enemyMuzzleFlashOffsets = glm::vec3(0.0f);
	float m_muzzleOffset = 2.4f;

	std::vector<bool> m_renderEnemyTracer = { false, false, false, false };
	std::vector<float> m_enemyTracerStartTimes = { 0.0f, 0.0f, 0.0f, 0.0f };
	std::vector<float> m_enemyTracerTimesSinceStart = { 0.0f, 0.0f, 0.0f, 0.0f };
	std::vector<float> m_enemyTracerDurations = { 0.1f, 0.1f, 0.1f, 0.1f };
	std::vector<float> m_enemyTracerAlphas = { 0.0f, 0.0f, 0.0f, 0.0f };
	std::vector<glm::vec3> m_enemyTracerTints = { {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f, 1.0f} };
	std::vector<float> m_enemyTracerScales = { 1.0f, 1.0f, 1.0f, 1.0f };
	std::vector<glm::mat4> m_enemyTracerModelMatrices = { glm::mat4(1.0f), glm::mat4(1.0f), glm::mat4(1.0f), glm::mat4(1.0f) };
	glm::vec3 m_enemyTracerOffsets = glm::vec3(0.0f);
	float m_tracerOffset = 2.4f;

	glm::mat4 m_view = glm::mat4(1.0f);
	glm::mat4 m_projection = glm::mat4(1.0f);
	glm::mat4 m_cubemapView = glm::mat4(1.0f);
	glm::mat4 m_minimapView = glm::mat4(1.0f);
	glm::mat4 m_minimapProjection = glm::mat4(1.0f);
	glm::mat4 m_lightSpaceView = glm::mat4(1.0f);
	glm::mat4 m_lightSpaceProjection = glm::mat4(1.0f);
	glm::mat4 m_lightSpaceMatrix = glm::mat4(1.0f);

	bool m_firstFlyCamSwitch = true;
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


	std::vector<std::string> cubemapFaces;
	Cubemap* cubemap;

	int currentStateIndex = 0;

	SoundEvent mMusicEvent;

	bool firstFlyCamSwitch = true;

	AudioManager* mAudioManager;

	std::vector<float> navMeshVertices;
	std::vector<unsigned int> navMeshIndices;
	std::vector<glm::vec3> mapVerts;
	int* triIndices;
	unsigned char* triAreas;
	std::vector<glm::vec3> mapVertices;

	rcContext ctx;
	std::vector<rcHeightfield*> heightFields;
	std::vector<rcCompactHeightfield*> compactHeightFields;
	std::vector<rcContourSet*> contourSets;
	std::vector<rcPolyMesh*> polyMeshes;
	std::vector<rcPolyMeshDetail*> polyMeshDetails;
	dtCrowd* crowd;
	std::vector<int> enemyAgentIDs;
	float* targetPosOnNavMesh;
	dtQueryFilter filter;
	const float halfExtents[3] = { 500.0f, 50.0f, 500.0f };

	dtNavMesh* navMesh;
	dtNavMeshQuery* navMeshQuery;
	unsigned char* navData;
	int navDataSize;
	float snappedPos[3];
	float tileWorldSize;

	GLuint vao, vbo, ebo;
	Shader navMeshShader{};
	std::vector<float> navRenderMeshVertices;
	std::vector<unsigned int> navRenderMeshIndices;

	GLuint hfvao, hfvbo, hfebo;
	Shader hfnavMeshShader{};
	std::vector<float> hfnavRenderMeshVertices;
	std::vector<unsigned int> hfnavRenderMeshIndices; 

	glm::vec3 mapScale = glm::vec3(5.0f);
	glm::vec3 mapPos = glm::vec3(0, 0.f, 0.0f);

	Scene* m_activeScene;
};

struct DebugVertex {
	glm::vec3 position;
	glm::vec3 color;
};	