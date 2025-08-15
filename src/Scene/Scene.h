#pragma once

#include <EnTT/entt.hpp>

#include "Components.h"

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