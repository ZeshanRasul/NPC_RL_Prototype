#pragma once

struct GameTunables {
	//Camera
	float playerCamRearOffset = 27.0f;
	float playerCamHeightOffset = 12.0f;
	float playerPosOffset = 15.0f;
	float playerAimRightOffset = 5.0f;
	float enemyCamRearOffset = 17.0f;
	float enemyCamHeightOffset = 15.0f;
};

extern GameTunables g_tunables;