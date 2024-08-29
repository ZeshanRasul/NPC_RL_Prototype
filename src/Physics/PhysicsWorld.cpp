#include "PhysicsWorld.h"
#include <limits>
#include <glm/glm.hpp>

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
    glm::vec3 invDir = 1.0f / rayDirection + glm::vec3(1e-8);
    glm::vec3 t0 = (rayOrigin - aabb.transformedMin) * rayDirection;
    glm::vec3 t1 = (rayOrigin - aabb.transformedMax) * rayDirection;

    glm::vec3 tmin = glm::min(t0, t1);
    glm::vec3 tmax = glm::max(t0, t1);

    float tNear = glm::max(glm::max(tmin.x, tmin.y), tmin.z);
    float tFar = glm::min(glm::min(tmax.x, tmax.y), tmax.z);

    if (tNear > tFar || tFar < 0.0f) {
        return false;
    }

    hitPoint = rayOrigin + tNear * rayDirection;
    return true;
}
