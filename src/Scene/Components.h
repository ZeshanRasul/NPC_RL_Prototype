#pragma once
#include "glm/glm.hpp"

struct TransformComponent {
	glm::mat4 Transform{ 1.0f };

	TransformComponent() = default;
	TransformComponent(const TransformComponent&) = default;
	TransformComponent(glm::mat4 transform)
		: Transform(transform)
	{
	}

	operator glm::mat4()& { return Transform; }
	operator const glm::mat4& () const { return Transform; }
};