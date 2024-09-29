#include "GameManager.h"
#include "Components/AudioComponent.h"

#include "imgui/imgui.h"
#include "imgui/backend/imgui_impl_glfw.h"
#include "imgui/backend/imgui_impl_opengl3.h"

DirLight dirLight = {
        glm::vec3(-0.2f, -1.0f, -0.3f),

        glm::vec3(0.55f, 0.55f, 0.55f),
        glm::vec3(0.8f),
        glm::vec3(0.1f, 0.1f, 0.1f)
};

GameManager::GameManager(Window* window, unsigned int width, unsigned int height)
    : window(window)
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

    playerShader.loadShaders("C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Shaders/vertex_gpu_dquat.glsl", "C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Shaders/fragment_gpu_dquat.glsl");
    enemyShader.loadShaders("C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Shaders/vertex_gpu_dquat.glsl", "C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Shaders/fragment_gpu_dquat.glsl");
    gridShader.loadShaders("C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Shaders/gridVertex.glsl", "C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Shaders/gridFragment.glsl");
	crosshairShader.loadShaders("C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Shaders/crosshair_vert.glsl", "C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Shaders/crosshair_frag.glsl");
	lineShader.loadShaders("C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Shaders/line_vert.glsl", "C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Shaders/line_frag.glsl");
	aabbShader.loadShaders("C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Shaders/aabb_vert.glsl", "C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Shaders/aabb_frag.glsl");   
    cubeShader.loadShaders("C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Shaders/vertex.glsl", "C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Shaders/fragment.glsl");

	physicsWorld = new PhysicsWorld();

    cell = new Cell();
    cell->SetUpVAO();
    cell->LoadTexture("C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Assets/Textures/Cell.png");
    gameGrid = new Grid();
	cover1 = new Cube(gameGrid->snapToGrid(gameGrid->coverPositions[0]), glm::vec3(gameGrid->GetCellSize()), &cubeShader, false, this);
    cover1->SetAABBShader(&aabbShader);
    cover1->LoadMesh();
	cover2 = new Cube(gameGrid->snapToGrid(gameGrid->coverPositions[1]), glm::vec3(gameGrid->GetCellSize()), &cubeShader, false, this);
    cover2->SetAABBShader(&aabbShader);
    cover2->LoadMesh();
	cover3 = new Cube(gameGrid->snapToGrid(gameGrid->coverPositions[2]), glm::vec3(gameGrid->GetCellSize()), &cubeShader, false, this);
    cover3->SetAABBShader(&aabbShader);
    cover3->LoadMesh();
	cover4 = new Cube(gameGrid->snapToGrid(gameGrid->coverPositions[3]), glm::vec3(gameGrid->GetCellSize()), &cubeShader, false, this);
    cover4->SetAABBShader(&aabbShader);
    cover4->LoadMesh();

    gameGrid->initializeGrid();

    camera = new Camera(glm::vec3(50.0f, 3.0f, 80.0f));
    player = new Player(gameGrid->snapToGrid(glm::vec3(90.0f, 0.0f, 25.0f)), glm::vec3(3.0f), &playerShader, true, this);
    player->aabbShader = &aabbShader;

    std::string texture = "C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Assets/Models/GLTF/Enemies/Ely/ely-vanguardsoldier-kerwinatienza_diffuse.png";
    std::string texture2 = "C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Assets/Models/GLTF/Enemies/Ely/ely-vanguardsoldier-kerwinatienza_diffuse 2.png";
    std::string texture3 = "C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Assets/Models/GLTF/Enemies/Ely/ely-vanguardsoldier-kerwinatienza_diffuse 3.png";
    std::string texture4 = "C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Assets/Models/GLTF/Enemies/Ely/ely-vanguardsoldier-kerwinatienza_diffuse 4.png";

    enemy = new Enemy(gameGrid->snapToGrid(glm::vec3(33.0f, 0.0f, 23.0f)), glm::vec3(3.0f), &enemyShader, true, this, gameGrid, texture, 0, GetEventManager(), *player);
	enemy->aabbShader = &aabbShader;
    enemy->SetUpAABB();

    enemy2 = new Enemy(gameGrid->snapToGrid(glm::vec3(3.0f, 0.0f, 53.0f)), glm::vec3(3.0f), &enemyShader, true, this, gameGrid, texture2, 1, GetEventManager(), *player);
	enemy2->aabbShader = &aabbShader;
    enemy2->SetUpAABB();

    enemy3 = new Enemy(gameGrid->snapToGrid(glm::vec3(43.0f, 0.0f, 53.0f)), glm::vec3(3.0f), &enemyShader, true, this, gameGrid, texture3, 2, GetEventManager(), *player);
    enemy3->aabbShader = &aabbShader;
    enemy3->SetUpAABB();

    enemy4 = new Enemy(gameGrid->snapToGrid(glm::vec3(11.0f, 0.0f, 23.0f)), glm::vec3(3.0f), &enemyShader, true, this, gameGrid, texture4, 3, GetEventManager(), *player);
    enemy4->aabbShader = &aabbShader;
    enemy4->SetUpAABB();

	crosshair = new Crosshair(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f), &crosshairShader, false, this);
	crosshair->LoadMesh();
	crosshair->LoadTexture("C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Assets/Textures/Crosshair.png");
	line = new Line(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f), &lineShader, false, this);
    line->LoadMesh();

    enemyLine = new Line(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f), &lineShader, false, this);
    enemy2Line = new Line(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f), &lineShader, false, this);
    enemy3Line = new Line(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f), &lineShader, false, this);
    enemy4Line = new Line(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f), &lineShader, false, this);
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
    gameObjects.push_back(enemy2);  
    gameObjects.push_back(enemy3);  
    gameObjects.push_back(enemy4);  

    enemies.push_back(enemy);
    enemies.push_back(enemy2);
    enemies.push_back(enemy3);
    enemies.push_back(enemy4);

    mMusicEvent = audioSystem->PlayEvent("event:/Music");
}

void GameManager::setupCamera(unsigned int width, unsigned int height)
{
    if (camera->Mode == PLAYER_FOLLOW)
    {
        camera->FollowTarget(player->getPosition(), player->PlayerFront, camera->playerCamRearOffset, camera->playerCamHeightOffset);
        view = camera->GetViewMatrixPlayerFollow(player->getPosition(), glm::vec3(0.0f, 1.0f, 0.0f));
    }
    else if (camera->Mode == ENEMY_FOLLOW)
    {
        if (enemy->isDestroyed)
        {
			camera->Mode = FLY;
            return;
        }
        camera->FollowTarget(enemy->getPosition(), enemy->EnemyFront, camera->enemyCamRearOffset, camera->enemyCamHeightOffset);
        view = camera->GetViewMatrixEnemyFollow(enemy->getPosition(), glm::vec3(0.0f, 1.0f, 0.0f));
    }
    else if (camera->Mode == FLY)
        view = camera->GetViewMatrix();
    else if (camera->Mode == PLAYER_AIM)
    {
        camera->FollowTarget(player->getPosition() + (player->PlayerAimRight * -1.3f), player->PlayerAimFront, camera->playerCamRearOffset, camera->playerCamHeightOffset);
        view = camera->GetViewMatrixPlayerFollow(player->getPosition() + (player->PlayerAimRight * -1.3f), glm::vec3(0.0f, 1.0f, 0.0f));
    }

    projection = glm::perspective(glm::radians(camera->Zoom), (float)width / (float)height, 0.1f, 500.0f);

    audioSystem->SetListener(view);
}

void GameManager::setSceneData()
{
    renderer->setScene(view, projection, dirLight);
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

    ImGui::End();

    if (!enemy->isDestroyed)
    {

        ImGui::Begin("Enemy");

        float newYaw;

        ImGui::InputFloat3("Position", &enemy->getPosition()[0]);
        ImGui::InputFloat("EnemyCamPitch", &enemy->EnemyCameraPitch);
        ImGui::InputFloat("EnemyYaw", &newYaw);
        ImGui::InputFloat3("EnemyFront", &enemy->Front[0]);
        enemy->SetYaw(newYaw);

        if (enemy->state == PATROL)
            currentStateIndex = 0;
        if (enemy->state == ATTACK)
            currentStateIndex = 1;

        if (ImGui::Combo("Enemy States", &currentStateIndex, EnemyStateNames, 2))
        {
            enemy->state = static_cast<EnemyState>(currentStateIndex);
        }

        ImGui::End();
    }

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

        ImGui::ColorEdit4("Specular", (float*)&light.specular);

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
        ImGui::Begin("Enemy States");

        for (Enemy* e : enemies)
        {
			ImTextureID texID = (void*)(intptr_t)e->mTex.getTexID();
			ImGui::Text("Enemy %d", e->GetID());

			ImGui::Image(texID, ImVec2(100, 100));
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

    void GameManager::RemoveDestroyedGameObjects()
    {
        for (auto it = gameObjects.begin(); it != gameObjects.end(); ) {
            if ((*it)->isDestroyed) {
                delete* it; 
                *it = nullptr;
                it = gameObjects.erase(it); 
            }
            else {
                ++it;
            }
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

		for (Enemy* e : enemies)
		{
			if (e == nullptr || e->isDestroyed)
				continue;

			e->dt = deltaTime;
			e->Update();
		}
 
 	    audioSystem->Update(deltaTime);

		calculatePerformance(deltaTime);
}

void GameManager::render()
{
    static bool blendingChanged = playerCrossBlend;
    if (blendingChanged != playerCrossBlend)
    {
        blendingChanged = playerCrossBlend;
        if (!playerCrossBlend)
        {
            playerAdditiveBlend = false;
        }
        player->model->resetNodeData();
    }

    static bool additiveBlendingChanged = playerAdditiveBlend;
    if (additiveBlendingChanged != playerAdditiveBlend) {
        additiveBlendingChanged = playerAdditiveBlend;
        /* reset split when additive blending is disabled */
        if (!playerAdditiveBlend) {
            playerSkeletonSplitNode = player->model->getNodeCount() - 1;
        }
        player->model->resetNodeData();
    }

    static int skelSplitNode = playerSkeletonSplitNode;
    if (skelSplitNode != playerSkeletonSplitNode) {
        player->model->setSkeletonSplitNode(playerSkeletonSplitNode);
        playerSkeletonSplitNodeName = player->model->getNodeName(playerSkeletonSplitNode);
        skelSplitNode = playerSkeletonSplitNode;
        player->model->resetNodeData();
    }

    if (playerCrossBlend)
    {
        player->model->playAnimation(playerCrossBlendSourceClip, playerCrossBlendDestClip, 1.0f, playerAnimCrossBlendFactor, false);
    }
    else
    {
        if (player->GetVelocity() > 0.01f && player->GetVelocity() < 0.4f)
            player->SetAnimNum(15);
        else if (player->GetVelocity() >= 0.4f)
            player->SetAnimNum(10);
        else
            player->SetAnimNum(0);
    }

	glEnable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);

    for (auto obj : gameObjects) {
        renderer->draw(obj, view, projection);
    }
    
    gameGrid->drawGrid(gridShader, view, projection);

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
        
        crosshair->DrawCrosshair(glm::vec2(ndcX, ndcY));
        line->UpdateVertexBuffer(rayO, rayEnd);
        line->DrawLine(view, projection, lineColor);

    }
    if (camSwitchedToAim)
		camSwitchedToAim = false;

    if (enemy != nullptr && !enemy->isDestroyed)
    {
        if (enemy->enemyHasShot && enemy->enemyRayDebugRenderTimer > 0.0f)
        {
	    	glm::vec3 enemyLineColor = glm::vec3(1.0f, 1.0f, 0.0f);
	    	glm::vec3 enemyRayEnd = enemy->enemyShootPos + enemy->enemyShootDir * enemy->enemyShootDistance;
            enemyLine->UpdateVertexBuffer(enemy->enemyShootPos, enemyRayEnd);
	    	enemyLine->DrawLine(view, projection, enemyLineColor);
        }
    }

    if (enemy2 != nullptr && !enemy2->isDestroyed)
    {
        if (enemy2->enemyHasShot && enemy2->enemyRayDebugRenderTimer > 0.0f)
        {
            glm::vec3 enemy2LineColor = glm::vec3(1.0f, 1.0f, 0.0f);
            glm::vec3 enemy2RayEnd = enemy2->enemyShootPos + enemy2->enemyShootDir * enemy2->enemyShootDistance;
            enemy2Line->UpdateVertexBuffer(enemy2->enemyShootPos, enemy2RayEnd);
            enemy2Line->DrawLine(view, projection, enemy2LineColor);
        }
    }

    if (enemy3 != nullptr && !enemy3->isDestroyed)
    {
        if (enemy3->enemyHasShot && enemy3->enemyRayDebugRenderTimer > 0.0f)
        {
            glm::vec3 enemy3LineColor = glm::vec3(1.0f, 1.0f, 0.0f);
            glm::vec3 enemy3RayEnd = enemy3->enemyShootPos + enemy3->enemyShootDir * enemy3->enemyShootDistance;
            enemy3Line->UpdateVertexBuffer(enemy3->enemyShootPos, enemy3RayEnd);
            enemy3Line->DrawLine(view, projection, enemy3LineColor);
        }
    }

    if (enemy4 != nullptr && !enemy4->isDestroyed)
    {
        if (enemy4->enemyHasShot && enemy4->enemyRayDebugRenderTimer > 0.0f)
        {
            glm::vec3 enemy4LineColor = glm::vec3(1.0f, 1.0f, 0.0f);
            glm::vec3 enemy4RayEnd = enemy4->enemyShootPos + enemy4->enemyShootDir * enemy4->enemyShootDistance;
            enemy4Line->UpdateVertexBuffer(enemy4->enemyShootPos, enemy4RayEnd);
            enemy4Line->DrawLine(view, projection, enemy4LineColor);
        }
    }
}