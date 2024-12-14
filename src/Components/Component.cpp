#include "Component.h"
#include "GameObjects/GameObject.h"

Component::Component(GameObject* owner, int updateOrder)
	: m_owner(owner),
	  m_updateOrder(updateOrder)
{
	m_owner->AddComponent(this);
}

Component::~Component()
{
	m_owner->RemoveComponent(this);
}

void Component::Update(float deltaTime)
{
}
