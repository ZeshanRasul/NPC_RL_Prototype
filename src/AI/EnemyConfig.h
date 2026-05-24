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
	std::string texturePath  = "src\\Assets\\Models\\New_Enemies\\Armour7\\armor7_painter_armor7_mat_BaseColor.png";
	// Directory prefix used to derive all PBR map filenames for this enemy type.
	// Maps: _BaseColor[2-4].png (random), _Metallic.png, _Roughness.png, _Normal.png, _Emissive.png
	std::string textureBasePath = "src/Assets/Models/New_Enemies/Armour7/armor7_painter_armor7_mat_";
	bool loadPBRTextures = true; // false → use embedded GLB textures (Drone/Mech)
	bool        hasSkin      = true;
	bool        rotateOnDraw = false;
	bool        useAltShader = false; // true → enemyShader2 (vertex.glsl/fragment.glsl)
	float       metallicScale = 1.0f; // multiplied into sampled metallic; lower to recover diffuse without IBL
	glm::vec3 rotationAxis = glm::vec3(0.0f, 1.0f, 0.0f);
	float rotationAngle = 0.0f;
	float maxHealth   = 100.0f;
	float accuracy    = 60.0f;
	float moveSpeed   = 7.5f;

	// Perception
	float sightRange         = 260.0f;  // max detection distance
	float sightFovDeg        = 65.0f;  // half-angle of forward cone; >= 180 = all-around
	float alertRadius        = 80.0f;  // PlayerDetectedEvent only reaches allies within this distance
	float alertTimeout       = 10.0f;  // seconds before resetting to patrol once LOS is lost
	float patrolWanderRadius = 380.0f;  // max wander radius from spawn position

	int animIdle       = 1;
	int animWalk       = 5;
	int animShoot      = 2;
	int animTakeDamage = 3;
	int animDeath      = 0;

	glm::vec3 aabbScale  = glm::vec3(3.8f, 3.3f, 3.5f);
	glm::vec3 modelScale = glm::vec3(5.0f);

	static EnemyConfig FromType(EnemyType t)
	{
		switch (t) {
		case SCOUT:       return Scout();
		case HEAVY_SCOUT: return HeavyScout();
		case DRONE:       return Drone();
		case MECH:        return Mech();
		default:          return Scout();
		}
	}

	static EnemyConfig Scout()
	{
		EnemyConfig c;
		c.type         = EnemyType::SCOUT;
		c.modelPath    = "src/Assets/Models/New_Enemies/Armour7/Scout.glb";
		c.rotateOnDraw = true;
		c.rotationAngle = -90.0f;
		c.metallicScale = 0.2f; // Substance Painter metallic maps are ~1.0; scale down so diffuse is visible without IBL
		return c;
	}

	static EnemyConfig HeavyScout()
	{
		EnemyConfig c;
		c.type             = EnemyType::HEAVY_SCOUT;
		c.modelPath        = "src/Assets/Models/New_Enemies/Armour9/Heavy_Scout.glb";
		c.textureBasePath  = "src/Assets/Models/New_Enemies/Armour9/BlueMetallic/PBR_MetalRough/armor9_painter_armor9_mat_";
		c.maxHealth        = 150.0f;
		c.moveSpeed        = 5.0f;
		c.sightRange       = 250.0f;
		c.sightFovDeg      = 60.0f;
		c.alertRadius      = 70.0f;
		c.patrolWanderRadius = 325.0f;
		c.useAltShader	   = false;
		c.rotateOnDraw	   = true;
		c.rotationAngle    = -90.0f;
		c.metallicScale = 0.2f; // Substance Painter metallic maps are ~1.0; scale down so diffuse is visible without IBL
		return c;
	}

	static EnemyConfig Drone()
	{
		EnemyConfig c;
		c.type               = EnemyType::DRONE;
		c.modelPath          = "src/Assets/Models/New_Enemies/Drone/Drone.glb";
		c.textureBasePath    = "src/Assets/Models/New_Enemies/Drone/War_Drone_Main_Body1_"; // single texture for entire model (no PBR)
		c.hasSkin            = false;
		c.rotateOnDraw       = true;
		c.rotationAxis       = glm::vec3(1.0f, 0.0f, 0.0f);
		c.rotationAngle		 = 90.0f;
		c.useAltShader       = false;
		c.accuracy           = 40.0f;
		c.aabbScale          = glm::vec3(2.5f, 2.5f, 2.5f);
		c.modelScale         = glm::vec3(0.01f);
		c.sightRange         = 280.0f;
		c.sightFovDeg        = 180.0f; // all-around (no cone check)
		c.alertRadius        = 100.0f;
		c.patrolWanderRadius = 350.0f;
		return c;
	}

	static EnemyConfig Mech()
	{
		EnemyConfig c;
		c.type               = EnemyType::MECH;
		c.modelPath          = "src/Assets/Models/New_Enemies/MechStandard/Mecha-HM4_Rigged+Anim.glb";
		c.hasSkin            = false;
		c.useAltShader       = true;
		c.maxHealth          = 300.0f;
		c.moveSpeed          = 4.0f;
		c.accuracy           = 80.0f;
		c.aabbScale          = glm::vec3(5.0f, 5.0f, 5.0f);
		c.modelScale         = glm::vec3(0.1f);
		c.loadPBRTextures = false; // use embedded GLB textures
		c.sightRange         = 200.0f;
		c.sightFovDeg        = 45.0f;
		c.alertRadius        = 120.0f;
		c.patrolWanderRadius = 100.0f;
		return c;
	}
};
