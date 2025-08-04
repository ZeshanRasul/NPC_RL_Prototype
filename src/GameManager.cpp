#include "GameManager.h"
#include "Components/AudioComponent.h"

#include "imgui/imgui.h"
#include "imgui/backend/imgui_impl_glfw.h"
#include "imgui/backend/imgui_impl_opengl3.h"

DirLight dirLight = {
		glm::vec3(-3.0f, -2.0f, -3.0f),

		glm::vec3(0.15f, 0.2f, 0.25f),
		glm::vec3(0.8f),
		glm::vec3(0.8f, 0.9f, 1.0f)
};

glm::vec3 dirLightPBRColour = glm::vec3(10.f, 10.0f, 10.0f);

void generateDebugSpanMeshFromHeightfield(
	const rcCompactHeightfield* chf,
	std::vector<DebugVertex>& outVertices,
	std::vector<GLuint>& outIndices)
{
	outVertices.clear();
	outIndices.clear();
	GLuint indexOffset = 0;

	for (int z = 0; z < chf->height; ++z) {
		for (int x = 0; x < chf->width; ++x) {
			const rcCompactCell& cell = chf->cells[x + z * chf->width];

			for (unsigned i = cell.index, ni = cell.index + cell.count; i < ni; ++i) {
				const rcCompactSpan& span = chf->spans[i];
				const unsigned char area = chf->areas[i];

				float fx = x * chf->cs;
				float fz = z * chf->cs;
				float fy = span.y * chf->ch;

				glm::vec3 color = (area == RC_NULL_AREA)
					? glm::vec3(1.0f, 0.0f, 0.0f) // Red for non-walkable
					: glm::vec3(0.0f, 1.0f, 0.0f); // Green for walkable

				float cs = chf->cs;

				// Define quad vertices (CCW)
				outVertices.push_back({ glm::vec3(fx, fy, fz), color });             // 0
				outVertices.push_back({ glm::vec3(fx + cs, fy, fz), color });        // 1
				outVertices.push_back({ glm::vec3(fx + cs, fy, fz + cs), color });   // 2
				outVertices.push_back({ glm::vec3(fx, fy, fz + cs), color });        // 3

				// Two triangles per quad
				outIndices.push_back(indexOffset + 0);
				outIndices.push_back(indexOffset + 1);
				outIndices.push_back(indexOffset + 2);

				outIndices.push_back(indexOffset + 0);
				outIndices.push_back(indexOffset + 2);
				outIndices.push_back(indexOffset + 3);

				indexOffset += 4;
			}
		}
	}
}

dtPolyRef FindNearestPolyWithPreferredY(
	dtNavMeshQuery* navQuery,
	const dtQueryFilter& filter,
	const float pos[3],
	float preferredY,
	float outNearestPoint[3],
	float initialHeight = 5.0f,
	float maxHeight = 500.0f,
	float step = 10.0f)
{
	dtPolyRef nearestPoly = 0;
	float bestDist = FLT_MAX;
	float tempNearest[3];

	// Step through extents gradually (starting narrow, expanding outward)
	for (float halfHeight = initialHeight; halfHeight <= maxHeight; halfHeight += step)
	{
		const float extents[3] = { 200.0f, 200, 200.0f };

		dtStatus status = navQuery->findNearestPoly(pos, extents, &filter, &nearestPoly, tempNearest);

		if (dtStatusSucceed(status) && nearestPoly)
		{
			// Measure how close this polygon's snapped Y is to preferredY
			float yDiff = fabs(tempNearest[1] - preferredY);

			if (yDiff < bestDist)
			{
				bestDist = yDiff;
				memcpy(outNearestPoint, tempNearest, sizeof(float) * 3);
			}

			// If we found something very close to preferred Y, stop early
			if (yDiff < 0.5f) // tweak tolerance (e.g., 0.5m)
			{
				return nearestPoly;
			}
		}
	}

	return nearestPoly;
}


// Debug function: check if the player and enemies are on the same connected component
void DebugNavmeshConnectivity(
	dtNavMeshQuery* navQuery,
	dtNavMesh* navMesh,
	const dtQueryFilter& filter,
	const float playerPos[3],
	const float enemyPositions[3])
{
	// Small AABB around query point to find nearest polys
	const float extents[3] = { 600.0f, 400.0f, 600.0f }; // x/z search size and y search size

	float playpos[3] = { playerPos[0], playerPos[1], playerPos[2] };
	float snapped[3];

	dtPolyRef playerPoly = FindNearestPolyWithPreferredY(navQuery, filter, extents, playpos[0], snapped, -300.0f, 500.0f );
	//dtStatus playerStatus = navQuery->findNearestPoly(playerPos, extents, &filter, &playerPoly, nullptr);

	if (!playerPoly)
	{
		Logger::log(1, "[DEBUG] Player is NOT on navmesh (no nearest poly found)\n");
		return;
	}

	Logger::log(1, "[DEBUG] Player nearest poly: %llu\n", (unsigned long long)playerPoly);

	// Track islands by walking links in navmesh
	const dtMeshTile* playerTile = nullptr;
	const dtPoly* playerDtPoly = nullptr;
	navMesh->getTileAndPolyByRefUnsafe(playerPoly, &playerTile, &playerDtPoly);
	unsigned short playerRegionId = playerDtPoly->getArea();

	Logger::log(1, "[DEBUG] Player is in region ID: %d\n", playerRegionId);

	// Now check each enemy
	for (size_t i = 0; i < 4; i++)
	{
		dtPolyRef enemyPoly = 0;
		dtStatus enemyStatus = navQuery->findNearestPoly(enemyPositions, extents, &filter, &enemyPoly, nullptr);

		if (dtStatusFailed(enemyStatus) || !enemyPoly)
		{
			Logger::log(1, "[DEBUG] Enemy %zu is NOT on navmesh (no nearest poly found)\n", i);
			continue;
		}

		const dtMeshTile* enemyTile = nullptr;
		const dtPoly* enemyDtPoly = nullptr;
		navMesh->getTileAndPolyByRefUnsafe(enemyPoly, &enemyTile, &enemyDtPoly);
		unsigned short enemyRegionId = enemyDtPoly->getArea();

		Logger::log(1, "[DEBUG] Enemy %zu nearest poly: %llu (region %d)\n", i,
			(unsigned long long)enemyPoly, enemyRegionId);

		if (enemyRegionId != playerRegionId)
		{
			Logger::log(1, "   Enemy %zu is in a DIFFERENT navmesh region (not connected)\n", i);
		}
		else
		{
			Logger::log(1, "   Enemy %zu is in the SAME navmesh region as player\n", i);
		}
	}
}


void ProbeNavmesh(dtNavMeshQuery* navQuery, dtQueryFilter* filter,
	float x, float y, float z)
{
	float pos[3] = { x, y, z };
	float halfExtents[3] = { 500.0f, 500.0f, 500.0f };   // Big search area
	dtPolyRef ref;
	float snapped[3];

	ref = FindNearestPolyWithPreferredY(navQuery, *filter, pos, y, halfExtents, *snapped);
	Logger::log(1, "Probe at (%.2f, %.2f, %.2f): ", x, y, z);

	if (ref == 0)
	{
		Logger::log(1, "No polygon found.\n");
	}
	else
	{
		Logger::log(1, "Found polygon %llu at snapped position (%.2f, %.2f, %.2f)\n",
			(unsigned long long)ref,
			snapped[0], snapped[1], snapped[2]);
	}
}


bool AddAgentToCrowd(dtCrowd* crowd,
	dtNavMeshQuery* navQuery,
	const float* startPos,
	const dtCrowdAgentParams* params,
	const dtQueryFilter* filter,
	float* outSnappedPos,
	int& outAgentID)
{
	// Make a local copy of startPos
	float searchPos[3] = { startPos[0], startPos[1], startPos[2] };

	// Broad search extents for safety (you can tune smaller later)
	float halfExtents[3] = { 500.0f, 500.0f, 500.0f };   // Big search area

	// Query nearest poly
	dtPolyRef startPoly = 0;
	dtStatus status = navQuery->findNearestPoly(searchPos, halfExtents, filter, &startPoly, outSnappedPos);

	Logger::log(1, "[CrowdSpawn] Query start(%.2f, %.2f, %.2f)\n",
		searchPos[0], searchPos[1], searchPos[2]);

	if (dtStatusFailed(status))
	{
		Logger::log(1, "[CrowdSpawn] findNearestPoly FAILED (status=0x%X)\n", status);
		outAgentID = -1;
		return false;
	}

	if (startPoly == 0)
	{
		Logger::log(1, "[CrowdSpawn] No polygon found near startPos (%.2f, %.2f, %.2f)\n",
			searchPos[0], searchPos[1], searchPos[2]);
		outAgentID = -1;
		return false;
	}

	Logger::log(1, "[CrowdSpawn] Found poly %llu, snapped to (%.2f, %.2f, %.2f)\n",
		(unsigned long long)startPoly,
		outSnappedPos[0], outSnappedPos[1], outSnappedPos[2]);

	// Add agent at snapped position
	outAgentID = crowd->addAgent(outSnappedPos, params);
	if (outAgentID == -1)
	{
		Logger::log(1, "[CrowdSpawn] Crowd is full or agent failed to add!\n");
		return false;
	}

	// Check agent state after adding
	const dtCrowdAgent* ag = crowd->getAgent(outAgentID);
	if (!ag)
	{
		Logger::log(1, "[CrowdSpawn] Agent pointer null after addAgent.\n");
		return false;
	}

	Logger::log(1, "[CrowdSpawn] Agent %d added: state=%d (0=INVALID, 1=WALKING, 2=OFFMESH, 3=IDLE)\n",
		outAgentID, ag->state);

	return (ag->state != DT_CROWDAGENT_STATE_INVALID);
}


GameManager::GameManager(Window* window, unsigned int width, unsigned int height)
	: window(window), screenWidth(width), screenHeight(height)
{
	inputManager = new InputManager();
	audioSystem = new AudioSystem(this);

	if (!audioSystem->Initialize())
	{
		Logger::log(1, "%s error: AudioSystem init error\n", __FUNCTION__);
		audioSystem->Shutdown();
		delete audioSystem;
		audioSystem = nullptr;
	}

	mAudioManager = new AudioManager(this);

	window->setInputManager(inputManager);

	renderer = window->getRenderer();
	renderer->SetUpMinimapFBO(width, height);
	renderer->SetUpShadowMapFBO(SHADOW_WIDTH, SHADOW_HEIGHT);

	playerShader.loadShaders("C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Shaders/vertex_gpu_dquat_player.glsl", "C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Shaders/pbr_fragment.glsl");
	groundShader.loadShaders("C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Shaders/vertex2.glsl", "C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Shaders/fragment2.glsl");
	enemyShader.loadShaders("C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Shaders/vertex_gpu_dquat_enemy.glsl", "C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Shaders/pbr_fragment_emissive.glsl");
	gridShader.loadShaders("C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Shaders/pbr_vertex.glsl", "C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Shaders/pbr_fragment.glsl");
	crosshairShader.loadShaders("C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Shaders/crosshair_vert.glsl", "C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Shaders/crosshair_frag.glsl");
	lineShader.loadShaders("C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Shaders/line_vert.glsl", "C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Shaders/line_frag.glsl");
	aabbShader.loadShaders("C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Shaders/aabb_vert.glsl", "C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Shaders/aabb_frag.glsl");
	cubeShader.loadShaders("C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Shaders/pbr_vertex.glsl", "C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Shaders/pbr_fragment_emissive.glsl");
	cubemapShader.loadShaders("C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Shaders/cubemap_vertex.glsl", "C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Shaders/cubemap_fragment.glsl");
	minimapShader.loadShaders("C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Shaders/quad_vertex.glsl", "C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Shaders/quad_fragment.glsl");
	shadowMapShader.loadShaders("C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Shaders/shadow_map_vertex.glsl", "C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Shaders/shadow_map_fragment.glsl");
	playerShadowMapShader.loadShaders("C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Shaders/shadow_map_player_vertex.glsl", "C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Shaders/shadow_map_fragment.glsl");
	groundShadowShader.loadShaders("C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Shaders/shadow_map_vertex.glsl", "C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Shaders/shadow_map_fragment.glsl");
	enemyShadowMapShader.loadShaders("C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Shaders/shadow_map_enemy_vertex.glsl", "C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Shaders/shadow_map_fragment.glsl");
	shadowMapQuadShader.loadShaders("C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Shaders/shadow_map_quad_vertex.glsl", "C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Shaders/shadow_map_quad_fragment.glsl");
	playerMuzzleFlashShader.loadShaders("C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Shaders/muzzle_flash_vertex.glsl", "C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Shaders/muzzle_flash_fragment.glsl");
	navMeshShader.loadShaders("C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Shaders/navmesh_vert.glsl", "C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Shaders/navmesh_frag.glsl");
	hfnavMeshShader.loadShaders("C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Shaders/hf_vert.glsl", "C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Shaders/hf_frag.glsl");

	physicsWorld = new PhysicsWorld();

	cubemapFaces = {
		"C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Assets/Textures/Skybox/right.png",
		"C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Assets/Textures/Skybox/left.png",
		"C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Assets/Textures/Skybox/top.png",
		"C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Assets/Textures/Skybox/bottom.png",
		"C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Assets/Textures/Skybox/front.png",
		"C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Assets/Textures/Skybox/back.png"
	};

	cubemap = new Cubemap(&cubemapShader);
	cubemap->LoadMesh();
	cubemap->LoadCubemap(cubemapFaces);

	cell = new Cell();
	cell->SetUpVAO();
	//    cell->LoadTexture("C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Assets/Textures/Ground.png", cell->mTex);
	std::string cubeTexFilename = "C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Assets/Textures/Cover.png";

	gameGrid = new Grid();

	for (glm::vec3 coverPos : gameGrid->coverPositions)
	{
		Cube* cover = new Cube((coverPos), glm::vec3((float)gameGrid->GetCellSize()), &cubeShader, &shadowMapShader, false, this, cubeTexFilename);
		cover->SetAABBShader(&aabbShader);
		cover->LoadMesh();
		coverSpots.push_back(cover);
	}

	ground = new Ground(mapPos, mapScale, &groundShader, &groundShadowShader, false, this);

	//size_t vertexOffset = 0;
	//int vertexCount = 0;
	//
	//int coverCount = 0;
	//
	//for (Cube* coverSpot : coverSpots)
	//{
	//	std::vector<glm::vec3> coverVerts = coverSpot->GetPositionVertices();
	//	for (glm::vec3 coverPosVerts : coverVerts)
	//	{
	//		navMeshVertices.push_back(coverPosVerts.x);
	//		navMeshVertices.push_back(coverPosVerts.y);
	//		navMeshVertices.push_back(coverPosVerts.z);
	//		vertexCount += 3;
	//	}
	//
	//	GLuint* indices = coverSpot->indices;
	//
	//	int numIndices = 36;
	//
	//	for (int i = 0; i < numIndices; i++)
	//	{
	//		navMeshIndices.push_back(indices[i]);
	//	}
	//
	//	vertexOffset += coverVerts.size();
	//
	//	coverCount++;
	//}

	std::vector<Ground::GLTFMesh> meshDataGrnd = ground->meshData;
	int mapVertCount = 0;
	int mapIndCount = 0;
	int triCount = 0;
	int vertexOffset = 0;

	for (Ground::GLTFMesh& mesh : meshDataGrnd)
	{
		for (Ground::GLTFPrimitive& prim : mesh.primitives)
		{
			for (glm::vec3 vert : prim.verts)
			{
				glm::vec4 newVert = glm::vec4(vert.x, vert.y, vert.z, 1.0f);
				glm::mat4 model = glm::mat4(1.0f);
				model = glm::translate(model, mapPos);
				model = glm::rotate(model, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
				model = glm::scale(model, mapScale);
				glm::vec4 newVertTr = model * newVert;

				// Add transformed vertices
				mapVerts.push_back(glm::vec3(newVertTr.x, newVertTr.y, newVertTr.z));
				navMeshVertices.push_back(newVertTr.x);
				navMeshVertices.push_back(newVertTr.y);
				navMeshVertices.push_back(newVertTr.z);
				mapVertCount += 3;
			}

			for (unsigned int idx : prim.indices)
			{
				navMeshIndices.push_back(idx + vertexOffset);
				mapIndCount++;
			}

			vertexOffset += static_cast<int>(prim.verts.size());
		}
	}


	//navMeshVertices.resize(1162447);
	//navMeshIndices.resize(1162447);

	gameGrid->initializeGrid();

	std::vector<glm::vec3> gridVerts = gameGrid->GetWSVertices();

	Logger::log(1, "Map vertices count: %i\n", mapVerts.size());

	int gridVertCount = 0;
	//std::vector<glm::mat4> models = gameGrid->GetModels();

	for (glm::vec3 mapVertex : mapVerts)
	{
		//navMeshVertices.push_back(mapVertex.x);
		//navMeshVertices.push_back(mapVertex.y);
		//navMeshVertices.push_back(mapVertex.z);

	}
























	Logger::log(1, "navMeshVertices count: %zu\n", navMeshVertices.size());
	for (size_t i = 0; i < std::min((size_t)10, navMeshVertices.size() / 3); ++i)
	{
		Logger::log(1, "Vertex %zu: %.2f %.2f %.2f\n",
			i,
			navMeshVertices[i * 3],
			navMeshVertices[i * 3 + 1],
			navMeshVertices[i * 3 + 2]);
	}


	//for (int index : gameGrid->GetIndices())
	//{
	//	navMeshIndices.push_back(index + vertexOffset);
	//}

	int indexCount = (int)navMeshIndices.size();
	Logger::log(1, "Index Count: %zu\n", (int)navMeshIndices.size());

	triIndices = new int[indexCount];

	for (int i = 0; i < navMeshIndices.size(); i++)
	{
		triIndices[i] = navMeshIndices[i];
	}

	Logger::log(1, "Index Count: %zu\n", indexCount);




	int triangleCount = indexCount / 3;

	triAreas = new unsigned char[triangleCount];

	filter.setIncludeFlags(0xFFFF); // Include all polygons for testing
	filter.setExcludeFlags(0);      // Exclude no polygons

	Logger::log(1, "Tri Areas: %s", triAreas);

	int vertexCount = navMeshVertices.size() / 3;

	for (int i = 0; i < triangleCount * 3; i++) {
		if (triIndices[i] < 0 || triIndices[i] >= vertexCount) {
			Logger::log(1, "Invalid triangle index %d: %d (vertexCount=%d)\n",
				i, triIndices[i], vertexCount);
		}
	}


	// Slope threshold (in degrees) – typical value for shooters is 45
	const float WALKABLE_SLOPE = 35.0f;
	ctx = new rcContext();

	for (int i = 0; i < triangleCount; ++i) {
		float norm[3];
		const float* v0 = &navMeshVertices[triIndices[i * 3 + 0] * 3];
		const float* v1 = &navMeshVertices[triIndices[i * 3 + 1] * 3];
		const float* v2 = &navMeshVertices[triIndices[i * 3 + 2] * 3];

		glm::vec3 v0_glm(v0[0], v0[1], v0[2]);
		glm::vec3 v1_glm(v1[0], v1[1], v1[2]);
		glm::vec3 v2_glm(v2[0], v2[1], v2[2]);

		// Compute edges
		glm::vec3 e0 = v1_glm - v0_glm;
		glm::vec3 e1 = v2_glm - v0_glm;

		// Compute face normal
		glm::vec3 faceNormal = glm::normalize(glm::cross(e0, e1));

		// Flip winding if normal points down
		if (faceNormal.y < 0.0f) {
			std::swap(triIndices[i * 3 + 1], triIndices[i * 3 + 2]);
		}

	}


	// Mark which triangles are walkable based on slope
	rcMarkWalkableTriangles(ctx,
		WALKABLE_SLOPE,
		navMeshVertices.data(), navMeshVertices.size() / 3,
		triIndices, triangleCount,
		triAreas);
	// Count how many triangles are walkable
	int walkableCount = 0;
	for (int i = 0; i < triangleCount; ++i)
	{
		if (triAreas[i] == RC_WALKABLE_AREA)
			walkableCount++;
	}

	Logger::log(1, "[Recast] Triangles processed: %d\n", triangleCount);
	Logger::log(1, "[Recast] Walkable triangles:  %d\n", walkableCount);
	Logger::log(1, "[Recast] Non-walkable:        %d\n", triangleCount - walkableCount);

	// Optional sanity check: Print first few triangles and their walkable flag
	for (int i = 0; i < std::min(triangleCount, 5); ++i)
	{
		Logger::log(1, "  Tri %d: %s\n", i,
			(triAreas[i] == RC_WALKABLE_AREA) ? "WALKABLE" : "NOT WALKABLE");
	}
	//for (int i = 0; i < triangleCount; ++i)
	//{
	//	triAreas[i] = RC_WALKABLE_AREA;
	//}
	for (int i = 0; i < std::min(triangleCount, 5); ++i)
	{
		Logger::log(1, "  Tri %d: %s\n", i,
			(triAreas[i] == RC_WALKABLE_AREA) ? "WALKABLE" : "NOT WALKABLE");
	}




	rcConfig cfg{};

	cfg.cs = 0.3f;                      // Cell size
	cfg.ch = 0.1f;                      // Cell height
	cfg.walkableSlopeAngle = WALKABLE_SLOPE;     // Steeper slopes allowed
	cfg.walkableHeight = 0.01f;          // Min agent height
	cfg.walkableClimb = 10.0f;           // Step height
	cfg.walkableRadius = 0.01f;          // Agent radius
	cfg.maxEdgeLen = 24;                // Longer edges for smoother polys
	cfg.minRegionArea = 4;              // Retain smaller regions
	cfg.mergeRegionArea = 16;           // Merge small regions
	cfg.maxSimplificationError = 0.05f;  // Less aggressive simplification
	cfg.detailSampleDist = cfg.cs * 6;  // Balanced detail
	cfg.maxVertsPerPoly = 6;            // Max verts per poly
	cfg.tileSize = 32;                  // Tile size
	cfg.borderSize = (int)(cfg.walkableRadius / cfg.cs + 0.5f); // Tile overlap

	Logger::log(1, "Num verts %zu", navMeshVertices.size());

	Logger::log(1, "navMeshVertices count: %zu", navMeshVertices.size());
	for (size_t i = 0; i < std::min((size_t)10, navMeshVertices.size() / 3); ++i)
	{
		Logger::log(1, "Vertex %zu: %.2f %.2f %.2f",
			i,
			navMeshVertices[i * 3],
			navMeshVertices[i * 3 + 1],
			navMeshVertices[i * 3 + 2]);
	}


	float minBounds[3] = { -261.04f, -2.39, -231.76 };
	float maxBounds[3] = { 251.37f, 213.66f, 304.81f };

	//rcCalcBounds(navMeshVertices.data(), navMeshVertices.size() / 3, cfg.bmin, cfg.bmax);

	rcCalcBounds(navMeshVertices.data(), navMeshVertices.size() / 3, cfg.bmin, cfg.bmax);

	cfg.width = (int)((cfg.bmax[0] - cfg.bmin[0]) / cfg.cs + 0.5f);
	cfg.height = (int)((cfg.bmax[2] - cfg.bmin[2]) / cfg.cs + 0.5f);

	//cfg.width = (cfg.bmax[0] - cfg.bmin[0]);
	//cfg.height = (cfg.bmax[2] - cfg.bmin[2]);
	//cfg.bmax = maxBounds;
	//cfg.bmin = minBounds;

	//cfg.bmin[0] = minBounds[0];
	//cfg.bmin[1] = minBounds[1];
	//cfg.bmin[2] = minBounds[2];
	//cfg.bmax[0] = maxBounds[0];
	//cfg.bmax[1] = maxBounds[1];
	//cfg.bmax[2] = maxBounds[2];

	//cfg.cs = 0.3f;                      // Cell size
	//cfg.ch = 0.2f;                      // Cell height
	//cfg.walkableSlopeAngle = 30.0f;     // Allow steeper slopes
	//cfg.walkableHeight = 2.0f;          // Agent height (in grid units)
	//cfg.walkableClimb = 1.0f;           // Max climb height
	//cfg.walkableRadius = 1.0f;          // Agent radius
	//cfg.maxEdgeLen = 24;                // Larger edge length
	//cfg.minRegionArea = 8;  // Keep smaller regions
	//cfg.maxSimplificationError = 0.5f;  // Fine simplification
	//cfg.detailSampleDist = cfg.cs * 6;  // Balance detail
	//cfg.maxVertsPerPoly = 6;			// Fewer verts per poly
	//cfg.tileSize = 32;					// Tile size
	//cfg.borderSize = (int)(cfg.walkableRadius / cfg.cs + 0.5f);
	//cfg.width = 500.0f;
	//cfg.height = 500.0f;

	//float minBounds[3] = { 0.0f, 0.0f, 0.0f };
	//float maxBounds[3] = { 500.0f, 1.0f, 500.0f };

	heightField = rcAllocHeightfield();
	if (!rcCreateHeightfield(ctx, *heightField, cfg.bmax[0], cfg.bmax[2], cfg.bmin, cfg.bmax, cfg.cs, cfg.ch))
	{
		Logger::log(1, "%s error: Could not create heightfield\n", __FUNCTION__);
	}
	else
	{
		Logger::log(1, "%s: Heightfield successfully created\n", __FUNCTION__);
	};

	int numTris = navMeshIndices.size() / 3;

	Logger::log(1, "Triangle indices: %d\n", indexCount);
	Logger::log(1, "Triangle count: %d\n", numTris);

	//rcMarkWalkableTriangles(ctx, cfg.walkableSlopeAngle, navMeshVertices.data(), navMeshVertices.size() / 3, triIndices, triangleCount, triAreas);

	//for (int i = 0; i < indexCount / 3; ++i) {
	//	if (i < (coverCount * 36) / 3) {
	//		triAreas[i] = RC_NULL_AREA; // Mark this triangle as non-walkable
	//	}

	//}

	if (!rcRasterizeTriangles(ctx, navMeshVertices.data(), navMeshVertices.size() / 3, triIndices, triAreas, triangleCount, *heightField, cfg.walkableClimb))
	{
		Logger::log(1, "%s error: Could not rasterize triangles\n", __FUNCTION__);
	}
	else
	{
		Logger::log(1, "%s: rasterize triangles successfully created\n", __FUNCTION__);
	};

	//rcFilterLowHangingWalkableObstacles(ctx, cfg.walkableClimb, *heightField);
	rcFilterWalkableLowHeightSpans(ctx, cfg.walkableHeight, *heightField);
	rcFilterLedgeSpans(ctx, cfg.walkableHeight, cfg.walkableClimb, *heightField);

	compactHeightField = rcAllocCompactHeightfield();
	if (!rcBuildCompactHeightfield(ctx, cfg.walkableHeight, cfg.walkableClimb, *heightField, *compactHeightField))
	{
		Logger::log(1, "%s error: Could not build compact heightfield\n", __FUNCTION__);
	}
	else
	{
		Logger::log(1, "%s: compact heightfield successfully created\n", __FUNCTION__);
	};

	Logger::log(1, "CompactHeightfield walkable spans count: %d\n", compactHeightField->spanCount);

	std::vector<DebugVertex> debugSpanVertices;
	std::vector<unsigned int> debugSpanIndices;

	generateDebugSpanMeshFromHeightfield(compactHeightField, debugSpanVertices, debugSpanIndices);

	hfnavRenderMeshVertices.resize(debugSpanVertices.size() * 6);
	hfnavRenderMeshIndices.resize(debugSpanIndices.size());

	for (const DebugVertex& v : debugSpanVertices)
	{
		hfnavRenderMeshVertices.push_back(v.position.x);
		hfnavRenderMeshVertices.push_back(v.position.y);
		hfnavRenderMeshVertices.push_back(v.position.z);
		hfnavRenderMeshVertices.push_back(v.color.x);
		hfnavRenderMeshVertices.push_back(v.color.y);
		hfnavRenderMeshVertices.push_back(v.color.z);
	}

	for (unsigned int index : debugSpanIndices)
	{
		hfnavRenderMeshIndices.push_back(index);
	}



	if (!rcBuildLayerRegions(ctx, *compactHeightField, 0, cfg.minRegionArea))
	{
		Logger::log(1, "buildNavigation: Could not build layer regions.");
	}
	else
	{
		Logger::log(1, "buildNavigation: Layer regions successfully built.");

	}

	contourSet = rcAllocContourSet();

	if (!rcBuildContours(ctx, *compactHeightField, cfg.maxSimplificationError, cfg.maxEdgeLen, *contourSet))
	{
		Logger::log(1, "%s error: Could not build contours\n", __FUNCTION__);
	}
	else
	{
		Logger::log(1, "%s: contours successfully created\n", __FUNCTION__);
		Logger::log(1, "Contours count: %d\n", contourSet->nconts);
	};

	polyMesh = rcAllocPolyMesh();

	if (!rcBuildPolyMesh(ctx, *contourSet, cfg.maxVertsPerPoly, *polyMesh))
	{
		Logger::log(1, "%s error: Could not build polymesh\n", __FUNCTION__);
	}
	else
	{
		Logger::log(1, "%s: polymesh successfully created\n", __FUNCTION__);
	};

	polyMeshDetail = rcAllocPolyMeshDetail();
	if (!rcBuildPolyMeshDetail(ctx, *polyMesh, *compactHeightField, cfg.detailSampleDist, cfg.detailSampleMaxError, *polyMeshDetail))
	{
		Logger::log(1, "%s error: Could not build polymesh detail\n", __FUNCTION__);
	}
	else
	{
		Logger::log(1, "%s: polymesh detail successfully created\n", __FUNCTION__);
	};

	Logger::log(1, "NavMesh vertices count: %d\n", navMeshVertices.size());
	Logger::log(1, "NavMesh indices count: %d\n", navMeshIndices.size());
	Logger::log(1, "Heightfield span count: %d\n", heightField->width * heightField->height);
	Logger::log(1, "Compact heightfield span count: %d\n", compactHeightField->spanCount);
	Logger::log(1, "PolyMesh vertices count: %d\n", polyMesh->nverts);
	Logger::log(1, "PolyMesh polygons count: %d\n", polyMesh->npolys);
	Logger::log(1, "PolyMeshDetail vertices count: %d\n", polyMeshDetail->nverts);
	Logger::log(1, "PolyMesh bounds: Min(%.2f, %.2f, %.2f) Max(%.2f, %.2f, %.2f)\n",
		polyMesh->bmin[0], polyMesh->bmin[1], polyMesh->bmin[2],
		polyMesh->bmax[0], polyMesh->bmax[1], polyMesh->bmax[2]);

	for (int i = 0; i < std::min(10, (int)navMeshVertices.size()); i += 3) {
		Logger::log(1, "NavMesh Vert %d: %.2f %.2f %.2f", i / 3,
			navMeshVertices[i], navMeshVertices[i + 1], navMeshVertices[i + 2]);
	}

	for (int i = 0; i < std::min(10, (int)navMeshIndices.size()); ++i) {
#
		Logger::log(1, "Logging bad indices: \n");
		if (navMeshIndices[i] >= navMeshVertices.size()) {
			Logger::log(1, "BAD INDEX : %zu, out of range, %zu\n", navMeshIndices[i], navMeshVertices.size());
		}
	}

	for (int i = 0; i < std::min(10, (int)navMeshIndices.size()); i += 3) {

		Logger::log(1, "Logging degenerate triangles: \n");
		int a = navMeshIndices[i];
		int b = navMeshIndices[i + 1];
		int c = navMeshIndices[i + 2];
		if (a == b || b == c || a == c) {
			Logger::log(1, "DEGENERATE TRIANGLE: %zu, %zu, %zu\n", a, b, c);
		}
	}



	dtNavMeshCreateParams params;

	params.verts = polyMesh->verts;
	params.vertCount = polyMesh->nverts;
	params.polys = polyMesh->polys;
	params.polyAreas = polyMesh->areas;
	static const unsigned short DT_POLYFLAGS_WALK = 0x01;

	for (int i = 0; i < polyMesh->npolys; ++i)
	{
		if (polyMesh->areas[i] == RC_WALKABLE_AREA)
		{
			polyMesh->flags[i] = DT_POLYFLAGS_WALK;
		}
		else
		{
			polyMesh->flags[i] = DT_POLYFLAGS_WALK;
		}
		params.polyAreas = polyMesh->areas;
	};

	rcVcopy(params.bmin, polyMesh->bmin);
	rcVcopy(params.bmax, polyMesh->bmax);

	params.polyFlags = polyMesh->flags;


	params.polyCount = polyMesh->npolys;
	params.nvp = polyMesh->nvp;
	params.walkableHeight = cfg.walkableHeight;
	params.walkableRadius = cfg.walkableRadius;
	params.walkableClimb = cfg.walkableClimb;
	params.detailMeshes = polyMeshDetail->meshes;
	params.detailVerts = polyMeshDetail->verts;
	params.detailVertsCount = polyMeshDetail->nverts;
	params.detailTris = polyMeshDetail->tris;
	params.detailTriCount = polyMeshDetail->ntris;
	params.offMeshConVerts = nullptr;
	params.offMeshConRad = nullptr;
	params.offMeshConDir = nullptr;
	params.offMeshConAreas = nullptr;
	params.offMeshConFlags = nullptr;
	params.offMeshConUserID = nullptr;
	params.offMeshConCount = 0;
	params.cs = cfg.cs;
	params.ch = cfg.ch;
	params.buildBvTree = true;

	dtCreateNavMeshData(&params, &navData, &navDataSize);

	navMesh = dtAllocNavMesh();

	dtStatus navInitStatus = navMesh->init(navData, navDataSize, DT_TILE_FREE_DATA);

	if (dtStatusFailed(navInitStatus))
	{
		Logger::log(1, "%s error: Could not init Detour navMesh\n", __FUNCTION__);
	}

	navMeshQuery = dtAllocNavMeshQuery();

	dtStatus status = navMeshQuery->init(navMesh, 10);
	if (dtStatusFailed(status))
	{
		Logger::log(1, "%s error: Could not init Detour navMeshQuery\n", __FUNCTION__);
	}
	else
	{
		Logger::log(1, "%s: Detour navMeshQuery successfully initialized\n", __FUNCTION__);

	}

	const dtNavMeshParams* nmparams = navMesh->getParams();
	Logger::log(1, "Navmesh origin: %.2f %.2f %.2f\n", nmparams->orig[0], nmparams->orig[1], nmparams->orig[2]);

	for (int y = 0; y < navMesh->getMaxTiles(); ++y) {
		for (int x = 0; x < navMesh->getMaxTiles(); ++x) {
			const dtMeshTile* tile = navMesh->getTileAt(x, y, 0);  // layer = 0
			if (!tile || !tile->header) continue;

			Logger::log(1, "Tile at (%d,%d): Bmin(%.2f %.2f %.2f) Bmax(%.2f %.2f %.2f)\n",
				x, y,
				tile->header->bmin[0], tile->header->bmin[1], tile->header->bmin[2],
				tile->header->bmin[0], tile->header->bmin[1], tile->header->bmin[2],
				tile->header->bmax[0], tile->header->bmax[1], tile->header->bmax[2]);
		}
	}



	camera = new Camera(glm::vec3(50.0f, 3.0f, 80.0f));
	minimapCamera = new Camera(glm::vec3((gameGrid->GetCellSize() * gameGrid->GetGridSize()) / 2.0f, 140.0f, (gameGrid->GetCellSize() * gameGrid->GetGridSize()) / 2.0f), glm::vec3(0.0f, -1.0f, 0.0f), 0.0f, -90.0f, glm::vec3(0.0f, 0.0f, -1.0f));

	minimapQuad = new Quad();
	minimapQuad->SetUpVAO(false);

	shadowMapQuad = new Quad();
	shadowMapQuad->SetUpVAO(false);

	playerMuzzleFlashQuad = new Quad();
	playerMuzzleFlashQuad->SetUpVAO(true);
	playerMuzzleFlashQuad->SetShader(&playerMuzzleFlashShader);
	playerMuzzleFlashQuad->LoadTexture("C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Assets/Textures/muzzleflash.png");

	enemyMuzzleFlashQuad = new Quad();
	enemyMuzzleFlashQuad->SetUpVAO(true);
	enemyMuzzleFlashQuad->SetShader(&playerMuzzleFlashShader);
	enemyMuzzleFlashQuad->LoadTexture("C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Assets/Textures/muzzleflash.png");

	enemy2MuzzleFlashQuad = new Quad();
	enemy2MuzzleFlashQuad->SetUpVAO(true);
	enemy2MuzzleFlashQuad->SetShader(&playerMuzzleFlashShader);
	enemy2MuzzleFlashQuad->LoadTexture("C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Assets/Textures/muzzleflash.png");

	enemy3MuzzleFlashQuad = new Quad();
	enemy3MuzzleFlashQuad->SetUpVAO(true);
	enemy3MuzzleFlashQuad->SetShader(&playerMuzzleFlashShader);
	enemy3MuzzleFlashQuad->LoadTexture("C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Assets/Textures/muzzleflash.png");

	enemy4MuzzleFlashQuad = new Quad();
	enemy4MuzzleFlashQuad->SetUpVAO(true);
	enemy4MuzzleFlashQuad->SetShader(&playerMuzzleFlashShader);
	enemy4MuzzleFlashQuad->LoadTexture("C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Assets/Textures/muzzleflash.png");


	player = new Player(glm::vec3(-200.0f, 28.9f, -158.0f), glm::vec3(3.0f), &playerShader, &playerShadowMapShader, true, this);
	//player = new Player( (glm::vec3(23.0f, 0.0f, 37.0f)), glm::vec3(3.0f), &playerShader, &playerShadowMapShader, true, this);

	player->aabbShader = &aabbShader;

	std::string texture = "C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Assets/Models/GLTF/Enemies/Ely/EnemyEly_ely_vanguardsoldier_kerwinatienza_M2_BaseColor.png";
	std::string texture2 = "C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Assets/Models/GLTF/Enemies/Ely/ely-vanguardsoldier-kerwinatienza_diffuse_2.png";
	std::string texture3 = "C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Assets/Models/GLTF/Enemies/Ely/ely-vanguardsoldier-kerwinatienza_diffuse_3.png";
	std::string texture4 = "C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Assets/Models/GLTF/Enemies/Ely/ely-vanguardsoldier-kerwinatienza_diffuse_4.png";

	enemy = new Enemy(glm::vec3(-190.0f, 28.9f, -148.0f), glm::vec3(3.0f), &enemyShader, &enemyShadowMapShader, true, this, gameGrid, texture, 0, GetEventManager(), *player);
	enemy->SetAABBShader(&aabbShader);
	enemy->SetUpAABB();

	enemy2 = new Enemy(glm::vec3(-210.0f, 28.9f, -158.0f), glm::vec3(3.0f), &enemyShader, &enemyShadowMapShader, true, this, gameGrid, texture2, 1, GetEventManager(), *player);
	enemy2->SetAABBShader(&aabbShader);
	enemy2->SetUpAABB();

	enemy3 = new Enemy(glm::vec3(-200.0f, 28.9f, -158.0f), glm::vec3(3.0f), &enemyShader, &enemyShadowMapShader, true, this, gameGrid, texture3, 2, GetEventManager(), *player);
	enemy3->SetAABBShader(&aabbShader);
	enemy3->SetUpAABB();

	enemy4 = new Enemy(glm::vec3(-210.0f, 28.9f, -168.0f), glm::vec3(3.0f), &enemyShader, &enemyShadowMapShader, true, this, gameGrid, texture4, 3, GetEventManager(), *player);
	enemy4->SetAABBShader(&aabbShader);
	enemy4->SetUpAABB();

	crosshair = new Crosshair(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.3f), &crosshairShader, &shadowMapShader, false, this);
	crosshair->LoadMesh();
	crosshair->LoadTexture("C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Assets/Textures/Crosshair.png");
	line = new Line(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f), &lineShader, &shadowMapShader, false, this);
	line->LoadMesh();

	enemyLine = new Line(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f), &lineShader, &shadowMapShader, false, this);
	enemy2Line = new Line(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f), &lineShader, &shadowMapShader, false, this);
	enemy3Line = new Line(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f), &lineShader, &shadowMapShader, false, this);
	enemy4Line = new Line(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f), &lineShader, &shadowMapShader, false, this);
	enemyLine->LoadMesh();
	enemy2Line->LoadMesh();
	enemy3Line->LoadMesh();
	enemy4Line->LoadMesh();

	//ground = new Ground(mapPos, mapScale, &groundShader, &groundShadowShader, false, this);

	AudioComponent* fireAudioComponent = new AudioComponent(enemy);
	fireAudioComponent->PlayEvent("event:/FireLoop");


	inputManager->setContext(camera, player, enemy, width, height);

	/* reset skeleton split */
	playerSkeletonSplitNode = player->model->getNodeCount() - 1;
	enemySkeletonSplitNode = enemy->model->getNodeCount() - 1;

	std::srand(static_cast<unsigned int>(std::time(nullptr)));

	gameObjects.push_back(player);
	gameObjects.push_back(enemy);
	gameObjects.push_back(enemy2);
	gameObjects.push_back(enemy3);
	gameObjects.push_back(enemy4);
	gameObjects.push_back(ground);

	/*for (Cube* coverSpot : coverSpots)
	{
		gameObjects.push_back(coverSpot);
	}*/

	enemies.push_back(enemy);
	enemies.push_back(enemy2);
	enemies.push_back(enemy3);
	enemies.push_back(enemy4);

	if (initializeQTable)
	{
		for (auto& enem : enemies)
		{
			int enemyID = enem->GetID();
			Logger::log(1, "%s Initializing Q Table for Enemy %d\n", __FUNCTION__, enemyID);
			InitializeQTable(mEnemyStateQTable[enemyID]);
			Logger::log(1, "%s Initialized Q Table for Enemy %d\n", __FUNCTION__, enemyID);
		}
	}
	else if (loadQTable)
	{
		for (auto& enem : enemies)
		{
			int enemyID = enem->GetID();
			Logger::log(1, "%s Loading Q Table for Enemy %d\n", __FUNCTION__, enemyID);
			LoadQTable(mEnemyStateQTable[enemyID], std::to_string(enemyID) + mEnemyStateFilename);
			Logger::log(1, "%s Loaded Q Table for Enemy %d\n", __FUNCTION__, enemyID);
		}
	}

	//mMusicEvent = audioSystem->PlayEvent("event:/bgm");

	//for (int i = 0; i < polyMesh->nverts; ++i) {
	//	const unsigned short* v = &polyMesh->verts[i * 3];
	//	navRenderMeshVertices.push_back(v[0]); // X
	//	navRenderMeshVertices.push_back(v[1]); // Y
	//	navRenderMeshVertices.push_back(v[2]); // Z
	//}
	//for (int i = 0; i < polyMesh->npolys; ++i) {
	//	for (int j = 0; j < polyMesh->nvp; ++j) {
	//		unsigned short index = polyMesh->polys[i * polyMesh->nvp + j];
	//		if (index == RC_MESH_NULL_IDX) break;
	//		navRenderMeshIndices.push_back(static_cast<unsigned int>(index));
	//	}
	//}


	const int nvp = polyMesh->nvp;
	const float cs = polyMesh->cs;
	const float ch = polyMesh->ch;
	const float* orig = polyMesh->bmin;

	//navMeshVertices.clear();
	//navMeshIndices.clear();

	for (int i = 0, j = polyMesh->nverts - 1; i < polyMesh->nverts; j = i++)
	{
		const unsigned short* v = &polyMesh->verts[i * 3];
		const float x = orig[0] + v[0] * cs;
		const float y = orig[1] + v[1] * ch;
		const float z = orig[2] + v[2] * cs;
		navRenderMeshVertices.push_back(x);
		navRenderMeshVertices.push_back(y);
		navRenderMeshVertices.push_back(z);
	}

	// Process indices
	for (int i = 0; i < polyMesh->npolys; ++i)
	{
		const unsigned short* p = &polyMesh->polys[i * nvp * 2];
		for (int j = 2; j < nvp; ++j)
		{
			if (p[j] == RC_MESH_NULL_IDX) break;
			// Skip degenerate triangles
			if (p[0] == p[j - 1] || p[0] == p[j] || p[j - 1] == p[j]) continue;
			navRenderMeshIndices.push_back(p[0]);      // Triangle vertex 1
			navRenderMeshIndices.push_back(p[j - 1]); // Triangle vertex 2
			navRenderMeshIndices.push_back(p[j]);     // Triangle vertex 3
		}
	}
	ProbeNavmesh(navMeshQuery, &filter, -162.6f, 46.4f, -127.9f);
	ProbeNavmesh(navMeshQuery, &filter, -162.6f, 46.4f, -127.9f);
	ProbeNavmesh(navMeshQuery, &filter, -162.6f, 46.4f, -127.9f);
	ProbeNavmesh(navMeshQuery, &filter, 50.0f, -10.0f, 50.0f);
	ProbeNavmesh(navMeshQuery, &filter, 95.0f, -20.0f, 95.0f);

	// Create VAO
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	// Create VBO
	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, navRenderMeshVertices.size() * sizeof(float), navRenderMeshVertices.data(), GL_STATIC_DRAW);

	// Create EBO
	glGenBuffers(1, &ebo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, navRenderMeshIndices.size() * sizeof(unsigned int), navRenderMeshIndices.data(), GL_STATIC_DRAW);

	// Enable vertex attribute (e.g., position at location 0)
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	glBindVertexArray(0);


	//crowd = dtAllocCrowd();
	//crowd->init(enemies.size(), 1.0f, navMesh);

			// Create VAO
	glGenVertexArrays(1, &hfvao);
	glBindVertexArray(hfvao);

	// Create VBO
	glGenBuffers(1, &hfvbo);
	glBindBuffer(GL_ARRAY_BUFFER, hfvbo);
	glBufferData(GL_ARRAY_BUFFER, hfnavRenderMeshVertices.size() * sizeof(float), hfnavRenderMeshVertices.data(), GL_STATIC_DRAW);

	// Create EBO
	glGenBuffers(1, &hfebo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, hfebo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, hfnavRenderMeshIndices.size() * sizeof(unsigned int), hfnavRenderMeshIndices.data(), GL_STATIC_DRAW);

	// Enable vertex attribute (e.g., position at location 0)
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);


	glBindVertexArray(0);

	crowd = dtAllocCrowd();
	crowd->init(50, 2.0f, navMesh);

	//for (auto& enem : enemies)
	//{
	//	dtCrowdAgentParams ap;
	//	memset(&ap, 0, sizeof(ap));
	//	ap.radius = 0.01f;
	//	ap.height = 3.0f;
	//	ap.maxSpeed = 3.5f;
	//	ap.maxAcceleration = 8.0f; // Meters per second squared
	//	ap.collisionQueryRange = ap.radius * 12.0f;

	//	float startingPos[3] = { enem->getPosition().x, enem->getPosition().y, enem->getPosition().z };
	//	enemyAgentIDs.push_back(crowd->addAgent(startingPos, &ap));


	//};

	//for (auto& enem : enemies)
	//{
	//	dtCrowdAgentParams ap;
	//	memset(&ap, 0, sizeof(ap));
	//	ap.radius = 0.6f;
	//	ap.height = 2.0f;
	//	ap.maxSpeed = 3.5f;
	//	ap.maxAcceleration = 8.0f; // Meters per second squared
	//	ap.collisionQueryRange = ap.radius * 12.0f;


	//	float startingPos[3] = { enem->getPosition().x, enem->getPosition().y, enem->getPosition().z };
	//	float snappedPos[3];
	//	dtPolyRef startPoly;
	//	navMeshQuery->findNearestPoly(startingPos, halfExtents, &filter, &startPoly, snappedPos);
	//	enemyAgentIDs.push_back(crowd->addAgent(snappedPos, &ap));
	//}

	for (auto& enem : enemies)
	{
		dtCrowdAgentParams ap;
		memset(&ap, 0, sizeof(ap));
		ap.radius = 0.6f;
		ap.height = 2.0f;
		ap.maxSpeed = 3.5f;
		ap.maxAcceleration = 8.0f;
		ap.collisionQueryRange = ap.radius * 12.0f;

		float startPos[3] = { enem->getPosition().x, enem->getPosition().y, enem->getPosition().z };

		int agentID;

		bool added = AddAgentToCrowd(crowd, navMeshQuery, startPos, &ap, &filter, snappedPos, agentID);

		if (added)
		{
			Logger::log(1, "[Spawn] Enemy spawned as agent %d at (%.2f, %.2f, %.2f)\n",
				agentID, snappedPos[0], snappedPos[1], snappedPos[2]);

			enemyAgentIDs.push_back(agentID);
		}
		else
		{
			Logger::log(1, "[Spawn] Enemy spawn FAILED at (%.2f, %.2f, %.2f)\n",
				startPos[0], startPos[1], startPos[2]);
			Logger::log(1, "[Spawn] Enemy spawned as agent %d at (%.2f, %.2f, %.2f)\n",
				agentID, snappedPos[0], snappedPos[1], snappedPos[2]);
		}


		float snappedPos[3] = { 1.0f, 0.0f, 1.0f };

		dtPolyRef polyref = 0;
		dtStatus newstatus = navMeshQuery->findNearestPoly(snappedPos, halfExtents, &filter, &polyref, snappedPos);


		if (dtStatusFailed(newstatus) || polyref == 0)
		{
			Logger::log(1, "findNearestPoly failed: no polygon found near %.2f %.2f %.2f\n",
				85.0f, 0.0f, 25.0f);
			return; // stop here, don’t use snappedPos
		}

		float meshheight = 0.0f;
		if (navMeshQuery->getPolyHeight(polyref, snappedPos, &meshheight) == DT_SUCCESS)
		{
			Logger::log(1, "Navmesh height at %.2f %.2f: %.2f\n", snappedPos[0], snappedPos[2], meshheight);
		}

	}
}



void GameManager::setupCamera(unsigned int width, unsigned int height)
{
	camera->Zoom = 45.0f;
	if (camera->Mode == PLAYER_FOLLOW)
	{
		camera->Pitch = 45.0f;
		camera->FollowTarget(player->getPosition() + (player->PlayerFront * camera->playerPosOffset), player->PlayerFront, camera->playerCamRearOffset, camera->playerCamHeightOffset);
		view = camera->GetViewMatrixPlayerFollow(player->getPosition() + (player->PlayerFront * camera->playerPosOffset), glm::vec3(0.0f, 1.0f, 0.0f));
	}
	else if (camera->Mode == ENEMY_FOLLOW)
	{
		if (enemy->isDestroyed)
		{
			camera->Mode = FLY;
			return;
		}
		camera->FollowTarget(enemy->getPosition(), enemy->GetEnemyFront(), camera->enemyCamRearOffset, camera->enemyCamHeightOffset);
		view = camera->GetViewMatrixEnemyFollow(enemy->getPosition(), glm::vec3(0.0f, 1.0f, 0.0f));
	}
	else if (camera->Mode == FLY)
	{
		if (firstFlyCamSwitch)
		{
			camera->FollowTarget(player->getPosition(), player->PlayerFront, camera->playerCamRearOffset, camera->playerCamHeightOffset);
			firstFlyCamSwitch = false;
			return;
		}
		view = camera->GetViewMatrix();
	}
	else if (camera->Mode == PLAYER_AIM)
	{
		camera->Zoom = 40.0f;
		if (camera->Pitch > 16.0f)
			camera->Pitch = 16.0f;

		camera->FollowTarget(player->getPosition() + (player->PlayerFront * camera->playerPosOffset) + (player->PlayerRight * camera->playerAimRightOffset), player->PlayerAimFront, camera->playerCamRearOffset, camera->playerCamHeightOffset);
		glm::vec3 target = player->getPosition() + (player->PlayerFront * camera->playerPosOffset);
		if (target.y < 0.0f)
			target.y = 0.0f;
		view = camera->GetViewMatrixPlayerFollow(target, player->PlayerAimUp);
	}
	cubemapView = glm::mat4(glm::mat3(camera->GetViewMatrixPlayerFollow(player->getPosition(), glm::vec3(0.0f, 1.0f, 0.0f))));

	projection = glm::perspective(glm::radians(camera->Zoom), (float)width / (float)height, 0.1f, 300.0f);

	minimapView = minimapCamera->GetViewMatrix();
	minimapProjection = glm::perspective(glm::radians(camera->Zoom), (float)width / (float)height, 0.1f, 500.0f);

	player->SetCameraMatrices(view, projection);

	audioSystem->SetListener(view);
}

void GameManager::setSceneData()
{
	renderer->setScene(view, projection, cubemapView, dirLight);
}

void GameManager::setUpDebugUI()
{
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();
}

void GameManager::showDebugUI()
{
	ShowLightControlWindow(dirLight);
	ShowCameraControlWindow(*camera);

	ImGui::Begin("Player");

	ImGui::InputFloat3("Position", &player->position[0]);
	ImGui::InputFloat("Yaw", &player->PlayerYaw);
	ImGui::InputFloat3("Player Front", &player->PlayerFront[0]);
	ImGui::InputFloat3("Player Aim Front", &player->PlayerAimFront[0]);
	ImGui::InputFloat("Player Aim Pitch", &player->aimPitch);
	ImGui::InputFloat("Player Rear Offset", &camera->playerCamRearOffset);
	ImGui::InputFloat("Player Height Offset", &camera->playerCamHeightOffset);
	ImGui::InputFloat("Player Pos Offset", &camera->playerPosOffset);
	ImGui::InputFloat("Player Aim Right Offset", &camera->playerAimRightOffset);
	ImGui::End();
	ShowAnimationControlWindow();

	ShowPerformanceWindow();
	ShowEnemyStateWindow();
}

void GameManager::renderDebugUI()
{
	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void GameManager::ShowLightControlWindow(DirLight& light)
{
	ImGui::Begin("Map Settings");

	ImGui::Text("Position");
	ImGui::DragFloat3("Position", (float*)&mapPos, mapPos.x, mapPos.y, mapPos.z);
	ground->SetPosition(mapPos);

	ImGui::Text("Scale");
	ImGui::DragFloat3("Scale", (float*)&mapScale, mapScale.x, mapScale.y, mapScale.z);
	ground->SetScale(mapScale);
	

	ImGui::End();


	ImGui::Begin("Directional Light Control");

	ImGui::Text("Light Direction");
	ImGui::DragFloat3("Direction", (float*)&light.direction, dirLight.direction.x, dirLight.direction.y, dirLight.direction.z);

	ImGui::ColorEdit4("Ambient", (float*)&light.ambient);

	ImGui::ColorEdit4("Diffuse", (float*)&light.diffuse);
	ImGui::ColorEdit4("PBR Color", (float*)&dirLightPBRColour);

	ImGui::ColorEdit4("Specular", (float*)&light.specular);

	ImGui::DragFloat("Ortho Left", (float*)&orthoLeft);
	ImGui::DragFloat("Ortho Right", (float*)&orthoRight);
	ImGui::DragFloat("Ortho Bottom", (float*)&orthoBottom);
	ImGui::DragFloat("Ortho Top", (float*)&orthoTop);
	ImGui::DragFloat("Near Plane", (float*)&near_plane);
	ImGui::DragFloat("Far Plane", (float*)&far_plane);

	ImGui::End();
}

void GameManager::ShowAnimationControlWindow()
{
	ImGui::Begin("Player Animation Control");

	ImGui::Checkbox("Blending Type: ", &playerCrossBlend);
	ImGui::SameLine();
	if (playerCrossBlend)
	{
		ImGui::Text("Cross");
	}
	else
	{
		ImGui::Text("Single");
	}

	if (playerCrossBlend)
		ImGui::BeginDisabled();

	ImGui::Text("Player Blend Factor");
	ImGui::SameLine();
	ImGui::SliderFloat("##PlayerBlendFactor", &playerAnimBlendFactor, 0.0f, 1.0f);

	if (playerCrossBlend)
		ImGui::EndDisabled();

	if (!playerCrossBlend)
		ImGui::BeginDisabled();

	ImGui::Text("Source Clip   ");
	ImGui::SameLine();
	ImGui::SliderInt("##SourceClip", &playerCrossBlendSourceClip, 0, player->model->getAnimClipsSize() - 1);


	ImGui::Text("Dest Clip   ");
	ImGui::SameLine();
	ImGui::SliderInt("##DestClip", &playerCrossBlendDestClip, 0, player->model->getAnimClipsSize() - 1);

	ImGui::Text("Cross Blend ");
	ImGui::SameLine();
	ImGui::SliderFloat("##CrossBlendFactor", &playerAnimCrossBlendFactor, 0.0f, 1.0f);

	ImGui::Checkbox("Additive Blending", &playerAdditiveBlend);

	if (!playerAdditiveBlend) {
		ImGui::BeginDisabled();
	}
	ImGui::Text("Split Node  ");
	ImGui::SameLine();
	ImGui::SliderInt("##SplitNode", &playerSkeletonSplitNode, 0, player->model->getNodeCount() - 1);
	ImGui::Text("Split Node Name: %s", playerSkeletonSplitNodeName.c_str());

	if (!playerAdditiveBlend) {
		ImGui::EndDisabled();
	}

	if (!playerCrossBlend)
		ImGui::EndDisabled();

	ImGui::End();

}

void GameManager::ShowPerformanceWindow()
{
	ImGui::Begin("Performance");

	ImGui::Text("FPS: %.1f", fps);
	ImGui::Text("Avg FPS: %.1f", avgFPS);
	ImGui::Text("Frame Time: %.1f ms", frameTime);
	ImGui::Text("Elapsed Time: %.1f s", elapsedTime);

	ImGui::End();
}

void GameManager::ShowEnemyStateWindow()
{
	ImGui::Begin("Game States");

	ImGui::Checkbox("Use EDBT", &useEDBT);

	ImGui::InputFloat("Speed Divider", &speedDivider);
	ImGui::InputFloat("Blend Factor", &blendFac);

	ImGui::Text("Player Velocity %s", std::to_string(player->GetVelocity()));

	ImGui::Text("Player Health: %d", (int)player->GetHealth());

	for (Enemy* e : enemies)
	{
		if (e == nullptr || e->isDestroyed)
			continue;
		ImTextureID texID = (void*)(intptr_t)e->mTex.getTexID();
		ImGui::Image(texID, ImVec2(100, 100));
		ImGui::SameLine();
		ImGui::Text("Enemy %d", e->GetID());
		ImGui::SameLine();
		ImGui::Text("State: %s", e->GetEDBTState().c_str());
		ImGui::SameLine();
		ImGui::Text("Health %d", (int)e->GetHealth());
		//ImGui::InputFloat3("Position", &e->position[0]);
	}

	ImGui::End();
}

void GameManager::calculatePerformance(float deltaTime)
{
	fps = 1.0f / deltaTime;

	fpsSum += fps;
	frameCount++;

	if (frameCount == numFramesAvg)
	{
		avgFPS = fpsSum / numFramesAvg;
		fpsSum = 0.0f;
		frameCount = 0;
	}

	frameTime = deltaTime * 1000.0f;

	elapsedTime += deltaTime;
}

void GameManager::SetUpAndRenderNavMesh()
{

}

void GameManager::CreateLightSpaceMatrices()
{
	float gridWidth = gameGrid->GetCellSize() * gameGrid->GetGridSize();
	glm::vec3 sceneCenter = glm::vec3(gridWidth / 2.0f, 0.0f, gridWidth / 2.0f);

	glm::vec3 lightDir = glm::normalize(dirLight.direction);

	float sceneDiagonal = glm::sqrt(gridWidth * gridWidth + gridWidth * gridWidth);

	//orthoLeft = -gridWidth * 2.0f;
	//orthoRight = gridWidth * 2.0f;
	//orthoBottom = -gridWidth * 2.0f;
	//orthoTop = gridWidth * 2.0f;
//	near_plane = 1.0f;
//	far_plane = 150.0f;
	lightSpaceProjection = glm::ortho(orthoLeft, orthoRight, orthoBottom, orthoTop, near_plane, far_plane);

	glm::vec3 lightPos = sceneCenter - lightDir * sceneDiagonal;
	//lightPos.y += 50.0f;

	lightSpaceView = glm::lookAt(lightPos, sceneCenter, glm::vec3(0.0f, -1.0f, 0.0f));
	lightSpaceMatrix = lightSpaceProjection * lightSpaceView;
}

void GameManager::RemoveDestroyedGameObjects()
{
	//for (auto it = gameObjects.begin(); it != gameObjects.end(); ) {
	//    if ((*it)->isDestroyed) {
	//        if ((*it)->isEnemy)
	//        {
				//for (auto it2 = enemies.begin(); it2 != enemies.end(); )
				//{
				//	if ((*it2) == (*it))
				//	{
				//		delete* it2;
				//		*it2 = nullptr;
				//		it2 = enemies.erase(it2);
				//	}
				//	else
				//	{
				//		++it2;
				//	}
				//}
	//        }
	//        else
	//        {
	//            delete* it; 
	//            *it = nullptr;
	//        }
	//        it = gameObjects.erase(it); 
	//    }
	//    else {
	//        ++it;
	//    }
	//}

	if ((enemy->isDestroyed && enemy2->isDestroyed && enemy3->isDestroyed && enemy4->isDestroyed) || player->isDestroyed)
		ResetGame();
}

void GameManager::ResetGame()
{
	camera->SetMode(PLAYER_FOLLOW);
	mAudioManager->ClearQueue();
	player->setPosition(player->initialPos);
	player->SetYaw(player->GetInitialYaw());
	player->SetAnimNum(0);
	player->isDestroyed = false;
	player->SetHealth(100.0f);
	player->UpdatePlayerVectors();
	player->UpdatePlayerAimVectors();
	player->SetPlayerState(PlayerState::MOVING);
	player->aabbColor = glm::vec3(0.0f, 0.0f, 1.0f);
	enemy->isDestroyed = false;
	enemy2->isDestroyed = false;
	enemy3->isDestroyed = false;
	enemy4->isDestroyed = false;
	enemy->isDead_ = false;
	enemy2->isDead_ = false;
	enemy3->isDead_ = false;
	enemy4->isDead_ = false;
	enemy->setPosition(enemy->getInitialPosition());
	enemy2->setPosition(enemy2->getInitialPosition());
	enemy3->setPosition(enemy3->getInitialPosition());
	enemy4->setPosition(enemy4->getInitialPosition());
	enemyStates = {
		{ false, false, 100.0f, 100.0f, false },
		{ false, false, 100.0f, 100.0f, false },
		{ false, false, 100.0f, 100.0f, false },
		{ false, false, 100.0f, 100.0f, false }
	};
	//enemies.push_back(enemy);
	//enemies.push_back(enemy2);
	//enemies.push_back(enemy3);
	//enemies.push_back(enemy4);
	//gameObjects.push_back(enemy);
	//gameObjects.push_back(enemy2);
	//gameObjects.push_back(enemy3);
	//gameObjects.push_back(enemy4);

	for (Enemy* emy : enemies)
	{
		emy->ResetState();
		physicsWorld->addCollider(emy->GetAABB());
		physicsWorld->addEnemyCollider(emy->GetAABB());
		emy->SetHealth(100.0f);
	}

}

void GameManager::ShowCameraControlWindow(Camera& cam)
{
	ImGui::Begin("Camera Control");

	std::string modeText = "";

	if (cam.Mode == FLY)
	{
		modeText = "Flycam";


		cam.UpdateCameraVectors();
	}
	else if (cam.Mode == PLAYER_FOLLOW)
		modeText = "Player Follow";
	else if (cam.Mode == ENEMY_FOLLOW)
		modeText = "Enemy Follow";
	else if (cam.Mode == PLAYER_AIM)
		modeText = "Player Aim"
		;
	ImGui::Text(modeText.c_str());

	ImGui::InputFloat3("Position", (float*)&cam.Position);

	ImGui::InputFloat("Pitch", (float*)&cam.Pitch);
	ImGui::InputFloat("Yaw", (float*)&cam.Yaw);
	ImGui::InputFloat("Zoom", (float*)&cam.Zoom);

	ImGui::End();
}

void GameManager::update(float deltaTime)
{
	inputManager->processInput(window->getWindow(), deltaTime);
	player->UpdatePlayerVectors();
	player->UpdatePlayerAimVectors();

	player->Update(deltaTime);

	int enemyID = 0;
	for (Enemy* e : enemies)
	{
		if (e == nullptr || e->isDestroyed)
			continue;

		e->SetDeltaTime(deltaTime);

		if (!useEDBT)
		{
			if (training)
			{
				e->EnemyDecision(enemyStates[e->GetID()], e->GetID(), squadActions, deltaTime, mEnemyStateQTable);
			}
			else
			{
				e->EnemyDecisionPrecomputedQ(enemyStates[e->GetID()], e->GetID(), squadActions, deltaTime, mEnemyStateQTable);
			}
		}
//		e->Update(useEDBT, speedDivider, blendFac);

		float targetPos[3] = { player->getPosition().x, player->getPosition().y, player->getPosition().z };

	/*	targetPos[0] = 0.0f;
		targetPos[1] = 0.0f;
		targetPos[2] = .0f;*/

		dtPolyRef playerPoly;		
		float targetPlayerPosOnNavMesh[3];
		filter.setIncludeFlags(0xFFFF); // Include all polygons for testing
		filter.setExcludeFlags(0);      // Exclude no polygons

		Logger::log(1, "Target position on nav mesh before query: %f, %f, %f\n", targetPos[0], targetPos[1], targetPos[2]);
		navMeshQuery->findNearestPoly(targetPos, halfExtents, &filter, &playerPoly, targetPlayerPosOnNavMesh);
		Logger::log(1, "Player position: %f %f %f\n", player->getPosition().x, player->getPosition().y, player->getPosition().z);
		Logger::log(1, "Target position on nav mesh after query: %f, %f, %f\n", targetPlayerPosOnNavMesh[0], targetPlayerPosOnNavMesh[1], targetPlayerPosOnNavMesh[2]);

		std::vector<float* [3]> enemPos;
		
		float enemyPosition[3] = { e->getPosition().x, e->getPosition().y, e->getPosition().z };

		//DebugNavmeshConnectivity(navMeshQuery, navMesh, filter, targetPos, enemyPosition);


		dtPolyRef targetPoly;
		float targetPosOnNavMesh[3];
		filter.setIncludeFlags(0xFFFF); // Include all polygons for testing
		filter.setExcludeFlags(0);      // Exclude no polygons

		if (!navMeshQuery)
		{
			Logger::log(1, "%s error: NavMeshQuery is null\n", __FUNCTION__);
		}
		dtStatus status;
			if (navMeshQuery)
				status = navMeshQuery->findNearestPoly(targetPos, halfExtents, &filter, &playerPoly, targetPlayerPosOnNavMesh);

		Logger::log(1, "Player position: %f %f %f\n", player->getPosition().x, player->getPosition().y, player->getPosition().z);
		Logger::log(1, "Target position on nav mesh after query: %f, %f, %f\n", targetPlayerPosOnNavMesh[0], targetPlayerPosOnNavMesh[1], targetPlayerPosOnNavMesh[2]);

		if (dtStatusFailed(status))
		{
			Logger::log(1, "%s error: Could not find nearest poly enemy %d\n", __FUNCTION__, e->GetID());
			Logger::log(1, "findNearestPoly failed: %u\n", status);
		}
		else
		{
			Logger::log(1, "%s: Found nearest poly enemy %d\n", __FUNCTION__, e->GetID());
			Logger::log(1, "findNearestPoly succeeded: PolyRef = %u, Pos = %f %f %f\n", playerPoly, targetPlayerPosOnNavMesh[0], targetPlayerPosOnNavMesh[1], targetPlayerPosOnNavMesh[2]);
		}

		status = crowd->requestMoveTarget(e->GetID(), playerPoly, targetPlayerPosOnNavMesh);

		if (dtStatusFailed(status))
		{
			Logger::log(1, "%s error: Could not set move target for enemy %d\n", __FUNCTION__, e->GetID());
		}
		else
		{
			Logger::log(1, "%s: Move target set for enemy %d %f, %f, %f\n", __FUNCTION__, e->GetID(), targetPlayerPosOnNavMesh[0], targetPlayerPosOnNavMesh[1], targetPlayerPosOnNavMesh[2]);
		}


	}
	crowd->update(deltaTime, nullptr);


	for (Enemy* e : enemies)
	{

		const dtCrowdAgent* agent = crowd->getAgent(e->GetID());
		float agentPos[3];
		dtVcopy(agentPos, agent->npos);
		Logger::log(1, "%s: Agent %d position: %f %f %f\n", __FUNCTION__, e->GetID(), agentPos[0], agentPos[1], agentPos[2]);
		Logger::log(1, "%s: Crowd Agent %d position: %f %f %f\n", __FUNCTION__, e->GetID(), agent->npos[0], agent->npos[1], agent->npos[2]);
		e->Update(false, speedDivider, blendFac);
		e->setPosition(glm::vec3(agentPos[0], agentPos[1], agentPos[2]));
	}

	mAudioManager->Update(deltaTime);
	audioSystem->Update(deltaTime);

	calculatePerformance(deltaTime);
}

void GameManager::render(bool isMinimapRenderPass, bool isShadowMapRenderPass, bool isMainRenderPass)
{
	glEnable(GL_DEPTH_TEST);
	glDisable(GL_BLEND);
	//glEnable(GL_CULL_FACE);
	if (!isShadowMapRenderPass)
	{
		renderer->ResetViewport(screenWidth, screenHeight);
		renderer->clear();
	}

	if (isMinimapRenderPass)
		renderer->bindMinimapFBO(screenWidth, screenHeight);

	if (isShadowMapRenderPass)
		renderer->bindShadowMapFBO(SHADOW_WIDTH, SHADOW_HEIGHT);




	for (auto obj : gameObjects) {
		if (obj->isDestroyed)
			continue;
		if (isMinimapRenderPass)
		{
			renderer->draw(obj, minimapView, minimapProjection, camera->Position, false, lightSpaceMatrix);
		}
		else if (isShadowMapRenderPass)
		{
			renderer->draw(obj, lightSpaceView, lightSpaceProjection, camera->Position, true, lightSpaceMatrix);
		}
		else
		{
			renderer->draw(obj, view, projection, camera->Position, false, lightSpaceMatrix);
		}
	}

	if (isShadowMapRenderPass)
	{
		shadowMapShader.use();
		shadowMapShader.setVec3("dirLight.direction", dirLight.direction);
		shadowMapShader.setVec3("dirLight.ambient", dirLight.ambient);
		shadowMapShader.setVec3("dirLight.diffuse", dirLight.diffuse);
		shadowMapShader.setVec3("dirLight.specular", dirLight.specular);
	}
	else
	{
		gridShader.use();
		gridShader.setVec3("dirLight.direction", dirLight.direction);
		gridShader.setVec3("dirLight.ambient", dirLight.ambient);
		gridShader.setVec3("dirLight.diffuse", dirLight.diffuse);
		gridShader.setVec3("dirLight.specular", dirLight.specular);

	}

	if (isMinimapRenderPass)
	{
		//gameGrid->drawGrid(gridShader, minimapView, minimapProjection, camera->Position, false, lightSpaceMatrix, renderer->GetShadowMapTexture());
	}
	else if (isShadowMapRenderPass)
	{
		//gameGrid->drawGrid(shadowMapShader, lightSpaceView, lightSpaceProjection, camera->Position, true, lightSpaceMatrix, renderer->GetShadowMapTexture());
	}
	else
	{
		//gameGrid->drawGrid(gridShader, view, projection, camera->Position, false, lightSpaceMatrix, renderer->GetShadowMapTexture());
	}

	navMeshShader.use();
	navMeshShader.setMat4("view", view);
	navMeshShader.setMat4("projection", projection);
	glm::mat4 modelMat = glm::mat4(1.0f);
	modelMat = glm::translate(modelMat, mapPos);
	modelMat = glm::rotate(modelMat, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
	modelMat = glm::scale(modelMat, mapScale);
	navMeshShader.setMat4("model", modelMat);
	//glBindVertexArray(vao);
	//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	//glCullFace(GL_FRONT);
	//glDrawElements(GL_TRIANGLES, navMeshIndices.size(), GL_UNSIGNED_INT, 0);

	//glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	//glBindVertexArray(vao);
	//glDisable(GL_CULL_FACE);

	//glEnable(GL_CULL_FACE);
	//glCullFace(GL_BACK);
	// Draw navmesh
	/*glEnable(GL_DEPTH_TEST);
	glDisable(GL_BLEND);*/

	glDisable(GL_CULL_FACE);
	//glEnable(GL_CULL_FACE);
	//glCullFace(GL_BACK);
	glBindVertexArray(vao);
	glDisable(GL_CULL_FACE);
	//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	glDrawElements(GL_TRIANGLES, navRenderMeshIndices.size(), GL_UNSIGNED_INT, 0);
	//glDrawArrays(GL_TRIANGLES, 0, navMeshVertices.size() / 3);
//	glDrawElements(GL_TRIANGLES, navMesh.size(), GL_UNSIGNED_INT, 0);
	//glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	//glDisable(GL_POLYGON_OFFSET_FILL);
	glBindVertexArray(0);
	glDisable(GL_CULL_FACE);


	hfnavMeshShader.use();
	hfnavMeshShader.setMat4("view", view);
	hfnavMeshShader.setMat4("projection", projection);

	glDisable(GL_CULL_FACE);
	//glEnable(GL_CULL_FACE);
	//glCullFace(GL_BACK);
	glBindVertexArray(hfvao);
	glDisable(GL_CULL_FACE);
	//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	glDrawElements(GL_TRIANGLES, hfnavRenderMeshIndices.size(), GL_UNSIGNED_INT, 0);
	//glDrawArrays(GL_TRIANGLES, 0, navMeshVertices.size() / 3);
//	glDrawElements(GL_TRIANGLES, navMesh.size(), GL_UNSIGNED_INT, 0);
	//glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	//glDisable(GL_POLYGON_OFFSET_FILL);
	glBindVertexArray(0);
	glDisable(GL_CULL_FACE);


	if (camSwitchedToAim)
		camSwitchedToAim = false;

	if (!enemy->isDestroyed)
	{
		glm::vec3 enemyRayEnd = glm::vec3(0.0f);

		if (enemy->GetEnemyHasShot())
		{
			float enemyMuzzleCurrentTime = glfwGetTime();

			if (renderEnemyMuzzleFlash && enemyMuzzleFlashStartTime + enemyMuzzleFlashDuration > enemyMuzzleCurrentTime)
			{
				renderEnemyMuzzleFlash = false;
			}
			else
			{
				renderEnemyMuzzleFlash = true;
				enemyMuzzleFlashStartTime = enemyMuzzleCurrentTime;
			}

			if (renderEnemyMuzzleFlash && isMainRenderPass)
			{
				enemyMuzzleTimeSinceStart = enemyMuzzleCurrentTime - enemyMuzzleFlashStartTime;
				enemyMuzzleAlpha = glm::max(0.0f, 1.0f - (enemyMuzzleTimeSinceStart / enemyMuzzleFlashDuration));
				enemyMuzzleFlashScale = 1.0f + (0.5f * enemyMuzzleAlpha);

				enemyMuzzleModel = glm::mat4(1.0f);

				enemyMuzzleModel = glm::translate(enemyMuzzleModel, enemy->GetEnemyShootPos());
				enemyMuzzleModel = glm::rotate(enemyMuzzleModel, enemy->yaw, glm::vec3(0.0f, 1.0f, 0.0f));
				enemyMuzzleModel = glm::scale(enemyMuzzleModel, glm::vec3(enemyMuzzleFlashScale, enemyMuzzleFlashScale, 1.0f));
				enemyMuzzleFlashQuad->Draw3D(enemyMuzzleTint, enemyMuzzleAlpha, projection, view, enemyMuzzleModel);
			}
		}

		if (enemy->GetEnemyHasShot() && enemy->GetEnemyDebugRayRenderTimer() > 0.0f)
		{
			glm::vec3 enemyLineColor = glm::vec3(0.2f, 0.2f, 0.2f);

			if (enemy->GetEnemyHasHit())
			{
				enemyRayEnd = enemy->GetEnemyHitPoint();
			}
			else
			{
				enemyRayEnd = enemy->GetEnemyShootPos() + enemy->GetEnemyShootDir() * enemy->GetEnemyShootDistance();
			}

			glm::vec3 enemyRayEnd = enemy->GetEnemyShootPos() + enemy->GetEnemyShootDir() * enemy->GetEnemyShootDistance();
			enemyLine->UpdateVertexBuffer(enemy->GetEnemyShootPos(), enemyRayEnd);
			if (isMinimapRenderPass)
			{
				enemyLine->DrawLine(minimapView, minimapProjection, enemyLineColor, lightSpaceMatrix, renderer->GetShadowMapTexture(), false, enemy->GetEnemyDebugRayRenderTimer());
			}
			else if (isShadowMapRenderPass)
			{
				enemyLine->DrawLine(lightSpaceView, lightSpaceProjection, enemyLineColor, lightSpaceMatrix, renderer->GetShadowMapTexture(), true, enemy->GetEnemyDebugRayRenderTimer());
			}
			else
			{
				enemyLine->DrawLine(view, projection, enemyLineColor, lightSpaceMatrix, renderer->GetShadowMapTexture(), false, enemy->GetEnemyDebugRayRenderTimer());
			}
		}
	}

	if (!enemy2->isDestroyed)
	{
		glm::vec3 enemy2RayEnd = glm::vec3(0.0f);

		if (enemy2->GetEnemyHasShot())
		{
			float enemy2MuzzleCurrentTime = glfwGetTime();

			if (renderEnemy2MuzzleFlash && enemy2MuzzleFlashStartTime + enemy2MuzzleFlashDuration > enemy2MuzzleCurrentTime)
			{
				renderEnemy2MuzzleFlash = false;
			}
			else
			{
				renderEnemy2MuzzleFlash = true;
				enemy2MuzzleFlashStartTime = enemy2MuzzleCurrentTime;
			}

			if (renderEnemy2MuzzleFlash && isMainRenderPass)
			{
				enemy2MuzzleTimeSinceStart = enemy2MuzzleCurrentTime - enemy2MuzzleFlashStartTime;
				enemy2MuzzleAlpha = glm::max(0.0f, 1.0f - (enemy2MuzzleTimeSinceStart / enemy2MuzzleFlashDuration));
				enemy2MuzzleFlashScale = 1.0f + (0.5f * enemy2MuzzleAlpha);

				enemy2MuzzleModel = glm::mat4(1.0f);

				enemy2MuzzleModel = glm::translate(enemy2MuzzleModel, enemy2->GetEnemyShootPos());
				enemy2MuzzleModel = glm::rotate(enemy2MuzzleModel, enemy2->yaw, glm::vec3(0.0f, 1.0f, 0.0f));
				enemy2MuzzleModel = glm::scale(enemy2MuzzleModel, glm::vec3(enemy2MuzzleFlashScale, enemy2MuzzleFlashScale, 1.0f));
				enemy2MuzzleFlashQuad->Draw3D(enemy2MuzzleTint, enemy2MuzzleAlpha, projection, view, enemy2MuzzleModel);
			}
		}

		if (enemy2->GetEnemyHasShot() && enemy2->GetEnemyDebugRayRenderTimer() > 0.0f)
		{
			glm::vec3 enemy2LineColor = glm::vec3(0.2f, 0.2f, 0.2f);
			if (enemy2->GetEnemyHasHit())
			{
				enemy2RayEnd = enemy2->GetEnemyHitPoint();
			}
			else
			{
				enemy2RayEnd = enemy2->GetEnemyShootPos() + enemy2->GetEnemyShootDir() * enemy2->GetEnemyShootDistance();
			}

			enemy2Line->UpdateVertexBuffer(enemy2->GetEnemyShootPos(), enemy2RayEnd);
			if (isMinimapRenderPass)
			{
				enemy2Line->DrawLine(minimapView, minimapProjection, enemy2LineColor, lightSpaceMatrix, renderer->GetShadowMapTexture(), false, enemy2->GetEnemyDebugRayRenderTimer());
			}
			else if (isShadowMapRenderPass)
			{
				enemy2Line->DrawLine(lightSpaceView, lightSpaceProjection, enemy2LineColor, lightSpaceMatrix, renderer->GetShadowMapTexture(), true, enemy2->GetEnemyDebugRayRenderTimer());
			}
			else
			{
				enemy2Line->DrawLine(view, projection, enemy2LineColor, lightSpaceMatrix, renderer->GetShadowMapTexture(), false, enemy2->GetEnemyDebugRayRenderTimer());
			}
		}
	}

	if (!enemy3->isDestroyed)
	{
		glm::vec3 enemy3RayEnd = glm::vec3(0.0f);

		if (enemy3->GetEnemyHasShot())
		{
			float enemy3MuzzleCurrentTime = glfwGetTime();

			if (renderEnemy3MuzzleFlash && enemy3MuzzleFlashStartTime + enemy3MuzzleFlashDuration > enemy3MuzzleCurrentTime)
			{
				renderEnemy3MuzzleFlash = false;
			}
			else
			{
				renderEnemy3MuzzleFlash = true;
				enemy3MuzzleFlashStartTime = enemy3MuzzleCurrentTime;
			}

			if (renderEnemy3MuzzleFlash && isMainRenderPass)
			{
				enemy3MuzzleTimeSinceStart = enemy3MuzzleCurrentTime - enemy3MuzzleFlashStartTime;
				enemy3MuzzleAlpha = glm::max(0.0f, 1.0f - (enemy3MuzzleTimeSinceStart / enemy3MuzzleFlashDuration));
				enemy3MuzzleFlashScale = 1.0f + (0.5f * enemy3MuzzleAlpha);

				enemy3MuzzleModel = glm::mat4(1.0f);

				enemy3MuzzleModel = glm::translate(enemy3MuzzleModel, enemy3->GetEnemyShootPos());
				enemy3MuzzleModel = glm::rotate(enemy3MuzzleModel, enemy3->yaw, glm::vec3(0.0f, 1.0f, 0.0f));
				enemy3MuzzleModel = glm::scale(enemy3MuzzleModel, glm::vec3(enemy3MuzzleFlashScale, enemy3MuzzleFlashScale, 1.0f));
				enemy3MuzzleFlashQuad->Draw3D(enemy3MuzzleTint, enemy3MuzzleAlpha, projection, view, enemy3MuzzleModel);
			}
		}

		if (enemy3->GetEnemyHasShot() && enemy3->GetEnemyDebugRayRenderTimer() > 0.0f)
		{
			glm::vec3 enemy3LineColor = glm::vec3(0.2f, 0.2f, 0.2f);

			if (enemy3->GetEnemyHasHit())
			{
				enemy3RayEnd = enemy3->GetEnemyHitPoint();
			}
			else
			{
				enemy3RayEnd = enemy3->GetEnemyShootPos() + enemy3->GetEnemyShootDir() * enemy3->GetEnemyShootDistance();
			}

			enemy3Line->UpdateVertexBuffer(enemy3->GetEnemyShootPos(), enemy3RayEnd);
			if (isMinimapRenderPass)
			{
				enemy3Line->DrawLine(minimapView, minimapProjection, enemy3LineColor, lightSpaceMatrix, renderer->GetShadowMapTexture(), false, enemy3->GetEnemyDebugRayRenderTimer());
			}
			else if (isShadowMapRenderPass)
			{
				enemy3Line->DrawLine(lightSpaceView, lightSpaceProjection, enemy3LineColor, lightSpaceMatrix, renderer->GetShadowMapTexture(), true, enemy3->GetEnemyDebugRayRenderTimer());
			}
			else
			{
				enemy3Line->DrawLine(view, projection, enemy3LineColor, lightSpaceMatrix, renderer->GetShadowMapTexture(), false, enemy3->GetEnemyDebugRayRenderTimer());
			}
		}
	}

	if (!enemy4->isDestroyed)
	{
		glm::vec3 enemy4RayEnd = glm::vec3(0.0f);

		if (enemy4->GetEnemyHasShot())
		{
			float enemy4MuzzleCurrentTime = glfwGetTime();

			if (renderEnemy4MuzzleFlash && enemy4MuzzleFlashStartTime + enemy4MuzzleFlashDuration > enemy4MuzzleCurrentTime)
			{
				renderEnemy4MuzzleFlash = false;
			}
			else
			{
				renderEnemy4MuzzleFlash = true;
				enemy4MuzzleFlashStartTime = enemy4MuzzleCurrentTime;
			}

			if (renderEnemy4MuzzleFlash && isMainRenderPass)
			{
				enemy4MuzzleTimeSinceStart = enemy4MuzzleCurrentTime - enemy4MuzzleFlashStartTime;
				enemy4MuzzleAlpha = glm::max(0.0f, 1.0f - (enemy4MuzzleTimeSinceStart / enemy4MuzzleFlashDuration));
				enemy4MuzzleFlashScale = 1.0f + (0.5f * enemy4MuzzleAlpha);

				enemy4MuzzleModel = glm::mat4(1.0f);

				enemy4MuzzleModel = glm::translate(enemy4MuzzleModel, enemy4->GetEnemyShootPos());
				enemy4MuzzleModel = glm::rotate(enemy4MuzzleModel, enemy4->yaw, glm::vec3(0.0f, 1.0f, 0.0f));
				enemy4MuzzleModel = glm::scale(enemy4MuzzleModel, glm::vec3(enemy4MuzzleFlashScale, enemy4MuzzleFlashScale, 1.0f));
				enemy4MuzzleFlashQuad->Draw3D(enemy4MuzzleTint, enemy4MuzzleAlpha, projection, view, enemy4MuzzleModel);
			}
		}

		if (enemy4->GetEnemyHasShot() && enemy4->GetEnemyDebugRayRenderTimer() > 0.0f)
		{
			glm::vec3 enemy4LineColor = glm::vec3(0.2f, 0.2f, 0.2f);
			if (enemy3->GetEnemyHasHit())
			{
				enemy4RayEnd = enemy4->GetEnemyHitPoint();
			}
			else
			{
				enemy4RayEnd = enemy4->GetEnemyShootPos() + enemy4->GetEnemyShootDir() * enemy4->GetEnemyShootDistance();
			}

			enemy4Line->UpdateVertexBuffer(enemy4->GetEnemyShootPos(), enemy4RayEnd);
			if (isMinimapRenderPass)
			{
				enemy4Line->DrawLine(minimapView, minimapProjection, enemy4LineColor, lightSpaceMatrix, renderer->GetShadowMapTexture(), false, enemy4->GetEnemyDebugRayRenderTimer());
			}
			else if (isShadowMapRenderPass)
			{
				enemy4Line->DrawLine(lightSpaceView, lightSpaceProjection, enemy4LineColor, lightSpaceMatrix, renderer->GetShadowMapTexture(), true, enemy4->GetEnemyDebugRayRenderTimer());
			}
			else
			{
				enemy4Line->DrawLine(view, projection, enemy4LineColor, lightSpaceMatrix, renderer->GetShadowMapTexture(), false, enemy4->GetEnemyDebugRayRenderTimer());
			}
		}
	}
	//	renderer->setScene(view, projection, dirLight);
	renderer->drawCubemap(cubemap);
	if ((player->GetPlayerState() == AIMING || player->GetPlayerState() == SHOOTING) && camSwitchedToAim == false && isMainRenderPass)
	{
		glDisable(GL_DEPTH_TEST);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		glm::vec3 rayO = player->GetShootPos();
		glm::vec3 rayD = glm::normalize(player->PlayerAimFront);
		float dist = player->GetShootDistance();

		glm::vec3 rayEnd = rayO + rayD * dist;

		glm::vec3 lineColor = glm::vec3(1.0f, 0.0f, 0.0f);

		glm::vec3 hitPoint;

		if (player->GetPlayerState() == SHOOTING)
		{
			lineColor = glm::vec3(0.0f, 1.0f, 0.0f);

			float currentTime = glfwGetTime();

			if (renderPlayerMuzzleFlash && playerMuzzleFlashStartTime + playerMuzzleFlashDuration > currentTime)
			{
				renderPlayerMuzzleFlash = false;
			}
			else
			{
				renderPlayerMuzzleFlash = true;
				playerMuzzleFlashStartTime = currentTime;
			}


			if (renderPlayerMuzzleFlash)

			{
				playerMuzzleTimeSinceStart = currentTime - playerMuzzleFlashStartTime;
				playerMuzzleAlpha = glm::max(0.0f, 1.0f - (playerMuzzleTimeSinceStart / playerMuzzleFlashDuration));
				playerMuzzleFlashScale = 1.0f + (0.5f * playerMuzzleAlpha);

				playerMuzzleModel = glm::mat4(1.0f);

				playerMuzzleModel = glm::translate(playerMuzzleModel, player->GetShootPos());
				playerMuzzleModel = glm::rotate(playerMuzzleModel, (-player->yaw + 180.0f), glm::vec3(0.0f, 1.0f, 0.0f));
				playerMuzzleModel = glm::scale(playerMuzzleModel, glm::vec3(playerMuzzleFlashScale, playerMuzzleFlashScale, 1.0f));
				playerMuzzleFlashQuad->Draw3D(playerMuzzleTint, playerMuzzleAlpha, projection, view, playerMuzzleModel);
			}
		}


		glm::vec4 rayEndWorldSpace = glm::vec4(rayEnd, 1.0f);
		glm::vec4 rayEndCameraSpace = view * rayEndWorldSpace;
		glm::vec4 rayEndNDC = projection * rayEndCameraSpace;

		glm::vec4 targetNDC(0.0f, 0.5f, rayEndNDC.z / rayEndNDC.w, 1.0f);
		glm::vec4 targetCameraSpace = glm::inverse(projection) * targetNDC;
		glm::vec4 targetWorldSpace = glm::inverse(view) * targetCameraSpace;

		rayEnd = glm::vec3(targetWorldSpace) / targetWorldSpace.w;

		glm::vec3 crosshairHitpoint;
		glm::vec3 crosshairCol;

		if (physicsWorld->rayEnemyCrosshairIntersect(rayO, glm::normalize(rayEnd - rayO), crosshairHitpoint))
		{
			crosshairCol = glm::vec3(1.0f, 0.0f, 0.0f);
		}
		else
		{
			crosshairCol = glm::vec3(1.0f, 1.0f, 1.0f);
		}

		glm::vec2 ndcPos = crosshair->CalculateCrosshairPosition(rayEnd, window->GetWidth(), window->GetHeight(), projection, view);

		float ndcX = (ndcPos.x / window->GetWidth()) * 2.0f - 1.0f;
		float ndcY = (ndcPos.y / window->GetHeight()) * 2.0f - 1.0f;

		if (isMainRenderPass)
			crosshair->DrawCrosshair(glm::vec2(0.0f, 0.5f), crosshairCol);


		//line->UpdateVertexBuffer(rayO, rayEnd);

		//if (minimap)
		//{
		//	line->DrawLine(minimapView, minimapProjection, lineColor, lightSpaceMatrix, renderer->GetShadowMapTexture(), false);
		//}
		//else if (shadowMap)
		//{
		//	line->DrawLine(lightSpaceView, lightSpaceProjection, lineColor, lightSpaceMatrix, renderer->GetShadowMapTexture(), true);
		//}
		//else
		//{
		//	line->DrawLine(view, projection, lineColor, lightSpaceMatrix, renderer->GetShadowMapTexture(), false);
		//}

		glEnable(GL_DEPTH_TEST);
		glDisable(GL_BLEND);

	}

	if (isMainRenderPass)
	{
		renderer->drawMinimap(minimapQuad, &minimapShader);

	}

#ifdef _DEBUG
	if (isMainRenderPass)
	{
		renderer->drawShadowMap(shadowMapQuad, &shadowMapQuadShader);
	}
#endif

	if (isMinimapRenderPass)
	{
		renderer->unbindMinimapFBO();
	}
	else if (isShadowMapRenderPass)
	{
		renderer->unbindShadowMapFBO();
	}


}