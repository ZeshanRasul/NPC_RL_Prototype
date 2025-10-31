#include <limits>
#include <algorithm>
#include <glm/glm.hpp>

#include "PhysicsWorld.h"
#include "GameObjects/Ground.h"

PhysicsWorld::PhysicsWorld()
{
}

void PhysicsWorld::AddCollider(AABB* collider)
{
	m_colliders.push_back(collider);
}

void PhysicsWorld::AddEnemyCollider(AABB* collider)
{
	m_enemyColliders.push_back(collider);
}

void PhysicsWorld::AddPlaneCollider(PlaneCollider* collider)
{
	m_planeColliders.push_back(collider);
}

void PhysicsWorld::RemoveCollider(AABB* collider)
{
	auto it = std::remove(m_colliders.begin(), m_colliders.end(), collider);
	if (it != m_colliders.end())
	{
		m_colliders.erase(it, m_colliders.end());
	}
}

void PhysicsWorld::RemoveEnemyCollider(AABB* collider)
{
	auto it = std::remove(m_enemyColliders.begin(), m_enemyColliders.end(), collider);
	if (it != m_enemyColliders.end())
	{
		m_enemyColliders.erase(it, m_enemyColliders.end());
	}
}

bool PhysicsWorld::RayIntersect(const glm::vec3& rayOrigin, const glm::vec3& rayDirection, glm::vec3& hitPoint,
	AABB* selfAABB)
{
	bool hit = false;
	float closestDistance = std::numeric_limits<float>::max();

	AABB* closestAABB = nullptr;
	std::vector<AABB*> missedAABBs = {};

	for (AABB* collider : m_colliders)
	{
		glm::vec3 tempHitPoint;
		if (RayAABBIntersect(rayOrigin, rayDirection, collider, tempHitPoint))
		{
			float distance = length(tempHitPoint - rayOrigin);
			if (distance < closestDistance && collider != selfAABB)
			{
				closestDistance = distance;
				hitPoint = tempHitPoint;
				hit = true;
				closestAABB = collider;
			}
		}
		else
		{
			missedAABBs.push_back(collider);
		}
	}

	if (hit && closestAABB && !closestAABB->GetOwner()->IsDestroyed())
		closestAABB->GetOwner()->OnHit();
	else if (size(missedAABBs) > 0)
	{
		for (AABB* missedAABB : missedAABBs)
		{
			if (missedAABB->GetOwner() != nullptr && !missedAABB->GetOwner()->IsDestroyed())
				missedAABB->GetOwner()->OnMiss();
		}
	}

	return hit;
}

bool PhysicsWorld::RayEnemyIntersect(const glm::vec3& rayOrigin, const glm::vec3& rayDirection, glm::vec3& hitPoint)
{
	bool hit = false;
	float closestDistance = std::numeric_limits<float>::max();

	AABB* closestAABB = nullptr;
	AABB* missedAABB = nullptr;

	for (AABB* collider : m_colliders)
	{
		glm::vec3 tempHitPoint;
		if (RayAABBIntersect(rayOrigin, rayDirection, collider, tempHitPoint) && !collider->GetIsPlayer())
		{
			float distance = length(tempHitPoint - rayOrigin);
			if (distance < closestDistance && collider->GetIsEnemy() && !collider->GetIsPlayer())
			{
				closestDistance = distance;
				hitPoint = tempHitPoint;
				hit = true;
				closestAABB = collider;
			}
		}
		else
		{
			missedAABB = collider;
		}
	}

	if (hit && closestAABB && !closestAABB->GetIsPlayer() && !closestAABB->GetOwner()->IsDestroyed())
		closestAABB->GetOwner()->OnHit();
	else if (missedAABB && !missedAABB->GetIsPlayer() && !missedAABB->GetOwner()->IsDestroyed())
	{
		hit = false;
		missedAABB->GetOwner()->OnMiss();
	}

	return hit;
}

bool PhysicsWorld::RayEnemyCrosshairIntersect(const glm::vec3& rayOrigin, const glm::vec3& rayDirection,
	glm::vec3& hitPoint)
{
	bool hit = false;
	float closestDistance = std::numeric_limits<float>::max();


	for (AABB* collider : m_colliders)
	{
		glm::vec3 tempHitPoint;
		if (RayAABBIntersect(rayOrigin, rayDirection, collider, tempHitPoint) && collider->GetIsEnemy() && !collider->GetIsPlayer())
		{
			float distance = length(tempHitPoint - rayOrigin);
			if (distance < closestDistance)
			{
				closestDistance = distance;
				hitPoint = tempHitPoint;
				if (collider->GetIsEnemy() && !collider->GetOwner()->IsDestroyed())
				{
					hit = true;
				}
				else
				{
					hit = false;
				}
			}
		}
	}

	return hit;
}


bool PhysicsWorld::RayPlayerIntersect(const glm::vec3& rayOrigin, const glm::vec3& rayDirection, glm::vec3& hitPoint,
	AABB* selfAABB)
{
	bool hit = false;
	float closestDistance = std::numeric_limits<float>::max();

	AABB* closestAABB = nullptr;
	AABB* missedAABB = nullptr;

	for (AABB* collider : m_colliders)
	{
		glm::vec3 tempHitPoint;
		if (RayAABBIntersect(rayOrigin, rayDirection, collider, tempHitPoint))
		{
			float distance = length(tempHitPoint - rayOrigin);
			if (distance < closestDistance && !collider->GetIsEnemy() && collider != selfAABB)
			{
				closestDistance = distance;
				hitPoint = tempHitPoint;
				hit = true;
				closestAABB = collider;
			}
		}
		else
		{
			missedAABB = collider;
		}
	}

	if (hit && closestAABB && !closestAABB->GetIsEnemy() && !closestAABB->GetOwner()->IsDestroyed())
	{
		closestAABB->GetOwner()->OnHit();
		if (closestAABB->GetIsPlayer())
		{
			selfAABB->GetOwner()->HasDealtDamage();
			if (closestAABB->GetOwner()->IsDestroyed() && closestAABB->GetIsPlayer())
			{
				selfAABB->GetOwner()->HasKilledPlayer();
			}
		}
	}
	else if (!hit && missedAABB && !missedAABB->GetIsEnemy() && !missedAABB->GetOwner()->IsDestroyed())
	{
		hit = false;
		missedAABB->GetOwner()->OnMiss();
	}


	return hit;
}

bool PhysicsWorld::CheckPlayerVisibility(const glm::vec3& rayOrigin, const glm::vec3& rayDirection, glm::vec3& hitPoint,
	AABB* selfAABB)
{
	bool hit = false;
	float closestDistance = std::numeric_limits<float>::max();

	AABB* closestAABB = nullptr;
	std::vector<AABB*> missedAABBs = {};

	for (AABB* collider : m_colliders)
	{
		glm::vec3 tempHitPoint;
		if (RayAABBIntersect(rayOrigin, rayDirection, collider, tempHitPoint))
		{
			float distance = length(tempHitPoint - rayOrigin);
			if (distance < closestDistance && collider != selfAABB)
			{
				closestDistance = distance;
				hitPoint = tempHitPoint;
				hit = true;
				closestAABB = collider;
			}
		}
		else
		{
			missedAABBs.push_back(collider);
		}
	}

	if (hit && closestAABB->GetIsPlayer())
	{
		return true;
	}
	if (hit && !closestAABB->GetIsPlayer())
	{
		return false;
	}

	return false;
}

glm::vec3 PhysicsWorld::RaycastPlane(const glm::vec3& ro, const glm::vec3& rd, float& tOut, glm::vec3& desiredDir)
{
	desiredDir = glm::normalize(desiredDir);

	for (PlaneCollider* pl : m_planeColliders)
	{
		if (glm::dot(pl->normal, glm::vec3(0.0f, 1.0f, 0.0f)) < 0.0f)
		{
			pl->normal = -pl->normal;
			pl->d = -pl->d;
			pl->tangent = -pl->tangent;
			pl->bitangent = -pl->bitangent;
		}

		float denom = glm::dot(pl->normal, rd);
		if (std::abs(denom) < 1e-8f)
			continue; // parallel

		Logger::Log(1, "Plane Normal: %f, %f, %f\n", pl->normal.x, pl->normal.y, pl->normal.z);

		tOut = -(glm::dot(pl->normal, ro) + pl->d) / denom;
		Logger::Log(1, "t = %f\n", tOut);

		if (tOut >= 0.0f && tOut <= 50.0f)
		{

			Logger::Log(1, "t greater than 0 = %f\n", tOut);

			glm::vec3 hit = ro + tOut * rd;

			glm::vec3 rel = hit - pl->center;
			float u = glm::dot(rel, pl->tangent);
			float v = glm::dot(rel, pl->bitangent);

			float tol = 1e-4f;
			//float tol = 1.5f;
			if (std::abs(u) > pl->halfSize.x + tol)
			{
				Logger::Log(1, "Outside of plane bounds\n");
				continue;
			}
			if (std::abs(v) > pl->halfSize.y + tol)
			{
				Logger::Log(1, "Outside of plane bounds\n");
				continue;
			}

			glm::vec3 slopeDir = desiredDir - glm::dot(desiredDir, pl->normal) * pl->normal;
			slopeDir = glm::normalize(slopeDir);
			Logger::Log(1, "Slope direction: %f, %f, %f\n", slopeDir.x, slopeDir.y, slopeDir.z);
			return slopeDir;
		};
	}
//	Logger::Log(1, "Desired direction: %f, %f, %f\n", desiredDir.x, desiredDir.y, desiredDir.z);

	return desiredDir;
}

bool PhysicsWorld::RayAABBIntersect(const glm::vec3& rayOrigin, const glm::vec3& rayDirection, AABB* aabb,
	glm::vec3& hitPoint)
{
	// Use the transformed AABB for the intersection test
	glm::vec3 min = aabb->GetTransformedMin();
	glm::vec3 max = aabb->GetTransformedMax();

	// Initialize tMin and tMax to infinite intervals
	float tMin = 0.0f;
	float tMax = std::numeric_limits<float>::max();

	// Iterate over each axis (x, y, z)
	for (int i = 0; i < 3; ++i)
	{
		if (std::abs(rayDirection[i]) < std::numeric_limits<float>::epsilon())
		{
			// Ray is parallel to the slab (axis-aligned plane)
			if (rayOrigin[i] < min[i] || rayOrigin[i] > max[i])
			{
				return false; // No intersection
			}
		}
		else
		{
			// Compute intersection t values for slabs
			float t1 = (min[i] - rayOrigin[i]) / (rayDirection[i] + std::numeric_limits<float>::epsilon());
			float t2 = (max[i] - rayOrigin[i]) / (rayDirection[i] + std::numeric_limits<float>::epsilon());

			if (t1 > t2) std::swap(t1, t2);

			tMin = std::max(tMin, t1);
			tMax = std::min(tMax, t2);

			if (tMin > tMax)
			{
				return false; // No intersection
			}
		}
	}

	hitPoint = rayOrigin + rayDirection * tMin;
	return true;
}
