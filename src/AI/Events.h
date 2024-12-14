#pragma once
#include "Event.h"

class PlayerDetectedEvent : public Event
{
public:
	PlayerDetectedEvent(int id) : npcID(id)
	{
	}

	int npcID;
};

class NPCDamagedEvent : public Event
{
public:
	NPCDamagedEvent(int id) : npcID(id)
	{
	}

	int npcID;
};

class NPCTakingCoverEvent : public Event
{
public:
	NPCTakingCoverEvent(int id) : npcID(id)
	{
	}

	int npcID;
};

class NPCDiedEvent : public Event
{
public:
	NPCDiedEvent(int id) : npcID(id)
	{
	}

	int npcID;
};
