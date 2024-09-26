#pragma once
#include "BehaviourTree.h"
#include <functional>

class ConditionNode : public BTNode
{
public:
    using ConditionFunc = std::function<bool()>;

    ConditionNode(ConditionFunc func) : conditionFunc_(func) {}

    NodeStatus Tick() override
    {
        return conditionFunc_() ? NodeStatus::Success : NodeStatus::Failure;
    }

private:
    ConditionFunc conditionFunc_;
};
