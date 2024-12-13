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
	: window(window), screenWidth(width), screenHeight(height)
{
	inputManager = new InputManager();
	audioSystem = new AudioSystem(this);

	if (!audioSystem->Initialize())
	{
		Logger::log(1, "%s error: AudioSystem init error\n", __FUNCTION__);
		audioSystem->Shutdown();
		delete audioSystem;
		audioSystem = nullptr;
	}

	mAudioManager = new AudioManager(this);

	window->setInputManager(inputManager);

	renderer = window->getRenderer();
	renderer->SetUpMinimapFBO(width, height);
	renderer->SetUpShadowMapFBO(SHADOW_WIDTH, SHADOW_HEIGHT);

	playerShader.loadShaders("C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Shaders/vertex_gpu_dquat_player.glsl", "C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Shaders/pbr_fragment.glsl");
	enemyShader.loadShaders("C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Shaders/vertex_gpu_dquat_enemy.glsl", "C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Shaders/pbr_fragment_emissive.glsl");
	
#ifdef DEBUG
	gridShader.loadShaders("C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Shaders/pbr_vertex.glsl", "C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Shaders/pbr_fragment.glsl");
#endif
	gridShader.loadShaders("C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Shaders/pbr_grid_debug_vert.glsl", "C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Shaders/pbr_grid_debug_frag.glsl");

	crosshairShader.loadShaders("C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Shaders/crosshair_vert.glsl", "C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Shaders/crosshair_frag.glsl");
	lineShader.loadShaders("C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Shaders/line_vert.glsl", "C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Shaders/line_frag.glsl");
	aabbShader.loadShaders("C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Shaders/aabb_vert.glsl", "C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Shaders/aabb_frag.glsl");
	cubeShader.loadShaders("C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Shaders/pbr_vertex.glsl", "C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Shaders/pbr_fragment_emissive.glsl");
	cubemapShader.loadShaders("C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Shaders/cubemap_vertex.glsl", "C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Shaders/cubemap_fragment.glsl");
	minimapShader.loadShaders("C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Shaders/quad_vertex.glsl", "C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Shaders/quad_fragment.glsl");
	shadowMapShader.loadShaders("C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Shaders/shadow_map_vertex.glsl", "C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Shaders/shadow_map_fragment.glsl");
	playerShadowMapShader.loadShaders("C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Shaders/shadow_map_player_vertex.glsl", "C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Shaders/shadow_map_fragment.glsl");
	enemyShadowMapShader.loadShaders("C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Shaders/shadow_map_enemy_vertex.glsl", "C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Shaders/shadow_map_fragment.glsl");
	shadowMapQuadShader.loadShaders("C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Shaders/shadow_map_quad_vertex.glsl", "C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Shaders/shadow_map_quad_fragment.glsl");
	playerMuzzleFlashShader.loadShaders("C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Shaders/muzzle_flash_vertex.glsl", "C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Shaders/muzzle_flash_fragment.glsl");

	physicsWorld = new PhysicsWorld();

	cubemapFaces = {
		"C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Assets/Textures/Skybox/right.png",
		"C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Assets/Textures/Skybox/left.png",
		"C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Assets/Textures/Skybox/top.png",
		"C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Assets/Textures/Skybox/bottom.png",
		"C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Assets/Textures/Skybox/front.png",
		"C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Assets/Textures/Skybox/back.png"
	};

	cubemap = new Cubemap(&cubemapShader);
	cubemap->LoadMesh();
	cubemap->LoadCubemap(cubemapFaces);

	cell = new Cell();
	cell->SetUpVAO();
	//    cell->LoadTexture("C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Assets/Textures/Ground.png", cell->mTex);
	std::string cubeTexFilename = "C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Assets/Textures/Cover.png";

	gameGrid = new Grid();

	for (glm::vec3 coverPos : gameGrid->coverPositions)
	{
		Cube* cover = new Cube(gameGrid->snapToGrid(coverPos), glm::vec3((float)gameGrid->GetCellSize()), &cubeShader, &shadowMapShader, false, this, cubeTexFilename);
		cover->SetAABBShader(&aabbShader);
		cover->LoadMesh();
		coverSpots.push_back(cover);
	}

	gameGrid->initializeGrid();

	camera = new Camera(glm::vec3(50.0f, 3.0f, 80.0f));
	minimapCamera = new Camera(glm::vec3((gameGrid->GetCellSize() * gameGrid->GetGridSize()) / 2.0f, 140.0f, (gameGrid->GetCellSize() * gameGrid->GetGridSize()) / 2.0f), glm::vec3(0.0f, -1.0f, 0.0f), 0.0f, -90.0f, glm::vec3(0.0f, 0.0f, -1.0f));

	minimapQuad = new Quad();
	minimapQuad->SetUpVAO(false);

	shadowMapQuad = new Quad();
	shadowMapQuad->SetUpVAO(false);

	playerMuzzleFlashQuad = new Quad();
	playerMuzzleFlashQuad->SetUpVAO(true);
	playerMuzzleFlashQuad->SetShader(&playerMuzzleFlashShader);
	playerMuzzleFlashQuad->LoadTexture("C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Assets/Textures/muzzleflash.png");

	enemyMuzzleFlashQuad = new Quad();
	enemyMuzzleFlashQuad->SetUpVAO(true);
	enemyMuzzleFlashQuad->SetShader(&playerMuzzleFlashShader);
	enemyMuzzleFlashQuad->LoadTexture("C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Assets/Textures/muzzleflash.png");

	enemy2MuzzleFlashQuad = new Quad();
	enemy2MuzzleFlashQuad->SetUpVAO(true);
	enemy2MuzzleFlashQuad->SetShader(&playerMuzzleFlashShader);
	enemy2MuzzleFlashQuad->LoadTexture("C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Assets/Textures/muzzleflash.png");

	enemy3MuzzleFlashQuad = new Quad();
	enemy3MuzzleFlashQuad->SetUpVAO(true);
	enemy3MuzzleFlashQuad->SetShader(&playerMuzzleFlashShader);
	enemy3MuzzleFlashQuad->LoadTexture("C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Assets/Textures/muzzleflash.png");

	enemy4MuzzleFlashQuad = new Quad();
	enemy4MuzzleFlashQuad->SetUpVAO(true);
	enemy4MuzzleFlashQuad->SetShader(&playerMuzzleFlashShader);
	enemy4MuzzleFlashQuad->LoadTexture("C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Assets/Textures/muzzleflash.png");


	player = new Player(gameGrid->snapToGrid(glm::vec3(90.0f, 0.0f, 25.0f)), glm::vec3(3.0f), &playerShader, &playerShadowMapShader, true, this);
	//	player = new Player(gameGrid->snapToGrid(glm::vec3(23.0f, 0.0f, 37.0f)), glm::vec3(3.0f), &playerShader, &playerShadowMapShader, true, this);

	player->aabbShader = &aabbShader;

	std::string texture = "C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Assets/Models/GLTF/Enemies/Ely/EnemyEly_ely_vanguardsoldier_kerwinatienza_M2_BaseColor.png";
	std::string texture2 = "C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Assets/Models/GLTF/Enemies/Ely/ely-vanguardsoldier-kerwinatienza_diffuse_2.png";
	std::string texture3 = "C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Assets/Models/GLTF/Enemies/Ely/ely-vanguardsoldier-kerwinatienza_diffuse_3.png";
	std::string texture4 = "C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Assets/Models/GLTF/Enemies/Ely/ely-vanguardsoldier-kerwinatienza_diffuse_4.png";

	enemy = new Enemy(gameGrid->snapToGrid(glm::vec3(33.0f, 0.0f, 23.0f)), glm::vec3(3.0f), &enemyShader, &enemyShadowMapShader, true, this, gameGrid, texture, 0, GetEventManager(), *player);
	enemy->SetAABBShader(&aabbShader);
	enemy->SetUpAABB();

	enemy2 = new Enemy(gameGrid->snapToGrid(glm::vec3(3.0f, 0.0f, 53.0f)), glm::vec3(3.0f), &enemyShader, &enemyShadowMapShader, true, this, gameGrid, texture2, 1, GetEventManager(), *player);
	enemy2->SetAABBShader(&aabbShader);
	enemy2->SetUpAABB();

	enemy3 = new Enemy(gameGrid->snapToGrid(glm::vec3(43.0f, 0.0f, 53.0f)), glm::vec3(3.0f), &enemyShader, &enemyShadowMapShader, true, this, gameGrid, texture3, 2, GetEventManager(), *player);
	enemy3->SetAABBShader(&aabbShader);
	enemy3->SetUpAABB();

	enemy4 = new Enemy(gameGrid->snapToGrid(glm::vec3(11.0f, 0.0f, 23.0f)), glm::vec3(3.0f), &enemyShader, &enemyShadowMapShader, true, this, gameGrid, texture4, 3, GetEventManager(), *player);
	enemy4->SetAABBShader(&aabbShader);
	enemy4->SetUpAABB();

	crosshair = new Crosshair(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.3f), &crosshairShader, &shadowMapShader, false, this);
	crosshair->LoadMesh();
	crosshair->LoadTexture("C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Assets/Textures/Crosshair.png");
	line = new Line(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f), &lineShader, &shadowMapShader, false, this);
	line->LoadMesh();

	inputManager->setContext(camera, player, enemy, width, height);

	/* reset skeleton split */
	playerSkeletonSplitNode = player->model->getNodeCount() - 1;
	enemySkeletonSplitNode = enemy->model->getNodeCount() - 1;

	std::srand(static_cast<unsigned int>(std::time(nullptr)));

	gameObjects.push_back(player);
	gameObjects.push_back(enemy);
	gameObjects.push_back(enemy2);
	gameObjects.push_back(enemy3);
	gameObjects.push_back(enemy4);

	for (Cube* coverSpot : coverSpots)
	{
		gameObjects.push_back(coverSpot);
	}

	enemies.push_back(enemy);
	enemies.push_back(enemy2);
	enemies.push_back(enemy3);
	enemies.push_back(enemy4);

	for (int i = 0; i < enemies.size(); i++)
	{
		line = new Line(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f), &lineShader, &shadowMapShader, false, this);
		line->LoadMesh();
		enemyLines.push_back(line);
	}

	if (initializeQTable)
	{
		for (auto& enem : enemies)
		{
			int enemyID = enem->GetID();
			Logger::log(1, "%s Initializing Q Table for Enemy %d\n", __FUNCTION__, enemyID);
			InitializeQTable(mEnemyStateQTable[enemyID]);
			Logger::log(1, "%s Initialized Q Table for Enemy %d\n", __FUNCTION__, enemyID);
		}
	}
	else if (loadQTable)
	{
		for (auto& enem : enemies)
		{
			int enemyID = enem->GetID();
			Logger::log(1, "%s Loading Q Table for Enemy %d\n", __FUNCTION__, enemyID);
			LoadQTable(mEnemyStateQTable[enemyID], std::to_string(enemyID) + mEnemyStateFilename);
			Logger::log(1, "%s Loaded Q Table for Enemy %d\n", __FUNCTION__, enemyID);
		}
	}

	mMusicEvent = audioSystem->PlayEvent("event:/bgm");
}

void GameManager::setupCamera(unsigned int width, unsigned int height)
{
	camera->Zoom = 45.0f;
	if (camera->Mode == PLAYER_FOLLOW)
	{
		camera->Pitch = 45.0f;
		camera->FollowTarget(player->getPosition() + (player->PlayerFront * camera->playerPosOffset), player->PlayerFront, camera->playerCamRearOffset, camera->playerCamHeightOffset);
		view = camera->GetViewMatrixPlayerFollow(player->getPosition() + (player->PlayerFront * camera->playerPosOffset), glm::vec3(0.0f, 1.0f, 0.0f));
	}
	else if (camera->Mode == ENEMY_FOLLOW)
	{
		if (enemy->isDestroyed)
		{
			camera->Mode = FLY;
			return;
		}
		camera->FollowTarget(enemy->getPosition(), enemy->GetEnemyFront(), camera->enemyCamRearOffset, camera->enemyCamHeightOffset);
		view = camera->GetViewMatrixEnemyFollow(enemy->getPosition(), glm::vec3(0.0f, 1.0f, 0.0f));
	}
	else if (camera->Mode == FLY)
	{
		if (firstFlyCamSwitch)
		{
			camera->FollowTarget(player->getPosition(), player->PlayerFront, camera->playerCamRearOffset, camera->playerCamHeightOffset);
			firstFlyCamSwitch = false;
			return;
		}
		view = camera->GetViewMatrix();
	}
	else if (camera->Mode == PLAYER_AIM)
	{
		camera->Zoom = 40.0f;
		if (camera->Pitch > 16.0f)
			camera->Pitch = 16.0f;

		camera->FollowTarget(player->getPosition() + (player->PlayerFront * camera->playerPosOffset) + (player->PlayerRight * camera->playerAimRightOffset), player->PlayerAimFront, camera->playerCamRearOffset, camera->playerCamHeightOffset);
		glm::vec3 target = player->getPosition() + (player->PlayerFront * camera->playerPosOffset);
		if (target.y < 0.0f)
			target.y = 0.0f;
		view = camera->GetViewMatrixPlayerFollow(target, player->PlayerAimUp);
	}
	cubemapView = glm::mat4(glm::mat3(camera->GetViewMatrixPlayerFollow(player->getPosition(), glm::vec3(0.0f, 1.0f, 0.0f))));

	projection = glm::perspective(glm::radians(camera->Zoom), (float)width / (float)height, 0.1f, 500.0f);

	minimapView = minimapCamera->GetViewMatrix();
	minimapProjection = glm::perspective(glm::radians(camera->Zoom), (float)width / (float)height, 0.1f, 500.0f);

	player->SetCameraMatrices(view, projection);

	audioSystem->SetListener(view);
}

void GameManager::setSceneData()
{
	renderer->setScene(view, projection, cubemapView, dirLight);
}

void GameManager::setUpDebugUI()
{
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();
}

void GameManager::showDebugUI()
{
	ShowLightControlWindow(dirLight);
	ShowCameraControlWindow(*camera);

	ImGui::Begin("Player");

	ImGui::InputFloat3("Position", &player->getPosition()[0]);
	ImGui::InputFloat("Yaw", &player->PlayerYaw);
	ImGui::InputFloat3("Player Front", &player->PlayerFront[0]);
	ImGui::InputFloat3("Player Aim Front", &player->PlayerAimFront[0]);
	ImGui::InputFloat("Player Aim Pitch", &player->aimPitch);
	ImGui::InputFloat("Player Rear Offset", &camera->playerCamRearOffset);
	ImGui::InputFloat("Player Height Offset", &camera->playerCamHeightOffset);
	ImGui::InputFloat("Player Pos Offset", &camera->playerPosOffset);
	ImGui::InputFloat("Player Aim Right Offset", &camera->playerAimRightOffset);
	ImGui::End();
	ShowAnimationControlWindow();

	ShowPerformanceWindow();
	ShowEnemyStateWindow();
}

void GameManager::renderDebugUI()
{
	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void GameManager::ShowLightControlWindow(DirLight& light)
{
	ImGui::Begin("Directional Light Control");

	ImGui::Text("Light Direction");
	ImGui::DragFloat3("Direction", (float*)&light.direction, dirLight.direction.x, dirLight.direction.y, dirLight.direction.z);

	ImGui::ColorEdit4("Ambient", (float*)&light.ambient);

	ImGui::ColorEdit4("Diffuse", (float*)&light.diffuse);
	ImGui::ColorEdit4("PBR Color", (float*)&dirLightPBRColour);

	ImGui::ColorEdit4("Specular", (float*)&light.specular);

	ImGui::DragFloat("Ortho Left", (float*)&orthoLeft);
	ImGui::DragFloat("Ortho Right", (float*)&orthoRight);
	ImGui::DragFloat("Ortho Bottom", (float*)&orthoBottom);
	ImGui::DragFloat("Ortho Top", (float*)&orthoTop);
	ImGui::DragFloat("Near Plane", (float*)&near_plane);
	ImGui::DragFloat("Far Plane", (float*)&far_plane);

	ImGui::End();
}

void GameManager::ShowAnimationControlWindow()
{
	ImGui::Begin("Player Animation Control");

	ImGui::Checkbox("Blending Type: ", &playerCrossBlend);
	ImGui::SameLine();
	if (playerCrossBlend)
	{
		ImGui::Text("Cross");
	}
	else
	{
		ImGui::Text("Single");
	}

	if (playerCrossBlend)
		ImGui::BeginDisabled();

	ImGui::Text("Player Blend Factor");
	ImGui::SameLine();
	ImGui::SliderFloat("##PlayerBlendFactor", &playerAnimBlendFactor, 0.0f, 1.0f);

	if (playerCrossBlend)
		ImGui::EndDisabled();

	if (!playerCrossBlend)
		ImGui::BeginDisabled();

	ImGui::Text("Source Clip   ");
	ImGui::SameLine();
	ImGui::SliderInt("##SourceClip", &playerCrossBlendSourceClip, 0, player->model->getAnimClipsSize() - 1);


	ImGui::Text("Dest Clip   ");
	ImGui::SameLine();
	ImGui::SliderInt("##DestClip", &playerCrossBlendDestClip, 0, player->model->getAnimClipsSize() - 1);

	ImGui::Text("Cross Blend ");
	ImGui::SameLine();
	ImGui::SliderFloat("##CrossBlendFactor", &playerAnimCrossBlendFactor, 0.0f, 1.0f);

	ImGui::Checkbox("Additive Blending", &playerAdditiveBlend);

	if (!playerAdditiveBlend) {
		ImGui::BeginDisabled();
	}
	ImGui::Text("Split Node  ");
	ImGui::SameLine();
	ImGui::SliderInt("##SplitNode", &playerSkeletonSplitNode, 0, player->model->getNodeCount() - 1);
	ImGui::Text("Split Node Name: %s", playerSkeletonSplitNodeName.c_str());

	if (!playerAdditiveBlend) {
		ImGui::EndDisabled();
	}

	if (!playerCrossBlend)
		ImGui::EndDisabled();

	ImGui::End();

}

void GameManager::ShowPerformanceWindow()
{
	ImGui::Begin("Performance");

	ImGui::Text("FPS: %.1f", fps);
	ImGui::Text("Avg FPS: %.1f", avgFPS);
	ImGui::Text("Frame Time: %.1f ms", frameTime);
	ImGui::Text("Elapsed Time: %.1f s", elapsedTime);

	ImGui::End();
}

void GameManager::ShowEnemyStateWindow()
{
	ImGui::Begin("Game States");

	ImGui::Checkbox("Use EDBT", &useEDBT);

	ImGui::InputFloat("Speed Divider", &speedDivider);
	ImGui::InputFloat("Blend Factor", &blendFac);
	ImGui::InputFloat("Muzzle Offset", &muzzleOffset);

	ImGui::InputFloat3("Enemy Muzzle Flash Offset", &enemyMuzzleFlashOffset[0]);


	ImGui::Text("Player Health: %d", (int)player->GetHealth());

	for (Enemy* e : enemies)
	{
		if (e == nullptr || e->isDestroyed)
			continue;
		ImTextureID texID = (void*)(intptr_t)e->mTex.getTexID();
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

void GameManager::calculatePerformance(float deltaTime)
{
	fps = 1.0f / deltaTime;

	fpsSum += fps;
	frameCount++;

	if (frameCount == numFramesAvg)
	{
		avgFPS = fpsSum / numFramesAvg;
		fpsSum = 0.0f;
		frameCount = 0;
	}

	frameTime = deltaTime * 1000.0f;

	elapsedTime += deltaTime;
}

void GameManager::CreateLightSpaceMatrices()
{
	float gridWidth = gameGrid->GetCellSize() * gameGrid->GetGridSize();
	glm::vec3 sceneCenter = glm::vec3(gridWidth / 2.0f, 0.0f, gridWidth / 2.0f);

	glm::vec3 lightDir = glm::normalize(dirLight.direction);

	float sceneDiagonal = glm::sqrt(gridWidth * gridWidth + gridWidth * gridWidth);

	//orthoLeft = -gridWidth * 2.0f;
	//orthoRight = gridWidth * 2.0f;
	//orthoBottom = -gridWidth * 2.0f;
	//orthoTop = gridWidth * 2.0f;
//	near_plane = 1.0f;
//	far_plane = 150.0f;
	lightSpaceProjection = glm::ortho(orthoLeft, orthoRight, orthoBottom, orthoTop, near_plane, far_plane);

	glm::vec3 lightPos = sceneCenter - lightDir * sceneDiagonal;
	//lightPos.y += 50.0f;

	lightSpaceView = glm::lookAt(lightPos, sceneCenter, glm::vec3(0.0f, -1.0f, 0.0f));
	lightSpaceMatrix = lightSpaceProjection * lightSpaceView;
}

void GameManager::RemoveDestroyedGameObjects()
{
	//for (auto it = gameObjects.begin(); it != gameObjects.end(); ) {
	//    if ((*it)->isDestroyed) {
	//        if ((*it)->isEnemy)
	//        {
				//for (auto it2 = enemies.begin(); it2 != enemies.end(); )
				//{
				//	if ((*it2) == (*it))
				//	{
				//		delete* it2;
				//		*it2 = nullptr;
				//		it2 = enemies.erase(it2);
				//	}
				//	else
				//	{
				//		++it2;
				//	}
				//}
	//        }
	//        else
	//        {
	//            delete* it; 
	//            *it = nullptr;
	//        }
	//        it = gameObjects.erase(it); 
	//    }
	//    else {
	//        ++it;
	//    }
	//}

	if ((enemy->isDestroyed && enemy2->isDestroyed && enemy3->isDestroyed && enemy4->isDestroyed) || player->isDestroyed)
		ResetGame();
}

void GameManager::ResetGame()
{
	camera->SetMode(PLAYER_FOLLOW);
	mAudioManager->ClearQueue();
	player->setPosition(player->initialPos);
	player->SetYaw(player->GetInitialYaw());
	player->SetAnimNum(0);
	player->isDestroyed = false;
	player->SetHealth(100.0f);
	player->UpdatePlayerVectors();
	player->UpdatePlayerAimVectors();
	player->SetPlayerState(PlayerState::MOVING);
	player->aabbColor = glm::vec3(0.0f, 0.0f, 1.0f);
	enemy->isDestroyed = false;
	enemy2->isDestroyed = false;
	enemy3->isDestroyed = false;
	enemy4->isDestroyed = false;
	enemy->isDead_ = false;
	enemy2->isDead_ = false;
	enemy3->isDead_ = false;
	enemy4->isDead_ = false;
	enemy->setPosition(enemy->getInitialPosition());
	enemy2->setPosition(enemy2->getInitialPosition());
	enemy3->setPosition(enemy3->getInitialPosition());
	enemy4->setPosition(enemy4->getInitialPosition());
	enemyStates = {
		{ false, false, 100.0f, 100.0f, false },
		{ false, false, 100.0f, 100.0f, false },
		{ false, false, 100.0f, 100.0f, false },
		{ false, false, 100.0f, 100.0f, false }
	};
	//enemies.push_back(enemy);
	//enemies.push_back(enemy2);
	//enemies.push_back(enemy3);
	//enemies.push_back(enemy4);
	//gameObjects.push_back(enemy);
	//gameObjects.push_back(enemy2);
	//gameObjects.push_back(enemy3);
	//gameObjects.push_back(enemy4);

	for (Enemy* emy : enemies)
	{
		emy->ResetState();
		physicsWorld->addCollider(emy->GetAABB());
		physicsWorld->addEnemyCollider(emy->GetAABB());
		emy->SetHealth(100.0f);
	}

}

void GameManager::ShowCameraControlWindow(Camera& cam)
{
	ImGui::Begin("Camera Control");

	std::string modeText = "";

	if (cam.Mode == FLY)
	{
		modeText = "Flycam";


		cam.UpdateCameraVectors();
	}
	else if (cam.Mode == PLAYER_FOLLOW)
		modeText = "Player Follow";
	else if (cam.Mode == ENEMY_FOLLOW)
		modeText = "Enemy Follow";
	else if (cam.Mode == PLAYER_AIM)
		modeText = "Player Aim"
		;
	ImGui::Text(modeText.c_str());

	ImGui::InputFloat3("Position", (float*)&cam.Position);

	ImGui::InputFloat("Pitch", (float*)&cam.Pitch);
	ImGui::InputFloat("Yaw", (float*)&cam.Yaw);
	ImGui::InputFloat("Zoom", (float*)&cam.Zoom);

	ImGui::End();
}

void GameManager::update(float deltaTime)
{
	inputManager->processInput(window->getWindow(), deltaTime);
	player->UpdatePlayerVectors();
	player->UpdatePlayerAimVectors();

	player->Update(deltaTime);

	int enemyID = 0;
	for (Enemy* e : enemies)
	{
		if (e == nullptr || e->isDestroyed)
			continue;

		e->SetDeltaTime(deltaTime);

		if (!useEDBT)
		{
			if (training)
			{
				e->EnemyDecision(enemyStates[e->GetID()], e->GetID(), squadActions, deltaTime, mEnemyStateQTable);
			}
			else
			{
				e->EnemyDecisionPrecomputedQ(enemyStates[e->GetID()], e->GetID(), squadActions, deltaTime, mEnemyStateQTable);
			}
		}

		e->Update(useEDBT, speedDivider, blendFac);
	}

	mAudioManager->Update(deltaTime);
	audioSystem->Update(deltaTime);

	calculatePerformance(deltaTime);
}

void GameManager::render(bool isMinimapRenderPass, bool isShadowMapRenderPass, bool isMainRenderPass)
{

	renderer->ResetRenderStates();


	if (!isShadowMapRenderPass)
	{
		renderer->ResetViewport(screenWidth, screenHeight);
		renderer->clear();
	}

	if (isMinimapRenderPass)
		renderer->bindMinimapFBO(screenWidth, screenHeight);

	if (isShadowMapRenderPass)
		renderer->bindShadowMapFBO(SHADOW_WIDTH, SHADOW_HEIGHT);



	for (auto obj : gameObjects) {
		if (obj->isDestroyed)
			continue;
		if (isMinimapRenderPass)
		{
			renderer->draw(obj, minimapView, minimapProjection, camera->Position, false, lightSpaceMatrix);
		}
		else if (isShadowMapRenderPass)
		{
			renderer->draw(obj, lightSpaceView, lightSpaceProjection, camera->Position, true, lightSpaceMatrix);
		}
		else
		{
			renderer->draw(obj, view, projection, camera->Position, false, lightSpaceMatrix);
		}
	}

	if (isShadowMapRenderPass)
	{
		shadowMapShader.use();
		shadowMapShader.setVec3("dirLight.direction", dirLight.direction);
		shadowMapShader.setVec3("dirLight.ambient", dirLight.ambient);
		shadowMapShader.setVec3("dirLight.diffuse", dirLight.diffuse);
		shadowMapShader.setVec3("dirLight.specular", dirLight.specular);
	}
	else
	{
		gridShader.use();
		gridShader.setVec3("dirLight.direction", dirLight.direction);
		gridShader.setVec3("dirLight.ambient", dirLight.ambient);
		gridShader.setVec3("dirLight.diffuse", dirLight.diffuse);
		gridShader.setVec3("dirLight.specular", dirLight.specular);
	}

	if (isMinimapRenderPass)
	{
		gameGrid->drawGrid(gridShader, minimapView, minimapProjection, camera->Position, false, lightSpaceMatrix, renderer->GetShadowMapTexture());
	}
	else if (isShadowMapRenderPass)
	{
		gameGrid->drawGrid(shadowMapShader, lightSpaceView, lightSpaceProjection, camera->Position, true, lightSpaceMatrix, renderer->GetShadowMapTexture());
	}
	else
	{
		gameGrid->drawGrid(gridShader, view, projection, camera->Position, false, lightSpaceMatrix, renderer->GetShadowMapTexture());
	}

	if (camSwitchedToAim)
		camSwitchedToAim = false;

	for (auto& enem : enemies)
	{
		if (!enem->isDestroyed)
		{
			glm::vec3 enemyRayEnd = glm::vec3(0.0f);

			int enemyID = enem->GetID();

			if (enem->GetEnemyHasShot())
			{
				float enemyMuzzleCurrentTime = glfwGetTime();

				if (renderEnemyMuzzleFlash.at(enemyID) && enemyMuzzleFlashStartTime.at(enemyID) + enemyMuzzleFlashDuration.at(enemyID) > enemyMuzzleCurrentTime)
				{
					renderEnemyMuzzleFlash.at(enemyID) = false;
				}
				else
				{
					renderEnemyMuzzleFlash.at(enemyID) = true;
					enemyMuzzleFlashStartTime.at(enemyID) = enemyMuzzleCurrentTime;
				}

				if (renderEnemyMuzzleFlash.at(enemyID) && isMainRenderPass)
				{
					enemyMuzzleTimeSinceStart.at(enemyID) = enemyMuzzleCurrentTime - enemyMuzzleFlashStartTime.at(enemyID);
					enemyMuzzleAlpha.at(enemyID) = glm::max(0.0f, 1.0f - (enemyMuzzleTimeSinceStart.at(enemyID) / enemyMuzzleFlashDuration.at(enemyID)));
					enemyMuzzleFlashScale.at(enemyID) = 1.0f + (0.5f * enemyMuzzleAlpha.at(enemyID));

					enemyMuzzleModel.at(enemyID) = glm::mat4(1.0f);

					enemyMuzzleModel.at(enemyID) = glm::translate(enemyMuzzleModel.at(enemyID), enem->GetEnemyShootPos(muzzleOffset));
					enemyMuzzleModel.at(enemyID) = glm::rotate(enemyMuzzleModel.at(enemyID), enem->yaw, glm::vec3(0.0f, 1.0f, 0.0f));
					enemyMuzzleModel.at(enemyID) = glm::scale(enemyMuzzleModel.at(enemyID), glm::vec3(enemyMuzzleFlashScale.at(enemyID), enemyMuzzleFlashScale.at(enemyID), 1.0f));
					enemyMuzzleFlashQuad->Draw3D(enemyMuzzleFlashTint.at(enemyID), enemyMuzzleAlpha.at(enemyID), projection, view, enemyMuzzleModel.at(enemyID));
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
					enemyRayEnd = enem->GetEnemyShootPos(muzzleOffset) + enem->GetEnemyShootDir() * enem->GetEnemyShootDistance();
				}

				enemyLines.at(enemyID)->UpdateVertexBuffer(enem->GetEnemyShootPos(muzzleOffset), enemyRayEnd);
				if (isMinimapRenderPass)
				{
					enemyLines.at(enemyID)->DrawLine(minimapView, minimapProjection, enemyLineColor, lightSpaceMatrix, renderer->GetShadowMapTexture(), false, enem->GetEnemyDebugRayRenderTimer());
				}
				else if (isShadowMapRenderPass)
				{
					enemyLines.at(enemyID)->DrawLine(lightSpaceView, lightSpaceProjection, enemyLineColor, lightSpaceMatrix, renderer->GetShadowMapTexture(), true, enem->GetEnemyDebugRayRenderTimer());
				}
				else
				{
					enemyLines.at(enemyID)->DrawLine(view, projection, enemyLineColor, lightSpaceMatrix, renderer->GetShadowMapTexture(), false, enem->GetEnemyDebugRayRenderTimer());
				}
			}
		}
	}
	//	renderer->setScene(view, projection, dirLight);
	renderer->drawCubemap(cubemap);
	if ((player->GetPlayerState() == AIMING || player->GetPlayerState() == SHOOTING) && camSwitchedToAim == false && isMainRenderPass)
	{
		renderer->RemoveDepthAndSetBlending();

		glm::vec3 rayO = player->GetShootPos();
		glm::vec3 rayD = glm::normalize(player->PlayerAimFront);
		float dist = player->GetShootDistance();

		glm::vec3 rayEnd = rayO + rayD * dist;

		glm::vec3 lineColor = glm::vec3(1.0f, 0.0f, 0.0f);

		glm::vec3 hitPoint;

		if (player->GetPlayerState() == SHOOTING)
		{
			lineColor = glm::vec3(0.0f, 1.0f, 0.0f);

			float currentTime = glfwGetTime();

			if (renderPlayerMuzzleFlash && playerMuzzleFlashStartTime + playerMuzzleFlashDuration > currentTime)
			{
				renderPlayerMuzzleFlash = false;
			}
			else
			{
				renderPlayerMuzzleFlash = true;
				playerMuzzleFlashStartTime = currentTime;
			}


			if (renderPlayerMuzzleFlash)

			{
				playerMuzzleTimeSinceStart = currentTime - playerMuzzleFlashStartTime;
				playerMuzzleAlpha = glm::max(0.0f, 1.0f - (playerMuzzleTimeSinceStart / playerMuzzleFlashDuration));
				playerMuzzleFlashScale = 1.0f + (0.5f * playerMuzzleAlpha);

				playerMuzzleModel = glm::mat4(1.0f);

				playerMuzzleModel = glm::translate(playerMuzzleModel, player->GetShootPos());
				playerMuzzleModel = glm::rotate(playerMuzzleModel, (-player->yaw + 180.0f), glm::vec3(0.0f, 1.0f, 0.0f));
				playerMuzzleModel = glm::scale(playerMuzzleModel, glm::vec3(playerMuzzleFlashScale, playerMuzzleFlashScale, 1.0f));
				playerMuzzleFlashQuad->Draw3D(playerMuzzleTint, playerMuzzleAlpha, projection, view, playerMuzzleModel);
			}
		}


		glm::vec4 rayEndWorldSpace = glm::vec4(rayEnd, 1.0f);
		glm::vec4 rayEndCameraSpace = view * rayEndWorldSpace;
		glm::vec4 rayEndNDC = projection * rayEndCameraSpace;

		glm::vec4 targetNDC(0.0f, 0.5f, rayEndNDC.z / rayEndNDC.w, 1.0f);
		glm::vec4 targetCameraSpace = glm::inverse(projection) * targetNDC;
		glm::vec4 targetWorldSpace = glm::inverse(view) * targetCameraSpace;

		rayEnd = glm::vec3(targetWorldSpace) / targetWorldSpace.w;

		glm::vec3 crosshairHitpoint;
		glm::vec3 crosshairCol;

		if (physicsWorld->rayEnemyCrosshairIntersect(rayO, glm::normalize(rayEnd - rayO), crosshairHitpoint))
		{
			crosshairCol = glm::vec3(1.0f, 0.0f, 0.0f);
		}
		else
		{
			crosshairCol = glm::vec3(1.0f, 1.0f, 1.0f);
		}

		glm::vec2 ndcPos = crosshair->CalculateCrosshairPosition(rayEnd, window->GetWidth(), window->GetHeight(), projection, view);

		float ndcX = (ndcPos.x / window->GetWidth()) * 2.0f - 1.0f;
		float ndcY = (ndcPos.y / window->GetHeight()) * 2.0f - 1.0f;

		if (isMainRenderPass)
			crosshair->DrawCrosshair(glm::vec2(0.0f, 0.5f), crosshairCol);

		renderer->ResetRenderStates();

	}

	if (isMainRenderPass)
	{
		renderer->drawMinimap(minimapQuad, &minimapShader);
	}

#ifdef _DEBUG
	if (isMainRenderPass)
	{
		renderer->drawShadowMap(shadowMapQuad, &shadowMapQuadShader);
	}
#endif

	if (isMinimapRenderPass)
	{
		renderer->unbindMinimapFBO();
	}
	else if (isShadowMapRenderPass)
	{
		renderer->unbindShadowMapFBO();
	}

}