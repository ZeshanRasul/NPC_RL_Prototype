#include "Component.h"
#include "GameObjects/GameObject.h"

Component::Component(GameObject* owner, int updateOrder)
	:mOwner(owner),
	mUpdateOrder(updateOrder)
{
	// Add to actor's vector of components
	mOwner->AddComponent(this);
}

Component::~Component()
{
	mOwner->RemoveComponent(this);
}

void Component::Update(float deltaTime)
{
}
