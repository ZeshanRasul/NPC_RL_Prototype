#pragma once
#include "BehaviourTree.h"
#include <functional>

class ConditionNode : public BTNode
{
public:
	using ConditionFunc = std::function<bool()>;

	ConditionNode(ConditionFunc func) : m_conditionFunc(func)
	{
	}

	NodeStatus Tick() override
	{
		return m_conditionFunc() ? NodeStatus::Success : NodeStatus::Failure;
	}

private:
	ConditionFunc m_conditionFunc;
};
