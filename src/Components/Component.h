#pragma once

class Component
{
public:
	Component(class GameObject* owner, int updateOrder = 100);

	virtual ~Component();

	virtual void Update(float deltaTime);

	virtual void OnUpdateWorldTransform()
	{
	}

	int GetUpdateOrder() const { return m_updateOrder; }

protected:
	class GameObject* m_owner;
	int m_updateOrder;
};
