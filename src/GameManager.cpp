#include "GameManager.h"
#include "Components/AudioComponent.h"
#include "SceneSettings.h"

#include "imgui/imgui.h"
#include "imgui/backend/imgui_impl_glfw.h"
#include "imgui/backend/imgui_impl_opengl3.h"
#include "ImGuizmo.h"

#include <glm/gtc/type_ptr.hpp>

#include <fstream>
#include <algorithm>
#include <cmath>
#include <cstring>

static const std::string SCENE_SETTINGS_PATH = NPC_SCENE_ROOT "/scene_settings.json";

static void UploadLightsToShader(LightingSystem& lighting, Shader& shader)
{
    shader.Use();
    int np = std::min((int)lighting.pointLights.size(), MAX_POINT_LIGHTS);
    shader.SetInt("u_numPointLights", np);
    for (int i = 0; i < np; ++i) {
        std::string b = "u_pointLights[" + std::to_string(i) + "].";
        shader.SetVec3(b + "position",  lighting.pointLights[i].position);
        shader.SetVec3(b + "color",     lighting.pointLights[i].color);
        shader.SetFloat(b + "intensity", lighting.pointLights[i].intensity);
    }
    int ns = std::min((int)lighting.spotLights.size(), MAX_SPOT_LIGHTS);
    shader.SetInt("u_numSpotLights", ns);
    for (int i = 0; i < ns; ++i) {
        std::string b = "u_spotLights[" + std::to_string(i) + "].";
        SceneSpotLight& sl = lighting.spotLights[i];
        shader.SetVec3(b + "position",    sl.position);
        shader.SetVec3(b + "direction",   sl.direction);
        shader.SetVec3(b + "color",       sl.color);
        shader.SetFloat(b + "intensity",  sl.intensity);
        shader.SetFloat(b + "innerCutoff", std::cos(glm::radians(sl.innerAngle)));
        shader.SetFloat(b + "outerCutoff", std::cos(glm::radians(sl.outerAngle)));
    }
}

GameManager::GameManager(Window* window, unsigned int width, unsigned int height)
	: m_window(window), m_screenWidth(width), m_screenHeight(height)
{
	m_inputManager = new InputManager();
	m_audioSystem = new AudioSystem(this);

	if (!m_audioSystem->Initialize())
	{
		Logger::Log(1, "%s error: AudioSystem init error\n", __FUNCTION__);
		m_audioSystem->Shutdown();
		delete m_audioSystem;
		m_audioSystem = nullptr;
	}

	m_audioManager = new AudioManager(this);

	window->SetInputManager(m_inputManager);

	m_renderer = window->GetRenderer();
	m_renderer->SetUpMinimapFBO(width, height);
	m_renderer->SetUpShadowMapFBO(SHADOW_WIDTH, SHADOW_HEIGHT);

	playerShader.LoadShaders("src/Shaders/vertex_pbr_skinned.glsl", "src/Shaders/fragment_pbr_skinned.glsl");
	groundShader.LoadShaders("src/Shaders/vertex2.glsl", "src/Shaders/fragment2.glsl");
	enemyShader.LoadShaders("src/Shaders/vertex_pbr_skinned_enemy.glsl", "src/Shaders/pbr_fragment_emissive.glsl");
	enemyShader2.LoadShaders("src/Shaders/vertex.glsl", "src/Shaders/fragment.glsl");
	crosshairShader.LoadShaders("src/Shaders/crosshair_vert.glsl", "src/Shaders/crosshair_frag.glsl");
	lineShader.LoadShaders("src/Shaders/line_vert.glsl", "src/Shaders/line_frag.glsl");
	aabbShader.LoadShaders("src/Shaders/aabb_vert.glsl", "src/Shaders/aabb_frag.glsl");
	shadowMapShader.LoadShaders("src/Shaders/shadow_map_vertex.glsl", "src/Shaders/shadow_map_fragment.glsl");
	playerShadowMapShader.LoadShaders("src/Shaders/shadow_map_player_vertex.glsl", "src/Shaders/shadow_map_fragment.glsl");
	groundShadowShader.LoadShaders("src/Shaders/shadow_map_vertex.glsl", "src/Shaders/shadow_map_fragment.glsl");
	enemyShadowMapShader.LoadShaders("src/Shaders/shadow_map_enemy_vertex.glsl", "src/Shaders/shadow_map_fragment.glsl");
	playerMuzzleFlashShader.LoadShaders("src/Shaders/muzzle_flash_vertex.glsl", "src/Shaders/muzzle_flash_fragment.glsl");

	m_lineShader.LoadShaders("src/Shaders/line_vert.glsl", "src/Shaders/line_frag.glsl");
	m_cubemapShader.LoadShaders("src/Shaders/cubemap_vertex.glsl", "src/Shaders/cubemap_fragment.glsl");
	m_minimapShader.LoadShaders("src/Shaders/quad_vertex.glsl", "src/Shaders/quad_fragment.glsl");
	m_shadowMapQuadShader.LoadShaders("src/Shaders/shadow_map_quad_vertex.glsl", "src/Shaders/shadow_map_quad_fragment.glsl");

	m_physicsWorld = new PhysicsWorld();

	m_cubemapFaces = {
		"src/Assets/Textures/Skybox/T3Nebula/right.png",
		"src/Assets/Textures/Skybox/T3Nebula/left.png",
		"src/Assets/Textures/Skybox/T3Nebula/top.png",
		"src/Assets/Textures/Skybox/T3Nebula/bottom.png",
		"src/Assets/Textures/Skybox/T3Nebula/front.png",
		"src/Assets/Textures/Skybox/T3Nebula/back.png"
	};

	m_cubemap = new Cubemap(&m_cubemapShader);
	m_cubemap->LoadMesh();
	m_cubemap->LoadCubemap(m_cubemapFaces);

	ground = new Ground(mapPos, mapScale, &groundShader, &groundShadowShader, false, this);

	ground->SetAABBShader(&aabbShader);
	ground->SetUpAABB();
	ground->SetPlaneShader(&m_lineShader);


	std::vector<float> navMeshVertices;
	std::vector<unsigned int> navMeshIndices;
	std::vector<glm::vec3> mapVerts;

	std::vector<Ground::GLTFMesh> meshDataGrnd = ground->meshData;
	int mapVertCount = 0;
	int mapIndCount = 0;
	int triCount = 0;
	int vertexOffset = 0;

	for (Ground::GLTFMesh& mesh : meshDataGrnd)
	{
		for (Ground::GLTFPrimitive& prim : mesh.primitives)
		{
			for (glm::vec3 vert : prim.verts)
			{
				glm::vec4 newVert = glm::vec4(vert.x, vert.y, vert.z, 1.0f);
				glm::mat4 model = glm::mat4(1.0f);
				model = glm::translate(model, mapPos);
				//model = glm::rotate(model, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
				model = glm::scale(model, mapScale);
				glm::vec4 newVertTr = model * newVert;

				// Add transformed vertices
				mapVerts.push_back(glm::vec3(newVertTr.x, newVertTr.y, newVertTr.z));
				navMeshVertices.push_back(newVertTr.x);
				navMeshVertices.push_back(newVertTr.y);
				navMeshVertices.push_back(newVertTr.z);
				mapVertCount += 3;
			}

			for (unsigned int idx : prim.indices)
			{
				navMeshIndices.push_back(idx + vertexOffset);
				mapIndCount++;
			}

			vertexOffset += static_cast<int>(prim.verts.size());
		}
	}

	m_navMeshManager = std::make_unique<NavMeshManager>();
	m_navMeshManager->Build(navMeshVertices, navMeshIndices,
		"src/Shaders/navmesh_vert.glsl", "src/Shaders/navmesh_frag.glsl");

	m_camera = new Camera(glm::vec3(50.0f, 3.0f, 80.0f));

	m_minimapQuad = new Quad();
	m_minimapQuad->SetUpVAO(false);

	m_shadowMapQuad = new Quad();
	m_shadowMapQuad->SetUpVAO(false);

	m_playerMuzzleFlashQuad = new Quad();
	m_playerMuzzleFlashQuad->SetUpVAO(true);
	m_playerMuzzleFlashQuad->SetShader(&playerMuzzleFlashShader);
	m_playerMuzzleFlashQuad->LoadTexture("src/Assets/Textures/muzzleflash.png");

	m_enemyMuzzleFlashQuad = new Quad();
	m_enemyMuzzleFlashQuad->SetUpVAO(true);
	m_enemyMuzzleFlashQuad->SetShader(&playerMuzzleFlashShader);
	m_enemyMuzzleFlashQuad->LoadTexture("src/Assets/Textures/muzzleflash.png");

	m_enemyTracerQuad = new Quad();
	m_enemyTracerQuad->SetUpVAO(true);
	m_enemyTracerQuad->SetShader(&playerMuzzleFlashShader);
	m_enemyTracerQuad->LoadTexture("src/Assets/Textures/muzzleflash.png");

	m_player = new Player((glm::vec3(27.0f, -43.35, 416.0f)), glm::vec3(5.0f), &playerShader, &playerShadowMapShader, true, this, 0.0f);

	m_player->SetAABBShader(&aabbShader);
	m_player->SetUpAABB();

	std::string texture = "src\\Assets\\Models\\New_Enemies\\Armour7\\armor7_painter_armor7_mat_BaseColor.png";
	std::string texture2 = "src\\Assets\\Models\\New_Enemies\\Armour7\\armor7_painter_armor7_mat_BaseColor2.png";
	std::string texture3 = "src\\Assets\\Models\\New_Enemies\\Armour7\\armor7_painter_armor7_mat_BaseColor3.png";
	std::string texture4 = "src\\Assets\\Models\\New_Enemies\\Armour7\\armor7_painter_armor7_mat_BaseColor4.png";

	m_enemy = new Enemy(glm::vec3(50.0f, 1.73f, 214.0f), glm::vec3(5.0f), &enemyShader, &enemyShadowMapShader, true, this, texture, 0, GetEventManager(), *m_player, EnemyConfig::Scout());
	m_enemy->SetAABBShader(&aabbShader);
	m_enemy->SetUpAABB();

	m_enemy2 = new Enemy(glm::vec3(-60.0f, 1.73f, -6.2f), glm::vec3(5.0f), &enemyShader, &enemyShadowMapShader, true, this, texture2, 1, GetEventManager(), *m_player, EnemyConfig::Scout());
	m_enemy2->SetAABBShader(&aabbShader);
	m_enemy2->SetUpAABB();

	m_enemy3 = new Enemy(glm::vec3(138.0f, 1.73f, -37.0f), glm::vec3(5.0f), &enemyShader, &enemyShadowMapShader, true, this, texture3, 2, GetEventManager(), *m_player, EnemyConfig::Scout());
	m_enemy3->SetAABBShader(&aabbShader);
	m_enemy3->SetUpAABB();

	m_enemy4 = new Enemy(glm::vec3(42.0f, -34.73f, -155.0f), glm::vec3(5.0f), &enemyShader, &enemyShadowMapShader, true, this, texture4, 3, GetEventManager(), *m_player, EnemyConfig::Scout());
	m_enemy4->SetAABBShader(&aabbShader);
	m_enemy4->SetUpAABB();

	m_enemy5 = new Enemy(glm::vec3(-68.0f, 1.73f, 144.0f), glm::vec3(5.0f), &enemyShader, &enemyShadowMapShader, true, this, texture4, 4, GetEventManager(), *m_player, EnemyConfig::HeavyScout());
	m_enemy5->SetAABBShader(&aabbShader);
	m_enemy5->SetUpAABB();

	m_enemy6 = new Enemy(glm::vec3(-92.0f, -38.73f, 8.0f), glm::vec3(5.0f), &enemyShader, &enemyShadowMapShader, true, this, texture4, 5, GetEventManager(), *m_player, EnemyConfig::HeavyScout());
	m_enemy6->SetAABBShader(&aabbShader);
	m_enemy6->SetUpAABB();

	m_enemy7 = new Enemy(glm::vec3(100.0f, 3.73f, 241.0f), glm::vec3(0.01f), &enemyShader2, &enemyShadowMapShader, true, this, texture4, 6, GetEventManager(), *m_player, EnemyConfig::Drone());
	m_enemy7->SetAABBShader(&aabbShader);
	m_enemy7->SetUpAABB();

	m_enemy8 = new Enemy(glm::vec3(150.0f, 3.73f, 241.0f), glm::vec3(0.1f), &enemyShader2, &enemyShadowMapShader, true, this, texture4, 7, GetEventManager(), *m_player, EnemyConfig::Mech());
	m_enemy8->SetAABBShader(&aabbShader);
	m_enemy8->SetUpAABB();

	m_crosshair = new Crosshair(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.3f), &crosshairShader, &shadowMapShader, false, this);
	m_crosshair->LoadMesh();
	m_crosshair->LoadTexture("src/Assets/Textures/Crosshair.png");
	m_playerLine = new Line(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f), &lineShader, &shadowMapShader, false, this);
	m_playerLine->LoadMesh();

	for (auto& line : m_enemyLines)
	{
		line = new Line(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f), &lineShader, &shadowMapShader, false, this);
		line->LoadMesh();
	}

	m_inputManager->SetContext(m_camera, m_player, m_enemy, width, height);

	/* reset skeleton split */

	std::srand(static_cast<unsigned int>(std::time(nullptr)));

	m_gameObjects.push_back(m_player);
	m_gameObjects.push_back(m_enemy);
	m_gameObjects.push_back(m_enemy2);
	m_gameObjects.push_back(m_enemy3);
	m_gameObjects.push_back(m_enemy4);
	m_gameObjects.push_back(m_enemy5);
	m_gameObjects.push_back(m_enemy6);
	m_gameObjects.push_back(m_enemy7);
	m_gameObjects.push_back(m_enemy8);
	m_gameObjects.push_back(ground);

	m_enemies.push_back(m_enemy);
	m_enemies.push_back(m_enemy2);
	m_enemies.push_back(m_enemy3);
	m_enemies.push_back(m_enemy4);
	m_enemies.push_back(m_enemy5);
	m_enemies.push_back(m_enemy6);
	m_enemies.push_back(m_enemy7);
	m_enemies.push_back(m_enemy8);

#ifdef NPC_RL_QLEARNING
	if (m_initializeQTable)
	{
		for (auto& enem : m_enemies)
		{
			int enemyID = enem->GetID();
			Logger::Log(1, "%s Initializing Q Table for Enemy %d\n", __FUNCTION__, enemyID);
			InitializeQTable(m_enemyStateQTable[enemyID]);
			Logger::Log(1, "%s Initialized Q Table for Enemy %d\n", __FUNCTION__, enemyID);
		}
	}
	else if (m_loadQTable)
	{
		for (auto& enem : m_enemies)
		{
			int enemyID = enem->GetID();
			Logger::Log(1, "%s Loading Q Table for Enemy %d\n", __FUNCTION__, enemyID);
			LoadQTable(m_enemyStateQTable[enemyID], std::to_string(enemyID) + m_enemyStateFilename);
			Logger::Log(1, "%s Loaded Q Table for Enemy %d\n", __FUNCTION__, enemyID);
		}
	}
#endif // NPC_RL_QLEARNING

	m_musicEvent = m_audioSystem->PlayEvent("event:/bgm");

	glm::vec3 playerSnapped;
	if (m_navMeshManager->SnapToNavMesh(m_player->GetPosition(), playerSnapped))
		m_player->SetPosition(playerSnapped);

	LoadSceneSettings();

	m_navMeshManager->InitCrowd(m_enemies);
}

void GameManager::SetupCamera(unsigned int width, unsigned int height, float deltaTime)
{
	m_camera->SetZoom(45.0f);

	if (m_camera->GetMode() == PLAYER_FOLLOW)
	{
		if (m_camera->isBlending)
		{
			//m_camera->SetPitch(45.0f);
			glm::vec3 camPos = m_camera->GetPosition();
			if (camPos.y < 0.0f)
			{
				camPos.y = m_camera->GetPlayerCamHeightOffset();
				m_camera->SetPosition(camPos);
			}
			m_view = m_camera->UpdateCameraLerp(m_camera->GetPosition() + (glm::vec3(0.0f, 1.0f, 0.0f) * m_camera->GetPlayerCamHeightOffset()), m_player->GetPosition() + (m_player->GetPlayerFront() * m_camera->GetPlayerPosOffset()), m_player->GetPlayerFront(), glm::vec3(0.0f, 1.0f, 0.0f), deltaTime);

		}
		else {
			//m_camera->SetPitch(45.0f);
			m_camera->FollowTarget(m_player->GetPosition() + (m_player->GetPlayerFront() * m_camera->GetPlayerPosOffset()), m_player->GetPlayerFront(), m_camera->GetPlayerCamRearOffset(), m_camera->GetPlayerCamHeightOffset());
			if (m_camera->HasSwitched())
				m_camera->StorePrevCam(m_camera->GetPosition() + (glm::vec3(0.0f, 1.0f, 0.0f) * m_camera->GetPlayerCamHeightOffset()), m_player->GetPosition() + (m_player->GetPlayerFront() * m_camera->GetPlayerPosOffset()));

			glm::vec3 camPos = m_camera->GetPosition();
			if (camPos.y < 0.0f)
			{
				camPos.y = m_camera->GetPlayerCamHeightOffset();
				m_camera->SetPosition(camPos);
			}

			m_view = m_camera->GetViewMatrixPlayerFollow(m_player->GetPosition() + (m_player->GetPlayerFront() * m_camera->GetPlayerPosOffset()), glm::vec3(0.0f, 1.0f, 0.0f));
		}

	}
	else if (m_camera->GetMode() == ENEMY_FOLLOW)
	{
		if (m_enemy->IsDestroyed())
		{
			m_camera->SetMode(FLY);
			return;
		}
		m_camera->FollowTarget(m_enemy->GetPosition(), m_enemy->GetEnemyFront(), m_camera->GetEnemyCamRearOffset(), m_camera->GetEnemyCamHeightOffset());
		m_view = m_camera->GetViewMatrixEnemyFollow(m_enemy->GetPosition(), glm::vec3(0.0f, 1.0f, 0.0f));
	}
	else if (m_camera->GetMode() == FLY)
	{
		if (m_firstFlyCamSwitch)
		{
			m_camera->FollowTarget(m_player->GetPosition(), m_player->GetPlayerFront(), m_camera->GetPlayerCamRearOffset(), m_camera->GetPlayerCamHeightOffset());
			m_firstFlyCamSwitch = false;
			return;
		}
		m_view = m_camera->GetViewMatrix();
	}
	else if (m_camera->GetMode() == PLAYER_AIM)
	{

		m_camera->SetZoom(33.0f);
		glm::vec3 target = m_player->GetPosition() + (m_player->GetPlayerFront() * m_camera->GetPlayerPosOffset()) + (m_player->GetPlayerRight() * m_camera->GetPlayerAimRightOffset());
		if (target.y < m_player->GetShootPos().y)
			target.y = m_player->GetShootPos().y;


		if (m_camera->isBlending)
		{
			//glm::vec3 newPos =
			//	(m_player->GetPosition())  +
			//	(m_player->GetPlayerFront() * m_camera->GetPlayerPosOffset()) +
			//	(m_player->GetPlayerRight() * m_camera->GetPlayerAimRightOffset());

			//if (newPos.y < m_player->GetPosition().y)
			//newPos.y = m_player->GetPosition().y + m_camera->playerCamHeightOffset;

			//glm::vec3 camPos = m_camera->GetPosition();
			//if (camPos.y <= m_player->GetPosition().y)
			//{
			//	camPos.y = m_player->GetPosition().y + 5.0f;
			//	m_camera->SetPosition(camPos);
			//}
			//m_camera->FollowTarget(m_player->GetPosition() + (m_player->GetPlayerFront() * m_camera->GetPlayerPosOffset()) + (m_player->GetPlayerRight() * m_camera->GetPlayerAimRightOffset()),
			//	m_player->GetPlayerFront(), m_camera->GetPlayerCamRearOffset(), m_camera->GetPlayerCamHeightOffset());
			//
			//glm::vec3 targetPos = m_camera->GetPosition();

			glm::vec3 camPos = m_camera->GetPosition();

			m_camera->FollowTarget(m_player->GetPosition() + (m_player->GetPlayerFront() * m_camera->GetPlayerPosOffset()) + (m_player->GetPlayerRight() * m_camera->GetPlayerAimRightOffset()),
				m_player->GetPlayerFront(), m_camera->GetPlayerAimCamRearOffset(), m_camera->GetPlayerAimCamHeightOffset());

			camPos = m_camera->GetPosition();

			m_view = m_camera->UpdateCameraLerp(camPos,
				m_player->GetPosition() + (m_player->GetPlayerFront() * m_camera->GetPlayerPosOffset()) + (m_player->GetPlayerRight() * m_camera->GetPlayerAimRightOffset()),
				m_player->GetPlayerFront(), m_player->GetPlayerAimUp(), deltaTime);
			m_camera->StorePrevCam(m_camera->GetPosition(), target);

		}
		else {

			glm::vec3 camPos = m_camera->GetPosition();

			m_camera->FollowTarget(m_player->GetPosition() + (m_player->GetPlayerFront() * m_camera->GetPlayerPosOffset()) + (m_player->GetPlayerRight() * m_camera->GetPlayerAimRightOffset()),
				m_player->GetPlayerFront(), m_camera->GetPlayerAimCamRearOffset(), m_camera->GetPlayerAimCamHeightOffset());

			if (m_camera->HasSwitched())
				m_camera->StorePrevCam(m_camera->GetPosition() + m_player->GetPlayerAimUp() * m_camera->GetPlayerAimCamHeightOffset(), m_player->GetPosition() + (m_player->GetPlayerFront() * m_camera->GetPlayerPosOffset()) + (m_player->GetPlayerRight() * m_camera->GetPlayerAimRightOffset()) + (m_player->GetPlayerAimUp() * m_camera->GetPlayerAimCamHeightOffset()));

			//if (camPos.y <= m_player->GetPosition().y)
			//{
			//	camPos.y = m_player->GetPosition().y + 5.0f;
			//	m_camera->SetPosition(camPos);
			//}
			m_camera->StorePrevCam(camPos, m_camera->GetPosition());
			m_view = m_camera->GetViewMatrixPlayerFollow(target, m_player->GetPlayerAimUp());
		}

	}

	m_cubemapView = glm::mat4(glm::mat3(m_camera->GetViewMatrixPlayerFollow(m_player->GetPosition(), glm::vec3(0.0f, 1.0f, 0.0f))));

	m_projection = glm::perspective(glm::radians(m_camera->GetZoom()), (float)width / (float)height, 0.1f, 500.0f);

	// Top-down orthographic minimap — eye directly above the chosen centre point
	glm::vec3 minimapEye = glm::vec3(m_minimapCenter.x, m_minimapHeight, m_minimapCenter.y);
	glm::vec3 minimapTarget = glm::vec3(m_minimapCenter.x, 0.0f, m_minimapCenter.y);
	m_minimapView = glm::lookAt(minimapEye, minimapTarget, glm::vec3(0.0f, 0.0f, -1.0f));
	m_minimapProjection = glm::ortho(-m_minimapExtent, m_minimapExtent,
		-m_minimapExtent, m_minimapExtent,
		0.1f, m_minimapHeight + 200.0f);

	m_player->SetCameraMatrices(m_view, m_projection);

	m_audioSystem->SetListener(m_view);
}

void GameManager::SetSceneData()
{
	m_renderer->SetScene(m_view, m_projection, m_cubemapView, m_lighting.dirLight);
}

void GameManager::SetUpDebugUi()
{
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();
	ImGuizmo::BeginFrame();
	ImGuizmo::SetOrthographic(false);
	ImGuizmo::SetRect(0.0f, 0.0f, (float)m_screenWidth, (float)m_screenHeight);
}

static const char* EnemyTypeName(EnemyType t)
{
	switch (t) {
	case SCOUT:       return "Scout";
	case HEAVY_SCOUT: return "Heavy Scout";
	case DRONE:       return "Drone";
	case MECH:        return "Mech";
	default:          return "Unknown";
	}
}

static const char* CameraModeName(CameraMode m)
{
	switch (m) {
	case FLY:           return "Fly";
	case PLAYER_FOLLOW: return "Player Follow";
	case PLAYER_AIM:    return "Player Aim";
	case ENEMY_FOLLOW:  return "Enemy Follow";
	default:            return "Unknown";
	}
}

void GameManager::ShowDebugUi()
{
	if (!m_inputManager->GetShowDevOverlay())
		return;

	ShowSceneOutliner();
	ShowEntityInspector();
	ShowLightingPanel();
	ShowLightsPanel();
	ShowCameraPanel();
	ShowAIDebugPanel();
	ShowPerformanceWindow();
}

void GameManager::ShowSceneOutliner()
{
	ImGui::Begin("Scene Outliner");

	// Edit / Play toggle
	if (m_editMode)
	{
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.4f, 0.0f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.9f, 0.5f, 0.1f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.7f, 0.3f, 0.0f, 1.0f));
		if (ImGui::Button("  EDIT MODE  "))
			m_editMode = false;
		ImGui::PopStyleColor(3);
		ImGui::SameLine();
		ImGui::TextDisabled("(AI paused)");

		ImGui::SameLine();
		if (ImGui::Button("Save Scene"))
			SaveSceneSettings();
		ImGui::SameLine();
		ImGui::TextDisabled("scene_settings.json");
	}
	else
	{
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.1f, 0.5f, 0.1f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.2f, 0.6f, 0.2f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.0f, 0.4f, 0.0f, 1.0f));
		if (ImGui::Button("  PLAY MODE  "))
		{
			SaveSceneSettings();
			m_editMode = true;
		}
		ImGui::PopStyleColor(3);
	}

	ImGui::Checkbox("Show NavMesh", &m_showNavMesh);

	ImGui::Separator();
	ImGui::Text("Player");
	ImGui::SameLine();
	ImGui::Text("HP: %.0f", m_player->GetHealth());

	ImGui::Separator();
	ImGui::Text("Enemies");

	if (m_editMode)
	{
		static const char* enemyTypeNames[] = { "Scout", "Heavy Scout", "Drone", "Mech" };
		ImGui::SetNextItemWidth(120.0f);
		ImGui::Combo("##EnemyType", &m_pendingEnemyType, enemyTypeNames, IM_ARRAYSIZE(enemyTypeNames));
		ImGui::SameLine();
		if (ImGui::Button("Add Enemy"))
		{
			EnemyType type = static_cast<EnemyType>(m_pendingEnemyType);
			glm::vec3 spawnPos = m_camera->GetPosition() + m_camera->GetFront() * 20.0f;
			SpawnEnemy(type, spawnPos);
		}
	}

	for (int i = 0; i < (int)m_enemies.size(); i++)
	{
		Enemy* e = m_enemies[i];
		if (!e) continue;

		bool dead = e->IsDestroyed();
		char label[64];
		snprintf(label, sizeof(label), "[%d] %s%s", e->GetID(), EnemyTypeName(e->GetConfig().type), dead ? " (dead)" : "");

		bool selected = (m_selectedEnemyIndex == i);
		ImGui::PushStyleColor(ImGuiCol_Text, dead ? ImVec4(0.5f, 0.5f, 0.5f, 1.0f) : ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
		if (ImGui::Selectable(label, selected))
			m_selectedEnemyIndex = selected ? -1 : i;
		ImGui::PopStyleColor();
	}

	ImGui::End();
}

void GameManager::ShowEntityInspector()
{
	ImGui::Begin("Entity Inspector");

	if (m_selectedEnemyIndex < 0 || m_selectedEnemyIndex >= (int)m_enemies.size())
	{
		ImGui::Text("No entity selected.");
		ImGui::Text("(Click an entry in Scene Outliner)");
		ImGui::End();
		return;
	}

	Enemy* e = m_enemies[m_selectedEnemyIndex];
	if (!e) { ImGui::End(); return; }

	const EnemyConfig& cfg = e->GetConfig();

	ImGui::Text("Type:  %s", EnemyTypeName(cfg.type));
	ImGui::Text("ID:    %d", e->GetID());
	ImGui::Text("State: %s", e->GetEDBTState().c_str());
	ImGui::Text("Dead:  %s", e->IsDestroyed() ? "Yes" : "No");

	ImGui::Separator();
	glm::vec3 pos = e->GetPosition();
	if (ImGui::DragFloat3("Position", glm::value_ptr(pos), 0.5f))
	{
		glm::vec3 snapped = m_navMeshManager->TeleportAgent(e->GetID(), pos);
		e->SetPosition(snapped);
	}

	float health = e->GetHealth();
	float maxHP = cfg.maxHealth;
	ImGui::Text("Health: %.0f / %.0f", health, maxHP);
	ImGui::ProgressBar(maxHP > 0.0f ? health / maxHP : 0.0f, ImVec2(-1.0f, 0.0f));

	ImGui::Separator();
	ImGui::Text("Config");
	ImGui::Text("  Max Health:  %.0f", cfg.maxHealth);
	ImGui::Text("  Accuracy:    %.0f%%", cfg.accuracy);
	ImGui::Text("  Move Speed:  %.1f", cfg.moveSpeed);
	ImGui::Text("  Has Skin:    %s", cfg.hasSkin ? "Yes" : "No");
	ImGui::Text("  Anims  walk:%d  shoot:%d  dmg:%d  death:%d",
		cfg.animWalk, cfg.animShoot, cfg.animTakeDamage, cfg.animDeath);

	ImGui::End();

	// ImGuizmo translate gizmo — only in edit mode (play mode has AI overriding position)
	if (m_editMode && !e->IsDestroyed())
	{
		glm::mat4 model = glm::translate(glm::mat4(1.0f), e->GetPosition());
		ImGuizmo::SetDrawlist(ImGui::GetBackgroundDrawList());
		ImGuizmo::Manipulate(
			glm::value_ptr(m_view),
			glm::value_ptr(m_projection),
			ImGuizmo::TRANSLATE,
			ImGuizmo::WORLD,
			glm::value_ptr(model)
		);
		if (ImGuizmo::IsUsing())
		{
			glm::vec3 snapped = m_navMeshManager->TeleportAgent(e->GetID(), glm::vec3(model[3]));
			e->SetPosition(snapped);
		}
	}
	else if (!m_editMode)
	{
		ImGui::TextDisabled("(gizmo disabled in play mode)");
	}
}

void GameManager::ShowLightsPanel()
{
	auto& pls = m_lighting.pointLights;
	auto& sls = m_lighting.spotLights;

	ImGui::Begin("Lights");

	// ---- Point lights -------------------------------------------------------
	ImGui::SeparatorText("Point Lights");
	if (ImGui::Button("+ Add Point Light")) {
		ScenePointLight pl;
		pl.position = m_camera->GetPosition();
		pl.name     = "Point Light " + std::to_string(pls.size() + 1);
		pls.push_back(pl);
		m_selectedLightIndex  = (int)pls.size() - 1;
		m_selectedLightIsSpot = false;
	}

	for (int i = 0; i < (int)pls.size(); ++i) {
		bool sel = (!m_selectedLightIsSpot && m_selectedLightIndex == i);
		ImGui::PushID(i);
		if (ImGui::Selectable(pls[i].name.c_str(), sel)) {
			m_selectedLightIndex  = sel ? -1 : i;
			m_selectedLightIsSpot = false;
		}
		ImGui::SameLine();
		if (ImGui::SmallButton("X")) {
			pls.erase(pls.begin() + i);
			if (m_selectedLightIndex == i) m_selectedLightIndex = -1;
			ImGui::PopID();
			break;
		}
		ImGui::PopID();
	}

	if (!m_selectedLightIsSpot && m_selectedLightIndex >= 0 && m_selectedLightIndex < (int)pls.size()) {
		ScenePointLight& pl = pls[m_selectedLightIndex];
		ImGui::Separator();
		char nameBuf[64]; std::strncpy(nameBuf, pl.name.c_str(), sizeof(nameBuf));
		if (ImGui::InputText("Name##pl", nameBuf, sizeof(nameBuf))) pl.name = nameBuf;
		ImGui::DragFloat3("Position##pl",  glm::value_ptr(pl.position),  0.5f);
		ImGui::ColorEdit3("Color##pl",     glm::value_ptr(pl.color));
		ImGui::DragFloat("Intensity##pl",  &pl.intensity, 0.5f, 0.0f, 10000.0f);
	}

	// ---- Spot lights --------------------------------------------------------
	ImGui::SeparatorText("Spot Lights");
	if (ImGui::Button("+ Add Spot Light")) {
		SceneSpotLight sl;
		sl.position = m_camera->GetPosition();
		sl.name     = "Spot Light " + std::to_string(sls.size() + 1);
		sls.push_back(sl);
		m_selectedLightIndex  = (int)sls.size() - 1;
		m_selectedLightIsSpot = true;
	}

	for (int i = 0; i < (int)sls.size(); ++i) {
		bool sel = (m_selectedLightIsSpot && m_selectedLightIndex == i);
		ImGui::PushID(1000 + i);
		if (ImGui::Selectable(sls[i].name.c_str(), sel)) {
			m_selectedLightIndex  = sel ? -1 : i;
			m_selectedLightIsSpot = true;
		}
		ImGui::SameLine();
		if (ImGui::SmallButton("X")) {
			sls.erase(sls.begin() + i);
			if (m_selectedLightIndex == i) m_selectedLightIndex = -1;
			ImGui::PopID();
			break;
		}
		ImGui::PopID();
	}

	if (m_selectedLightIsSpot && m_selectedLightIndex >= 0 && m_selectedLightIndex < (int)sls.size()) {
		SceneSpotLight& sl = sls[m_selectedLightIndex];
		ImGui::Separator();
		char nameBuf[64]; std::strncpy(nameBuf, sl.name.c_str(), sizeof(nameBuf));
		if (ImGui::InputText("Name##sl",      nameBuf, sizeof(nameBuf))) sl.name = nameBuf;
		ImGui::DragFloat3("Position##sl",  glm::value_ptr(sl.position),  0.5f);
		ImGui::DragFloat3("Direction##sl", glm::value_ptr(sl.direction), 0.01f, -1.0f, 1.0f);
		if (ImGui::Button("Normalize Dir")) sl.direction = glm::normalize(sl.direction);
		ImGui::ColorEdit3("Color##sl",     glm::value_ptr(sl.color));
		ImGui::DragFloat("Intensity##sl",  &sl.intensity,  0.5f, 0.0f, 10000.0f);
		ImGui::DragFloat("Inner Angle",    &sl.innerAngle, 0.1f, 1.0f, 89.0f);
		ImGui::DragFloat("Outer Angle",    &sl.outerAngle, 0.1f, sl.innerAngle, 90.0f);
	}

	ImGui::End();

	// ImGuizmo translate gizmo for selected light (edit mode only)
	if (m_editMode && m_selectedLightIndex >= 0) {
		glm::vec3* lightPos = nullptr;
		if (!m_selectedLightIsSpot && m_selectedLightIndex < (int)pls.size())
			lightPos = &pls[m_selectedLightIndex].position;
		else if (m_selectedLightIsSpot && m_selectedLightIndex < (int)sls.size())
			lightPos = &sls[m_selectedLightIndex].position;

		if (lightPos) {
			glm::mat4 model = glm::translate(glm::mat4(1.0f), *lightPos);
			ImGuizmo::SetDrawlist(ImGui::GetBackgroundDrawList());
			ImGuizmo::Manipulate(
				glm::value_ptr(m_view), glm::value_ptr(m_projection),
				ImGuizmo::TRANSLATE, ImGuizmo::WORLD,
				glm::value_ptr(model)
			);
			if (ImGuizmo::IsUsing())
				*lightPos = glm::vec3(model[3]);
		}
	}
}

void GameManager::ShowLightingPanel()
{
	ImGui::Begin("Lighting");

	ImGui::Text("Directional Light");
	ImGui::DragFloat3("Direction", glm::value_ptr(m_lighting.dirLight.m_direction), 0.01f, -1.0f, 1.0f);
	ImGui::ColorEdit3("Ambient", glm::value_ptr(m_lighting.dirLight.m_ambient));
	ImGui::ColorEdit3("Diffuse", glm::value_ptr(m_lighting.dirLight.m_diffuse));
	ImGui::ColorEdit3("Specular", glm::value_ptr(m_lighting.dirLight.m_specular));
	ImGui::DragFloat3("PBR Color", glm::value_ptr(m_lighting.pbrColor), 1.0f, 0.0f, 1000.0f);

	ImGui::Separator();
	ImGui::Text("Shadow Framing");
	ImGui::DragFloat3("Scene Centre", glm::value_ptr(m_lighting.sceneCenter), 0.5f);
	ImGui::DragFloat("Light Distance", &m_lighting.lightDistance, 1.0f, 10.0f, 2000.0f);

	ImGui::Separator();
	ImGui::Text("Shadow Frustum (ortho)");
	ImGui::DragFloat("Ortho Left", &m_lighting.orthoLeft, 0.5f);
	ImGui::DragFloat("Ortho Right", &m_lighting.orthoRight, 0.5f);
	ImGui::DragFloat("Ortho Bottom", &m_lighting.orthoBottom, 0.5f);
	ImGui::DragFloat("Ortho Top", &m_lighting.orthoTop, 0.5f);
	ImGui::DragFloat("Near Plane", &m_lighting.nearPlane, 0.1f);
	ImGui::DragFloat("Far Plane", &m_lighting.farPlane, 1.0f);

	ImGui::End();

	// ImGuizmo: translate a "light anchor" at -dir*80; dragging it changes direction
	const float LIGHT_DIST = 80.0f;
	glm::vec3 anchor = -glm::normalize(m_lighting.dirLight.m_direction) * LIGHT_DIST;
	glm::mat4 lightMat = glm::translate(glm::mat4(1.0f), anchor);

	ImGuizmo::SetDrawlist(ImGui::GetBackgroundDrawList());
	ImGuizmo::Manipulate(
		glm::value_ptr(m_view),
		glm::value_ptr(m_projection),
		ImGuizmo::TRANSLATE,
		ImGuizmo::WORLD,
		glm::value_ptr(lightMat)
	);
	if (ImGuizmo::IsUsing())
	{
		glm::vec3 newAnchor = glm::vec3(lightMat[3]);
		if (glm::length(newAnchor) > 0.001f)
			m_lighting.dirLight.m_direction = glm::normalize(-newAnchor);
	}
}

void GameManager::ShowCameraPanel()
{
	ImGui::Begin("Camera & Map");

	const char* modeStr = CameraModeName(m_camera->GetMode());
	ImGui::Text("Camera Mode: %s  (Ctrl to cycle)", modeStr);
	ImGui::DragFloat3("Cam Position", glm::value_ptr(m_camera->m_position));
	ImGui::DragFloat("Pitch", &m_camera->m_pitch, 0.1f);
	ImGui::DragFloat("Yaw", &m_camera->m_yaw, 0.1f);
	ImGui::DragFloat("Zoom", &m_camera->m_zoom, 0.1f);
	ImGui::DragFloat("Blend Time", &m_camera->cameraBlendTime, 0.01f);

	ImGui::Separator();
	ImGui::Text("Follow Offsets");
	if (ImGui::DragFloat("Rear Offset", &m_camera->playerCamRearOffset, 0.1f))
		m_camera->SetPlayerCamRearOffset(m_camera->playerCamRearOffset);
	if (ImGui::DragFloat("Height Offset", &m_camera->playerCamHeightOffset, 0.1f))
		m_camera->SetPlayerCamHeightOffset(m_camera->playerCamHeightOffset);
	if (ImGui::DragFloat("Pos Offset", &m_camera->playerPosOffset, 0.1f))
		m_camera->SetPlayerPosOffset(m_camera->playerPosOffset);
	if (ImGui::DragFloat("Aim Right Offset", &m_camera->playerAimRightOffset, 0.1f))
		m_camera->SetPlayerAimRightOffset(m_camera->playerAimRightOffset);

	ImGui::Separator();
	ImGui::Text("Player Debug");
	ImGui::DragFloat3("Player Pos", glm::value_ptr(m_player->m_position));
	ImGui::DragFloat("Player Yaw", &m_player->m_playerYaw, 0.1f);
	ImGui::DragFloat("Player Aim Pitch", &m_player->m_aimPitch, 0.1f);
	ImGui::InputInt("Player Anim", &m_player->m_animNum);

	ImGui::Separator();
	ImGui::Text("Map / Ground");
	if (ImGui::DragFloat3("Map Position", glm::value_ptr(mapPos), 0.1f))
		ground->SetPosition(mapPos);
	if (ImGui::DragFloat3("Map Scale", glm::value_ptr(mapScale), 0.01f))
		ground->SetScale(mapScale);

	ImGui::Separator();
	ImGui::Text("Minimap Camera");
	ImGui::DragFloat2("Centre (XZ)", glm::value_ptr(m_minimapCenter), 0.5f);
	ImGui::DragFloat("Height", &m_minimapHeight, 1.0f, 50.0f, 2000.0f);
	ImGui::DragFloat("Extent (half-w)", &m_minimapExtent, 1.0f, 10.0f, 1000.0f);

	ImGui::End();
}

void GameManager::ShowAIDebugPanel()
{
	ImGui::Begin("AI Debug");

	ImGui::Checkbox("Use EDBT", &m_useEdbt);
	ImGui::Separator();
	ImGui::Text("Player HP: %.0f", m_player->GetHealth());
	ImGui::Separator();

	for (Enemy* e : m_enemies)
	{
		if (!e || e->IsDestroyed()) continue;
		float hp = e->GetHealth();
		float maxHP = e->GetConfig().maxHealth;
		ImGui::Text("[%d] %-12s  %-16s  HP %.0f/%.0f",
			e->GetID(),
			EnemyTypeName(e->GetConfig().type),
			e->GetEDBTState().c_str(),
			hp, maxHP);
	}

	ImGui::End();
}

void GameManager::RenderDebugUi()
{
	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void GameManager::ShowPerformanceWindow()
{
	ImGui::Begin("Performance");

	ImGui::Text("FPS: %.1f", m_fps);
	ImGui::Text("Avg FPS: %.1f", m_avgFps);
	ImGui::Text("Frame Time: %.3f ms", m_frameTime);
	ImGui::Text("Elapsed: %.1f s", m_elapsedTime);

	ImGui::End();
}

void GameManager::SpawnEnemy(EnemyType type, glm::vec3 position)
{
    EnemyConfig cfg = EnemyConfig::FromType(type);

    int newId = 0;
    for (Enemy* e : m_enemies)
        newId = std::max(newId, e->GetID() + 1);

    Shader* shader = cfg.useAltShader ? &enemyShader2 : &enemyShader;

    Enemy* newEnemy = new Enemy(
        position,
        cfg.modelScale,
        shader,
        &enemyShadowMapShader,
        cfg.hasSkin,
        this,
        cfg.texturePath,
        newId,
        GetEventManager(),
        *m_player,
        cfg
    );
    newEnemy->SetAABBShader(&aabbShader);
    newEnemy->SetUpAABB();

    // Extend per-enemy muzzle flash state (indexed by enemy ID)
    m_renderEnemyMuzzleFlash.push_back(false);
    m_enemyMuzzleFlashStartTimes.push_back(0.0f);
    m_enemyMuzzleTimesSinceStart.push_back(0.0f);
    m_enemyMuzzleFlashDurations.push_back(0.1f);
    m_enemyMuzzleAlphas.push_back(0.0f);
    m_enemyMuzzleFlashTints.push_back({1.0f, 1.0f, 1.0f});
    m_enemyMuzzleFlashScales.push_back(1.0f);
    m_enemyMuzzleModelMatrices.push_back(glm::mat4(1.0f));

    // Extend per-enemy tracer state (indexed by enemy ID)
    m_renderEnemyTracer.push_back(false);
    m_enemyTracerStartTimes.push_back(0.0f);
    m_enemyTracerTimesSinceStart.push_back(0.0f);
    m_enemyTracerDurations.push_back(0.1f);
    m_enemyTracerAlphas.push_back(0.0f);
    m_enemyTracerTints.push_back({1.0f, 1.0f, 1.0f});
    m_enemyTracerScales.push_back(1.0f);
    m_enemyTracerModelMatrices.push_back(glm::mat4(1.0f));

    // Register navmesh crowd agent
    glm::vec3 snapped;
    m_navMeshManager->RegisterCrowdAgent(position, snapped);
    newEnemy->SetPosition(snapped);

    m_enemies.push_back(newEnemy);
    m_gameObjects.push_back(newEnemy);

    Logger::Log(1, "[SpawnEnemy] Spawned %s as ID %d at (%.1f, %.1f, %.1f)\n",
                EnemyTypeName(type), newId, snapped.x, snapped.y, snapped.z);
}

void GameManager::SaveSceneSettings()
{
	SceneSettings s;

	s.lightDirection = m_lighting.dirLight.m_direction;
	s.lightAmbient   = m_lighting.dirLight.m_ambient;
	s.lightDiffuse   = m_lighting.dirLight.m_diffuse;
	s.lightSpecular  = m_lighting.dirLight.m_specular;
	s.pbrColor       = m_lighting.pbrColor;

	s.sceneCenter   = m_lighting.sceneCenter;
	s.lightDistance = m_lighting.lightDistance;
	s.orthoLeft     = m_lighting.orthoLeft;
	s.orthoRight    = m_lighting.orthoRight;
	s.orthoBottom   = m_lighting.orthoBottom;
	s.orthoTop      = m_lighting.orthoTop;
	s.nearPlane     = m_lighting.nearPlane;
	s.farPlane      = m_lighting.farPlane;

	s.minimapCenter = m_minimapCenter;
	s.minimapHeight = m_minimapHeight;
	s.minimapExtent = m_minimapExtent;

	for (Enemy* e : m_enemies)
	{
		if (e) s.enemySpawns.push_back({ e->GetID(), e->GetPosition(), EnemyTypeName(e->GetConfig().type) });
	}

	s.pointLights = m_lighting.pointLights;
	s.spotLights  = m_lighting.spotLights;

	s.Save(SCENE_SETTINGS_PATH);
}

void GameManager::LoadSceneSettings()
{
	SceneSettings s;
	if (!s.Load(SCENE_SETTINGS_PATH))
		return; // No saved file yet — keep coded defaults

	m_lighting.dirLight.m_direction = s.lightDirection;
	m_lighting.dirLight.m_ambient   = s.lightAmbient;
	m_lighting.dirLight.m_diffuse   = s.lightDiffuse;
	m_lighting.dirLight.m_specular  = s.lightSpecular;
	m_lighting.pbrColor             = s.pbrColor;

	m_lighting.sceneCenter   = s.sceneCenter;
	m_lighting.lightDistance = s.lightDistance;
	m_lighting.orthoLeft     = s.orthoLeft;
	m_lighting.orthoRight    = s.orthoRight;
	m_lighting.orthoBottom   = s.orthoBottom;
	m_lighting.orthoTop      = s.orthoTop;
	m_lighting.nearPlane     = s.nearPlane;
	m_lighting.farPlane      = s.farPlane;
	m_lighting.UpdateLightSpaceMatrix();

	m_minimapCenter = s.minimapCenter;
	m_minimapHeight = s.minimapHeight;
	m_minimapExtent = s.minimapExtent;

	if (!s.pointLights.empty()) m_lighting.pointLights = s.pointLights;
	if (!s.spotLights.empty())  m_lighting.spotLights  = s.spotLights;

	for (const EnemySpawn& es : s.enemySpawns)
	{
		bool found = false;
		for (Enemy* e : m_enemies)
		{
			if (e && e->GetID() == es.id)
			{
				e->SetPosition(es.position);
				found = true;
				break;
			}
		}
		if (!found)
		{
			// Runtime-spawned enemy — recreate it
			static const std::unordered_map<std::string, EnemyType> typeMap = {
				{"Scout",       SCOUT},
				{"Heavy Scout", HEAVY_SCOUT},
				{"Drone",       DRONE},
				{"Mech",        MECH},
			};
			auto it = typeMap.find(es.typeName);
			EnemyType t = (it != typeMap.end()) ? it->second : SCOUT;
			SpawnEnemy(t, es.position);
		}
	}
}

void GameManager::CalculatePerformance(float deltaTime)
{
	m_fps = 1.0f / deltaTime;

	m_fpsSum += m_fps;
	m_frameCount++;

	if (m_frameCount == m_numFramesAvg)
	{
		m_avgFps = m_fpsSum / m_numFramesAvg;
		m_fpsSum = 0.0f;
		m_frameCount = 0;
	}

	m_frameTime = deltaTime * 1000.0f;

	m_elapsedTime += deltaTime;
}

void GameManager::SetUpAndRenderNavMesh()
{

}

void GameManager::CheckGameOver()
{
	if ((m_enemy->IsDestroyed() && m_enemy2->IsDestroyed() && m_enemy3->IsDestroyed() && m_enemy4->IsDestroyed()) || m_player->IsDestroyed())
		ResetGame();
}

void GameManager::ResetGame()
{
	m_camera->SetMode(PLAYER_FOLLOW);
	m_audioManager->ClearQueue();
	m_player->SetPosition(m_player->GetInitialPos());
	m_player->SetYaw(m_player->GetInitialYaw());
	m_player->SetAnimNum(0);
	m_player->SetIsDestroyed(false);
	m_player->SetHealth(100.0f);
	m_player->UpdatePlayerVectors();
	m_player->UpdatePlayerAimVectors();
	m_player->SetPlayerState(PlayerState::MOVING);
	m_player->SetAabbColor(glm::vec3(0.0f, 0.0f, 1.0f));
	m_enemy->SetIsDestroyed(false);
	m_enemy2->SetIsDestroyed(false);
	m_enemy3->SetIsDestroyed(false);
	m_enemy4->SetIsDestroyed(false);
	m_enemy->SetIsDead(false);
	m_enemy2->SetIsDead(false);
	m_enemy3->SetIsDead(false);
	m_enemy4->SetIsDead(false);
	m_enemy->SetPosition(m_enemy->GetInitialPosition());
	m_enemy2->SetPosition(m_enemy2->GetInitialPosition());
	m_enemy3->SetPosition(m_enemy3->GetInitialPosition());
	m_enemy4->SetPosition(m_enemy4->GetInitialPosition());
#ifdef NPC_RL_QLEARNING
	m_enemyStates = {
		{ false, false, 100.0f, 100.0f, false },
		{ false, false, 100.0f, 100.0f, false },
		{ false, false, 100.0f, 100.0f, false },
		{ false, false, 100.0f, 100.0f, false }
	};
#endif // NPC_RL_QLEARNING

	for (Enemy* emy : m_enemies)
	{
		emy->ResetState();
		m_physicsWorld->AddCollider(emy->GetAABB());
		m_physicsWorld->AddEnemyCollider(emy->GetAABB());
		emy->SetHealth(100.0f);
	}

}


void GameManager::RenderEnemyLineAndMuzzleFlash(bool isMainPass, bool isMinimapPass, bool isShadowPass)
{
	for (auto& enem : m_enemies)
	{
		if (!enem->IsDestroyed())
		{
			glm::vec3 enemyTracerEnd = glm::vec3(0.0f);

			int enemyID = enem->GetID();

			if (enem->GetEnemyHasShot())
			{
				float enemyMuzzleCurrentTime = (float)glfwGetTime();

				if (m_renderEnemyMuzzleFlash.at(enemyID) && m_enemyMuzzleFlashStartTimes.at(enemyID) + m_enemyMuzzleFlashDurations.at(enemyID) > enemyMuzzleCurrentTime)
				{
					m_renderEnemyMuzzleFlash.at(enemyID) = false;
				}
				else
				{
					m_renderEnemyMuzzleFlash.at(enemyID) = true;
					m_enemyMuzzleFlashStartTimes.at(enemyID) = enemyMuzzleCurrentTime;
				}

				if (m_renderEnemyMuzzleFlash.at(enemyID) && isMainPass)
				{
					m_enemyMuzzleTimesSinceStart.at(enemyID) = enemyMuzzleCurrentTime - m_enemyMuzzleFlashStartTimes.at(enemyID);
					m_enemyMuzzleAlphas.at(enemyID) = glm::max(0.0f, 1.0f - (m_enemyMuzzleTimesSinceStart.at(enemyID) / m_enemyMuzzleFlashDurations.at(enemyID)));
					m_enemyMuzzleFlashScales.at(enemyID) = 1.0f + (0.5f * m_enemyMuzzleAlphas.at(enemyID));

					m_enemyMuzzleModelMatrices.at(enemyID) = glm::mat4(1.0f);

					m_enemyMuzzleModelMatrices.at(enemyID) = glm::translate(m_enemyMuzzleModelMatrices.at(enemyID), enem->GetEnemyShootPos(m_muzzleOffset));
					m_enemyMuzzleModelMatrices.at(enemyID) = glm::rotate(m_enemyMuzzleModelMatrices.at(enemyID), enem->GetYaw(), glm::vec3(0.0f, 1.0f, 0.0f));
					m_enemyMuzzleModelMatrices.at(enemyID) = glm::scale(m_enemyMuzzleModelMatrices.at(enemyID), glm::vec3(m_enemyMuzzleFlashScales.at(enemyID), m_enemyMuzzleFlashScales.at(enemyID), 1.0f));
					m_enemyMuzzleFlashQuad->Draw3D(m_enemyMuzzleFlashTints.at(enemyID), m_enemyMuzzleAlphas.at(enemyID), m_projection, m_view, m_enemyMuzzleModelMatrices.at(enemyID));
				}

				if (enem->GetEnemyHasShot() && enem->GetEnemyDebugRayRenderTimer() > 0.0f)
				{
					if (enem->GetEnemyHasHit())
					{
						enemyTracerEnd = enem->GetEnemyHitPoint();
					}
					else
					{
						enemyTracerEnd = enem->GetEnemyShootPos(m_muzzleOffset) + enem->GetEnemyShootDir() * enem->GetEnemyShootDistance();
					}

					float enemyTracerCurrentTime = (float)glfwGetTime();

					if (m_renderEnemyTracer.at(enemyID) && m_enemyTracerStartTimes.at(enemyID) + m_enemyTracerDurations.at(enemyID) > enemyTracerCurrentTime)
					{
						m_renderEnemyTracer.at(enemyID) = false;
					}
					else
					{
						m_renderEnemyTracer.at(enemyID) = true;
						m_enemyTracerStartTimes.at(enemyID) = enemyTracerCurrentTime;
					}

					if (m_renderEnemyTracer.at(enemyID) && isMainPass)
					{
						m_enemyTracerTimesSinceStart.at(enemyID) = enemyTracerCurrentTime - m_enemyTracerStartTimes.at(enemyID);
						m_enemyTracerAlphas.at(enemyID) = glm::max(0.0f, 1.0f - (m_enemyTracerTimesSinceStart.at(enemyID) / m_enemyTracerDurations.at(enemyID)));
						m_enemyTracerScales.at(enemyID) = 1.0f + (0.5f * m_enemyTracerAlphas.at(enemyID));

						m_enemyTracerModelMatrices.at(enemyID) = glm::mat4(1.0f);

						glm::vec3 tracerDir = glm::normalize(enemyTracerEnd - enem->GetEnemyShootPos(m_tracerOffset));

						m_enemyTracerModelMatrices.at(enemyID) = glm::translate(m_enemyTracerModelMatrices.at(enemyID), enem->GetEnemyShootPos(m_tracerOffset));
						glm::quat tracerRotation = glm::rotation(glm::vec3(0.0f, 0.0f, 1.0f), tracerDir);
						m_enemyTracerModelMatrices.at(enemyID) *= glm::toMat4(tracerRotation);
						//m_enemyTracerModelMatrices.at(enemyID) = glm::rotate(m_enemyTracerModelMatrices.at(enemyID), (-enem->GetYaw() + 90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
						m_enemyTracerModelMatrices.at(enemyID) = glm::scale(m_enemyTracerModelMatrices.at(enemyID), glm::vec3(m_enemyTracerScales.at(enemyID), m_enemyTracerScales.at(enemyID), length(enemyTracerEnd - enem->GetEnemyShootPos(m_tracerOffset))));
						m_enemyTracerModelMatrices.at(enemyID) = glm::translate(m_enemyTracerModelMatrices.at(enemyID), glm::vec3(0.0f, 0.0f, 0.27f));

						m_enemyTracerQuad->Draw3D(m_enemyTracerTints.at(enemyID), m_enemyTracerAlphas.at(enemyID) - m_dt, m_projection, m_view, m_enemyTracerModelMatrices.at(enemyID));
					}
				}

			}
		}
	}
}

void GameManager::RenderPlayerCrosshairAndMuzzleFlash(bool isMainPass)
{
	if ((m_player->GetPlayerState() == AIMING || m_player->GetPlayerState() == SHOOTING) && m_camSwitchedToAim == false && isMainPass)
	{
		m_renderer->RemoveDepthAndSetBlending();

		glm::vec3 rayO = m_player->GetShootPos();
		glm::vec3 rayD = glm::normalize(m_player->GetPlayerAimFront());
		float dist = m_player->GetShootDistance();

		glm::vec3 rayEnd = rayO + rayD * dist;

		glm::vec3 lineColor = glm::vec3(1.0f, 0.0f, 0.0f);


		if (m_player->GetPlayerState() == SHOOTING)
		{
			lineColor = glm::vec3(0.0f, 1.0f, 0.0f);

			float currentTime = (float)glfwGetTime();

			if (m_renderPlayerMuzzleFlash && m_playerMuzzleFlashStartTime + m_playerMuzzleFlashDuration > currentTime)
			{
				m_renderPlayerMuzzleFlash = false;
			}
			else
			{
				m_renderPlayerMuzzleFlash = true;
				m_playerMuzzleFlashStartTime = currentTime;
			}


			if (m_renderPlayerMuzzleFlash)

			{
				m_playerMuzzleTimeSinceStart = currentTime - m_playerMuzzleFlashStartTime;
				m_playerMuzzleAlpha = glm::max(0.0f, 1.0f - (m_playerMuzzleTimeSinceStart / m_playerMuzzleFlashDuration));
				m_playerMuzzleFlashScale = 1.0f + (0.5f * m_playerMuzzleAlpha);

				m_playerMuzzleModel = glm::mat4(1.0f);

				m_playerMuzzleModel = glm::translate(m_playerMuzzleModel, m_player->GetShootPos());
				m_playerMuzzleModel = glm::rotate(m_playerMuzzleModel, (-m_player->GetYaw() + 90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
				m_playerMuzzleModel = glm::scale(m_playerMuzzleModel, glm::vec3(m_playerMuzzleFlashScale, m_playerMuzzleFlashScale, 1.0f));
				m_playerMuzzleFlashQuad->Draw3D(m_playerMuzzleTint, m_playerMuzzleAlpha, m_projection, m_view, m_playerMuzzleModel);
			}
		}


		glm::vec4 rayEndWorldSpace = glm::vec4(rayEnd, 1.0f);
		glm::vec4 rayEndCameraSpace = m_view * rayEndWorldSpace;
		glm::vec4 rayEndNDC = m_projection * rayEndCameraSpace;

		glm::vec4 targetNDC(0.0f, 0.5f, rayEndNDC.z / rayEndNDC.w, 1.0f);
		glm::vec4 targetCameraSpace = glm::inverse(m_projection) * targetNDC;
		glm::vec4 targetWorldSpace = glm::inverse(m_view) * targetCameraSpace;

		rayEnd = glm::vec3(targetWorldSpace) / targetWorldSpace.w;

		glm::vec3 crosshairHitpoint;
		glm::vec3 crosshairCol;

		auto clipCoords = glm::vec4(0.15f, 0.5f, 1.0f, 1.0f);

		glm::vec4 cameraCoords = inverse(m_projection) * clipCoords;
		cameraCoords /= cameraCoords.w;

		glm::vec4 worldCoords = inverse(m_view) * cameraCoords;
		rayEnd = glm::vec3(worldCoords) / worldCoords.w;

		rayD = normalize(rayEnd - rayO);

		if (m_physicsWorld->RayEnemyCrosshairIntersect(rayO, rayD, crosshairHitpoint))
		{
			crosshairCol = glm::vec3(1.0f, 0.0f, 0.0f);
		}
		else
		{
			crosshairCol = glm::vec3(1.0f, 1.0f, 1.0f);
		}

		glm::vec2 ndcPos = m_crosshair->CalculateCrosshairPosition(rayEnd, m_window->GetWidth(), m_window->GetHeight(), m_projection, m_view);

		float ndcX = (ndcPos.x / m_window->GetWidth()) * 2.0f - 1.0f;
		float ndcY = (ndcPos.y / m_window->GetHeight()) * 2.0f - 1.0f;

		if (isMainPass)
			m_crosshair->DrawCrosshair(glm::vec2(0.0f, 0.5f), crosshairCol);

		m_renderer->ResetRenderStates();
	}
}


void GameManager::Update(float deltaTime)
{
	m_inputManager->ProcessInput(m_window->GetWindow(), deltaTime);
	float pauseFactor = m_inputManager->GetPauseFactor();
	float timeScaleFactor = m_inputManager->GetTimeScaleFacotr();

	float scaledDeltaTime = deltaTime;


	bool isPaused = m_inputManager->GetIsPaused();
	bool isTimeScaled = m_inputManager->GetIsTimeScaled();

	if (isPaused)
		scaledDeltaTime *= pauseFactor;

	if (isTimeScaled)
		scaledDeltaTime *= timeScaleFactor;

	m_player->UpdatePlayerVectors();
	m_player->UpdatePlayerAimVectors();

	m_player->Update(scaledDeltaTime, isPaused, isTimeScaled);

	m_dt = scaledDeltaTime;


	if (!m_editMode)
	{
		for (Enemy* e : m_enemies)
		{
			if (e == nullptr || e->IsDead())
				continue;

			e->SetDeltaTime(deltaTime);
			e->Update(true, false, false);
		}

		m_navMeshManager->Update(deltaTime, m_enemies, m_player->GetPosition());
	}
	m_audioManager->Update(scaledDeltaTime);
	m_audioSystem->Update(scaledDeltaTime);

	CalculatePerformance(deltaTime);
}

void GameManager::Render(bool isMinimapRenderPass, bool isShadowMapRenderPass, bool isMainRenderPass)
{
	glEnable(GL_DEPTH_TEST);
	glDisable(GL_BLEND);

	m_renderer->ResetRenderStates();


	if (!isShadowMapRenderPass)
	{
		m_renderer->ResetViewport(m_screenWidth, m_screenHeight);
		m_renderer->Clear();
	}

	if (isMinimapRenderPass)
		m_renderer->BindMinimapFbo(m_screenWidth, m_screenHeight);

	if (isShadowMapRenderPass)
		m_renderer->BindShadowMapFbo(SHADOW_WIDTH, SHADOW_HEIGHT);



	// Upload dynamic lights to every shader that has point/spot light uniforms
	if (!isShadowMapRenderPass) {
		UploadLightsToShader(m_lighting, groundShader);
		UploadLightsToShader(m_lighting, playerShader);
		UploadLightsToShader(m_lighting, enemyShader);
		UploadLightsToShader(m_lighting, enemyShader2);
	}

	for (auto obj : m_gameObjects) {
		if (obj->IsDestroyed())
			continue;
		if (isMinimapRenderPass)
		{
			m_renderer->Draw(obj, m_minimapView, m_minimapProjection, m_camera->GetPosition(), false, m_lighting.matrix);
		}
		else if (isShadowMapRenderPass)
		{
			m_renderer->Draw(obj, m_lighting.view, m_lighting.projection, m_camera->GetPosition(), true, m_lighting.matrix);
		}
		else
		{
			m_renderer->Draw(obj, m_view, m_projection, m_camera->GetPosition(), false, m_lighting.matrix);
		}
	}

	if (m_showNavMesh)
		m_navMeshManager->RenderDebug(m_view, m_projection);

	if (m_camSwitchedToAim)
		m_camSwitchedToAim = false;

	RenderEnemyLineAndMuzzleFlash(isMainRenderPass, isMinimapRenderPass, isShadowMapRenderPass);

	m_renderer->DrawCubemap(m_cubemap);

	RenderPlayerCrosshairAndMuzzleFlash(isMainRenderPass);

	if (isMainRenderPass)
	{
		m_renderer->DrawMinimap(m_minimapQuad, &m_minimapShader);
	}

	m_renderer->DrawShadowMap(m_shadowMapQuad, &m_shadowMapQuadShader);
#ifdef _DEBUG
	if (isMainRenderPass)
	{
	}
#endif

	if (isMinimapRenderPass)
	{
		m_renderer->UnbindMinimapFbo();
	}
	else if (isShadowMapRenderPass)
	{
		m_renderer->UnbindShadowMapFbo();
	}

}