#pragma once

#include <vector>
#include "AABB.h"
#include <glm/glm.hpp>

class PhysicsWorld
{
public:
	PhysicsWorld();

	void AddCollider(AABB* collider);
	void AddEnemyCollider(AABB* collider);
	void RemoveCollider(AABB* collider);
	void RemoveEnemyCollider(AABB* collider);

	bool RayIntersect(const glm::vec3& rayOrigin, const glm::vec3& rayDirection, glm::vec3& hitPoint, AABB* selfAABB);
	bool RayEnemyIntersect(const glm::vec3& rayOrigin, const glm::vec3& rayDirection, glm::vec3& hitPoint);
	bool RayEnemyCrosshairIntersect(const glm::vec3& rayOrigin, const glm::vec3& rayDirection, glm::vec3& hitPoint);
	bool RayPlayerIntersect(const glm::vec3& rayOrigin, const glm::vec3& rayDirection, glm::vec3& hitPoint,
	                        AABB* selfAABB);
	bool CheckPlayerVisibility(const glm::vec3& rayOrigin, const glm::vec3& rayDirection, glm::vec3& hitPoint,
	                           AABB* selfAABB);

private:
	std::vector<AABB*> m_colliders;
	std::vector<AABB*> m_enemyColliders;

	bool RayAABBIntersect(const glm::vec3& rayOrigin, const glm::vec3& rayDirection, AABB* aabb, glm::vec3& hitPoint);
};
