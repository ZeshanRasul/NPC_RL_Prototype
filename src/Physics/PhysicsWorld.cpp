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

bool PhysicsWorld::rayIntersect(const glm::vec3& rayOrigin, const glm::vec3& rayDirection, glm::vec3& hitPoint) {
    bool hit = false;
    float closestDistance = std::numeric_limits<float>::max();

    AABB* closestAABB = nullptr;
    AABB* missedAABB = nullptr;

    for (AABB* collider : colliders) {
        glm::vec3 tempHitPoint;
        if (rayAABBIntersect(rayOrigin, rayDirection, collider, tempHitPoint)) {
            float distance = glm::length(tempHitPoint - rayOrigin);
            if (distance < closestDistance) {
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

    if (hit && closestAABB && !closestAABB->isPlayer)
        closestAABB->owner->OnHit();
    else if (missedAABB && !missedAABB->isPlayer)
    {
        hit = false;
        missedAABB->owner->OnMiss();
    }

    return hit;
}

bool PhysicsWorld::rayEnemyIntersect(const glm::vec3& rayOrigin, const glm::vec3& rayDirection, glm::vec3& hitPoint)
{
    bool hit = false;
    float closestDistance = std::numeric_limits<float>::max();

	AABB* closestAABB = nullptr;

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
    }

	if (hit && closestAABB && !closestAABB->isPlayer)
	    closestAABB->owner->OnHit();
	else if (closestAABB && !closestAABB->isPlayer)
    {
		hit = false;
        closestAABB->owner->OnMiss();
    }

    return hit;
}

bool PhysicsWorld::rayAABBIntersect(const glm::vec3& rayOrigin, const glm::vec3& rayDirection, AABB* aabb, glm::vec3& hitPoint) {

	aabb->update(aabb->mModelMatrix);

    // Use the transformed AABB for the intersection test
    glm::vec3 min = aabb->transformedMin;
    glm::vec3 max = aabb->transformedMax;

	std::cout << "TransformedMin: " << min.x << ", " << min.y << ", " << min.z << "\n";
	std::cout << "TransformedMax: " << max.x << ", " << max.y << ", " << max.z << "\n";

	std::cout << "Position: " << aabb->owner->position.x << ", " << aabb->owner->position.y << ", " << aabb->owner->position.z << "\n";

	std::cout << "Ray Origin: " << rayOrigin.x << ", " << rayOrigin.y << ", " << rayOrigin.z << "\n";
    std::cout << "Ray Direction: " << rayDirection.x << ", " << rayDirection.y << ", " << rayDirection.z << "\n";
    
    // Check if the ray origin is inside the AABB
    if (rayOrigin.x >= min.x && rayOrigin.x <= max.x &&
        rayOrigin.y >= min.y && rayOrigin.y <= max.y &&
        rayOrigin.z >= min.z && rayOrigin.z <= max.z) {
        return false; // Ray origin is inside the AABB, so we skip this intersection
    }

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

    //if (tMin > 0.0f) {
    //    hitPoint = rayOrigin + rayDirection * tMin; // Ensure tMin > 0
    //    return true;
    //}    
    
    hitPoint = rayOrigin + rayDirection * tMin; // Ensure tMin > 0
    return true;

    //return false;
}
