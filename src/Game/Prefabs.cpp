#include "glm/glm.hpp"
#include "glm/gtc/quaternion.hpp"

#include "Game/Prefabs.h"

entt::entity CreateLevel(entt::registry& registry, AssetManager& assetManager, const std::string& pathToGltf)
{
	entt::entity groundEntity = registry.create();
	ModelHandle groundModel = assetManager.LoadStaticModel(pathToGltf);

	registry.emplace<StaticModelRendererComponent>(groundEntity, groundModel);
	registry.emplace<TransformComponent>(groundEntity, glm::vec3(0.0f), glm::quat(1.0f, 0.0f, 0.0f, 0.0f), glm::vec3(1.0f));

	return groundEntity;
}
