#pragma once
#include "EnTT/entt.hpp"

#include "Engine/ECS/Components/TransformComponent.h"

inline void UpdateTransforms(entt::registry& r)
{
	auto view = r.view<TransformComponent>();
	
	for (auto& e : view)
	{
		auto& t = view.get<TransformComponent>(e);
		RecomputeTransform(t);
	}
}