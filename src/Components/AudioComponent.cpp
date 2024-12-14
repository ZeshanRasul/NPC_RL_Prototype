#include <glm/glm.hpp>

#include "AudioComponent.h"
#include "GameObjects/GameObject.h"
#include "GameManager.h"
#include "Audio/AudioSystem.h"

AudioComponent::AudioComponent(GameObject* owner, int updateOrder)
	: Component(owner, updateOrder)
{
}

AudioComponent::~AudioComponent()
{
	StopAllEvents();
}

void AudioComponent::Update(float deltaTime)
{
	Component::Update(deltaTime);

	// Remove invalid 2D events
	auto iter = m_events2D.begin();
	while (iter != m_events2D.end())
	{
		if (!iter->IsValid())
		{
			iter = m_events2D.erase(iter);
		}
		else
		{
			++iter;
		}
	}

	// Remove invalid 3D events
	iter = m_events3D.begin();
	while (iter != m_events3D.end())
	{
		if (!iter->IsValid())
		{
			iter = m_events3D.erase(iter);
		}
		else
		{
			++iter;
		}
	}
}

void AudioComponent::OnUpdateWorldTransform()
{
	// Update 3D events' world transforms
	glm::mat4 world = m_owner->GetAudioWorldTransform();
	for (auto& event : m_events3D)
	{
		if (event.IsValid())
		{
			event.Set3DAttributes(world);
		}
	}
}

SoundEvent AudioComponent::PlayEvent(const std::string& name)
{
	SoundEvent e = m_owner->GetGameManager()->GetAudioSystem()->PlayEvent(name);
	// Is this 2D or 3D?
	if (e.Is3D())
	{
		m_events3D.emplace_back(e);
		// Set initial 3D attributes
		e.Set3DAttributes(m_owner->GetAudioWorldTransform());
	}
	else
	{
		m_events2D.emplace_back(e);
	}
	return e;
}

void AudioComponent::StopAllEvents()
{
	// Stop all sounds
	for (auto& e : m_events2D)
	{
		e.Stop();
	}
	for (auto& e : m_events3D)
	{
		e.Stop();
	}
	// Clear events
	m_events2D.clear();
	m_events3D.clear();
}
