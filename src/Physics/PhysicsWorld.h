#pragma once

#include <vector>
#include "AABB.h"
#include <glm/glm.hpp>

class PhysicsWorld {
public:
    PhysicsWorld();

    void addCollider(const AABB& collider);
    void addEnemyCollider(const AABB& collider);
    bool rayIntersect(const glm::vec3& rayOrigin, const glm::vec3& rayDirection, glm::vec3& hitPoint);
    bool rayEnemyIntersect(const glm::vec3& rayOrigin, const glm::vec3& rayDirection, glm::vec3& hitPoint);

private:
    std::vector<AABB> colliders;
    std::vector<AABB> enemyColliders;

    bool rayAABBIntersect(const glm::vec3& rayOrigin, const glm::vec3& rayDirection, const AABB& aabb, glm::vec3& hitPoint);
};

