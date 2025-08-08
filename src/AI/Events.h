#pragma once
#include "Event.h"

class PlayerDetectedEvent : public Event
{
public:
	PlayerDetectedEvent(int id) : m_npcId(id)
	{
	}

	int m_npcId;
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
