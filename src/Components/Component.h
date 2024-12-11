#pragma once

class Component
{
public:
	// (the lower the update order, the earlier the component updates)
	Component(class GameObject* owner, int updateOrder = 100);

	virtual ~Component();

	virtual void Update(float deltaTime);
	virtual void OnUpdateWorldTransform() { }

	int GetUpdateOrder() const { return mUpdateOrder; }
protected:
	class GameObject* mOwner;
	// Update order of component
	int mUpdateOrder;
};