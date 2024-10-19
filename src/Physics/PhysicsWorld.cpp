#include <limits>
#include <algorithm>
#include <glm/glm.hpp>

#include "PhysicsWorld.h"

PhysicsWorld::PhysicsWorld() {}

void PhysicsWorld::addCollider(AABB* collider) {
    colliders.push_back(collider);
}

void PhysicsWorld::addEnemyCollider(AABB* collider)
{
	enemyColliders.push_back(collider);
}

void PhysicsWorld::removeCollider(AABB* collider)
{
    auto it = std::remove(colliders.begin(), colliders.end(), collider);
    if (it != colliders.end()) {
        colliders.erase(it, colliders.end());
    }
}

void PhysicsWorld::removeEnemyCollider(AABB* collider)
{
    auto it = std::remove(enemyColliders.begin(), enemyColliders.end(), collider);
    if (it != enemyColliders.end()) {
        enemyColliders.erase(it, enemyColliders.end());
    }
}

bool PhysicsWorld::rayIntersect(const glm::vec3& rayOrigin, const glm::vec3& rayDirection, glm::vec3& hitPoint, AABB* selfAABB) {
    bool hit = false;
    float closestDistance = std::numeric_limits<float>::max();

    AABB* closestAABB = nullptr;
    std::vector<AABB*> missedAABBs = {};

    for (AABB* collider : colliders) {
        glm::vec3 tempHitPoint;
        if (rayAABBIntersect(rayOrigin, rayDirection, collider, tempHitPoint)) {
            float distance = glm::length(tempHitPoint - rayOrigin);
            if (distance < closestDistance && collider != selfAABB) {
                closestDistance = distance;
                hitPoint = tempHitPoint;
                hit = true;
                closestAABB = collider;
            }
		}
		else {
			missedAABBs.push_back(collider);
		}
    }

    if (hit && closestAABB && !closestAABB->owner->isDestroyed)
        closestAABB->owner->OnHit();
    else if (size(missedAABBs) > 0) 
    {
        for (AABB* missedAABB : missedAABBs)
        {
            if (missedAABB->owner != nullptr && !missedAABB->owner->isDestroyed)
                missedAABB->owner->OnMiss();

        }
    }

    return hit;
}

bool PhysicsWorld::rayEnemyIntersect(const glm::vec3& rayOrigin, const glm::vec3& rayDirection, glm::vec3& hitPoint)
{
    bool hit = false;
    float closestDistance = std::numeric_limits<float>::max();

	AABB* closestAABB = nullptr;
    AABB* missedAABB = nullptr;

    for (AABB* collider : enemyColliders) {
        glm::vec3 tempHitPoint;
        if (rayAABBIntersect(rayOrigin, rayDirection, collider, tempHitPoint) && !collider->isPlayer) {
            float distance = glm::length(tempHitPoint - rayOrigin);
            if (distance < closestDistance && !collider->isPlayer) {
                closestDistance = distance;
                hitPoint = tempHitPoint;
                hit = true;
                closestAABB = collider;
            } 
        }
        else {
            missedAABB = collider;
        }
    }

    if (hit && closestAABB && !closestAABB->isPlayer && !closestAABB->owner->isDestroyed)
        closestAABB->owner->OnHit();
    else if (missedAABB && !missedAABB->isPlayer && !missedAABB->owner->isDestroyed)
    {
        hit = false;
        missedAABB->owner->OnMiss();
    }

    return hit;
}

bool PhysicsWorld::rayPlayerIntersect(const glm::vec3& rayOrigin, const glm::vec3& rayDirection, glm::vec3& hitPoint, AABB* selfAABB)
{
    bool hit = false;
    float closestDistance = std::numeric_limits<float>::max();

    AABB* closestAABB = nullptr;
    AABB* missedAABB = nullptr;

    for (AABB* collider : colliders) {
        glm::vec3 tempHitPoint;
        if (rayAABBIntersect(rayOrigin, rayDirection, collider, tempHitPoint)) {
            float distance = glm::length(tempHitPoint - rayOrigin);
            if (distance < closestDistance && !collider->isEnemy && collider != selfAABB) {
                closestDistance = distance;
                hitPoint = tempHitPoint;
                hit = true;
                closestAABB = collider;
            }
        }
        else {
            missedAABB = collider;
        }
    }

    if (hit && closestAABB && !closestAABB->isEnemy && !closestAABB->owner->isDestroyed)
        closestAABB->owner->OnHit();
    else if (missedAABB && !missedAABB->isEnemy && !missedAABB->owner->isDestroyed)
    {
        hit = false;
        missedAABB->owner->OnMiss();
    }

    return hit;
}

bool PhysicsWorld::checkPlayerVisibility(const glm::vec3& rayOrigin, const glm::vec3& rayDirection, glm::vec3& hitPoint, AABB* selfAABB)
{
    bool hit = false;
    float closestDistance = std::numeric_limits<float>::max();

    AABB* closestAABB = nullptr;
    std::vector<AABB*> missedAABBs = {};

    for (AABB* collider : colliders) {
        glm::vec3 tempHitPoint;
        if (rayAABBIntersect(rayOrigin, rayDirection, collider, tempHitPoint)) {
            float distance = glm::length(tempHitPoint - rayOrigin);
            if (distance < closestDistance && collider != selfAABB) {
                closestDistance = distance;
                hitPoint = tempHitPoint;
                hit = true;
                closestAABB = collider;
            }
        }
        else {
            missedAABBs.push_back(collider);
        }
    }

    if (hit && closestAABB->isPlayer) {
        return true;
    }
    else if (hit && !closestAABB->isPlayer)
    {
        return false;
    }

    return false;
}

bool PhysicsWorld::rayAABBIntersect(const glm::vec3& rayOrigin, const glm::vec3& rayDirection, AABB* aabb, glm::vec3& hitPoint) {

    // Use the transformed AABB for the intersection test
    glm::vec3 min = aabb->transformedMin;
    glm::vec3 max = aabb->transformedMax;

    // Initialize tMin and tMax to infinite intervals
    float tMin = 0.0f;
    float tMax = std::numeric_limits<float>::max();

    // Iterate over each axis (x, y, z)
    for (int i = 0; i < 3; ++i) {
        if (std::abs(rayDirection[i]) < std::numeric_limits<float>::epsilon()) {
            // Ray is parallel to the slab (axis-aligned plane)
            if (rayOrigin[i] < min[i] || rayOrigin[i] > max[i]) {
                return false; // No intersection
            }
        }
        else {
            // Compute intersection t values for slabs
            float t1 = (min[i] - rayOrigin[i]) / (rayDirection[i] + std::numeric_limits<float>::epsilon());
            float t2 = (max[i] - rayOrigin[i]) / (rayDirection[i] + std::numeric_limits<float>::epsilon());

            if (t1 > t2) std::swap(t1, t2);

            tMin = std::max(tMin, t1);
            tMax = std::min(tMax, t2);

            if (tMin > tMax) {
                return false; // No intersection
            }
        }
    }  
    
    hitPoint = rayOrigin + rayDirection * tMin; 
    return true;
}
