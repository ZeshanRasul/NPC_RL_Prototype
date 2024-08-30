#pragma once

#include <vector>
#include "AABB.h"
#include <glm/glm.hpp>

class PhysicsWorld {
public:
    PhysicsWorld();

    void addCollider(AABB* collider);
    void addEnemyCollider(AABB* collider);
    bool rayIntersect(const glm::vec3& rayOrigin, const glm::vec3& rayDirection, glm::vec3& hitPoint);
    bool rayEnemyIntersect(const glm::vec3& rayOrigin, const glm::vec3& rayDirection, glm::vec3& hitPoint);

private:
    std::vector<AABB*> colliders;
    std::vector<AABB*> enemyColliders;

    bool rayAABBIntersect(const glm::vec3& rayOrigin, const glm::vec3& rayDirection, AABB* aabb, glm::vec3& hitPoint);
};

