#include "Player.h"
#include "GameManager.h"

void Player::drawObject(glm::mat4 viewMat, glm::mat4 proj)
{
    glm::mat4 modelMat = glm::mat4(1.0f);
    modelMat = glm::translate(modelMat, position);
    modelMat = glm::rotate(modelMat, glm::radians(-yaw + 180.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    modelMat = glm::scale(modelMat, scale);
    std::vector<glm::mat4> matrixData;
    matrixData.push_back(viewMat);
    matrixData.push_back(proj);
    matrixData.push_back(modelMat);
    mUniformBuffer.uploadUboData(matrixData, 0);

    mPlayerDualQuatSSBuffer.uploadSsboData(model->getJointDualQuats(), 2);

//    model->playAnimation(0, 0.8f);

    if (uploadVertexBuffer)
    {
        model->uploadVertexBuffers();
		aabb = new AABB();
		aabb->calculateAABB(model->getVertices());
        aabb->setShader(aabbShader);
        aabb->setUpMesh();
        aabb->owner = this;
		aabb->isPlayer = true;
        updateAABB();
        GameManager* gameMgr = GetGameManager();
        gameMgr->GetPhysicsWorld()->addCollider(GetAABB());
        uploadVertexBuffer = false;
    }

    updateAABB();
    model->draw(mTex);
	aabb->render(viewMat, proj, modelMat, aabbColor);
}

void Player::Update(float dt) 
{
 //   updateAABB();
    ComputeAudioWorldTransform();
    UpdateComponents(dt);
    SetAnimation(GetAnimNum(), 1.0f, 1.0f, false);
}


void Player::ComputeAudioWorldTransform()
{
    if (mRecomputeWorldTransform)
    {
        mRecomputeWorldTransform = false;
        glm::mat4 worldTransform = glm::mat4(1.0f);
        // Scale, then rotate, then translate
        audioWorldTransform = glm::translate(worldTransform, position);
        audioWorldTransform = glm::rotate(worldTransform, glm::radians(-yaw + 180.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        audioWorldTransform = glm::scale(worldTransform, scale);

        // Inform components world transform updated
        for (auto comp : mComponents)
        {
            comp->OnUpdateWorldTransform();
        }
    }
}

void Player::UpdatePlayerVectors()
{
    glm::vec3 front = glm::vec3(1.0f);
    front.x = cos(glm::radians(yaw - 90.0f));
    front.y = 0.0f;
    front.z = sin(glm::radians(yaw - 90.0f));
    PlayerFront = glm::normalize(front);
    PlayerRight = glm::normalize(glm::cross(PlayerFront, glm::vec3(0.0f, 1.0f, 0.0f)));  
    PlayerUp = glm::normalize(glm::cross(PlayerRight, PlayerFront));
}

void Player::UpdatePlayerAimVectors()
{
    glm::vec3 front;
    front.x = glm::cos(glm::radians(yaw - 90.0f)) * glm::cos(glm::radians(aimPitch));
    front.y = glm::sin(glm::radians(aimPitch));
    front.z = glm::sin(glm::radians(yaw-90.0f)) * glm::cos(glm::radians(aimPitch));

    PlayerAimFront = glm::normalize(front);
    PlayerAimRight = glm::normalize(glm::cross(PlayerAimFront, glm::vec3(0.0f, 1.0f, 0.0f)));
    PlayerAimUp = glm::normalize(glm::cross(PlayerAimRight, PlayerAimFront));
}

void Player::PlayerProcessKeyboard(CameraMovement direction, float deltaTime)
{
    mVelocity = MovementSpeed * deltaTime;

    if (direction == FORWARD)
    {
        position += PlayerFront * mVelocity;
		mRecomputeWorldTransform = true;
        ComputeAudioWorldTransform();
        UpdateComponents(deltaTime);
    }
    if (direction == BACKWARD)
    {
        position -= PlayerFront * mVelocity;
        mRecomputeWorldTransform = true;
        ComputeAudioWorldTransform();
        UpdateComponents(deltaTime);
    }
    if (direction == LEFT)
    {
        position -= PlayerRight * mVelocity;
        mRecomputeWorldTransform = true;
        ComputeAudioWorldTransform();
        UpdateComponents(deltaTime);
        SetAnimNum(13);
    }
    if (direction == RIGHT)
    {
        position += PlayerRight * mVelocity;
        mRecomputeWorldTransform = true;
        ComputeAudioWorldTransform();
        UpdateComponents(deltaTime);
        SetAnimNum(14);
    }
}

void Player::PlayerProcessMouseMovement(float xOffset)
{
//    xOffset *= SENSITIVITY;  

    yaw += xOffset;
    mRecomputeWorldTransform = true;
    ComputeAudioWorldTransform();
    if (GetPlayerState() == MOVING)
        UpdatePlayerVectors();
	else if (GetPlayerState() == AIMING)
		UpdatePlayerAimVectors();
}

void Player::SetAnimation(int animNum, float speedDivider, float blendFactor, bool playAnimBackwards)
{
    model->playAnimation(animNum, speedDivider, blendFactor, playAnimBackwards);
}

void Player::SetPlayerState(PlayerState newState)
{
    mPlayerState = newState;
    if (mPlayerState == MOVING)
        UpdatePlayerVectors();
    else if (mPlayerState == AIMING)
    {
        UpdatePlayerAimVectors();
        GameManager* gmeMgr = GetGameManager();
        if (gmeMgr->camSwitchedToAim == false)
        {
            gmeMgr->camSwitchedToAim = true;
        }
    }
}

void Player::Shoot()
{
	if (GetPlayerState() != SHOOTING)
		return;

    shootAC->PlayEvent("event:/PlayerShoot");
    SetAnimNum(12);
    UpdatePlayerVectors();
    UpdatePlayerAimVectors();

    glm::vec3 rayO = GetShootPos();
    glm::vec3 rayD = glm::normalize(PlayerAimFront);
    float dist = GetShootDistance();

    glm::vec3 hitPoint = glm::vec3(0.0f);

	GameManager* gmeMgr = GetGameManager();
    //gmeMgr->GetPhysicsWorld()->rayEnemyIntersect(rayO, rayD, hitPoint);

	bool hit = false;
    hit = gmeMgr->GetPhysicsWorld()->rayIntersect(rayO, rayD, hitPoint, aabb);

    if (hit) {
        std::cout << "\nRay hit at: " << hitPoint.x << ", " << hitPoint.y << ", " << hitPoint.z << std::endl;
		hitPoint = glm::vec3(0.0f);
    }
    else {
        std::cout << "\nNo hit detected." << std::endl;
    }
	rayD = glm::vec3(0.0f);
    UpdatePlayerAimVectors();
}

void Player::renderAABB(glm::mat4 proj, glm::mat4 viewMat, glm::mat4 model, Shader* aabbSdr)
{
    glm::vec3 min = aabb->transformedMin;
    glm::vec3 max = aabb->transformedMax;

    std::vector<glm::vec3> lineVertices = {
        {min.x, min.y, min.z}, {max.x, min.y, min.z},
        {max.x, min.y, min.z}, {max.x, max.y, min.z},
        {max.x, max.y, min.z}, {min.x, max.y, min.z},
        {min.x, max.y, min.z}, {min.x, min.y, min.z},

        {min.x, min.y, max.z}, {max.x, min.y, max.z},
        {max.x, min.y, max.z}, {max.x, max.y, max.z},
        {max.x, max.y, max.z}, {min.x, max.y, max.z},
        {min.x, max.y, max.z}, {min.x, min.y, max.z},

        {min.x, min.y, min.z}, {min.x, min.y, max.z},
        {max.x, min.y, min.z}, {max.x, min.y, max.z},
        {max.x, max.y, min.z}, {max.x, max.y, max.z},
        {min.x, max.y, min.z}, {min.x, max.y, max.z}
    };

    GLuint VAO, VBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, lineVertices.size() * sizeof(glm::vec3), lineVertices.data(), GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    aabbShader->use();

    aabbShader->setMat4("projection", proj);
    aabbShader->setMat4("view", viewMat);
    aabbShader->setMat4("model", model);
    aabbShader->setVec3("color", aabbColor);

    glBindVertexArray(VAO);
    glDrawArrays(GL_LINES, 0, lineVertices.size());
    glBindVertexArray(0);

    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
}

void Player::OnHit()
{
	std::cout << "Player hit!" << std::endl;
	setAABBColor(glm::vec3(1.0f, 0.0f, 1.0f));
    TakeDamage(50.0f);
}

void Player::OnDeath()
{
	std::cout << "Player died!" << std::endl;
//	deathAC->PlayEvent("event:/PlayerDeath");
}

