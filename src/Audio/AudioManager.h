#pragma once
#include <unordered_map>
#include <queue>
#include <string>
#include <functional>
#include "Components/AudioComponent.h"

class GameManager;

struct AudioRequest
{
	int enemyId;                 
	std::string eventName;       
	float priority;              
	float cooldown;              
};

struct ComparePriority
{
	bool operator()(const AudioRequest& a, const AudioRequest& b)
	{
		return a.priority < b.priority;
	}
};

class AudioManager
{
public:
	AudioManager(GameManager* gameMgr)
	{
		gameManager = gameMgr;
	};

	void SubmitAudioRequest(int enemyId, const std::string& eventName, float priority, float cooldown);
	void Update(float deltaTime);

private:
	std::unordered_map<int, float> enemyCooldowns;
	std::priority_queue<AudioRequest, std::vector<AudioRequest>, ComparePriority> audioQueue;
	float globalCooldown = 1.5f;
	float globalCooldownTimer = 0.0f;             
	float priorityThreshold = 2.5f;

	void ProcessNextAudioRequest();

	GameManager* gameManager;
};
