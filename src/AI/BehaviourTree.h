#pragma once

#include <vector>
#include <memory>

enum class NodeStatus
{
	Success,
	Failure,
	Running
};

class BTNode
{
public:
	virtual ~BTNode() = default;
	virtual NodeStatus Tick() = 0;
};

using BTNodePtr = std::shared_ptr<BTNode>;

class CompositeNode : public BTNode
{
public:
	void AddChild(BTNodePtr child)
	{
		children_.push_back(child);
	}

protected:
	std::vector<BTNodePtr> children_;
};

class SequenceNode : public CompositeNode
{
public:
	NodeStatus Tick() override
	{
		for (auto& child : children_)
		{
			NodeStatus status = child->Tick();
			if (status != NodeStatus::Success)
			{
				return status;
			}
		}
		return NodeStatus::Success;
	}
};

class SelectorNode : public CompositeNode
{
public:
	NodeStatus Tick() override
	{
		for (auto& child : children_)
		{
			NodeStatus status = child->Tick();
			if (status != NodeStatus::Failure)
			{
				return status;
			}
		}
		return NodeStatus::Failure;
	}
};
