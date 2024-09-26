#pragma once
#include "BehaviourTree.h"
#include <functional>

class ActionNode : public BTNode
{
public:
    using ActionFunc = std::function<NodeStatus()>;

    ActionNode(ActionFunc func) : actionFunc_(func) {}

    NodeStatus Tick() override
    {
        return actionFunc_();
    }

private:
    ActionFunc actionFunc_;
};
