#pragma once
#include "EnTT/entt.hpp"

#include "Scene/Components/Components.h"

inline void UpdateTransforms(entt::registry& r)
{
	auto view = r.view<TransformComponent>();
	
	for (auto& e : view)
	{
		auto& t = view.get<TransformComponent>(e);
		RecomputeTransform(t);
	}
}