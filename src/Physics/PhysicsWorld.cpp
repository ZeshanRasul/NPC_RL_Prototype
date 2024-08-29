#include <limits>
#include <algorithm>
#include <glm/glm.hpp>

#include "PhysicsWorld.h"

PhysicsWorld::PhysicsWorld() {}

void PhysicsWorld::addCollider(const AABB& collider) {
    colliders.push_back(collider);
}

void PhysicsWorld::addEnemyCollider(const AABB& collider)
{
	enemyColliders.push_back(collider);
}

bool PhysicsWorld::rayIntersect(const glm::vec3& rayOrigin, const glm::vec3& rayDirection, glm::vec3& hitPoint) {
    bool hit = false;
    float closestDistance = std::numeric_limits<float>::max();

    for (const auto& collider : colliders) {
        glm::vec3 tempHitPoint;
        if (rayAABBIntersect(rayOrigin, rayDirection, collider, tempHitPoint)) {
            float distance = glm::length(tempHitPoint - rayOrigin);
            if (distance < closestDistance) {
                closestDistance = distance;
                hitPoint = tempHitPoint;
                hit = true;
            }
        }
    }

    return hit;
}

bool PhysicsWorld::rayEnemyIntersect(const glm::vec3& rayOrigin, const glm::vec3& rayDirection, glm::vec3& hitPoint)
{
    bool hit = false;
    float closestDistance = std::numeric_limits<float>::max();

	AABB closestAABB;

    for (const auto& collider : enemyColliders) {
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
    }

	if (hit)
	    closestAABB.owner->OnHit();

    return hit;
}

bool PhysicsWorld::rayAABBIntersect(const glm::vec3& rayOrigin, const glm::vec3& rayDirection, const AABB& aabb, glm::vec3& hitPoint) {

    // Use the transformed AABB for the intersection test
    glm::vec3 min = aabb.transformedMin;
    glm::vec3 max = aabb.transformedMax;

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
            float t1 = (min[i] - rayOrigin[i]) / rayDirection[i];
            float t2 = (max[i] - rayOrigin[i]) / rayDirection[i];

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
