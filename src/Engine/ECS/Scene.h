#pragma once

#include <EnTT/entt.hpp>

#include "Engine/ECS/Components/TransformComponent.h"

class Scene {
public:
	Scene();
	~Scene();

	entt::entity CreateEntity();

	entt::registry& GetRegistry() { return m_registry; }

	void OnUpdate(float deltaTime);

private:
	entt::registry m_registry;
};


