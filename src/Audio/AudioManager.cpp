#include "AudioManager.h"
#include "GameManager.h"

void AudioManager::SubmitAudioRequest(int enemyId, const std::string& eventName, float priority, float cooldown)
{
	bool isHighPriority = (priority >= priorityThreshold);

	if (isHighPriority || (enemyCooldowns[enemyId] <= 0.0f && globalCooldownTimer <= 0.0f))
	{
		AudioRequest request{ enemyId, eventName, priority, cooldown };
		audioQueue.push(request);
	}
}

void AudioManager::Update(float deltaTime)
{
	globalCooldownTimer -= deltaTime;

	for (auto& [enemyId, cooldown] : enemyCooldowns)
	{
		cooldown -= deltaTime;
	}

	if (!audioQueue.empty() && globalCooldownTimer <= 0.0f)
	{
		ProcessNextAudioRequest();
	}
}

void AudioManager::ClearQueue()
{
	while (!audioQueue.empty())
	{
		audioQueue.pop();
	}
}

void AudioManager::ProcessNextAudioRequest()
{
	AudioRequest request = audioQueue.top();
	audioQueue.pop();

	Enemy* enemy = gameManager->GetEnemyByID(request.enemyId);
	if (enemy)
	{
		AudioComponent* audioComp = enemy->GetAudioComponent();
		if (audioComp)
		{
			audioComp->PlayEvent(request.eventName);
		}
	}

	globalCooldownTimer = globalCooldown;
	enemyCooldowns[request.enemyId] = request.cooldown;

}
