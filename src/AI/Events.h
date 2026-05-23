#pragma once
#include "Event.h"
#include <glm/glm.hpp>

class PlayerDetectedEvent : public Event
{
public:
	PlayerDetectedEvent(int id, glm::vec3 pos) : m_npcId(id), m_detectorPos(pos) {}

	int       m_npcId;
	glm::vec3 m_detectorPos;
};

class NPCDamagedEvent : public Event
{
public:
	NPCDamagedEvent(int id) : m_npcId(id)
	{
	}

	int m_npcId;
};

class NPCTakingCoverEvent : public Event
{
public:
	NPCTakingCoverEvent(int id) : m_npcId(id)
	{
	}

	int m_npcId;
};

class NPCDiedEvent : public Event
{
public:
	NPCDiedEvent(int id) : m_npcId(id)
	{
	}

	int m_npcId;
};
