#include "GameManager.h"
#include "imgui/imgui.h"
#include "imgui/backend/imgui_impl_glfw.h"
#include "imgui/backend/imgui_impl_opengl3.h"

GameManager::GameManager(Window* window, unsigned int width, unsigned int height)
    : window(window)
{
    inputManager = new InputManager();
    
    inputManager->setContext(camera, player, enemy, width, height);
    
    window->setInputManager(inputManager);
    
    renderer = window->getRenderer();

    playerShader.loadShaders("C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Shaders/vertex.glsl", "C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Shaders/fragment.glsl");

    enemyShader.loadShaders("C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Shaders/vertex.glsl", "C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Shaders/fragment.glsl");

    gridShader.loadShaders("C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Shaders/gridVertex.glsl", "C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Shaders/gridFragment.glsl");


    // TODO: Initialise Game Objects

    camera = new Camera(glm::vec3(50.0f, 3.0f, 80.0f));
    player = new Player(snapToGrid(glm::vec3(130.0f, 0.0f, 25.0f)), glm::vec3(0.02f, 0.02f, 0.02f), &playerShader);
    enemy = new Enemy(snapToGrid(glm::vec3(13.0f, 0.0f, 13.0f)), glm::vec3(0.02f, 0.02f, 0.02f), &enemyShader);

    waypoint1 = new Waypoint(snapToGrid(waypointPositions[0]), glm::vec3(5.0f, 10.0f, 5.0f), &gridShader);
    waypoint2 = new Waypoint(snapToGrid(waypointPositions[1]), glm::vec3(5.0f, 10.0f, 5.0f), &gridShader);
    waypoint3 = new Waypoint(snapToGrid(waypointPositions[2]), glm::vec3(5.0f, 10.0f, 5.0f), &gridShader);
    waypoint4 = new Waypoint(snapToGrid(waypointPositions[3]), glm::vec3(5.0f, 10.0f, 5.0f), &gridShader);

//  TODO: Render Ground
// 
//    Ground ground(glm::vec3(-100.0f, -0.3f, 50.0f), glm::vec3(100.0f, 1.0f, 100.0f));

    cell.SetUpVAO();

    initializeGrid();

    std::srand(static_cast<unsigned int>(std::time(nullptr)));

    currentWaypoint = waypointPositions[std::rand() % waypointPositions.size()];


    gameObjects.push_back(player);
    gameObjects.push_back(enemy);  
    gameObjects.push_back(waypoint1);
    gameObjects.push_back(waypoint2);
    gameObjects.push_back(waypoint3);
    gameObjects.push_back(waypoint4);
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
        camera->FollowTarget(enemy->getPosition(), enemy->EnemyFront, camera->enemyCamRearOffset, camera->enemyCamHeightOffset);
        view = camera->GetViewMatrixEnemyFollow(enemy->getPosition(), glm::vec3(0.0f, 1.0f, 0.0f));
    }
    else if (camera->Mode == FLY)
        view = camera->GetViewMatrix();

    projection = glm::perspective(glm::radians(camera->Zoom), (float)width / (float)height, 0.1f, 100.0f);

}

void GameManager::setSceneData()
{
    renderer->setScene(view, projection, dirLight);
}

void GameManager::showDebugUI()
{
    // Start the Dear ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    ShowLightControlWindow(dirLight);
    ShowCameraControlWindow(*camera);

    ImGui::Begin("Player");

    ImGui::InputFloat3("Position", &player->getPosition()[0]);
    ImGui::InputFloat("Yaw", &player->PlayerYaw);

    ImGui::End();

    ImGui::Begin("Enemy");

    ImGui::InputFloat3("Position", &enemy->getPosition()[0]);
    ImGui::InputFloat("EnemyCamPitch", &enemy->EnemyCameraPitch);
    ImGui::InputFloat("EnemyYaw", &enemy->Yaw);
    ImGui::InputFloat3("EnemyFront", &enemy->Front[0]);


    if (enemy->state == PATROL)
        currentStateIndex = 0;
    if (enemy->state == ATTACK)
        currentStateIndex = 1;

    if (ImGui::Combo("Enemy States", &currentStateIndex, EnemyStateNames, 2))
    {
        enemy->state = static_cast<EnemyState>(currentStateIndex);
    }

    ImGui::End();

    ShowLightControlWindow(dirLight);
    ShowCameraControlWindow(*camera);
}

void GameManager::ShowLightControlWindow(DirLight& light)
{
    ImGui::Begin("Directional Light Control");

    // Light direction control
    ImGui::Text("Light Direction");
    ImGui::DragFloat3("Direction", (float*)&light.direction, dirLight.direction.x, dirLight.direction.y, dirLight.direction.z);

    // Ambient color picker
    ImGui::ColorEdit4("Ambient", (float*)&light.ambient);

    // Diffuse color picker
    ImGui::ColorEdit4("Diffuse", (float*)&light.diffuse);

    // Specular color picker
    ImGui::ColorEdit4("Specular", (float*)&light.specular);

    ImGui::End();
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

    // TODO: Update Game Objects
    /*for (auto obj : gameObjects) {
        obj->update();
    }*/
}

void GameManager::render()
{
    // TODO:: Render Game Objects

    for (auto obj : gameObjects) {
       renderer->draw(obj);
    }

    gridShader.use();
    gridShader.setMat4("view", view);
    gridShader.setMat4("proj", projection);

    cell.BindVAO();

    drawGrid(gridShader);
}
