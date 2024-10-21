#pragma once
#include <unordered_map>

#include "src/OpenGL/Renderer.h"
#include "src/OpenGL/RenderData.h"
#include "src/OpenGL/ShaderStorageBuffer.h"
#include "src/Window/Window.h"

#include "src/InputManager.h"
#include "Audio/AudioSystem.h"
#include "Audio/SoundEvent.h"
#include "Physics/PhysicsWorld.h"

#include "src/Camera.h"
#include "GameObjects/Player.h"
#include "GameObjects/Enemy.h"
#include "GameObjects/Waypoint.h"
#include "GameObjects/Crosshair.h"
#include "GameObjects/Line.h"
#include "GameObjects/Cube.h"
#include "Model/GltfModel.h"
#include "src/Pathfinding/Grid.h"

#include "AI/Event.h"
#include "AI/Events.h"

class GameManager {
private:
	//void SaveQTable(const std::map<std::pair<Enemy::NashState, Enemy::NashAction>, float>& qTable, const std::string& filename) {
	//	std::ofstream outFile(filename, std::ios::app);
	//	if (!outFile) {
	//		std::cerr << "Error opening file for writing: " << filename << std::endl;
	//		return;
	//	}

	//	for (const auto& entry : qTable) {
	//		const Enemy::NashState& state = entry.first.first;
	//		const Enemy::NashAction& action = entry.first.second;
	//		float value = entry.second;

	//		// Save state, action, and Q-value as comma-separated values
	//		outFile << state.playerDetected << "," << state.playerVisible << "," << state.distanceToPlayer << ","
	//			<< state.isSuppressionFire << "," << state.health << ","
	//			<< action << "," << value << "\n";
	//	}
	//	outFile.close();
	//}

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

public:
    GameManager(Window* window, unsigned int width, unsigned int height);

    ~GameManager() {
		//for (int enemyID = 0; enemyID < 4; ++enemyID) {
		//	SaveQTable(mEnemyStateQTable[enemyID], std::to_string(enemyID) + mEnemyStateFilename);
		//}

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

    void render();

    void setUpDebugUI();
    void showDebugUI();
    void renderDebugUI();

    bool camSwitchedToAim = false;

    PhysicsWorld* GetPhysicsWorld() { return physicsWorld; }

    void RemoveDestroyedGameObjects();

    void ResetEnemies();

private:
    void ShowCameraControlWindow(Camera& cam);
    void ShowLightControlWindow(DirLight& light);
    void ShowAnimationControlWindow();
    void ShowPerformanceWindow();
    void ShowEnemyStateWindow();

    void calculatePerformance(float deltaTime);

    EventManager& GetEventManager() { return eventManager; }

    std::string mEnemyStateFilename = "EnemyStateQTable.csv";
    std::unordered_map<std::pair<NashState, NashAction>, float, PairHash> mEnemyStateQTable[4];
    std::vector<NashState> enemyStates =
    {

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
        NashAction::PATROL
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

    Renderer* renderer;
    Window* window;
    Camera* camera;

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
    Cube* cover1;
    Cube* cover2;
    Cube* cover3;
    Cube* cover4;

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


    glm::mat4 view = glm::mat4(1.0f);
    glm::mat4 projection = glm::mat4(1.0f);

    Cell* cell;
    Grid* gameGrid;

    Waypoint* waypoint1;
    Waypoint* waypoint2;
    Waypoint* waypoint3;
    Waypoint* waypoint4;

    int currentStateIndex = 0;

    SoundEvent mMusicEvent;

    bool firstFlyCamSwitch = true;
};
