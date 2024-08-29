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
    float tMin;
    float tMax;

    float t1 = (aabb.transformedMin.x - rayOrigin.x) / rayDirection.x;
    float t2 = (aabb.transformedMax.x - rayOrigin.x) / rayDirection.x;
    float t3 = (aabb.transformedMin.y - rayOrigin.y) / rayDirection.y;
    float t4 = (aabb.transformedMax.y - rayOrigin.y) / rayDirection.y;
    float t5 = (aabb.transformedMin.z - rayOrigin.z) / rayDirection.z;
    float t6 = (aabb.transformedMax.z - rayOrigin.z) / rayDirection.z;

    tMin = std::max({ std::min(t1, t2), std::min(t3, t4), std::min(t5, t6) });
    tMax = std::min({ std::max(t1, t2), std::max(t3, t4), std::max(t5, t6) });

    return tMax >= std::max(tMin, 0.0f);
}
