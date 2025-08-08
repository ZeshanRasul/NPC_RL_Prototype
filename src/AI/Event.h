#pragma once

#include <functional>
#include <vector>
#include <unordered_map>
#include <typeindex>

class Event
{
public:
	virtual ~Event() = default;
};

class EventManager
{
public:
	using EventCallback = std::function<void(const Event&)>;

	template <typename EventType>
	void Subscribe(EventCallback callback)
	{
		auto& subscribers = m_subscribers[std::type_index(typeid(EventType))];
		subscribers.push_back(callback);
	}

	void Publish(const Event& event)
	{
		auto it = m_subscribers.find(std::type_index(typeid(event)));
		if (it != m_subscribers.end())
		{
			for (auto& callback : it->second)
			{
				callback(event);
			}
		}
	}

private:
	std::unordered_map<std::type_index, std::vector<EventCallback>> m_subscribers;
};
