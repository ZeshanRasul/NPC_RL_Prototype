#include "AudioManager.h"
#include "GameManager.h"

void AudioManager::SubmitAudioRequest(int enemyId, const std::string& eventName, float priority, float cooldown)
{
	bool isHighPriority = (priority >= m_priorityThreshold);

	if (isHighPriority || (m_enemyCooldowns[enemyId] <= 0.0f && m_globalCooldownTimer <= 0.0f))
	{
		AudioRequest request{enemyId, eventName, priority, cooldown};
		m_audioQueue.push(request);
	}
}

void AudioManager::Update(float deltaTime)
{
	m_globalCooldownTimer -= deltaTime;

	for (auto& [enemyId, cooldown] : m_enemyCooldowns)
	{
		cooldown -= deltaTime;
	}

	if (!m_audioQueue.empty() && m_globalCooldownTimer <= 0.0f)
	{
		ProcessNextAudioRequest();
	}
}

void AudioManager::ClearQueue()
{
	while (!m_audioQueue.empty())
	{
		m_audioQueue.pop();
	}
}

void AudioManager::ProcessNextAudioRequest()
{
	AudioRequest request = m_audioQueue.top();
	m_audioQueue.pop();

	Enemy* enemy = m_gameManager->GetEnemyByID(request.m_enemyId);
	if (enemy && !enemy->IsDestroyed())
	{
		AudioComponent* audioComp = enemy->GetAudioComponent();
		if (audioComp)
		{
			audioComp->PlayEvent(request.m_eventName);
		}
	}

	m_globalCooldownTimer = m_globalCooldown;
	m_enemyCooldowns[request.m_enemyId] = request.m_cooldown;
}
