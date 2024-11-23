#include "GameManager.h"
#include "Components/AudioComponent.h"

#include "imgui/imgui.h"
#include "imgui/backend/imgui_impl_glfw.h"
#include "imgui/backend/imgui_impl_opengl3.h"

DirLight dirLight = {
		glm::vec3(1.0f, -1.0f, 1.0f),

        glm::vec3(0.15f, 0.2f, 0.25f),
		glm::vec3(0.8, 0.7f, 0.7f),
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

    window->setInputManager(inputManager);
    
    renderer = window->getRenderer();
	renderer->SetUpMinimapFBO(width, height);
	renderer->SetUpShadowMapFBO(SHADOW_WIDTH, SHADOW_HEIGHT);

    playerShader.loadShaders("C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Shaders/vertex_gpu_dquat_player.glsl", "C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Shaders/pbr_fragment.glsl");
    enemyShader.loadShaders("C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Shaders/vertex_gpu_dquat_enemy.glsl", "C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Shaders/pbr_fragment_enemy.glsl");
    gridShader.loadShaders("C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Shaders/pbr_vertex.glsl", "C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Shaders/pbr_fragment_enemy.glsl");
	crosshairShader.loadShaders("C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Shaders/crosshair_vert.glsl", "C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Shaders/crosshair_frag.glsl");
	lineShader.loadShaders("C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Shaders/line_vert.glsl", "C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Shaders/line_frag.glsl");
	aabbShader.loadShaders("C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Shaders/aabb_vert.glsl", "C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Shaders/aabb_frag.glsl");   
    cubeShader.loadShaders("C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Shaders/pbr_vertex.glsl", "C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Shaders/pbr_fragment.glsl");
    cubemapShader.loadShaders("C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Shaders/cubemap_vertex.glsl", "C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Shaders/cubemap_fragment.glsl");
    minimapShader.loadShaders("C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Shaders/quad_vertex.glsl", "C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Shaders/quad_fragment.glsl");
	shadowMapShader.loadShaders("C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Shaders/shadow_map_vertex.glsl", "C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Shaders/shadow_map_fragment.glsl");
	shadowMapQuadShader.loadShaders("C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Shaders/shadow_map_quad_vertex.glsl", "C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Shaders/shadow_map_quad_fragment.glsl");

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
	cover1 = new Cube(gameGrid->snapToGrid(gameGrid->coverPositions[0]), glm::vec3((float)gameGrid->GetCellSize()), &cubeShader, &shadowMapShader, false, this, cubeTexFilename);
    cover1->SetAABBShader(&aabbShader);
    cover1->LoadMesh();
	cover2 = new Cube(gameGrid->snapToGrid(gameGrid->coverPositions[1]), glm::vec3((float)gameGrid->GetCellSize()), &cubeShader, &shadowMapShader, false, this, cubeTexFilename);
    cover2->SetAABBShader(&aabbShader);
    cover2->LoadMesh();
	cover3 = new Cube(gameGrid->snapToGrid(gameGrid->coverPositions[2]), glm::vec3((float)gameGrid->GetCellSize()), &cubeShader, &shadowMapShader, false, this, cubeTexFilename);
    cover3->SetAABBShader(&aabbShader);
    cover3->LoadMesh();
	cover4 = new Cube(gameGrid->snapToGrid(gameGrid->coverPositions[3]), glm::vec3((float)gameGrid->GetCellSize()), &cubeShader, &shadowMapShader, false, this, cubeTexFilename);
    cover4->SetAABBShader(&aabbShader);
    cover4->LoadMesh();	
    cover5 = new Cube(gameGrid->snapToGrid(gameGrid->coverPositions[4]), glm::vec3((float)gameGrid->GetCellSize()), &cubeShader, &shadowMapShader, false, this, cubeTexFilename);
    cover5->SetAABBShader(&aabbShader);
    cover5->LoadMesh();
	cover6 = new Cube(gameGrid->snapToGrid(gameGrid->coverPositions[5]), glm::vec3((float)gameGrid->GetCellSize()), &cubeShader, &shadowMapShader, false, this, cubeTexFilename);
	cover6->SetAABBShader(&aabbShader);
	cover6->LoadMesh();
	cover7 = new Cube(gameGrid->snapToGrid(gameGrid->coverPositions[6]), glm::vec3((float)gameGrid->GetCellSize()), &cubeShader, &shadowMapShader, false, this, cubeTexFilename);
	cover7->SetAABBShader(&aabbShader);
	cover7->LoadMesh();
	cover8 = new Cube(gameGrid->snapToGrid(gameGrid->coverPositions[7]), glm::vec3((float)gameGrid->GetCellSize()), &cubeShader, &shadowMapShader, false, this, cubeTexFilename);
    cover8->SetAABBShader(&aabbShader);
    cover8->LoadMesh();
	//cover9 = new Cube(gameGrid->snapToGrid(gameGrid->coverPositions[8]), glm::vec3((float)gameGrid->GetCellSize()), &cubeShader, false, this);
	//cover9->SetAABBShader(&aabbShader);
	//cover9->LoadMesh();
	//cover10 = new Cube(gameGrid->snapToGrid(gameGrid->coverPositions[9]), glm::vec3((float)gameGrid->GetCellSize()), &cubeShader, false, this);
	//cover10->SetAABBShader(&aabbShader);
	//cover10->LoadMesh();
	//cover11 = new Cube(gameGrid->snapToGrid(gameGrid->coverPositions[10]), glm::vec3((float)gameGrid->GetCellSize()), &cubeShader, false, this);
	//cover11->SetAABBShader(&aabbShader);
	//cover11->LoadMesh();
	//cover12 = new Cube(gameGrid->snapToGrid(gameGrid->coverPositions[11]), glm::vec3((float)gameGrid->GetCellSize()), &cubeShader, false, this);
	//cover12->SetAABBShader(&aabbShader);
	//cover12->LoadMesh();


    gameGrid->initializeGrid();

    camera = new Camera(glm::vec3(50.0f, 3.0f, 80.0f));
	minimapCamera = new Camera(glm::vec3((gameGrid->GetCellSize() * gameGrid->GetGridSize()) / 2.0f, 140.0f, (gameGrid->GetCellSize() * gameGrid->GetGridSize()) / 2.0f), glm::vec3(0.0f, -1.0f, 0.0f), 0.0f, -90.0f, glm::vec3(0.0f, 0.0f, -1.0f));
	
	minimapQuad = new Quad();
	minimapQuad->SetUpVAO();

	shadowMapQuad = new Quad();
	shadowMapQuad->SetUpVAO();

    player = new Player(gameGrid->snapToGrid(glm::vec3(90.0f, 0.0f, 25.0f)), glm::vec3(3.0f), &playerShader, &shadowMapShader, true, this);
    player->aabbShader = &aabbShader;

	std::string texture = "C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Assets/Models/GLTF/Enemies/Ely/EnemyEly_ely_vanguardsoldier_kerwinatienza_M2_BaseColor.png";
	std::string texture2 = "C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Assets/Models/GLTF/Enemies/Ely/ely-vanguardsoldier-kerwinatienza_diffuse_2.png";
	std::string texture3 = "C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Assets/Models/GLTF/Enemies/Ely/ely-vanguardsoldier-kerwinatienza_diffuse_3.png";
	std::string texture4 = "C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Assets/Models/GLTF/Enemies/Ely/ely-vanguardsoldier-kerwinatienza_diffuse_4.png";

    enemy = new Enemy(gameGrid->snapToGrid(glm::vec3(33.0f, 0.0f, 23.0f)), glm::vec3(3.0f), &enemyShader, &shadowMapShader, true, this, gameGrid, texture, 0, GetEventManager(), *player);
    enemy->SetAABBShader(&aabbShader);
    enemy->SetUpAABB();

    enemy2 = new Enemy(gameGrid->snapToGrid(glm::vec3(3.0f, 0.0f, 53.0f)), glm::vec3(3.0f), &enemyShader, &shadowMapShader, true, this, gameGrid, texture2, 1, GetEventManager(), *player);
	enemy2->SetAABBShader(&aabbShader);
    enemy2->SetUpAABB();

	enemy3 = new Enemy(gameGrid->snapToGrid(glm::vec3(43.0f, 0.0f, 53.0f)), glm::vec3(3.0f), &enemyShader, &shadowMapShader, true, this, gameGrid, texture3, 2, GetEventManager(), *player);
	enemy3->SetAABBShader(&aabbShader);
    enemy3->SetUpAABB();

    enemy4 = new Enemy(gameGrid->snapToGrid(glm::vec3(11.0f, 0.0f, 23.0f)), glm::vec3(3.0f), &enemyShader, &shadowMapShader, true, this, gameGrid, texture4, 3, GetEventManager(), *player);
    enemy4->SetAABBShader(&aabbShader);
    enemy4->SetUpAABB();

	crosshair = new Crosshair(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f), &crosshairShader, &shadowMapShader, false, this);
	crosshair->LoadMesh();
	crosshair->LoadTexture("C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Assets/Textures/Crosshair.png");
	line = new Line(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f), &lineShader, &shadowMapShader, false, this);
    line->LoadMesh();

    enemyLine = new Line(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f), &lineShader, &shadowMapShader, false, this);
    enemy2Line = new Line(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f), &lineShader, &shadowMapShader, false, this);
    enemy3Line = new Line(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f), &lineShader, &shadowMapShader, false, this);
    enemy4Line = new Line(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f), &lineShader, &shadowMapShader, false, this);
    enemyLine->LoadMesh();
    enemy2Line->LoadMesh();
    enemy3Line->LoadMesh();
    enemy4Line->LoadMesh();

    AudioComponent* fireAudioComponent = new AudioComponent(enemy);
    fireAudioComponent->PlayEvent("event:/FireLoop");


    inputManager->setContext(camera, player, enemy, width, height);

    /* reset skeleton split */
    playerSkeletonSplitNode = player->model->getNodeCount() - 1;
    enemySkeletonSplitNode = enemy->model->getNodeCount() - 1;

    std::srand(static_cast<unsigned int>(std::time(nullptr)));

    gameObjects.push_back(player);
    gameObjects.push_back(enemy); 
    gameObjects.push_back(cover1);
    gameObjects.push_back(cover2);
    gameObjects.push_back(cover3);
	gameObjects.push_back(cover4);    
    gameObjects.push_back(cover5);
	gameObjects.push_back(cover6);
	gameObjects.push_back(cover7);
	gameObjects.push_back(cover8);    
 //   gameObjects.push_back(cover9);
	//gameObjects.push_back(cover10);
	//gameObjects.push_back(cover11);
	//gameObjects.push_back(cover12);
    gameObjects.push_back(enemy2);  
    gameObjects.push_back(enemy3);  
    gameObjects.push_back(enemy4);  

    enemies.push_back(enemy);
    enemies.push_back(enemy2);
    enemies.push_back(enemy3);
    enemies.push_back(enemy4);

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
    else
    {
        for (auto& enem : enemies)
        {
            int enemyID = enem->GetID();
            Logger::log(1, "%s Loading Q Table for Enemy %d\n", __FUNCTION__, enemyID);
            LoadQTable(mEnemyStateQTable[enemyID], std::to_string(enemyID) + mEnemyStateFilename);
            Logger::log(1, "%s Loaded Q Table for Enemy %d\n", __FUNCTION__, enemyID);
        }
    }

    mMusicEvent = audioSystem->PlayEvent("event:/Music");
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
        view = camera->GetViewMatrixPlayerFollow(player->getPosition() + (player->PlayerFront * camera->playerPosOffset), player->PlayerAimUp);
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

	orthoLeft = -gridWidth * 2.0f;
	orthoRight = gridWidth * 2.0f;
	orthoBottom = -gridWidth * 2.0f;
	orthoTop = gridWidth * 2.0f;
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
    player->setPosition(player->initialPos);
    player->SetYaw(player->GetInitialYaw());
    player->SetAnimNum(0);
    player->isDestroyed = false;
    player->SetHealth(100.0f);
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
		emy->SetHealth(100);
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

        e->Update(useEDBT);
    }

    audioSystem->Update(deltaTime);

	calculatePerformance(deltaTime);
}

void GameManager::render(bool minimap, bool shadowMap, bool showShadowMap)
{
	glEnable(GL_DEPTH_TEST);
	glDisable(GL_BLEND);

	if (!shadowMap)
	{
		renderer->ResetViewport(screenWidth, screenHeight);
		renderer->clear();
	}

	if (minimap)
		renderer->bindMinimapFBO(screenWidth, screenHeight);

	if (shadowMap)
		renderer->bindShadowMapFBO(SHADOW_WIDTH, SHADOW_HEIGHT);



	for (auto obj : gameObjects) {
        if (obj->isDestroyed)
            continue;
		if (minimap)
		{
			renderer->draw(obj, minimapView, minimapProjection, camera->Position, false);
		}
		else if (shadowMap)
		{
			renderer->draw(obj, lightSpaceView, lightSpaceProjection, camera->Position, true);
		}
		else
		{
			renderer->draw(obj, view, projection, camera->Position, false);
		}
	}

	if (shadowMap)
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

	if (minimap)
	{
		gameGrid->drawGrid(gridShader, minimapView, minimapProjection, camera->Position, false);
	}
	else if (shadowMap)
	{
		gameGrid->drawGrid(shadowMapShader, lightSpaceView, lightSpaceProjection, camera->Position, true);
	}
	else
	{
		gameGrid->drawGrid(gridShader, view, projection, camera->Position, false);
	}

	if (camSwitchedToAim)
		camSwitchedToAim = false;

	if (!enemy->isDestroyed)
	{
		glm::vec3 enemyRayEnd = glm::vec3(0.0f);

		if (enemy->GetEnemyHasShot() && enemy->GetEnemyDebugRayRenderTimer() > 0.0f)
		{
			glm::vec3 enemyLineColor = glm::vec3(1.0f, 1.0f, 0.0f);

			if (enemy->GetEnemyHasHit())
			{
				enemyRayEnd = enemy->GetEnemyHitPoint();
			}
			else
			{
				enemyRayEnd = enemy->GetEnemyShootPos() + enemy->GetEnemyShootDir() * enemy->GetEnemyShootDistance();
			}

			glm::vec3 enemyRayEnd = enemy->GetEnemyShootPos() + enemy->GetEnemyShootDir() * enemy->GetEnemyShootDistance();
			enemyLine->UpdateVertexBuffer(enemy->GetEnemyShootPos(), enemyRayEnd);
			if (minimap)
			{
				enemyLine->DrawLine(minimapView, minimapProjection, enemyLineColor, false);
			}
			else if (shadowMap)
			{
				enemyLine->DrawLine(lightSpaceView, lightSpaceProjection, enemyLineColor, true);
			}
			else
			{
				enemyLine->DrawLine(view, projection, enemyLineColor, false);
			}
		}
	}

	if (!enemy2->isDestroyed)
	{
		glm::vec3 enemy2RayEnd = glm::vec3(0.0f);
		if (enemy2->GetEnemyHasShot() && enemy2->GetEnemyDebugRayRenderTimer() > 0.0f)
		{
			glm::vec3 enemy2LineColor = glm::vec3(1.0f, 1.0f, 0.0f);
            if (enemy2->GetEnemyHasHit())
            {
                enemy2RayEnd = enemy2->GetEnemyHitPoint();
            } 
            else
            {
			    enemy2RayEnd = enemy2->GetEnemyShootPos() + enemy2->GetEnemyShootDir() * enemy2->GetEnemyShootDistance();
            }

			enemy2Line->UpdateVertexBuffer(enemy2->GetEnemyShootPos(), enemy2RayEnd);
			if (minimap)
			{
				enemy2Line->DrawLine(minimapView, minimapProjection, enemy2LineColor, false);
			}
			else if (shadowMap)
			{
				enemy2Line->DrawLine(lightSpaceView, lightSpaceProjection, enemy2LineColor, true);
			}
			else
			{
				enemy2Line->DrawLine(view, projection, enemy2LineColor, false);
			}
		}
	}

	if (!enemy3->isDestroyed)
	{
		glm::vec3 enemy3RayEnd = glm::vec3(0.0f);

		if (enemy3->GetEnemyHasShot() && enemy3->GetEnemyDebugRayRenderTimer() > 0.0f)
		{
			glm::vec3 enemy3LineColor = glm::vec3(1.0f, 1.0f, 0.0f);

			if (enemy3->GetEnemyHasHit())
			{
				enemy3RayEnd = enemy3->GetEnemyHitPoint();
			}
			else
			{
				enemy3RayEnd = enemy3->GetEnemyShootPos() + enemy3->GetEnemyShootDir() * enemy3->GetEnemyShootDistance();
			}

			enemy3Line->UpdateVertexBuffer(enemy3->GetEnemyShootPos(), enemy3RayEnd);
			if (minimap)
			{
				enemy3Line->DrawLine(minimapView, minimapProjection, enemy3LineColor, false);
			}
			else if (shadowMap)
			{
				enemy3Line->DrawLine(lightSpaceView, lightSpaceProjection, enemy3LineColor, true);
			}
			else
			{
				enemy3Line->DrawLine(view, projection, enemy3LineColor, false);
			}
		}
	}

	if (!enemy4->isDestroyed)
	{
		glm::vec3 enemy4RayEnd = glm::vec3(0.0f);

		if (enemy4->GetEnemyHasShot() && enemy4->GetEnemyDebugRayRenderTimer() > 0.0f)
		{
			glm::vec3 enemy4LineColor = glm::vec3(1.0f, 1.0f, 0.0f);
			if (enemy3->GetEnemyHasHit())
			{
				enemy4RayEnd = enemy4->GetEnemyHitPoint();
			}
			else
			{
				enemy4RayEnd = enemy4->GetEnemyShootPos() + enemy4->GetEnemyShootDir() * enemy4->GetEnemyShootDistance();
			}

			enemy4Line->UpdateVertexBuffer(enemy4->GetEnemyShootPos(), enemy4RayEnd);
			if (minimap)
			{
				enemy4Line->DrawLine(minimapView, minimapProjection, enemy4LineColor, false);
			}
			else if (shadowMap)
			{
				enemy4Line->DrawLine(lightSpaceView, lightSpaceProjection, enemy4LineColor, true);
			}
			else
			{
				enemy4Line->DrawLine(view, projection, enemy4LineColor, false);
			}
		}
	}
//	renderer->setScene(view, projection, dirLight);
	renderer->drawCubemap(cubemap);
	if ((player->GetPlayerState() == AIMING || player->GetPlayerState() == SHOOTING) && camSwitchedToAim == false)
	{
		glDisable(GL_DEPTH_TEST);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		glm::vec3 rayO = player->GetShootPos();
		glm::vec3 rayD = glm::normalize(player->PlayerAimFront);
		float dist = player->GetShootDistance();

		glm::vec3 rayEnd = rayO + rayD * dist;

		glm::vec3 lineColor = glm::vec3(1.0f, 0.0f, 0.0f);

		glm::vec3 hitPoint;

		if (player->GetPlayerState() == SHOOTING)
		{
			lineColor = glm::vec3(0.0f, 1.0f, 0.0f);
		}

		glm::vec2 ndcPos = crosshair->CalculateCrosshairPosition(rayEnd, window->GetWidth(), window->GetHeight(), projection, view);

		float ndcX = (ndcPos.x / window->GetWidth()) * 2.0f - 1.0f;
		float ndcY = (ndcPos.y / window->GetHeight()) * 2.0f - 1.0f;

		if (!minimap && !shadowMap && !showShadowMap)
			crosshair->DrawCrosshair(glm::vec2(0.0f, 0.5f));

		glm::vec4 rayEndWorldSpace = glm::vec4(rayEnd, 1.0f);
		glm::vec4 rayEndCameraSpace = view * rayEndWorldSpace;
		glm::vec4 rayEndNDC = projection * rayEndCameraSpace;
		
		glm::vec4 targetNDC(0.0f, 0.5f, rayEndNDC.z / rayEndNDC.w, 1.0f); 
		glm::vec4 targetCameraSpace = glm::inverse(projection) * targetNDC;
		glm::vec4 targetWorldSpace = glm::inverse(view) * targetCameraSpace;
		
		rayEnd = glm::vec3(targetWorldSpace) / targetWorldSpace.w;

		line->UpdateVertexBuffer(rayO, rayEnd);

		if (minimap)
		{
			line->DrawLine(minimapView, minimapProjection, lineColor, false);
		}
		else if (shadowMap)
		{
			line->DrawLine(lightSpaceView, lightSpaceProjection, lineColor, true);
		}
		else
		{
			line->DrawLine(view, projection, lineColor, false);
		}

		glEnable(GL_DEPTH_TEST);
		glDisable(GL_BLEND);

	}
	
	if (!minimap && !shadowMap && !showShadowMap)
	{
		renderer->drawMinimap(minimapQuad, &minimapShader);
	}

	if (showShadowMap)
	{
		renderer->drawShadowMap(shadowMapQuad, &shadowMapQuadShader);
	}

	if (minimap)
	{
		renderer->unbindMinimapFBO();
	}
	else if (shadowMap)
	{
		renderer->unbindShadowMapFBO();
	}

}