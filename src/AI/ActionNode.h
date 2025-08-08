#pragma once
#include "BehaviourTree.h"
#include <functional>

class ActionNode : public BTNode
{
public:
	using ActionFunc = std::function<NodeStatus()>;

	ActionNode(ActionFunc func) : m_actionFunc(func)
	{
	}

	NodeStatus Tick() override
	{
		return m_actionFunc();
	}

private:
	ActionFunc m_actionFunc;
};
