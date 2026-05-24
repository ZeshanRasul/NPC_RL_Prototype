#pragma once

#include <vector>
#include "AABB.h"
#include <glm/glm.hpp>


struct PlaneCollider;

class PhysicsWorld
{
public:
	PhysicsWorld();

	void AddCollider(AABB* collider);
	void AddEnemyCollider(AABB* collider);
	void AddPlaneCollider(PlaneCollider* collider);
	void RemoveCollider(AABB* collider);
	void RemoveEnemyCollider(AABB* collider);

	bool RayIntersect(const glm::vec3& rayOrigin, const glm::vec3& rayDirection, glm::vec3& hitPoint, AABB* selfAABB);
	bool RayEnemyIntersect(const glm::vec3& rayOrigin, const glm::vec3& rayDirection, glm::vec3& hitPoint);
	bool RayEnemyCrosshairIntersect(const glm::vec3& rayOrigin, const glm::vec3& rayDirection, glm::vec3& hitPoint);
	bool RayPlayerIntersect(const glm::vec3& rayOrigin, const glm::vec3& rayDirection, glm::vec3& hitPoint,
	                        AABB* selfAABB);
	bool CheckPlayerVisibility(const glm::vec3& rayOrigin, const glm::vec3& rayDirection, glm::vec3& hitPoint,
	                           AABB* selfAABB);

	// Returns distance to the first static-geometry hit (ignores player/enemy AABBs), or FLT_MAX if no hit.
	float RaycastStaticOnly(const glm::vec3& origin, const glm::vec3& dir, glm::vec3& hitPoint);

	glm::vec3 RaycastPlane(const glm::vec3& ro, const glm::vec3& rd, float& tOut, glm::vec3& desiredDir);



private:
	std::vector<AABB*> m_colliders;
	std::vector<AABB*> m_enemyColliders;
	std::vector<PlaneCollider*> m_planeColliders;

	bool RayAABBIntersect(const glm::vec3& rayOrigin, const glm::vec3& rayDirection, AABB* aabb, glm::vec3& hitPoint);
};
