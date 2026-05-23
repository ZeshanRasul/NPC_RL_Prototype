#pragma once
#include <unordered_map>
#include <iostream>
#include <fstream>
#include <sstream>
#include <memory>

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
#include "NavMesh/NavMeshManager.h"
#include "LightingSystem.h"

class GameManager {
private:
#ifdef NPC_RL_QLEARNING
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
#endif // NPC_RL_QLEARNING

public:
	GameManager(Window* window, unsigned int width, unsigned int height);

	~GameManager() {

#ifdef NPC_RL_QLEARNING
		if (m_training)
		{
			for (int enemyID = 0; enemyID < 4; ++enemyID) {
				SaveQTable(m_enemyStateQTable[enemyID], std::to_string(enemyID) + m_enemyStateFilename);
			}
		}
#endif // NPC_RL_QLEARNING

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
	void RenderDebugUi();


	PhysicsWorld* GetPhysicsWorld() const { return m_physicsWorld; }
	Camera* GetCamera() const { return m_camera; }
	AudioManager* GetAudioManager() const { return m_audioManager; }

	void CheckGameOver();
	void ResetGame();

	bool ShouldUseEDBT() const { return m_useEdbt; }
	void CreateLightSpaceMatrices() { m_lighting.UpdateLightSpaceMatrix(); }

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

private:
	void RenderEnemyLineAndMuzzleFlash(bool isMainPass, bool isMinimapPass, bool isShadowPass);
	void RenderPlayerCrosshairAndMuzzleFlash(bool isMainPass);

	void ShowSceneOutliner();
	void ShowEntityInspector();
	void ShowLightingPanel();
	void ShowCameraPanel();
	void ShowAIDebugPanel();
	void ShowPerformanceWindow();

	void SaveSceneSettings();
	void LoadSceneSettings();

	void CalculatePerformance(float deltaTime);

	EventManager& GetEventManager() { return m_eventManager; }

	void SetUpAndRenderNavMesh();

	float speedDivider = 1.0f;
	float blendFac = 1.0f;
	bool m_camSwitchedToAim = false;
	int  m_selectedEnemyIndex = -1;
	bool m_editMode    = false;
	bool m_showNavMesh = true;

	bool m_useEdbt = true;

#ifdef NPC_RL_QLEARNING
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
#endif // NPC_RL_QLEARNING

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

	LightingSystem m_lighting;

	Renderer* m_renderer;
	Window* m_window;
	Camera* m_camera;

	EventManager m_eventManager;
	InputManager* m_inputManager;
	AudioSystem* m_audioSystem;
	PhysicsWorld* m_physicsWorld;
	AudioManager* m_audioManager;

	std::vector<std::string> m_cubemapFaces;
	Cubemap* m_cubemap;
	Ground* ground;

	Player* m_player;
	Enemy* m_enemy;
	Enemy* m_enemy2;
	Enemy* m_enemy3;
	Enemy* m_enemy4;
	Enemy* m_enemy5;
	Enemy* m_enemy6;
	Enemy* m_enemy7;
	Enemy* m_enemy8;
	Crosshair* m_crosshair;
	Line* m_playerLine;
	std::vector<Line*> m_enemyLines;

	std::vector<GameObject*> m_gameObjects;
	std::vector<Enemy*> m_enemies;

	Quad* m_minimapQuad;
	Quad* m_shadowMapQuad;
	Quad* m_playerMuzzleFlashQuad;
	Quad* m_enemyMuzzleFlashQuad;
	Quad* m_enemyTracerQuad;
	Shader playerShader{};
	Shader groundShader{};
	Shader groundShadowShader{};
	Shader enemyShader{};
	Shader enemyShader2{};
	Shader crosshairShader{};
	Shader lineShader{};
	Shader aabbShader{};
	Shader shadowMapShader{};
	Shader playerShadowMapShader{};
	Shader enemyShadowMapShader{};
	Shader playerMuzzleFlashShader{};

	Shader m_lineShader{};
	Shader m_cubemapShader{};
	Shader m_minimapShader{};
	Shader m_shadowMapQuadShader{};

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

	std::vector<bool> m_renderEnemyMuzzleFlash = { false, false, false, false, false, false, false, false };
	std::vector<float> m_enemyMuzzleFlashStartTimes = { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };
	std::vector<float> m_enemyMuzzleTimesSinceStart = { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };
	std::vector<float> m_enemyMuzzleFlashDurations = { 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f };
	std::vector<float> m_enemyMuzzleAlphas = { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };
	std::vector<glm::vec3> m_enemyMuzzleFlashTints = { {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f, 1.0f} };
	std::vector<float> m_enemyMuzzleFlashScales = { 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f };
	std::vector<glm::mat4> m_enemyMuzzleModelMatrices = { glm::mat4(1.0f), glm::mat4(1.0f), glm::mat4(1.0f), glm::mat4(1.0f), glm::mat4(1.0f), glm::mat4(1.0f), glm::mat4(1.0f), glm::mat4(1.0f) };
	glm::vec3 m_enemyMuzzleFlashOffsets = glm::vec3(0.0f);
	float m_muzzleOffset = 2.4f;

	std::vector<bool> m_renderEnemyTracer = { false, false, false, false, false, false, false, false };
	std::vector<float> m_enemyTracerStartTimes = { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };
	std::vector<float> m_enemyTracerTimesSinceStart = { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };
	std::vector<float> m_enemyTracerDurations = { 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f };
	std::vector<float> m_enemyTracerAlphas = { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };
	std::vector<glm::vec3> m_enemyTracerTints = { {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f, 1.0f} };
	std::vector<float> m_enemyTracerScales = { 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f };
	std::vector<glm::mat4> m_enemyTracerModelMatrices = { glm::mat4(1.0f), glm::mat4(1.0f), glm::mat4(1.0f), glm::mat4(1.0f), glm::mat4(1.0f), glm::mat4(1.0f), glm::mat4(1.0f), glm::mat4(1.0f) };
	glm::vec3 m_enemyTracerOffsets = glm::vec3(0.0f);
	float m_tracerOffset = 2.4f;

	glm::mat4 m_view = glm::mat4(1.0f);
	glm::mat4 m_projection = glm::mat4(1.0f);
	glm::mat4 m_cubemapView = glm::mat4(1.0f);
	glm::mat4 m_minimapView = glm::mat4(1.0f);
	glm::mat4 m_minimapProjection = glm::mat4(1.0f);

	bool m_firstFlyCamSwitch = true;


	SoundEvent m_musicEvent;

	std::unique_ptr<NavMeshManager> m_navMeshManager;

	glm::vec3 mapScale = glm::vec3(5.0f);
	glm::vec3 mapPos = glm::vec3(0.0f, 0.0f, 0.0f);

	// Minimap top-down orthographic camera params (tunable via ImGui)
	glm::vec2 m_minimapCenter  = glm::vec2(27.0f, 130.0f); // world XZ centre
	float     m_minimapHeight  = 400.0f;                    // eye height above centre
	float     m_minimapExtent  = 250.0f;                    // half-width of ortho view
};

