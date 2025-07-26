#pragma once
#include <unordered_map>
#include <queue>
#include <string>
#include <functional>
#include "Components/AudioComponent.h"

class GameManager;

struct AudioRequest
{
	int m_enemyId;
	std::string m_eventName;
	float m_priority;
	float m_cooldown;
};

struct ComparePriority
{
	bool operator()(const AudioRequest& a, const AudioRequest& b)
	{
		return a.m_priority < b.m_priority;
	}
};

class AudioManager
{
public:
	AudioManager(GameManager* gameMgr)
	{
		m_gameManager = gameMgr;
	};

	void SubmitAudioRequest(int enemyId, const std::string& eventName, float priority, float cooldown);
	void Update(float deltaTime);

	void ClearQueue();

private:
	void ProcessNextAudioRequest();

	std::unordered_map<int, float> m_enemyCooldowns;
	std::unordered_map<int, float> m_enemySpeakTime;
	std::priority_queue<AudioRequest, std::vector<AudioRequest>, ComparePriority> m_audioQueue;
	float m_globalCooldown = 1.8f;
	float m_globalCooldownTimer = 0.0f;
	float m_priorityThreshold = 2.5f;

	GameManager* m_gameManager;
};
