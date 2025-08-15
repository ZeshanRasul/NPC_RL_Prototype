#include "Scene.h"

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
}
