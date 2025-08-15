#include "Scene.h"
#include "Systems/TransformSystem.h"

Scene::Scene()
{
}

Scene::~Scene()
{

}

entt::entity Scene::CreateEntity()
{
	return m_registry.create();
}

void Scene::OnUpdate(float deltaTime)
{
	UpdateTransforms(m_registry);
}
