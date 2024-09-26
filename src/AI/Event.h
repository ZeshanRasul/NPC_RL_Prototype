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

    template<typename EventType>
    void Subscribe(EventCallback callback)
    {
        auto& subscribers = subscribers_[std::type_index(typeid(EventType))];
        subscribers.push_back(callback);
    }

    void Publish(const Event& event)
    {
        auto it = subscribers_.find(std::type_index(typeid(event)));
        if (it != subscribers_.end())
        {
            for (auto& callback : it->second)
            {
                callback(event);
            }
        }
    }

private:
    std::unordered_map<std::type_index, std::vector<EventCallback>> subscribers_;
};
