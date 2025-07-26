#include "GameManager.h"
#include "Components/AudioComponent.h"

#include "imgui/imgui.h"
#include "imgui/backend/imgui_impl_glfw.h"
#include "imgui/backend/imgui_impl_opengl3.h"

DirLight dirLight = {
		glm::vec3(-3.0f, -2.0f, -3.0f),

		glm::vec3(0.15f, 0.2f, 0.25f),
		glm::vec3(7.0f),
		glm::vec3(0.8f, 0.9f, 1.0f)
};

glm::vec3 dirLightPBRColour = glm::vec3(10.f, 10.0f, 10.0f);

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

	m_playerShader.LoadShaders("C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Shaders/vertex_gpu_dquat_player.glsl", "C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Shaders/pbr_fragment.glsl");
	m_enemyShader.LoadShaders("C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Shaders/vertex_gpu_dquat_enemy.glsl", "C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Shaders/pbr_fragment_emissive.glsl");
	
#ifdef DEBUG
	m_gridShader.LoadShaders("C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Shaders/pbr_grid_debug_vert.glsl", "C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Shaders/pbr_grid_debug_frag.glsl");
#else
	m_gridShader.LoadShaders("C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Shaders/pbr_vertex.glsl", "C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Shaders/pbr_fragment.glsl");
#endif

	m_crosshairShader.LoadShaders("C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Shaders/crosshair_vert.glsl", "C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Shaders/crosshair_frag.glsl");
	m_lineShader.LoadShaders("C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Shaders/line_vert.glsl", "C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Shaders/line_frag.glsl");
	m_aabbShader.LoadShaders("C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Shaders/aabb_vert.glsl", "C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Shaders/aabb_frag.glsl");
	m_cubeShader.LoadShaders("C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Shaders/pbr_vertex.glsl", "C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Shaders/pbr_fragment_emissive.glsl");
	m_cubemapShader.LoadShaders("C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Shaders/cubemap_vertex.glsl", "C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Shaders/cubemap_fragment.glsl");
	m_minimapShader.LoadShaders("C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Shaders/quad_vertex.glsl", "C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Shaders/quad_fragment.glsl");
	m_shadowMapShader.LoadShaders("C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Shaders/shadow_map_vertex.glsl", "C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Shaders/shadow_map_fragment.glsl");
	m_playerShadowMapShader.LoadShaders("C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Shaders/shadow_map_player_vertex.glsl", "C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Shaders/shadow_map_fragment.glsl");
	m_enemyShadowMapShader.LoadShaders("C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Shaders/shadow_map_enemy_vertex.glsl", "C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Shaders/shadow_map_fragment.glsl");
	m_shadowMapQuadShader.LoadShaders("C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Shaders/shadow_map_quad_vertex.glsl", "C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Shaders/shadow_map_quad_fragment.glsl");
	m_playerMuzzleFlashShader.LoadShaders("C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Shaders/muzzle_flash_vertex.glsl", "C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Shaders/muzzle_flash_fragment.glsl");

	m_physicsWorld = new PhysicsWorld();

	m_cubemapFaces = {
		"C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Assets/Textures/Skybox/right.png",
		"C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Assets/Textures/Skybox/left.png",
		"C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Assets/Textures/Skybox/top.png",
		"C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Assets/Textures/Skybox/bottom.png",
		"C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Assets/Textures/Skybox/front.png",
		"C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Assets/Textures/Skybox/back.png"
	};

	m_cubemap = new Cubemap(&m_cubemapShader);
	m_cubemap->LoadMesh();
	m_cubemap->LoadCubemap(m_cubemapFaces);

	m_cell = new Cell();
	m_cell->SetUpVAO();
	//    m_cell->LoadTexture("C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Assets/Textures/Ground.png", m_cell->m_tex);
	std::string cubeTexFilename = "C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Assets/Textures/Cover.png";

	m_gameGrid = new Grid();

	for (glm::vec3 coverPos : m_gameGrid->GetCoverPositions())
	{
		Cube* cover = new Cube(m_gameGrid->SnapToGrid(coverPos), glm::vec3((float)m_gameGrid->GetCellSize()), &m_cubeShader, &m_shadowMapShader, false, this, cubeTexFilename);
		cover->SetAABBShader(&m_aabbShader);
		cover->LoadMesh();
		m_coverSpots.push_back(cover);
	}

	m_gameGrid->InitializeGrid();

	m_camera = new Camera(glm::vec3(50.0f, 3.0f, 80.0f));
	m_minimapCamera = new Camera(glm::vec3((m_gameGrid->GetCellSize() * m_gameGrid->GetGridSize()) / 2.0f, 140.0f, (m_gameGrid->GetCellSize() * m_gameGrid->GetGridSize()) / 2.0f), glm::vec3(0.0f, -1.0f, 0.0f), 0.0f, -90.0f, glm::vec3(0.0f, 0.0f, -1.0f));

	m_minimapQuad = new Quad();
	m_minimapQuad->SetUpVAO(false);

	m_shadowMapQuad = new Quad();
	m_shadowMapQuad->SetUpVAO(false);

	m_playerMuzzleFlashQuad = new Quad();
	m_playerMuzzleFlashQuad->SetUpVAO(true);
	m_playerMuzzleFlashQuad->SetShader(&m_playerMuzzleFlashShader);
	m_playerMuzzleFlashQuad->LoadTexture("C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Assets/Textures/muzzleflash.png");

	m_enemyMuzzleFlashQuad = new Quad();
	m_enemyMuzzleFlashQuad->SetUpVAO(true);
	m_enemyMuzzleFlashQuad->SetShader(&m_playerMuzzleFlashShader);
	m_enemyMuzzleFlashQuad->LoadTexture("C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Assets/Textures/muzzleflash.png");

	m_enemy2MuzzleFlashQuad = new Quad();
	m_enemy2MuzzleFlashQuad->SetUpVAO(true);
	m_enemy2MuzzleFlashQuad->SetShader(&m_playerMuzzleFlashShader);
	m_enemy2MuzzleFlashQuad->LoadTexture("C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Assets/Textures/muzzleflash.png");

	m_enemy3MuzzleFlashQuad = new Quad();
	m_enemy3MuzzleFlashQuad->SetUpVAO(true);
	m_enemy3MuzzleFlashQuad->SetShader(&m_playerMuzzleFlashShader);
	m_enemy3MuzzleFlashQuad->LoadTexture("C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Assets/Textures/muzzleflash.png");

	m_enemy4MuzzleFlashQuad = new Quad();
	m_enemy4MuzzleFlashQuad->SetUpVAO(true);
	m_enemy4MuzzleFlashQuad->SetShader(&m_playerMuzzleFlashShader);
	m_enemy4MuzzleFlashQuad->LoadTexture("C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Assets/Textures/muzzleflash.png");


	m_player = new Player(m_gameGrid->SnapToGrid(glm::vec3(95.0f, 0.0f, 25.0f)), glm::vec3(3.0f), &m_playerShader, &m_playerShadowMapShader, true, this);
	m_player->SetAABBShader(&m_aabbShader);
	m_player->SetUpAABB();

	std::string texture = "C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Assets/Models/GLTF/Enemies/Ely/EnemyEly_ely_vanguardsoldier_kerwinatienza_M2_BaseColor.png";
	std::string texture2 = "C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Assets/Models/GLTF/Enemies/Ely/ely-vanguardsoldier-kerwinatienza_diffuse_2.png";
	std::string texture3 = "C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Assets/Models/GLTF/Enemies/Ely/ely-vanguardsoldier-kerwinatienza_diffuse_3.png";
	std::string texture4 = "C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Assets/Models/GLTF/Enemies/Ely/ely-vanguardsoldier-kerwinatienza_diffuse_4.png";

	m_enemy = new Enemy(m_gameGrid->SnapToGrid(glm::vec3(33.0f, 0.0f, 23.0f)), glm::vec3(3.0f), &m_enemyShader, &m_enemyShadowMapShader, true, this, m_gameGrid, texture, 0, GetEventManager(), *m_player);
	m_enemy->SetAABBShader(&m_aabbShader);
	m_enemy->SetUpAABB();

	m_enemy2 = new Enemy(m_gameGrid->SnapToGrid(glm::vec3(3.0f, 0.0f, 53.0f)), glm::vec3(3.0f), &m_enemyShader, &m_enemyShadowMapShader, true, this, m_gameGrid, texture2, 1, GetEventManager(), *m_player);
	m_enemy2->SetAABBShader(&m_aabbShader);
	m_enemy2->SetUpAABB();

	m_enemy3 = new Enemy(m_gameGrid->SnapToGrid(glm::vec3(43.0f, 0.0f, 53.0f)), glm::vec3(3.0f), &m_enemyShader, &m_enemyShadowMapShader, true, this, m_gameGrid, texture3, 2, GetEventManager(), *m_player);
	m_enemy3->SetAABBShader(&m_aabbShader);
	m_enemy3->SetUpAABB();

	m_enemy4 = new Enemy(m_gameGrid->SnapToGrid(glm::vec3(11.0f, 0.0f, 23.0f)), glm::vec3(3.0f), &m_enemyShader, &m_enemyShadowMapShader, true, this, m_gameGrid, texture4, 3, GetEventManager(), *m_player);
	m_enemy4->SetAABBShader(&m_aabbShader);
	m_enemy4->SetUpAABB();

	m_crosshair = new Crosshair(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.3f), &m_crosshairShader, &m_shadowMapShader, false, this);
	m_crosshair->LoadMesh();
	m_crosshair->LoadTexture("C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Assets/Textures/Crosshair.png");
	m_playerLine = new Line(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f), &m_lineShader, &m_shadowMapShader, false, this);
	m_playerLine->LoadMesh();

	m_inputManager->SetContext(m_camera, m_player, m_enemy, width, height);

	std::srand(static_cast<unsigned int>(std::time(nullptr)));

	m_gameObjects.push_back(m_player);
	m_gameObjects.push_back(m_enemy);
	m_gameObjects.push_back(m_enemy2);
	m_gameObjects.push_back(m_enemy3);
	m_gameObjects.push_back(m_enemy4);

	for (Cube* coverSpot : m_coverSpots)
	{
		m_gameObjects.push_back(coverSpot);
	}

	m_enemies.push_back(m_enemy);
	m_enemies.push_back(m_enemy2);
	m_enemies.push_back(m_enemy3);
	m_enemies.push_back(m_enemy4);

	for (int i = 0; i < m_enemies.size(); i++)
	{
		m_playerLine = new Line(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f), &m_lineShader, &m_shadowMapShader, false, this);
		m_playerLine->LoadMesh();
		m_enemyLines.push_back(m_playerLine);
	}

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

	m_musicEvent = m_audioSystem->PlayEvent("event:/bgm");
}

void GameManager::SetupCamera(unsigned int width, unsigned int height, float deltaTime)
{
	m_camera->SetZoom(45.0f);

	if (m_camera->GetMode() == PLAYER_FOLLOW)
	{
		if (m_camera->isBlending)
		{
			m_camera->SetPitch(45.0f);
			m_view = m_camera->UpdateCameraLerp(m_camera->GetPosition(), m_player->GetPosition() + (m_player->GetPlayerFront() * m_camera->GetPlayerPosOffset()), glm::vec3(0.0f, 1.0f, 0.0f), deltaTime);
		} else {
			m_camera->SetPitch(45.0f);
			m_camera->FollowTarget(m_player->GetPosition() + (m_player->GetPlayerFront() * m_camera->GetPlayerPosOffset()), m_player->GetPlayerFront(), m_camera->GetPlayerCamRearOffset(), m_camera->GetPlayerCamHeightOffset());
			if (m_camera->HasSwitched())
				m_camera->StorePrevCam(m_camera->GetPosition(), m_player->GetPosition() + (m_player->GetPlayerFront() * m_camera->GetPlayerPosOffset()));

			glm::vec3 camPos = m_camera->GetPosition();
			if (camPos.y < 0.0f)
			{
				camPos.y = 0.0f;
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
		glm::vec3 camPos = m_camera->GetPosition();
		if (camPos.y < 0.0f)
		{
			camPos.y = 0.0f;
			m_camera->SetPosition(camPos);
		}

		glm::vec3 target = m_player->GetPosition() + (m_player->GetPlayerFront() * m_camera->GetPlayerPosOffset()) + (m_player->GetPlayerRight() * m_camera->GetPlayerAimRightOffset());
		if (target.y < 0.0f)
			target.y = 0.0f;

		m_camera->SetZoom(40.0f);
		if (m_camera->GetPitch() > 16.0f)
			m_camera->SetPitch(16.0f);

		if (m_camera->isBlending)
		{
			m_view = m_camera->UpdateCameraLerp(camPos, target, m_player->GetPlayerAimUp(), deltaTime);
		} else {


			m_camera->FollowTarget(m_player->GetPosition() + (m_player->GetPlayerFront() * m_camera->GetPlayerPosOffset()) + (m_player->GetPlayerRight() * m_camera->GetPlayerAimRightOffset()), m_player->GetPlayerAimFront(), m_camera->GetPlayerCamRearOffset(), m_camera->GetPlayerCamHeightOffset());
			

			glm::vec3 camPos = m_camera->GetPosition();
			if (camPos.y < 0.0f)
			{
				camPos.y = 0.0f;
				m_camera->SetPosition(camPos);
			}
			if (m_camera->HasSwitched())
				m_camera->StorePrevCam(m_camera->GetPosition(), m_player->GetPosition() + (m_player->GetPlayerFront() * m_camera->GetPlayerPosOffset()));

			m_view = m_camera->GetViewMatrixPlayerFollow(target, m_player->GetPlayerAimUp());
		}
	}

	m_cubemapView = glm::mat4(glm::mat3(m_camera->GetViewMatrixPlayerFollow(m_player->GetPosition(), glm::vec3(0.0f, 1.0f, 0.0f))));

	m_projection = glm::perspective(glm::radians(m_camera->GetZoom()), (float)width / (float)height, 0.1f, 500.0f);

	m_minimapView = m_minimapCamera->GetViewMatrix();
	m_minimapProjection = glm::perspective(glm::radians(m_camera->GetZoom()), (float)width / (float)height, 0.1f, 500.0f);

	m_player->SetCameraMatrices(m_view, m_projection);

	m_audioSystem->SetListener(m_view);
}

void GameManager::SetSceneData()
{
	m_renderer->SetScene(m_view, m_projection, m_cubemapView, dirLight);
}

void GameManager::SetUpDebugUi()
{
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();
}

void GameManager::ShowDebugUi()
{
#ifdef DEBUG
	ShowLightControlWindow(dirLight);
	ShowPerformanceWindow();
#endif

	if (!m_useEdbt)
	{
		ShowEnemyStateWindow();
	}
}

void GameManager::RenderDebugUi()
{
	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void GameManager::ShowLightControlWindow(DirLight& light)
{
	ImGui::Begin("Directional Light Control");

	ImGui::Text("Light Direction");
	ImGui::DragFloat3("Direction", (float*)&light.m_direction, dirLight.m_direction.x, dirLight.m_direction.y, dirLight.m_direction.z);

	ImGui::ColorEdit4("Ambient", (float*)&light.m_ambient);

	ImGui::ColorEdit4("Diffuse", (float*)&light.m_diffuse);
	ImGui::ColorEdit4("PBR Color", (float*)&dirLightPBRColour);

	ImGui::ColorEdit4("Specular", (float*)&light.m_specular);

	ImGui::DragFloat("Ortho Left", (float*)&m_orthoLeft);
	ImGui::DragFloat("Ortho Right", (float*)&m_orthoRight);
	ImGui::DragFloat("Ortho Bottom", (float*)&m_orthoBottom);
	ImGui::DragFloat("Ortho Top", (float*)&m_orthoTop);
	ImGui::DragFloat("Near Plane", (float*)&m_nearPlane);
	ImGui::DragFloat("Far Plane", (float*)&m_farPlane);

	ImGui::End();
}

void GameManager::ShowPerformanceWindow()
{
	ImGui::Begin("Performance");

	ImGui::Text("FPS: %.1f", m_fps);
	ImGui::Text("Avg FPS: %.1f", m_avgFps);
	ImGui::Text("Frame Time: %.1f ms", m_frameTime);
	ImGui::Text("Elapsed Time: %.1f s", m_elapsedTime);

	ImGui::End();
}

void GameManager::ShowEnemyStateWindow()
{
	ImGui::Begin("Game States");

	ImGui::Checkbox("Use EDBT", &m_useEdbt);

	ImGui::Text("Player Health: %d", (int)m_player->GetHealth());

	for (Enemy* e : m_enemies)
	{
		if (e == nullptr || e->IsDestroyed())
			continue;
		ImTextureID texID = (void*)(intptr_t)e->GetTexture().GetTexId();
		ImGui::Image(texID, ImVec2(100, 100));
		ImGui::SameLine();
		ImGui::Text("Enemy %d", e->GetID());
		ImGui::SameLine();
		ImGui::Text("State: %s", e->GetEDBTState().c_str());
		ImGui::SameLine();
		ImGui::Text("Health %d", (int)e->GetHealth());
	}

	ImGui::End();
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

void GameManager::CreateLightSpaceMatrices()
{
	int gridWidth = m_gameGrid->GetCellSize() * m_gameGrid->GetGridSize();
	glm::vec3 sceneCenter = glm::vec3(gridWidth / 2.0f, 0.0f, gridWidth / 2.0f);

	glm::vec3 lightDir = glm::normalize(dirLight.m_direction);

	float sceneDiagonal = (float)glm::sqrt(gridWidth * gridWidth + gridWidth * gridWidth);

	m_lightSpaceProjection = glm::ortho(m_orthoLeft, m_orthoRight, m_orthoBottom, m_orthoTop, m_nearPlane, m_farPlane);

	glm::vec3 lightPos = sceneCenter - lightDir * sceneDiagonal;

	m_lightSpaceView = glm::lookAt(lightPos, sceneCenter, glm::vec3(0.0f, -1.0f, 0.0f));
	m_lightSpaceMatrix = m_lightSpaceProjection * m_lightSpaceView;
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
	m_enemyStates = {
		{ false, false, 100.0f, 100.0f, false },
		{ false, false, 100.0f, 100.0f, false },
		{ false, false, 100.0f, 100.0f, false },
		{ false, false, 100.0f, 100.0f, false }
	};

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
			glm::vec3 enemyRayEnd = glm::vec3(0.0f);

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
			}

			if (enem->GetEnemyHasShot() && enem->GetEnemyDebugRayRenderTimer() > 0.0f)
			{
				glm::vec3 enemyLineColor = glm::vec3(0.2f, 0.2f, 0.2f);

				if (enem->GetEnemyHasHit())
				{
					enemyRayEnd = enem->GetEnemyHitPoint();
				}
				else
				{
					enemyRayEnd = enem->GetEnemyShootPos(m_muzzleOffset) + enem->GetEnemyShootDir() * enem->GetEnemyShootDistance();
				}

				m_enemyLines.at(enemyID)->UpdateVertexBuffer(enem->GetEnemyShootPos(m_muzzleOffset), enemyRayEnd);
				if (isMinimapPass)
				{
					m_enemyLines.at(enemyID)->DrawLine(m_minimapView, m_minimapProjection, enemyLineColor, m_lightSpaceMatrix, m_renderer->GetShadowMapTexture(), false, enem->GetEnemyDebugRayRenderTimer());
				}
				else if (isShadowPass)
				{
					m_enemyLines.at(enemyID)->DrawLine(m_lightSpaceView, m_lightSpaceProjection, enemyLineColor, m_lightSpaceMatrix, m_renderer->GetShadowMapTexture(), true, enem->GetEnemyDebugRayRenderTimer());
				}
				else
				{
					m_enemyLines.at(enemyID)->DrawLine(m_view, m_projection, enemyLineColor, m_lightSpaceMatrix, m_renderer->GetShadowMapTexture(), false, enem->GetEnemyDebugRayRenderTimer());
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
				m_playerMuzzleModel = glm::rotate(m_playerMuzzleModel, (-m_player->GetYaw() + 180.0f), glm::vec3(0.0f, 1.0f, 0.0f));
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

		if (m_physicsWorld->RayEnemyCrosshairIntersect(rayO, glm::normalize(rayEnd - rayO), crosshairHitpoint))
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
	m_player->UpdatePlayerVectors();
	m_player->UpdatePlayerAimVectors();

	m_player->Update(deltaTime);

	int enemyID = 0;
	for (Enemy* e : m_enemies)
	{
		if (e == nullptr || e->IsDestroyed())
			continue;

		e->SetDeltaTime(deltaTime);

		if (!m_useEdbt)
		{
			if (m_training)
			{
				e->EnemyDecision(m_enemyStates[e->GetID()], e->GetID(), m_squadActions, deltaTime, m_enemyStateQTable);
			}
			else
			{
				e->EnemyDecisionPrecomputedQ(m_enemyStates[e->GetID()], e->GetID(), m_squadActions, deltaTime, m_enemyStateQTable);
			}
		}

		e->Update(m_useEdbt);
	}

	m_audioManager->Update(deltaTime);
	m_audioSystem->Update(deltaTime);

	CalculatePerformance(deltaTime);
}

void GameManager::Render(bool isMinimapRenderPass, bool isShadowMapRenderPass, bool isMainRenderPass)
{

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



	for (auto obj : m_gameObjects) {
		if (obj->IsDestroyed())
			continue;
		if (isMinimapRenderPass)
		{
			m_renderer->Draw(obj, m_minimapView, m_minimapProjection, m_camera->GetPosition(), false, m_lightSpaceMatrix);
		}
		else if (isShadowMapRenderPass)
		{
			m_renderer->Draw(obj, m_lightSpaceView, m_lightSpaceProjection, m_camera->GetPosition(), true, m_lightSpaceMatrix);
		}
		else
		{
			m_renderer->Draw(obj, m_view, m_projection, m_camera->GetPosition(), false, m_lightSpaceMatrix);
		}
	}

	if (isMinimapRenderPass)
	{
		m_gameGrid->DrawGrid(m_gridShader, m_minimapView, m_minimapProjection, m_camera->GetPosition(), false, m_lightSpaceMatrix, m_renderer->GetShadowMapTexture(),
			dirLight.m_direction, dirLight.m_ambient, dirLight.m_diffuse, dirLight.m_specular);
	}
	else if (isShadowMapRenderPass)
	{
		m_gameGrid->DrawGrid(m_shadowMapShader, m_lightSpaceView, m_lightSpaceProjection, m_camera->GetPosition(), true, m_lightSpaceMatrix, m_renderer->GetShadowMapTexture(),
			dirLight.m_direction, dirLight.m_ambient, dirLight.m_diffuse, dirLight.m_specular);
	}
	else
	{
		m_gameGrid->DrawGrid(m_gridShader, m_view, m_projection, m_camera->GetPosition(), false, m_lightSpaceMatrix, m_renderer->GetShadowMapTexture(),
			dirLight.m_direction, dirLight.m_ambient, dirLight.m_diffuse, dirLight.m_specular);
	}

	if (m_camSwitchedToAim)
		m_camSwitchedToAim = false;

	RenderEnemyLineAndMuzzleFlash(isMainRenderPass, isMinimapRenderPass, isShadowMapRenderPass);

	m_renderer->DrawCubemap(m_cubemap);

	RenderPlayerCrosshairAndMuzzleFlash(isMainRenderPass);

	if (isMainRenderPass)
	{
		m_renderer->DrawMinimap(m_minimapQuad, &m_minimapShader);
	}

#ifdef _DEBUG
	if (isMainRenderPass)
	{
		m_renderer->DrawShadowMap(m_shadowMapQuad, &m_shadowMapQuadShader);
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