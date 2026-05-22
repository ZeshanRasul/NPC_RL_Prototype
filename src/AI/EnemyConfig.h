#pragma once
#include <string>
#include <glm/glm.hpp>

enum EnemyType
{
	SCOUT,
	HEAVY_SCOUT,
	DRONE,
	MECH
};

struct EnemyConfig {
	EnemyType   type;
	std::string modelPath;
	bool        hasSkin      = true;
	bool        rotateOnDraw = false;

	float maxHealth   = 100.0f;
	float accuracy    = 60.0f;
	float moveSpeed   = 7.5f;

	int animIdle       = 1;
	int animWalk       = 5;
	int animShoot      = 2;
	int animTakeDamage = 3;
	int animDeath      = 0;

	glm::vec3 aabbScale = glm::vec3(3.8f, 3.3f, 3.5f);

	static EnemyConfig Scout()
	{
		EnemyConfig c;
		c.type      = EnemyType::SCOUT;
		c.modelPath = "src/Assets/Models/New_Enemies/Armour7/Scout.glb";
		return c;
	}

	static EnemyConfig HeavyScout()
	{
		EnemyConfig c;
		c.type      = EnemyType::HEAVY_SCOUT;
		c.modelPath = "src/Assets/Models/New_Enemies/Armour9/Heavy_Scout.glb";
		c.maxHealth = 150.0f;
		c.moveSpeed = 5.0f;
		return c;
	}

	static EnemyConfig Drone()
	{
		EnemyConfig c;
		c.type         = EnemyType::DRONE;
		c.modelPath    = "src/Assets/Models/New_Enemies/Drone/Drone.glb";
		c.hasSkin      = false;
		c.rotateOnDraw = true;
		c.accuracy     = 40.0f;
		c.aabbScale    = glm::vec3(2.5f, 2.5f, 2.5f);
		return c;
	}

	static EnemyConfig Mech()
	{
		EnemyConfig c;
		c.type      = EnemyType::MECH;
		c.modelPath = "src/Assets/Models/New_Enemies/MechStandard/Mecha-HM4_Rigged+Anim.glb";
		c.hasSkin   = false;
		c.maxHealth = 300.0f;
		c.moveSpeed = 4.0f;
		c.accuracy  = 80.0f;
		c.aabbScale = glm::vec3(5.0f, 5.0f, 5.0f);
		return c;
	}
};
