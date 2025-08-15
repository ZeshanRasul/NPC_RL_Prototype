#pragma once
#include "glm/glm.hpp"
#include "glm/gtc/quaternion.hpp"

struct TransformComponent 
{

	TransformComponent() = default;
	TransformComponent(const TransformComponent&) = default;
	TransformComponent(glm::vec3 pos, glm::quat rot, glm::vec3 scale)
		: Position(pos), Rotation(rot), Scale(scale)
	{
	}

	glm::vec3 Position{ 0.0f };
	glm::vec3 Scale{ 1.0f };
	glm::quat Rotation{ 1.0f, 0.0f, 0.0f, 0.0f };

	glm::mat4 Local{ 1.0f };
	glm::mat4 World{ 1.0f };
	bool dirty{ true };
};


inline void RecomputeTransform(TransformComponent& t)
{
	if (!t.dirty) return;

	const glm::mat4 T = glm::translate(glm::mat4(1), t.Position);
	const glm::mat4 R = glm::mat4_cast(t.Rotation);
	const glm::mat4 S = glm::scale(glm::mat4(1), t.Scale);

	t.Local = T * R * S;
	t.World = t.Local;

	t.dirty = false;
};