#pragma once

#include <vector>
#include "AABB.h"
#include <glm/glm.hpp>

class PhysicsWorld {
public:
	PhysicsWorld();

	void addCollider(AABB* collider);
	void addEnemyCollider(AABB* collider);
	void removeCollider(AABB* collider);
	void removeEnemyCollider(AABB* collider);

	bool rayIntersect(const glm::vec3& rayOrigin, const glm::vec3& rayDirection, glm::vec3& hitPoint, AABB* selfAABB);
	bool rayEnemyIntersect(const glm::vec3& rayOrigin, const glm::vec3& rayDirection, glm::vec3& hitPoint);
	bool rayEnemyCrosshairIntersect(const glm::vec3& rayOrigin, const glm::vec3& rayDirection, glm::vec3& hitPoint);
	bool rayPlayerIntersect(const glm::vec3& rayOrigin, const glm::vec3& rayDirection, glm::vec3& hitPoint, AABB* selfAABB);
	bool checkPlayerVisibility(const glm::vec3& rayOrigin, const glm::vec3& rayDirection, glm::vec3& hitPoint, AABB* selfAABB);

private:
	std::vector<AABB*> colliders;
	std::vector<AABB*> enemyColliders;

	bool rayAABBIntersect(const glm::vec3& rayOrigin, const glm::vec3& rayDirection, AABB* aabb, glm::vec3& hitPoint);
};

