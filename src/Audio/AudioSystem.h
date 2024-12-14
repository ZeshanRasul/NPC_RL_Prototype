#pragma once
#include <unordered_map>
#include <string>
#include "glm/glm.hpp"

#include "SoundEvent.h"

// Forward declarations to avoid including FMOD header
namespace FMOD
{
	class System;

	namespace Studio
	{
		class Bank;
		class EventDescription;
		class EventInstance;
		class System;
		class Bus;
	};
};

class AudioSystem
{
public:
	AudioSystem(class GameManager* gameManager);
	~AudioSystem();

	bool Initialize();
	void Shutdown();

	void LoadBank(const std::string& name);
	void UnloadBank(const std::string& name);
	void UnloadAllBanks();

	SoundEvent PlayEvent(const std::string& name);

	void Update(float deltaTime);

	// For positional audio
	void SetListener(const glm::mat4& viewMatrix);
	// Control buses
	float GetBusVolume(const std::string& name) const;
	bool GetBusPaused(const std::string& name) const;
	void SetBusVolume(const std::string& name, float volume);
	void SetBusPaused(const std::string& name, bool pause);

protected:
	friend class SoundEvent;
	FMOD::Studio::EventInstance* GetEventInstance(unsigned int id);

private:
	static unsigned int s_nextID;

	class GameManager* m_gameManager;
	std::unordered_map<std::string, FMOD::Studio::Bank*> m_banks;
	std::unordered_map<std::string, FMOD::Studio::EventDescription*> m_events;
	std::unordered_map<unsigned int, FMOD::Studio::EventInstance*> m_eventInstances;
	std::unordered_map<std::string, FMOD::Studio::Bus*> m_buses;
	FMOD::Studio::System* m_system;
	FMOD::System* m_lowLevelSystem;
};
