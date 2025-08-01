#include <vector>
#include <fmod_studio.hpp>
#include <fmod_errors.h>

#include "AudioSystem.h"
#include <Logger.h>

unsigned int AudioSystem::s_nextID = 0;

AudioSystem::AudioSystem(GameManager* game)
	: m_gameManager(game)
	  , m_system(nullptr)
	  , m_lowLevelSystem(nullptr)
{
}

AudioSystem::~AudioSystem()
{
}

bool AudioSystem::Initialize()
{
	// Initialize debug logging
	FMOD::Debug_Initialize(
		FMOD_DEBUG_LEVEL_ERROR, // Log only errors
		FMOD_DEBUG_MODE_TTY // Output to stdout
	);

	// Create FMOD studio system object
	FMOD_RESULT result;
	result = FMOD::Studio::System::create(&m_system);
	if (result != FMOD_OK)
	{
		Logger::Log(1, "%s error: Failed to create FMOD system - %s\n", __FUNCTION__, FMOD_ErrorString(result));
		return false;
	}

	// Initialize FMOD studio system
	result = m_system->initialize(
		512, // Max number of concurrent sounds
		FMOD_STUDIO_INIT_NORMAL, // Use default settings
		FMOD_INIT_NORMAL, // Use default settings
		nullptr // Usually null
	);
	if (result != FMOD_OK)
	{
		Logger::Log(1, "%s error: Failed to initialize FMOD system - %s\n", __FUNCTION__, FMOD_ErrorString(result));
		return false;
	}

	// Save the low-level system pointer
	m_system->getLowLevelSystem(&m_lowLevelSystem);

	// Load the master banks (strings first)
	LoadBank("src/Assets/Audio/Master Bank.strings.bank");
	LoadBank("src/Assets/Audio/Master Bank.bank");

	return true;
}

void AudioSystem::Shutdown()
{
	// Unload all banks
	UnloadAllBanks();
	// Shutdown FMOD system
	if (m_system)
	{
		m_system->release();
	}
}

void AudioSystem::LoadBank(const std::string& name)
{
	// Prevent double-loading
	if (m_banks.contains(name))
	{
		return;
	}

	// Try to load bank
	FMOD::Studio::Bank* bank = nullptr;
	FMOD_RESULT result = m_system->loadBankFile(
		name.c_str(), // File name of bank
		FMOD_STUDIO_LOAD_BANK_NORMAL, // Normal loading
		&bank // Save pointer to bank
	);

	constexpr int maxPathLength = 512;
	if (result == FMOD_OK)
	{
		// Add bank to map
		m_banks.emplace(name, bank);
		// Load all non-streaming sample data
		bank->loadSampleData();
		// Get the number of events in this bank
		int numEvents = 0;
		bank->getEventCount(&numEvents);
		if (numEvents > 0)
		{
			// Get list of event descriptions in this bank
			std::vector<FMOD::Studio::EventDescription*> events(numEvents);
			bank->getEventList(events.data(), numEvents, &numEvents);
			char eventName[maxPathLength];
			for (int i = 0; i < numEvents; i++)
			{
				FMOD::Studio::EventDescription* e = events[i];
				// Get the path of this event (like event:/Explosion2D)
				e->getPath(eventName, maxPathLength, nullptr);
				// Add to event map
				m_events.emplace(eventName, e);
			}
		}
		// Get the number of buses in this bank
		int numBuses = 0;
		bank->getBusCount(&numBuses);
		if (numBuses > 0)
		{
			// Get list of buses in this bank
			std::vector<FMOD::Studio::Bus*> buses(numBuses);
			bank->getBusList(buses.data(), numBuses, &numBuses);
			char busName[512];
			for (int i = 0; i < numBuses; i++)
			{
				FMOD::Studio::Bus* bus = buses[i];
				// Get the path of this bus (like bus:/SFX)
				bus->getPath(busName, 512, nullptr);
				// Add to buses map
				m_buses.emplace(busName, bus);
			}
		}
	}
}

void AudioSystem::UnloadBank(const std::string& name)
{
	// Ignore if not loaded
	auto iter = m_banks.find(name);
	if (iter == m_banks.end())
	{
		return;
	}

	// First we need to remove all events from this bank
	FMOD::Studio::Bank* bank = iter->second;
	int numEvents = 0;
	bank->getEventCount(&numEvents);
	if (numEvents > 0)
	{
		// Get event descriptions for this bank
		std::vector<FMOD::Studio::EventDescription*> events(numEvents);
		// Get list of events
		bank->getEventList(events.data(), numEvents, &numEvents);
		char eventName[512];
		for (int i = 0; i < numEvents; i++)
		{
			FMOD::Studio::EventDescription* e = events[i];
			// Get the path of this event
			e->getPath(eventName, 512, nullptr);
			// Remove this event
			auto eventi = m_events.find(eventName);
			if (eventi != m_events.end())
			{
				m_events.erase(eventi);
			}
		}
	}
	// Get the number of buses in this bank
	int numBuses = 0;
	bank->getBusCount(&numBuses);
	if (numBuses > 0)
	{
		// Get list of buses in this bank
		std::vector<FMOD::Studio::Bus*> buses(numBuses);
		bank->getBusList(buses.data(), numBuses, &numBuses);
		char busName[512];
		for (int i = 0; i < numBuses; i++)
		{
			FMOD::Studio::Bus* bus = buses[i];
			// Get the path of this bus (like bus:/SFX)
			bus->getPath(busName, 512, nullptr);
			// Remove this bus
			auto busi = m_buses.find(busName);
			if (busi != m_buses.end())
			{
				m_buses.erase(busi);
			}
		}
	}

	// Unload sample data and bank
	bank->unloadSampleData();
	bank->unload();
	// Remove from banks map
	m_banks.erase(iter);
}

void AudioSystem::UnloadAllBanks()
{
	for (auto& iter : m_banks)
	{
		// Unload the sample data, then the bank itself
		iter.second->unloadSampleData();
		iter.second->unload();
	}
	m_banks.clear();
	// No banks means no events
	m_events.clear();
}

SoundEvent AudioSystem::PlayEvent(const std::string& name)
{
	unsigned int retID = 0;
	auto iter = m_events.find(name);
	if (iter != m_events.end())
	{
		// Create instance of event
		FMOD::Studio::EventInstance* event = nullptr;
		iter->second->createInstance(&event);
		if (event)
		{
			// Start the event instance
			event->start();
			// Get the next id, and add to map
			s_nextID++;
			retID = s_nextID;
			m_eventInstances.emplace(retID, event);
		}
	}
	return SoundEvent(this, retID);
}

void AudioSystem::Update(float deltaTime)
{
	// Find any stopped event instances
	std::vector<unsigned int> done;
	for (auto& iter : m_eventInstances)
	{
		FMOD::Studio::EventInstance* e = iter.second;
		// Get the state of this event
		FMOD_STUDIO_PLAYBACK_STATE state;
		e->getPlaybackState(&state);
		if (state == FMOD_STUDIO_PLAYBACK_STOPPED)
		{
			// Release the event and add id to done
			e->release();
			done.emplace_back(iter.first);
		}
	}

	// Remove done event instances from map
	for (auto id : done)
	{
		m_eventInstances.erase(id);
	}

	// Update FMOD
	m_system->update();
}

namespace
{
	FMOD_VECTOR VecToFMOD(const glm::vec3& in)
	{
		// Convert from our coordinates (+x forward, +y right, +z up)
		// to FMOD (+z forward, +x right, +y up)
		FMOD_VECTOR v;
		v.x = in.x;
		v.y = in.y;
		v.z = in.z;
		return v;
	}
}

void AudioSystem::SetListener(const glm::mat4& viewMatrix)
{
	// Invert the view matrix to get the correct vectors
	glm::mat4 invView = inverse(viewMatrix);
	FMOD_3D_ATTRIBUTES listener;
	// Set position, forward, up
	listener.position = VecToFMOD(glm::vec3(viewMatrix[3]));
	// In the inverted view, third row is forward
	listener.forward = VecToFMOD(glm::vec3(viewMatrix[0][2], viewMatrix[1][2], viewMatrix[2][2]));
	// In the inverted view, second row is up
	listener.up = VecToFMOD(glm::vec3(viewMatrix[0][1], viewMatrix[1][1], viewMatrix[2][1]));
	// Set velocity to zero (fix if using Doppler effect)
	listener.velocity = {0.0f, 0.0f, 0.0f};
	// Send to FMOD
	m_system->setListenerAttributes(0, &listener);
}

float AudioSystem::GetBusVolume(const std::string& name) const
{
	float retVal = 0.0f;
	const auto iter = m_buses.find(name);
	if (iter != m_buses.end())
	{
		iter->second->getVolume(&retVal);
	}
	return retVal;
}

bool AudioSystem::GetBusPaused(const std::string& name) const
{
	bool retVal = false;
	const auto iter = m_buses.find(name);
	if (iter != m_buses.end())
	{
		iter->second->getPaused(&retVal);
	}
	return retVal;
}

void AudioSystem::SetBusVolume(const std::string& name, float volume)
{
	auto iter = m_buses.find(name);
	if (iter != m_buses.end())
	{
		iter->second->setVolume(volume);
	}
}

void AudioSystem::SetBusPaused(const std::string& name, bool pause)
{
	auto iter = m_buses.find(name);
	if (iter != m_buses.end())
	{
		iter->second->setPaused(pause);
	}
}

FMOD::Studio::EventInstance* AudioSystem::GetEventInstance(unsigned int id)
{
	FMOD::Studio::EventInstance* event = nullptr;
	auto iter = m_eventInstances.find(id);
	if (iter != m_eventInstances.end())
	{
		event = iter->second;
	}
	return event;
}
