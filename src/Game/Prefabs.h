#pragma once

#include "EnTT/entt.hpp"
#include "Engine/Asset/AssetManager.h"
#include "Engine/ECS/Components/StaticModelRendererComponent.h"
#include "Engine/ECS/Components/TransformComponent.h"

entt::entity CreateLevel(entt::registry& registry, AssetManager& assetManager,
	const std::string& pathToGltf);