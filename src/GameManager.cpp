#include "GameManager.h"
#include "Components/AudioComponent.h"
#include "Game/Prefabs.h"

#include "imgui/imgui.h"
#include "imgui/backend/imgui_impl_glfw.h"
#include "imgui/backend/imgui_impl_opengl3.h"
#include "Engine/Render/RenderBackend.h"
#include "Engine/Render/RenderBackendGL.h"

DirLight dirLight = {
		glm::vec3(-0.5f, -1.0f, -0.3f),

		glm::vec3(1.0f),
		glm::vec3(0.8f),
		glm::vec3(0.8f, 0.9f, 1.0f)
};

const float AGENT_RADIUS = 0.6f;

glm::vec3 dirLightPBRColour = glm::vec3(300.f, 300.0f, 300.0f);

bool GameManager::BuildTile(int tx, int ty, float* bmin, float* bmax, rcConfig cfg, unsigned char*& navData, int* navDataSize, dtNavMeshParams parameters)
{
	rcConfig config = cfg;

	const float tileWorldSize = cfg.tileSize * cfg.cs;

	// Compute tile bounds
	float tileBMin[3] = {
			parameters.orig[0] + tx * tileWorldSize,
			bmin[1],
			parameters.orig[2] + ty * tileWorldSize
	};
	float tileBMax[3] = {
		parameters.orig[0] + (tx + 1) * tileWorldSize,
		bmax[1],
		parameters.orig[2] + (ty + 1) * tileWorldSize
	};

	config.borderSize = config.walkableRadius + 3; // cells
	const float border = config.borderSize * config.cs; // meters

	config.width = config.tileSize + config.borderSize * 2;
	config.height = config.tileSize + config.borderSize * 2;

	// Expand by border size
	float tbmin[3] = { tileBMin[0] - border, tileBMin[1], tileBMin[2] - border };
	float tbmax[3] = { tileBMax[0] + border, tileBMax[1], tileBMax[2] + border };

	//rcContext ctx;

	std::vector<float> tileVerts;
	std::vector<int> tileIndices;
	std::vector<unsigned char> tileAreas;
	std::unordered_map<int, int> globalToLocalVert;

	int triCount = static_cast<int>(navMeshIndices.size()) / 3;

	for (int i = 0; i < triCount; ++i)
	{
		int ia = navMeshIndices[i * 3 + 0];
		int ib = navMeshIndices[i * 3 + 1];
		int ic = navMeshIndices[i * 3 + 2];

		glm::vec3 a = glm::make_vec3(&navMeshVertices[ia * 3]);
		glm::vec3 b = glm::make_vec3(&navMeshVertices[ib * 3]);
		glm::vec3 c = glm::make_vec3(&navMeshVertices[ic * 3]);

		// Compute AABB of triangle
		glm::vec3 triMin = glm::min(a, glm::min(b, c));
		glm::vec3 triMax = glm::max(a, glm::max(b, c));

		// Reject if triangle doesn't intersect tile bounds
		if (triMax.x < tbmin[0] || triMin.x > tbmax[0] ||
			triMax.y < tbmin[1] || triMin.y > tbmax[1] ||
			triMax.z < tbmin[2] || triMin.z > tbmax[2])
			continue;

		// Remap and copy vertices
		auto remap = [&](int globalIndex) -> int {
			auto found = globalToLocalVert.find(globalIndex);
			if (found != globalToLocalVert.end())
				return found->second;

			int localIndex = static_cast<int>(tileVerts.size()) / 3;
			tileVerts.push_back(navMeshVertices[globalIndex * 3 + 0]);
			tileVerts.push_back(navMeshVertices[globalIndex * 3 + 1]);
			tileVerts.push_back(navMeshVertices[globalIndex * 3 + 2]);
			globalToLocalVert[globalIndex] = localIndex;
			return localIndex;
			};

		int localA = remap(ia);
		int localB = remap(ib);
		int localC = remap(ic);

		tileIndices.push_back(localA);
		tileIndices.push_back(localB);
		tileIndices.push_back(localC);

		tileAreas.push_back(RC_WALKABLE_AREA); // Mark triangle as walkable
	}
	rcHeightfield* heightField = rcAllocHeightfield();
	if (!rcCreateHeightfield(&ctx, *heightField, config.width, config.height, tbmin, tbmax, config.cs, config.ch))
	{
		Logger::Log(1, "%s error: Could not create heightfield\n", __FUNCTION__);
		return false;
	}
	else
	{
		Logger::Log(1, "%s: Heightfield successfully created\n", __FUNCTION__);
	}


	int triangleCount = static_cast<int>(navMeshIndices.size()) / 3;

	if (!rcRasterizeTriangles(&ctx, tileVerts.data(), static_cast<int>(tileVerts.size()) / 3, tileIndices.data(), tileAreas.data(), static_cast<int>(tileIndices.size()) / 3, *heightField, config.walkableClimb))
	{
		Logger::Log(1, "%s error: Could not rasterize triangles\n", __FUNCTION__);
	}
	else
	{
		Logger::Log(1, "%s: rasterize triangles successfully created\n", __FUNCTION__);
	};

	rcFilterWalkableLowHeightSpans(&ctx, config.walkableHeight, *heightField);
	rcFilterLedgeSpans(&ctx, cfg.walkableHeight, config.walkableClimb, *heightField);
	rcFilterLowHangingWalkableObstacles(&ctx, config.walkableHeight, *heightField);


	rcCompactHeightfield* chf = rcAllocCompactHeightfield();
	if (!rcBuildCompactHeightfield(&ctx, cfg.walkableHeight, cfg.walkableClimb, *heightField, *chf))
	{
		Logger::Log(1, "%s error: Could not build compact heightfield\n", __FUNCTION__);
	}
	else
	{
		Logger::Log(1, "%s: compact heightfield successfully created\n", __FUNCTION__);
	}


	rcErodeWalkableArea(&ctx, config.walkableRadius, *chf);
	rcBuildDistanceField(&ctx, *chf);
	rcBuildRegions(&ctx, *chf, config.borderSize, config.minRegionArea, config.mergeRegionArea);

	rcContourSet* cset = rcAllocContourSet();
	if (!rcBuildContours(&ctx, *chf, config.maxSimplificationError, config.maxEdgeLen, *cset))
	{
		Logger::Log(1, "%s error: Could not build contours\n", __FUNCTION__);
	}
	else
	{
		Logger::Log(1, "%s: contours successfully created\n", __FUNCTION__);
		Logger::Log(1, "Contours count: %d\n", cset->nconts);
	};

	rcPolyMesh* pmesh = rcAllocPolyMesh();
	if (!rcBuildPolyMesh(&ctx, *cset, cfg.maxVertsPerPoly, *pmesh))
	{
		Logger::Log(1, "%s error: Could not build polymesh\n", __FUNCTION__);
	}
	else
	{
		Logger::Log(1, "%s: polymesh successfully created\n", __FUNCTION__);
	};

	rcPolyMeshDetail* dmesh = rcAllocPolyMeshDetail();
	if (!rcBuildPolyMeshDetail(&ctx, *pmesh, *chf, config.detailSampleDist, config.detailSampleMaxError, *dmesh))
	{
		Logger::Log(1, "%s error: Could not build polymesh detail\n", __FUNCTION__);
	}
	else
	{
		Logger::Log(1, "%s: polymesh detail successfully created\n", __FUNCTION__);
	};

	if (pmesh->npolys == 0 || pmesh == nullptr || dmesh == nullptr || chf == nullptr || cset == nullptr || heightField == nullptr)
	{
		navData = nullptr;
		navDataSize = 0;
		Logger::Log(1, "%s error: One of the mesh components is null\n", __FUNCTION__);
		return true;
	}

	Logger::Log(1, "NavMesh vertices count: %d\n", tileVerts.size());
	Logger::Log(1, "NavMesh indices count: %d\n", tileIndices.size());
	Logger::Log(1, "Heightfield span count: %d\n", heightField->width * heightField->height);
	Logger::Log(1, "Compact heightfield span count: %d\n", chf->spanCount);
	Logger::Log(1, "PolyMesh vertices count: %d\n", pmesh->nverts);
	Logger::Log(1, "PolyMesh polygons count: %d\n", pmesh->npolys);
	Logger::Log(1, "PolyMeshDetail vertices count: %d\n", dmesh->nverts);
	Logger::Log(1, "PolyMesh bounds: Min(%.2f, %.2f, %.2f) Max(%.2f, %.2f, %.2f)\n",
		pmesh->bmin[0], pmesh->bmin[1], pmesh->bmin[2],
		pmesh->bmax[0], pmesh->bmax[1], pmesh->bmax[2]);

	for (int i = 0; i < std::min(10, (int)navMeshVertices.size()); i += 3) {
		Logger::Log(1, "NavMesh Vert %d: %.2f %.2f %.2f\n", i / 3,
			tileVerts[i], tileVerts[i + 1], tileVerts[i + 2]);
	}
	static const unsigned short DT_POLYFLAGS_WALK = 0x01;
	dtNavMeshCreateParams params = {};
	params.verts = pmesh->verts;
	params.vertCount = pmesh->nverts;
	params.polys = pmesh->polys;
	params.polyAreas = pmesh->areas;

	rcVcopy(params.bmin, tileBMin);
	rcVcopy(params.bmax, tileBMax);
	Logger::Log(1, "Tile (%d,%d) bmin: %.2f %.2f %.2f  bmax: %.2f %.2f %.2f\n",
		tx, ty, tbmin[0], tbmin[1], tbmin[2], tbmax[0], tbmax[1], tbmax[2]);

	for (int i = 0; i < pmesh->npolys; ++i)
	{
		if (pmesh->areas[i] == RC_WALKABLE_AREA)
		{
			pmesh->flags[i] = DT_POLYFLAGS_WALK;
		}
		else
		{
			pmesh->flags[i] = 0;
		}
		params.polyAreas = pmesh->areas;
	};	params.polyFlags = pmesh->flags;
	params.polyFlags = pmesh->flags;


	params.polyCount = pmesh->npolys;
	params.nvp = pmesh->nvp;
	params.walkableHeight = config.walkableHeight * config.ch;
	params.walkableRadius = config.walkableRadius * config.cs;
	params.walkableClimb = config.walkableClimb * config.ch;
	params.detailMeshes = dmesh->meshes;
	params.detailVerts = dmesh->verts;
	params.detailVertsCount = dmesh->nverts;
	params.detailTris = dmesh->tris;
	params.detailTriCount = dmesh->ntris;
	params.offMeshConVerts = nullptr;
	params.offMeshConRad = nullptr;
	params.offMeshConDir = nullptr;
	params.offMeshConAreas = nullptr;
	params.offMeshConFlags = nullptr;
	params.offMeshConUserID = nullptr;
	params.offMeshConCount = 0;
	params.cs = config.cs;
	params.ch = config.ch;
	params.buildBvTree = true;

	params.tileX = tx;
	params.tileY = ty;
	params.tileLayer = 0;

	Logger::Log(1, "Params before dtCreateNavMeshData: bmin(%.2f %.2f %.2f) bmax(%.2f %.2f %.2f)\n",
		params.bmin[0], params.bmin[1], params.bmin[2],
		params.bmax[0], params.bmax[1], params.bmax[2]);


	dtStatus status = dtCreateNavMeshData(&params, &navData, navDataSize);

	if (dtStatusFailed(status))
	{
		Logger::Log(1, "Create Nav Mesh Data failed: %u\n", status, __FUNCTION__);
		return false;
	}

	heightFields.push_back(heightField);
	compactHeightFields.push_back(chf);
	contourSets.push_back(cset);
	polyMeshes.push_back(pmesh);
	Logger::Log(1, "PolyMeshes count: %d\n", polyMeshes.size());
	polyMeshDetails.push_back(dmesh);

	return true;
}


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
		const float extents[3] = { 5.0f, 5.0f, 5.0f };

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

	dtPolyRef playerPoly = 0;
	dtStatus playerStatus = navQuery->findNearestPoly(playpos, extents, &filter, &playerPoly, snapped);
	//dtStatus playerStatus = navQuery->findNearestPoly(playerPos, extents, &filter, &playerPoly, nullptr);

	if (!playerPoly)
	{
		Logger::Log(1, "[DEBUG] Player is NOT on navmesh (no nearest poly found)\n");
		return;
	}

	Logger::Log(1, "[DEBUG] Player nearest poly: %llu\n", (unsigned long long)playerPoly);

	// Track islands by walking links in navmesh
	const dtMeshTile* playerTile = nullptr;
	const dtPoly* playerDtPoly = nullptr;
	navMesh->getTileAndPolyByRefUnsafe(playerPoly, &playerTile, &playerDtPoly);
	unsigned short playerRegionId = playerDtPoly->getArea();

	Logger::Log(1, "[DEBUG] Player is in region ID: %d\n", playerRegionId);

	// Now check each enemy
	for (size_t i = 0; i < 4; i++)
	{
		dtPolyRef enemyPoly = 0;
		dtStatus enemyStatus = navQuery->findNearestPoly(enemyPositions, extents, &filter, &enemyPoly, nullptr);

		if (dtStatusFailed(enemyStatus) || !enemyPoly)
		{
			Logger::Log(1, "[DEBUG] Enemy %zu is NOT on navmesh (no nearest poly found)\n", i);
			continue;
		}

		const dtMeshTile* enemyTile = nullptr;
		const dtPoly* enemyDtPoly = nullptr;
		navMesh->getTileAndPolyByRefUnsafe(enemyPoly, &enemyTile, &enemyDtPoly);
		unsigned short enemyRegionId = enemyDtPoly->getArea();

		Logger::Log(1, "[DEBUG] Enemy %zu nearest poly: %llu (region %d)\n", i,
			(unsigned long long)enemyPoly, enemyRegionId);

		if (enemyRegionId != playerRegionId)
		{
			Logger::Log(1, "   Enemy %zu is in a DIFFERENT navmesh region (not connected)\n", i);
		}
		else
		{
			Logger::Log(1, "   Enemy %zu is in the SAME navmesh region as player\n", i);
		}
	}
}


void ProbeNavmesh(dtNavMeshQuery* navQuery, dtQueryFilter* filter,
	float x, float y, float z)
{
	float pos[3] = { x, y, z };
	float halfExtents[3] = { 5.0f, 5.0f, 5.0f };   // Big search area
	dtPolyRef ref;
	float snapped[3];

	ref = FindNearestPolyWithPreferredY(navQuery, *filter, pos, y, halfExtents, *snapped);
	Logger::Log(1, "Probe at (%.2f, %.2f, %.2f): ", x, y, z);

	if (ref == 0)
	{
		Logger::Log(1, "No polygon found.\n");
	}
	else
	{
		Logger::Log(1, "Found polygon %llu at snapped position (%.2f, %.2f, %.2f)\n",
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
	float halfExtents[3] = { 5.0f, 0.5f, 5.0f };   // Big search area

	// Query nearest poly
	dtPolyRef startPoly = 0;
	dtStatus status = navQuery->findNearestPoly(searchPos, halfExtents, filter, &startPoly, outSnappedPos);

	Logger::Log(1, "[CrowdSpawn] Query start(%.2f, %.2f, %.2f)\n",
		searchPos[0], searchPos[1], searchPos[2]);

	if (dtStatusFailed(status))
	{
		Logger::Log(1, "[CrowdSpawn] findNearestPoly FAILED (status=0x%X)\n", status);
		outAgentID = -1;
		return false;
	}

	if (startPoly == 0)
	{
		Logger::Log(1, "[CrowdSpawn] No polygon found near startPos (%.2f, %.2f, %.2f)\n",
			searchPos[0], searchPos[1], searchPos[2]);
		outAgentID = -1;
		return false;
	}

	Logger::Log(1, "[CrowdSpawn] Found poly %llu, snapped to (%.2f, %.2f, %.2f)\n",
		(unsigned long long)startPoly,
		outSnappedPos[0], outSnappedPos[1], outSnappedPos[2]);

	// Add agent at snapped position
	outAgentID = crowd->addAgent(outSnappedPos, params);
	if (outAgentID == -1)
	{
		Logger::Log(1, "[CrowdSpawn] Crowd is full or agent failed to add!\n");
		return false;
	}

	// Check agent state after adding
	const dtCrowdAgent* ag = crowd->getAgent(outAgentID);
	if (!ag)
	{
		Logger::Log(1, "[CrowdSpawn] Agent pointer null after addAgent.\n");
		return false;
	}

	Logger::Log(1, "[CrowdSpawn] Agent %d added: state=%d (0=INVALID, 1=WALKING, 2=OFFMESH, 3=IDLE)\n",
		outAgentID, ag->state);

	return (ag->state != DT_CROWDAGENT_STATE_INVALID);
}


GameManager::GameManager(Window* window, unsigned int width, unsigned int height)
	: m_window(window), m_screenWidth(width), m_screenHeight(height)
{
	m_inputManager = new InputManager();
	m_audioSystem = new AudioSystem(this);

	if (!m_audioSystem->Initialize())
	{
		Logger::Log(1, "%s error: AudioSystem init error\n", __FUNCTION__);
		m_audioSystem->Shutdown();
		delete m_audioSystem;
		m_audioSystem = nullptr;
	}

	m_audioManager = new AudioManager(this);

	window->SetInputManager(m_inputManager);

	m_renderer = window->GetRenderer();
	m_renderer->SetUpMinimapFBO(width, height);
	m_renderer->SetUpShadowMapFBO(SHADOW_WIDTH, SHADOW_HEIGHT);

	m_activeScene = new Scene();

	playerShader.LoadShaders("src/Shaders/vertex_pbr_skinned.glsl", "src/Shaders/fragment_pbr_skinned.glsl");
	groundShader.LoadShaders("src/Shaders/vertex2.glsl", "src/Shaders/fragment2.glsl");
	enemyShader.LoadShaders("C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Shaders/vertex_gpu_dquat_enemy.glsl", "C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Shaders/pbr_fragment_emissive.glsl");
	gridShader.LoadShaders("C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Shaders/pbr_vertex.glsl", "C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Shaders/pbr_fragment.glsl");
	crosshairShader.LoadShaders("C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Shaders/crosshair_vert.glsl", "C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Shaders/crosshair_frag.glsl");
	//lineShader.LoadShaders("src/Shaders/line_vert.glsl", "src/Shaders/line_frag.glsl");
	aabbShader.LoadShaders("src/Shaders/aabb_vert.glsl", "src/Shaders/aabb_frag.glsl");
	cubeShader.LoadShaders("C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Shaders/pbr_vertex.glsl", "C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Shaders/pbr_fragment_emissive.glsl");
	cubemapShader.LoadShaders("C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Shaders/cubemap_vertex.glsl", "C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Shaders/cubemap_fragment.glsl");
	minimapShader.LoadShaders("C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Shaders/quad_vertex.glsl", "C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Shaders/quad_fragment.glsl");
	shadowMapShader.LoadShaders("C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Shaders/shadow_map_vertex.glsl", "C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Shaders/shadow_map_fragment.glsl");
	playerShadowMapShader.LoadShaders("C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Shaders/shadow_map_player_vertex.glsl", "C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Shaders/shadow_map_fragment.glsl");
	groundShadowShader.LoadShaders("C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Shaders/shadow_map_vertex.glsl", "C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Shaders/shadow_map_fragment.glsl");
	enemyShadowMapShader.LoadShaders("C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Shaders/shadow_map_enemy_vertex.glsl", "C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Shaders/shadow_map_fragment.glsl");
	shadowMapQuadShader.LoadShaders("C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Shaders/shadow_map_quad_vertex.glsl", "C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Shaders/shadow_map_quad_fragment.glsl");
	playerMuzzleFlashShader.LoadShaders("C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Shaders/muzzle_flash_vertex.glsl", "C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Shaders/muzzle_flash_fragment.glsl");
	navMeshShader.LoadShaders("C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Shaders/navmesh_vert.glsl", "C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Shaders/navmesh_frag.glsl");
//	hfnavMeshShader.LoadShaders("C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Shaders/hf_vert.glsl", "C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Shaders/hf_frag.glsl");

	m_crosshairShader.LoadShaders("src/Shaders/crosshair_vert.glsl", "src/Shaders/crosshair_frag.glsl");
	m_lineShader.LoadShaders("src/Shaders/line_vert.glsl", "src/Shaders/line_frag.glsl");
	aabbShader.LoadShaders("src/Shaders/aabb_vert.glsl", "src/Shaders/aabb_frag.glsl");
	m_cubeShader.LoadShaders("src/Shaders/pbr_vertex.glsl", "src/Shaders/pbr_fragment_emissive.glsl");
	m_cubemapShader.LoadShaders("src/Shaders/cubemap_vertex.glsl", "src/Shaders/cubemap_fragment.glsl");
	m_minimapShader.LoadShaders("src/Shaders/quad_vertex.glsl", "src/Shaders/quad_fragment.glsl");
	m_shadowMapShader.LoadShaders("src/Shaders/shadow_map_vertex.glsl", "src/Shaders/shadow_map_fragment.glsl");
	m_playerShadowMapShader.LoadShaders("src/Shaders/shadow_map_player_vertex.glsl", "src/Shaders/shadow_map_fragment.glsl");
	m_enemyShadowMapShader.LoadShaders("src/Shaders/shadow_map_enemy_vertex.glsl", "src/Shaders/shadow_map_fragment.glsl");
	m_shadowMapQuadShader.LoadShaders("src/Shaders/shadow_map_quad_vertex.glsl", "src/Shaders/shadow_map_quad_fragment.glsl");
	m_playerMuzzleFlashShader.LoadShaders("src/Shaders/muzzle_flash_vertex.glsl", "src/Shaders/muzzle_flash_fragment.glsl");
	m_playerTracerShader.LoadShaders("src/Shaders/muzzle_flash_vertex.glsl", "src/Shaders/muzzle_flash_fragment.glsl");

	m_physicsWorld = new PhysicsWorld();

	m_cubemapFaces = {
		"Assets/Textures/Skybox/T3Nebula/right.png",
		"Assets/Textures/Skybox/T3Nebula/left.png",
		"Assets/Textures/Skybox/T3Nebula/top.png",
		"Assets/Textures/Skybox/T3Nebula/bottom.png",
		"Assets/Textures/Skybox/T3Nebula/front.png",
		"Assets/Textures/Skybox/T3Nebula/back.png"
	};

	m_cubemap = new Cubemap(&m_cubemapShader);
	m_cubemap->LoadMesh();
	m_cubemap->LoadCubemap(m_cubemapFaces);

	//ground = new Ground(mapPos, mapScale, &groundShader, &groundShadowShader, false, this);
	//
	//ground->SetAABBShader(&aabbShader);
	//ground->SetUpAABB();
	//ground->SetPlaneShader(&m_lineShader);
	//
	//std::vector<Ground::GLTFMesh> meshDataGrnd = ground->meshData;
	//int mapVertCount = 0;
	//int mapIndCount = 0;
	//int triCount = 0;
	//int vertexOffset = 0;

	//for (Ground::GLTFMesh& mesh : meshDataGrnd)
	//{
	//	for (Ground::GLTFPrimitive& prim : mesh.primitives)
	//	{
	//		for (glm::vec3 vert : prim.verts)
	//		{
	//			glm::vec4 newVert = glm::vec4(vert.x, vert.y, vert.z, 1.0f);
	//			glm::mat4 model = glm::mat4(1.0f);
	//			model = glm::translate(model, mapPos);
	//			//model = glm::rotate(model, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
	//			model = glm::scale(model, mapScale);
	//			glm::vec4 newVertTr = model * newVert;
	//
	//			// Add transformed vertices
	//			mapVerts.push_back(glm::vec3(newVertTr.x, newVertTr.y, newVertTr.z));
	//			navMeshVertices.push_back(newVertTr.x);
	//			navMeshVertices.push_back(newVertTr.y);
	//			navMeshVertices.push_back(newVertTr.z);
	//			mapVertCount += 3;
	//		}
	//
	//		for (unsigned int idx : prim.indices)
	//		{
	//			navMeshIndices.push_back(idx + vertexOffset);
	//			mapIndCount++;
	//		}
	//
	//		vertexOffset += static_cast<int>(prim.verts.size());
	//	}
	//}




	//Logger::Log(1, "Map vertices count: %i\n", mapVerts.size());
	//
	//
	//Logger::Log(1, "navMeshVertices count: %zu\n", navMeshVertices.size());
	//for (size_t i = 0; i < std::min((size_t)10, navMeshVertices.size() / 3); ++i)
	//{
	//	Logger::Log(1, "Vertex %zu: %.2f %.2f %.2f\n",
	//		i,
	//		navMeshVertices[i * 3],
	//		navMeshVertices[i * 3 + 1],
	//		navMeshVertices[i * 3 + 2]);
	//}


	//int indexCount = (int)navMeshIndices.size();
	//Logger::Log(1, "Index Count: %zu\n", (int)navMeshIndices.size());
	//
	//triIndices = new int[indexCount];
	//
	//for (int i = 0; i < navMeshIndices.size(); i++)
	//{
	//	triIndices[i] = navMeshIndices[i];
	//}
	//
	//Logger::Log(1, "Index Count: %zu\n", indexCount);




	//int triangleCount = indexCount / 3;
	//
	//triAreas = new unsigned char[triangleCount];
	//
	//filter.setIncludeFlags(0xFFFF); // Include all polygons for testing
	//filter.setExcludeFlags(0);      // Exclude no polygons
	//
	//Logger::Log(1, "Tri Areas: %s", triAreas);
	//
	//int vertexCount = navMeshVertices.size() / 3;
	//
	//for (int i = 0; i < triangleCount * 3; i++) {
	//	if (triIndices[i] < 0 || triIndices[i] >= vertexCount) {
	//		Logger::Log(1, "Invalid triangle index %d: %d (vertexCount=%d)\n",
	//			i, triIndices[i], vertexCount);
	//	}
	//}


	// Slope threshold (in degrees) � typical value for shooters is 45
	//const float WALKABLE_SLOPE = 45.0f;
	//ctx = new rcContext();
	//
	//for (int i = 0; i < triangleCount; ++i) {
	//	float norm[3];
	//	const float* v0 = &navMeshVertices[triIndices[i * 3 + 0] * 3];
	//	const float* v1 = &navMeshVertices[triIndices[i * 3 + 1] * 3];
	//	const float* v2 = &navMeshVertices[triIndices[i * 3 + 2] * 3];
	//
	//	glm::vec3 v0_glm(v0[0], v0[1], v0[2]);
	//	glm::vec3 v1_glm(v1[0], v1[1], v1[2]);
	//	glm::vec3 v2_glm(v2[0], v2[1], v2[2]);
	//
	//	// Compute edges
	//	glm::vec3 e0 = v1_glm - v0_glm;
	//	glm::vec3 e1 = v2_glm - v0_glm;
	//
	//	// Compute face normal
	//	glm::vec3 faceNormal = glm::normalize(glm::cross(e0, e1));
	//
	//	// Flip winding if normal points down
	//	if (faceNormal.y < 0.0f) {
	//	//	std::swap(triIndices[i * 3 + 1], triIndices[i * 3 + 2]);
	//	}
	//
	//}


	// Mark which triangles are walkable based on slope
	//rcMarkWalkableTriangles(&ctx,
	//	WALKABLE_SLOPE,
	//	navMeshVertices.data(), navMeshVertices.size() / 3,
	//	triIndices, triangleCount,
	//	triAreas);
	//// Count how many triangles are walkable
	//int walkableCount = 0;
	//for (int i = 0; i < triangleCount; ++i)
	//{
	//	if (triAreas[i] == RC_WALKABLE_AREA)
	//		walkableCount++;
	//}
	//
	//Logger::Log(1, "[Recast] Triangles processed: %d\n", triangleCount);
	//Logger::Log(1, "[Recast] Walkable triangles:  %d\n", walkableCount);
	//Logger::Log(1, "[Recast] Non-walkable:        %d\n", triangleCount - walkableCount);
	//
	//// Optional sanity check: Print first few triangles and their walkable flag
	//for (int i = 0; i < std::min(triangleCount, 5); ++i)
	//{
	//	Logger::Log(1, "  Tri %d: %s\n", i,
	//		(triAreas[i] == RC_WALKABLE_AREA) ? "WALKABLE" : "NOT WALKABLE");
	//}
	////for (int i = 0; i < triangleCount; ++i)
	////{
	////	triAreas[i] = RC_WALKABLE_AREA;
	////}
	//for (int i = 0; i < std::min(triangleCount, 5); ++i)
	//{
	//	Logger::Log(1, "  Tri %d: %s\n", i,
	//		(triAreas[i] == RC_WALKABLE_AREA) ? "WALKABLE" : "NOT WALKABLE");
	//}
	//
	//
	//
	//
	//rcConfig cfg{};
	//
	//cfg.cs = 0.3f;                      // Cell size
	//cfg.ch = 0.2f;                      // Cell height
	//cfg.walkableSlopeAngle = WALKABLE_SLOPE;     // Steeper slopes allowed
	//cfg.walkableHeight = (int)ceilf(2.0f / cfg.ch);          // Min agent height
	//cfg.walkableClimb = (int)floorf(0.4f / cfg.ch);           // Step height
	//cfg.walkableRadius = (int)ceilf(AGENT_RADIUS / cfg.cs);          // Agent radius
	//cfg.maxEdgeLen = (int)(12.0f / cfg.cs);                // Longer edges for smoother polys
	//cfg.minRegionArea = rcSqr(12);              // Retain smaller regions
	//cfg.mergeRegionArea = rcSqr(30);           // Merge small regions
	//cfg.maxSimplificationError = 0.1f;  // Less aggressive simplification
	//cfg.detailSampleDist = cfg.cs * 6;  // Balanced detail
	//cfg.maxVertsPerPoly = 6;            // Max verts per poly
	//cfg.tileSize = 248;                  // Tile size
	//
	//rcCalcGridSize(cfg.bmin, cfg.bmax, cfg.cs, &cfg.width, &cfg.height);
	//cfg.borderSize = cfg.walkableRadius + 3;      // cells (important)
	//
	//cfg.borderSize = (int)ceil(cfg.walkableRadius / cfg.cs);
#	//
	//rcCalcBounds(navMeshVertices.data(), navMeshVertices.size() / 3, cfg.bmin, cfg.bmax);
	//
	////cfg.width = (int)((cfg.bmax[0] - cfg.bmin[0]) / cfg.cs + 0.5f);
	////cfg.height = (int)((cfg.bmax[2] - cfg.bmin[2]) / cfg.cs + 0.5f);
	//
	//int mapVoxelsX = int((cfg.bmax[0] - cfg.bmin[0]) / cfg.cs + 0.5f);
	//int mapVoxelsZ = int((cfg.bmax[2] - cfg.bmin[2]) / cfg.cs + 0.5f);
	//
	//int tileCountX = (mapVoxelsX + cfg.tileSize - 1) / cfg.tileSize;
	//int tileCountY = (mapVoxelsZ + cfg.tileSize - 1) / cfg.tileSize;
	//
	////int tileWidth, tileHeight;
	////tileWidth = (cfg.width + cfg.tileSize - 1) / cfg.tileSize;
	////tileHeight = (cfg.height + cfg.tileSize - 1) / cfg.tileSize;
	//
	//Logger::Log(1, "Num verts %zu", navMeshVertices.size());
	//
	//Logger::Log(1, "navMeshVertices count: %zu", navMeshVertices.size());
	//for (size_t i = 0; i < std::min((size_t)10, navMeshVertices.size() / 3); ++i)
	//{
	//	Logger::Log(1, "Vertex %zu: %.2f %.2f %.2f",
	//		i,
	//		navMeshVertices[i * 3],
	//		navMeshVertices[i * 3 + 1],
	//		navMeshVertices[i * 3 + 2]);
	//}
	//
	//
	//float minBounds[3] = { -261.04f, -2.39, -231.76 };
	//float maxBounds[3] = { 251.37f, 213.66f, 304.81f };
	//
	//rcCalcBounds(navMeshVertices.data(), navMeshVertices.size() / 3, cfg.bmin, cfg.bmax);
	//
	//
	//
	//
	//tileWorldSize = cfg.tileSize * cfg.cs;
	//
	//dtNavMeshParams params = {};
	//float tileWorldSize = cfg.tileSize * cfg.cs;
	//params.orig[0] = floor(cfg.bmin[0] / tileWorldSize) * tileWorldSize;
	//params.orig[1] = cfg.bmin[1];
	//params.orig[2] = floor(cfg.bmin[2] / tileWorldSize) * tileWorldSize;
	//params.tileWidth = tileWorldSize;
	//params.tileHeight = tileWorldSize;
	//params.maxTiles = tileCountX * tileCountY;   // now > 1
	//params.maxPolys = 2048; // Set a reasonable limit for the number of polygons per tile
	//Logger::Log(1, "Navmesh origin: %.2f %.2f %.2f\n", params.orig[0], params.orig[1], params.orig[2]);
	//
	//navMesh = dtAllocNavMesh();
	//
	//dtStatus navInitStatus = navMesh->init(&params);
	//
	//for (int y = 0; y < tileCountY; ++y)
	//{
	//	for (int x = 0; x < tileCountX; ++x)
	//	{
	//		unsigned char* navData = nullptr;
	//		int navDataSize = 0;
	//
	//		if (BuildTile(x, y, cfg.bmin, cfg.bmax, cfg, navData, &navDataSize, params))
	//		{
	//			// Add tile to Detour navmesh
	//			dtStatus status = navMesh->addTile(navData, navDataSize, DT_TILE_FREE_DATA, 0, nullptr);
	//		
	//			if (dtStatusFailed(status))
	//			{
	//				Logger::Log(1, "Error: Could not add tile to navmesh\n", __FUNCTION__);
	//			}
	//		}
	//	}
	//}
	//
	//navMeshQuery = dtAllocNavMeshQuery();
	//
	//dtStatus status = navMeshQuery->init(navMesh, 4096);
	//if (dtStatusFailed(status))
	//{
	//	Logger::Log(1, "%s error: Could not init Detour navMeshQuery\n", __FUNCTION__);
	//}
	//else
	//{
	//	Logger::Log(1, "%s: Detour navMeshQuery successfully initialized\n", __FUNCTION__);
	//
	//}
	//
	//
	//
	//const dtNavMeshParams* nmparams = navMesh->getParams();
	//Logger::Log(1, "Navmesh origin: %.2f %.2f %.2f\n", nmparams->orig[0], nmparams->orig[1], nmparams->orig[2]);
	//
	//for (int y = 0; y < navMesh->getMaxTiles(); ++y) {
	//	for (int x = 0; x < navMesh->getMaxTiles(); ++x) {
	//		const dtMeshTile* tile = navMesh->getTileAt(x, y, 0);  // layer = 0
	//		if (!tile || !tile->header) continue;
	//
	//		Logger::Log(1, "Tile at (%d,%d): Bmin(%.2f %.2f %.2f) Bmax(%.2f %.2f %.2f)\n",
	//			x, y,
	//			tile->header->bmin[0], tile->header->bmin[1], tile->header->bmin[2],
	//			tile->header->bmax[0], tile->header->bmax[1], tile->header->bmax[2]);
	//	}
	//}
	m_assetManager = new AssetManager();

	m_renderBackend = CreateRenderBackend();
	m_renderBackend->Initialize();

	PipelineDesc pipelineDesc{};
	pipelineDesc.vertexShaderPath = "src/Shaders/leveltestvert.glsl";
	pipelineDesc.fragmentShaderPath =  "src/Shaders/leveltestfrag.glsl";
	pipelineDesc.vertexStride = (uint32_t)((int)(sizeof(float)) * 12);

	pipes.staticPbr = m_renderBackend->CreatePipeline(pipelineDesc);
	
	uploader = new GpuUploader(m_renderBackend, m_assetManager);

	auto& reg = m_activeScene->GetRegistry();
	std::string levelPath = "Assets/Models/Game_Scene/V4/Aviary-Environment-V4-Box-Collider.glb";
	CreateLevel(reg, *m_assetManager, levelPath);

	BufferCreateInfo bufferCreateInfo = {};
	bufferCreateInfo.size = 3 * sizeof(glm::mat4);
	bufferCreateInfo.usage = BufferUsage::Uniform;
	bufferCreateInfo.initialData = nullptr;
	h = m_renderBackend->CreateBuffer(bufferCreateInfo);

	m_camera = new Camera(glm::vec3(50.0f, 3.0f, 80.0f));

	m_minimapQuad = new Quad();
	m_minimapQuad->SetUpVAO(false);

	m_shadowMapQuad = new Quad();
	m_shadowMapQuad->SetUpVAO(false);

	m_playerMuzzleFlashQuad = new Quad();
	m_playerMuzzleFlashQuad->SetUpVAO(true);
	m_playerMuzzleFlashQuad->SetShader(&playerMuzzleFlashShader);
	m_playerMuzzleFlashQuad->LoadTexture("C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/Assets/Textures/muzzleflash.png");

	m_enemyMuzzleFlashQuad = new Quad();
	m_enemyMuzzleFlashQuad->SetUpVAO(true);
	m_enemyMuzzleFlashQuad->SetShader(&playerMuzzleFlashShader);
	m_enemyMuzzleFlashQuad->LoadTexture("C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/Assets/Textures/muzzleflash.png");

	m_enemy2MuzzleFlashQuad = new Quad();
	m_enemy2MuzzleFlashQuad->SetUpVAO(true);
	m_enemy2MuzzleFlashQuad->SetShader(&playerMuzzleFlashShader);
	m_enemy2MuzzleFlashQuad->LoadTexture("C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/Assets/Textures/muzzleflash.png");

	m_enemy3MuzzleFlashQuad = new Quad();
	m_enemy3MuzzleFlashQuad->SetUpVAO(true);
	m_enemy3MuzzleFlashQuad->SetShader(&playerMuzzleFlashShader);
	m_enemy3MuzzleFlashQuad->LoadTexture("C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/Assets/Textures/muzzleflash.png");

	m_enemy4MuzzleFlashQuad = new Quad();
	m_enemy4MuzzleFlashQuad->SetUpVAO(true);
	m_enemy4MuzzleFlashQuad->SetShader(&playerMuzzleFlashShader);
	m_enemy4MuzzleFlashQuad->LoadTexture("C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/Assets/Textures/muzzleflash.png");


	m_player = new Player(glm::vec3(-9.0f, 354.6f, 166.0f), glm::vec3(5.0f), &playerShader, &groundShadowShader, true, this, 180.0f);
	//player = new Player( (glm::vec3(23.0f, 0.0f, 37.0f)), glm::vec3(3.0f), &playerShader, &playerShadowMapShader, true, this);

	m_player->SetAABBShader(&aabbShader);
	m_player->SetUpAABB();

	std::string texture = "C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/Assets/Models/GLTF/Enemies/Ely/EnemyEly_ely_vanguardsoldier_kerwinatienza_M2_BaseColor.png";
	std::string texture2 = "C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/Assets/Models/GLTF/Enemies/Ely/ely-vanguardsoldier-kerwinatienza_diffuse_2.png";
	std::string texture3 = "C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/Assets/Models/GLTF/Enemies/Ely/ely-vanguardsoldier-kerwinatienza_diffuse_3.png";
	std::string texture4 = "C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/Assets/Models/GLTF/Enemies/Ely/ely-vanguardsoldier-kerwinatienza_diffuse_4.png";


	m_enemy = new Enemy(glm::vec3(-9.0f, 354.6f, 166.0f), glm::vec3(5.0f), &playerShader, &enemyShadowMapShader, true, this, texture, 0, GetEventManager(), *m_player);
	m_enemy->SetAABBShader(&aabbShader);
	m_enemy->SetUpAABB();

	m_enemy2 = new Enemy(glm::vec3(-7.0f, 354.6f, 169.0f), glm::vec3(5.0f), &playerShader, &enemyShadowMapShader, true, this, texture2, 1, GetEventManager(), *m_player);
	m_enemy2->SetAABBShader(&aabbShader);
	m_enemy2->SetUpAABB();

	m_enemy3 = new Enemy(glm::vec3(-4.0f, 354.6f, 171.0f), glm::vec3(5.0f), &playerShader, &enemyShadowMapShader, true, this, texture3, 2, GetEventManager(), *m_player);
	m_enemy3->SetAABBShader(&aabbShader);
	m_enemy3->SetUpAABB();

	m_enemy4 = new Enemy(glm::vec3(-5.0f, 354.6f, 173.0f), glm::vec3(5.0f), &playerShader, &enemyShadowMapShader, true, this, texture4, 3, GetEventManager(), *m_player);
	m_enemy4->SetAABBShader(&aabbShader);
	m_enemy4->SetUpAABB();

	m_crosshair = new Crosshair(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.3f), &crosshairShader, &shadowMapShader, false, this);
	m_crosshair->LoadMesh();
	m_crosshair->LoadTexture("C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/Assets/Textures/Crosshair.png");
	m_playerLine = new Line(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f), &lineShader, &shadowMapShader, false, this);
	m_playerLine->LoadMesh();

	for (auto& line : m_enemyLines)
	{
		line = new Line(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f), &lineShader, &shadowMapShader, false, this);
		line->LoadMesh();
	}



	//ground = new Ground(mapPos, mapScale, &groundShader, &groundShadowShader, false, this);

	AudioComponent* fireAudioComponent = new AudioComponent(m_enemy);



	m_inputManager->SetContext(m_camera, m_player, m_enemy, width, height);

	/* reset skeleton split */

	std::srand(static_cast<unsigned int>(std::time(nullptr)));

	m_gameObjects.push_back(m_player);
	m_gameObjects.push_back(m_enemy);
	m_gameObjects.push_back(m_enemy2);
	m_gameObjects.push_back(m_enemy3);
	m_gameObjects.push_back(m_enemy4);
	//m_gameObjects.push_back(ground);

	/*for (Cube* coverSpot : coverSpots)
	{
		gameObjects.push_back(coverSpot);
	}*/

	m_enemies.push_back(m_enemy);
	m_enemies.push_back(m_enemy2);
	m_enemies.push_back(m_enemy3);
	m_enemies.push_back(m_enemy4);

	if (m_initializeQTable)
	{
		for (auto& enem : m_enemies)
		{
			int enemyID = enem->GetID();
			Logger::Log(1, "%s Initializing Q Table for Enemy %d\n", __FUNCTION__, enemyID);
			InitializeQTable(m_enemyStateQTable[enemyID]);
			Logger::Log(1, "%s Initialized Q Table for Enemy %d\n", __FUNCTION__, enemyID);
		}
	}
	else if (m_loadQTable)
	{
		for (auto& enem : m_enemies)
		{
			int enemyID = enem->GetID();
			Logger::Log(1, "%s Loading Q Table for Enemy %d\n", __FUNCTION__, enemyID);
			LoadQTable(m_enemyStateQTable[enemyID], std::to_string(enemyID) + m_enemyStateFilename);
			Logger::Log(1, "%s Loaded Q Table for Enemy %d\n", __FUNCTION__, enemyID);
		}
	}

	mMusicEvent = m_audioSystem->PlayEvent("event:/bgm");

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

	//for (rcPolyMesh* polyMesh : polyMeshes)
	//{
	//	const int nvp = polyMesh->nvp;
	//	const float cs = polyMesh->cs;
	//	const float ch = polyMesh->ch;
	//	const float* orig = polyMesh->bmin;
	//	size_t baseVertexIndex = navRenderMeshVertices.size() / 3;
	//
	//	for (int i = 0, j = polyMesh->nverts - 1; i < polyMesh->nverts; j = i++)
	//	{
	//		const unsigned short* v = &polyMesh->verts[i * 3];
	//		const float x = orig[0] + v[0] * cs;
	//		const float y = orig[1] + v[1] * ch;
	//		const float z = orig[2] + v[2] * cs;
	//		navRenderMeshVertices.push_back(x);
	//		navRenderMeshVertices.push_back(y);
	//		navRenderMeshVertices.push_back(z);
	//	}
	//
	//	// Process indices
	//	for (int i = 0; i < polyMesh->npolys; ++i)
	//	{
	//		const unsigned short* p = &polyMesh->polys[i * nvp * 2];
	//		for (int j = 2; j < nvp; ++j)
	//		{
	//			if (p[j] == RC_MESH_NULL_IDX) break;
	//			// Skip degenerate triangles
	//			if (p[0] == p[j - 1] || p[0] == p[j] || p[j - 1] == p[j]) continue;
	//			navRenderMeshIndices.push_back(baseVertexIndex + p[0]);      // Triangle vertex 1
	//			navRenderMeshIndices.push_back(baseVertexIndex + p[j - 1]); // Triangle vertex 2
	//			navRenderMeshIndices.push_back(baseVertexIndex + p[j]);     // Triangle vertex 3
	//		}
	//	}
	//}
	//
	//
	//Logger::Log(1, "Render NavMesh Vert Count: %zu", navRenderMeshVertices.size());
	//Logger::Log(1, "Render NavMesh Index Count: %zu", navRenderMeshIndices.size());
	//
	//// Create VAO
	//glGenVertexArrays(1, &vao);
	//glBindVertexArray(vao);
	//
	//// Create VBO
	//glGenBuffers(1, &vbo);
	//glBindBuffer(GL_ARRAY_BUFFER, vbo);
	//glBufferData(GL_ARRAY_BUFFER, navRenderMeshVertices.size() * sizeof(float), navRenderMeshVertices.data(), GL_STATIC_DRAW);
	//
	//// Create EBO
	//glGenBuffers(1, &ebo);
	//glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
	//glBufferData(GL_ELEMENT_ARRAY_BUFFER, navRenderMeshIndices.size() * sizeof(unsigned int), navRenderMeshIndices.data(), GL_STATIC_DRAW);
	//
	//// Enable vertex attribute (e.g., position at location 0)
	//glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
	//glEnableVertexAttribArray(0);
	//
	//glBindVertexArray(0);
	//
	//
	////crowd = dtAllocCrowd();
	////crowd->init(enemies.size(), 1.0f, navMesh);
	//
	//		// Create VAO
	//glGenVertexArrays(1, &hfvao);
	//glBindVertexArray(hfvao);
	//
	//// Create VBO
	//glGenBuffers(1, &hfvbo);
	//glBindBuffer(GL_ARRAY_BUFFER, hfvbo);
	//glBufferData(GL_ARRAY_BUFFER, hfnavRenderMeshVertices.size() * sizeof(float), hfnavRenderMeshVertices.data(), GL_STATIC_DRAW);
	//
	//// Create EBO
	//glGenBuffers(1, &hfebo);
	//glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, hfebo);
	//glBufferData(GL_ELEMENT_ARRAY_BUFFER, hfnavRenderMeshIndices.size() * sizeof(unsigned int), hfnavRenderMeshIndices.data(), GL_STATIC_DRAW);
	//
	//// Enable vertex attribute (e.g., position at location 0)
	//glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
	//glEnableVertexAttribArray(0);
	//glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
	//glEnableVertexAttribArray(1);
	//
	//
	//glBindVertexArray(0);
	//
	//float playerStartingPos[3] = { m_player->GetPosition().x, m_player->GetPosition().y, m_player->GetPosition().z };
	//float playerSnappedPos[3];
	//dtPolyRef playerStartPoly;
	//navMeshQuery->findNearestPoly(playerStartingPos, halfExtents, &filter, &playerStartPoly, playerSnappedPos);
	//m_player->SetPosition(glm::vec3(playerSnappedPos[0], playerSnappedPos[1], playerSnappedPos[2]));
	//
	//
	//crowd = dtAllocCrowd();
	//crowd->init(50, AGENT_RADIUS, navMesh);
	//
	////for (auto& enem : enemies)
	////{
	////	dtCrowdAgentParams ap;
	////	memset(&ap, 0, sizeof(ap));
	////	ap.radius = 0.01f;
	////	ap.height = 3.0f;
	////	ap.maxSpeed = 3.5f;
	////	ap.maxAcceleration = 8.0f; // Meters per second squared
	////	ap.collisionQueryRange = ap.radius * 12.0f;
	//
	////	float startingPos[3] = { enem->getPosition().x, enem->getPosition().y, enem->getPosition().z };
	////	enemyAgentIDs.push_back(crowd->addAgent(startingPos, &ap));
	//
	//
	////};
	//
	//for (auto& enem : m_enemies)
	//{
	//	dtCrowdAgentParams ap;
	//	memset(&ap, 0, sizeof(ap));
	//	ap.radius = AGENT_RADIUS;
	//	ap.height = 1.0f;
	//	ap.maxSpeed = 4.0f;
	//	ap.maxAcceleration = 12.0f;
	//	ap.collisionQueryRange = AGENT_RADIUS * 6.0f;
	//	ap.pathOptimizationRange = AGENT_RADIUS * 15.0f;
	//	ap.updateFlags = DT_CROWD_ANTICIPATE_TURNS
	//		| DT_CROWD_OPTIMIZE_VIS
	//		| DT_CROWD_OPTIMIZE_TOPO
	//		| DT_CROWD_SEPARATION;
	//	ap.separationWeight = 0.5f;
	//	float startingPos[3] = { enem->GetPosition().x, enem->GetPosition().y, enem->GetPosition().z };
	//	float snappedPos[3];
	//	dtPolyRef startPoly;
	//	navMeshQuery->findNearestPoly(startingPos, halfExtents, &filter, &startPoly, snappedPos);
	//	enemyAgentIDs.push_back(crowd->addAgent(snappedPos, &ap));
	//	Logger::Log(1, "[Spawn] Enemy spawned as agent %d at (%.2f, %.2f, %.2f)\n",
	//		enemyAgentIDs.back(), snappedPos[0], snappedPos[1], snappedPos[2]);
	//}

	//for (auto& enem : enemies)
	//{
	//	dtCrowdAgentParams ap;
	//	memset(&ap, 0, sizeof(ap));
	//	ap.radius = 0.6f;
	//	ap.height = 2.0f;
	//	ap.maxSpeed = 3.5f;
	//	ap.maxAcceleration = 8.0f;
	//	ap.collisionQueryRange = ap.radius * 12.0f;

	//	float startPos[3] = { enem->getPosition().x, enem->getPosition().y, enem->getPosition().z };

	//	int agentID;

	//	bool added = AddAgentToCrowd(crowd, navMeshQuery, startPos, &ap, &filter, snappedPos, agentID);

	//	if (added)
	//	{
	//		Logger::Log(1, "[Spawn] Enemy spawned as agent %d at (%.2f, %.2f, %.2f)\n",
	//			agentID, snappedPos[0], snappedPos[1], snappedPos[2]);

	//		enemyAgentIDs.push_back(agentID);
	//	}
	//	else
	//	{
	//		Logger::Log(1, "[Spawn] Enemy spawn FAILED at (%.2f, %.2f, %.2f)\n",
	//			startPos[0], startPos[1], startPos[2]);
	//		Logger::Log(1, "[Spawn] Enemy spawned as agent %d at (%.2f, %.2f, %.2f)\n",
	//			agentID, snappedPos[0], snappedPos[1], snappedPos[2]);
	//	}


	////	float snappedPos[3] = { 1.0f, 0.0f, 1.0f };

	////	dtPolyRef polyref = 0;
	////	dtStatus newstatus = navMeshQuery->findNearestPoly(snappedPos, halfExtents, &filter, &polyref, snappedPos);


	////	if (dtStatusFailed(newstatus) || polyref == 0)
	////	{
	////		Logger::Log(1, "findNearestPoly failed: no polygon found near %.2f %.2f %.2f\n",
	////			85.0f, 0.0f, 25.0f);
	////		return; // stop here, don�t use snappedPos
	////	}

	////	float meshheight = 0.0f;
	////	if (navMeshQuery->getPolyHeight(polyref, snappedPos, &meshheight) == DT_SUCCESS)
	////	{
	////		Logger::Log(1, "Navmesh height at %.2f %.2f: %.2f\n", snappedPos[0], snappedPos[2], meshheight);
	////	}

	//}
}

void GameManager::SetupCamera(unsigned int width, unsigned int height, float deltaTime)
{
	m_camera->SetZoom(45.0f);

	if (m_camera->GetMode() == PLAYER_FOLLOW)
	{
		if (m_camera->isBlending)
		{
			//m_camera->SetPitch(45.0f);
			glm::vec3 camPos = m_camera->GetPosition();
			if (camPos.y < 0.0f)
			{
				camPos.y = m_camera->GetPlayerCamHeightOffset();
				m_camera->SetPosition(camPos);
			}
			m_view = m_camera->UpdateCameraLerp(m_camera->GetPosition() + (glm::vec3(0.0f, 1.0f, 0.0f) * m_camera->GetPlayerCamHeightOffset()), m_player->GetPosition() + (m_player->GetPlayerFront() * m_camera->GetPlayerPosOffset()), m_player->GetPlayerFront(), glm::vec3(0.0f, 1.0f, 0.0f), deltaTime);

		}
		else {
			//m_camera->SetPitch(45.0f);
			m_camera->FollowTarget(m_player->GetPosition() + (m_player->GetPlayerFront() * m_camera->GetPlayerPosOffset()), m_player->GetPlayerFront(), m_camera->GetPlayerCamRearOffset(), m_camera->GetPlayerCamHeightOffset());
			if (m_camera->HasSwitched())
				m_camera->StorePrevCam(m_camera->GetPosition() + (glm::vec3(0.0f, 1.0f, 0.0f) * m_camera->GetPlayerCamHeightOffset()), m_player->GetPosition() + (m_player->GetPlayerFront() * m_camera->GetPlayerPosOffset()));

			glm::vec3 camPos = m_camera->GetPosition();
			if (camPos.y < 0.0f)
			{
				camPos.y = m_camera->GetPlayerCamHeightOffset();
				m_camera->SetPosition(camPos);
			}

			m_view = m_camera->GetViewMatrixPlayerFollow(m_player->GetPosition() + (m_player->GetPlayerFront() * m_camera->GetPlayerPosOffset()), glm::vec3(0.0f, 1.0f, 0.0f));
		}

	}
	else if (m_camera->GetMode() == ENEMY_FOLLOW)
	{
		if (m_enemy->IsDestroyed())
		{
			m_camera->SetMode(FLY);
			return;
		}
		m_camera->FollowTarget(m_enemy->GetPositionOld(), m_enemy->GetEnemyFront(), m_camera->GetEnemyCamRearOffset(), m_camera->GetEnemyCamHeightOffset());
		m_view = m_camera->GetViewMatrixEnemyFollow(m_enemy->GetPositionOld(), glm::vec3(0.0f, 1.0f, 0.0f));
	}
	else if (m_camera->GetMode() == FLY)
	{
		if (m_firstFlyCamSwitch)
		{
			m_camera->FollowTarget(m_player->GetPosition(), m_player->GetPlayerFront(), m_camera->GetPlayerCamRearOffset(), m_camera->GetPlayerCamHeightOffset());
			m_firstFlyCamSwitch = false;
			return;
		}
		m_view = m_camera->GetViewMatrix();
	}
	else if (m_camera->GetMode() == PLAYER_AIM)
	{

		m_camera->SetZoom(33.0f);
		glm::vec3 target = m_player->GetPosition() + (m_player->GetPlayerFront() * m_camera->GetPlayerPosOffset()) + (m_player->GetPlayerRight() * m_camera->GetPlayerAimRightOffset());
		if (target.y < m_player->GetShootPos().y)
			target.y = m_player->GetShootPos().y;


		if (m_camera->isBlending)
		{
			//glm::vec3 newPos =
			//	(m_player->GetPosition())  +
			//	(m_player->GetPlayerFront() * m_camera->GetPlayerPosOffset()) +
			//	(m_player->GetPlayerRight() * m_camera->GetPlayerAimRightOffset());

			//if (newPos.y < m_player->GetPosition().y)
			//newPos.y = m_player->GetPosition().y + m_camera->playerCamHeightOffset;

			//glm::vec3 camPos = m_camera->GetPosition();
			//if (camPos.y <= m_player->GetPosition().y)
			//{
			//	camPos.y = m_player->GetPosition().y + 5.0f;
			//	m_camera->SetPosition(camPos);
			//}
			//m_camera->FollowTarget(m_player->GetPosition() + (m_player->GetPlayerFront() * m_camera->GetPlayerPosOffset()) + (m_player->GetPlayerRight() * m_camera->GetPlayerAimRightOffset()),
			//	m_player->GetPlayerFront(), m_camera->GetPlayerCamRearOffset(), m_camera->GetPlayerCamHeightOffset());
			//
			//glm::vec3 targetPos = m_camera->GetPosition();

			glm::vec3 camPos = m_camera->GetPosition();

			m_camera->FollowTarget(m_player->GetPosition() + (m_player->GetPlayerFront() * m_camera->GetPlayerPosOffset()) + (m_player->GetPlayerRight() * m_camera->GetPlayerAimRightOffset()),
				m_player->GetPlayerFront(), m_camera->GetPlayerAimCamRearOffset(), m_camera->GetPlayerAimCamHeightOffset());

			camPos = m_camera->GetPosition();

			m_view = m_camera->UpdateCameraLerp(camPos,
				m_player->GetPosition() + (m_player->GetPlayerFront() * m_camera->GetPlayerPosOffset()) + (m_player->GetPlayerRight() * m_camera->GetPlayerAimRightOffset()),
				m_player->GetPlayerFront(), m_player->GetPlayerAimUp(), deltaTime);
			m_camera->StorePrevCam(m_camera->GetPosition(), target);

		}
		else {

			glm::vec3 camPos = m_camera->GetPosition();

			m_camera->FollowTarget(m_player->GetPosition() + (m_player->GetPlayerFront() * m_camera->GetPlayerPosOffset()) + (m_player->GetPlayerRight() * m_camera->GetPlayerAimRightOffset()),
				m_player->GetPlayerFront(), m_camera->GetPlayerAimCamRearOffset(), m_camera->GetPlayerAimCamHeightOffset());

			if (m_camera->HasSwitched())
				m_camera->StorePrevCam(m_camera->GetPosition() + m_player->GetPlayerAimUp() * m_camera->GetPlayerAimCamHeightOffset(), m_player->GetPosition() + (m_player->GetPlayerFront() * m_camera->GetPlayerPosOffset()) + (m_player->GetPlayerRight() * m_camera->GetPlayerAimRightOffset()) + (m_player->GetPlayerAimUp() * m_camera->GetPlayerAimCamHeightOffset()));

			//if (camPos.y <= m_player->GetPosition().y)
			//{
			//	camPos.y = m_player->GetPosition().y + 5.0f;
			//	m_camera->SetPosition(camPos);
			//}
			m_camera->StorePrevCam(camPos, m_camera->GetPosition());
			m_view = m_camera->GetViewMatrixPlayerFollow(target, m_player->GetPlayerAimUp());
		}

	}

	m_cubemapView = glm::mat4(glm::mat3(m_camera->GetViewMatrixPlayerFollow(m_player->GetPosition(), glm::vec3(0.0f, 1.0f, 0.0f))));

	m_projection = glm::perspective(glm::radians(m_camera->GetZoom()), (float)width / (float)height, 0.1f, 500.0f);

	m_minimapView = glm::mat4(1.0f);
	m_minimapProjection = glm::perspective(glm::radians(m_camera->GetZoom()), (float)width / (float)height, 0.1f, 500.0f);

	m_player->SetCameraMatrices(m_view, m_projection);

	m_audioSystem->SetListener(m_view);
}

void GameManager::SetSceneData()
{
	m_renderer->SetScene(m_view, m_projection, m_cubemapView, dirLight);
}

void GameManager::SetUpDebugUi()
{
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();
}

void GameManager::ShowDebugUi()
{
	ShowCameraControlWindow(*m_camera);
	ShowLightControlWindow(dirLight);
	ShowCameraControlWindow(*m_camera);

	ImGui::Begin("Player");

	ImGui::InputFloat3("Position", &m_player->m_position[0]);
	ImGui::InputFloat("Yaw", &m_player->m_playerYaw);
	ImGui::InputFloat3("Player Front", &m_player->m_playerFront[0]);
	ImGui::InputFloat3("Player Aim Front", &m_player->m_playerAimFront[0]);
	ImGui::InputFloat("Player Aim Pitch", &m_player->m_aimPitch);
	ImGui::InputFloat("Player Rear Offset", &m_camera->playerCamRearOffset);
	m_camera->SetPlayerCamRearOffset(m_camera->playerCamRearOffset);
	ImGui::InputFloat("Player Height Offset", &m_camera->playerCamHeightOffset);
	m_camera->SetPlayerCamHeightOffset(m_camera->playerCamHeightOffset);
	ImGui::InputFloat("Player Pos Offset", &m_camera->playerPosOffset);
	m_camera->SetPlayerPosOffset(m_camera->playerPosOffset);
	ImGui::InputFloat("Player Aim Right Offset", &m_camera->playerAimRightOffset);
	ImGui::InputInt("Player Animation", &m_player->m_animNum);
	ImGui::End();

	ShowPerformanceWindow();
#ifdef DEBUG
#endif

	if (!m_useEdbt)
	{
		ShowEnemyStateWindow();
	}
}

void GameManager::ShowCameraControlWindow(Camera& cam)
{
	ImGui::Begin("Map Settings");

	ImGui::Text("Position");
	ImGui::DragFloat3("Position", (float*)&mapPos, mapPos.x, mapPos.y, mapPos.z);
	//ground->SetPosition(mapPos);

	ImGui::Text("Scale");
	ImGui::DragFloat3("Scale", (float*)&mapScale, mapScale.x, mapScale.y, mapScale.z);
	//ground->SetScale(mapScale);


	ImGui::End();


	ImGui::Begin("Directional Light Control");

	std::string modeText = "";

	if (cam.GetMode() == FLY)
	{
		modeText = "Flycam";


		cam.UpdateCameraVectors();
	}
	else if (cam.GetMode() == PLAYER_FOLLOW)
		modeText = "Player Follow";
	else if (cam.GetMode() == ENEMY_FOLLOW)
		modeText = "Enemy Follow";
	else if (cam.GetMode() == PLAYER_AIM)
		modeText = "Player Aim"
		;
	ImGui::Text(modeText.c_str());

	ImGui::InputFloat3("Position", (float*)&cam.m_position);

	ImGui::InputFloat("Pitch", (float*)&cam.m_pitch);
	ImGui::InputFloat("Blend Time", (float*)&m_camera->cameraBlendTime);

	ImGui::InputFloat("Yaw", (float*)&cam.m_yaw);
	ImGui::InputFloat("Zoom", (float*)&cam.m_zoom);

	ImGui::End();
}

void GameManager::RenderDebugUi()
{
	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void GameManager::ShowLightControlWindow(DirLight& light)
{
	ImGui::Begin("Directional Light Control");

	ImGui::Text("Light Direction");
	ImGui::DragFloat3("Direction", (float*)&light.m_direction, dirLight.m_direction.x, dirLight.m_direction.y, dirLight.m_direction.z);

	ImGui::ColorEdit4("Ambient", (float*)&light.m_ambient);

	ImGui::ColorEdit4("Diffuse", (float*)&light.m_diffuse);
	ImGui::ColorEdit4("PBR Color", (float*)&dirLightPBRColour);

	ImGui::ColorEdit4("Specular", (float*)&light.m_specular);

	ImGui::DragFloat("Ortho Left", (float*)&m_orthoLeft);
	ImGui::DragFloat("Ortho Right", (float*)&m_orthoRight);
	ImGui::DragFloat("Ortho Bottom", (float*)&m_orthoBottom);
	ImGui::DragFloat("Ortho Top", (float*)&m_orthoTop);
	ImGui::DragFloat("Near Plane", (float*)&m_nearPlane);
	ImGui::DragFloat("Far Plane", (float*)&m_farPlane);

	ImGui::End();
}

void GameManager::ShowPerformanceWindow()
{
	ImGui::Begin("Performance");

	ImGui::Text("FPS: %.1f", m_fps);
	ImGui::Text("Avg FPS: %.1f", m_avgFps);
	ImGui::Text("Frame Time: %.1f ms", m_frameTime);
	ImGui::Text("Elapsed Time: %.1f s", m_elapsedTime);

	ImGui::End();
}

void GameManager::ShowEnemyStateWindow()
{
	ImGui::Begin("Game States");

	ImGui::Checkbox("Use EDBT", &m_useEdbt);

	ImGui::Text("Player Health: %d", (int)m_player->GetHealth());

	for (Enemy* e : m_enemies)
	{
		if (e == nullptr || e->IsDestroyed())
			continue;
		ImTextureID texID = (void*)(intptr_t)e->GetTexture().GetTexId();
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

void GameManager::CalculatePerformance(float deltaTime)
{
	m_fps = 1.0f / deltaTime;

	m_fpsSum += m_fps;
	m_frameCount++;

	if (m_frameCount == m_numFramesAvg)
	{
		m_avgFps = m_fpsSum / m_numFramesAvg;
		m_fpsSum = 0.0f;
		m_frameCount = 0;
	}

	m_frameTime = deltaTime * 1000.0f;

	m_elapsedTime += deltaTime;
}

void GameManager::SetUpAndRenderNavMesh()
{

}

void GameManager::CreateLightSpaceMatrices()
{
	// TODO: Set up light space matrices for shadow mapping based on new model

	glm::vec3 sceneCenter = glm::vec3(500.0f / 2.0f, 0.0f, 500.0f / 2.0f);

	glm::vec3 lightDir = glm::normalize(dirLight.m_direction);

	float sceneDiagonal = (float)glm::sqrt(500.0f * 500.0f + 500.0f * 500.0f);

	m_lightSpaceProjection = glm::ortho(m_orthoLeft, m_orthoRight, m_orthoBottom, m_orthoTop, m_nearPlane, m_farPlane);

	glm::vec3 lightPos = sceneCenter - lightDir * sceneDiagonal;

	m_lightSpaceView = glm::lookAt(lightPos, sceneCenter, glm::vec3(0.0f, -1.0f, 0.0f));
	m_lightSpaceMatrix = m_lightSpaceProjection * m_lightSpaceView;
}

void GameManager::CheckGameOver()
{
	if ((m_enemy->IsDestroyed() && m_enemy2->IsDestroyed() && m_enemy3->IsDestroyed() && m_enemy4->IsDestroyed()) || m_player->IsDestroyed())
		ResetGame();
}

void GameManager::ResetGame()
{
	m_camera->SetMode(PLAYER_FOLLOW);
	m_audioManager->ClearQueue();
	m_player->SetPosition(m_player->GetInitialPos());
	m_player->SetYaw(m_player->GetInitialYaw());
	m_player->SetAnimNum(4);
	m_player->SetIsDestroyed(false);
	m_player->SetHealth(100.0f);
	m_player->UpdatePlayerVectors();
	m_player->UpdatePlayerAimVectors();
	m_player->SetPlayerState(PlayerState::MOVING);
	m_player->SetAabbColor(glm::vec3(0.0f, 0.0f, 1.0f));
	m_enemy->SetIsDestroyed(false);
	m_enemy2->SetIsDestroyed(false);
	m_enemy3->SetIsDestroyed(false);
	m_enemy4->SetIsDestroyed(false);
	m_enemy->SetIsDead(false);
	m_enemy2->SetIsDead(false);
	m_enemy3->SetIsDead(false);
	m_enemy4->SetIsDead(false);
	m_enemy->SetPositionOld(m_enemy->GetInitialPosition());
	m_enemy2->SetPositionOld(m_enemy2->GetInitialPosition());
	m_enemy3->SetPositionOld(m_enemy3->GetInitialPosition());
	m_enemy4->SetPositionOld(m_enemy4->GetInitialPosition());
	m_enemyStates = {
		{ false, false, 100.0f, 100.0f, false },
		{ false, false, 100.0f, 100.0f, false },
		{ false, false, 100.0f, 100.0f, false },
		{ false, false, 100.0f, 100.0f, false }
	};

	for (Enemy* emy : m_enemies)
	{
		emy->ResetState();
		m_physicsWorld->AddCollider(emy->GetAABB());
		m_physicsWorld->AddEnemyCollider(emy->GetAABB());
		emy->SetHealth(100.0f);
	}

}

static float frand() {
	return static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
}

bool findRandomNavMeshPoint(dtNavMeshQuery* navQuery, dtQueryFilter* filter, float outPos[3], dtPolyRef* outRef)
{
	if (!navQuery || !filter || !outPos || !outRef)
		return false;

	dtPolyRef startRef;
	float startPos[3];

	// 1. Find a starting point somewhere on the navmesh
	float center[3] = { 0.0f, 0.0f, 0.0f }; // Change to center of your map
	float extents[3] = { 500.0f, 100.0f, 500.0f }; // Generous search extents
	dtStatus status = navQuery->findNearestPoly(center, extents, filter, &startRef, startPos);

	if (dtStatusFailed(status)) {
		return false;
	}

	// 2. Generate a truly random point from that reference
	status = navQuery->findRandomPoint(filter, frand, outRef, outPos);

	return dtStatusSucceed(status);
}


void GameManager::RenderEnemyLineAndMuzzleFlash(bool isMainPass, bool isMinimapPass, bool isShadowPass)
{
	for (auto& enem : m_enemies)
	{
		if (!enem->IsDestroyed())
		{
			glm::vec3 enemyTracerEnd = glm::vec3(0.0f);

			int enemyID = enem->GetID();

			if (enem->GetEnemyHasShot())
			{
				float enemyMuzzleCurrentTime = (float)glfwGetTime();

				if (m_renderEnemyMuzzleFlash.at(enemyID) && m_enemyMuzzleFlashStartTimes.at(enemyID) + m_enemyMuzzleFlashDurations.at(enemyID) > enemyMuzzleCurrentTime)
				{
					m_renderEnemyMuzzleFlash.at(enemyID) = false;
				}
				else
				{
					m_renderEnemyMuzzleFlash.at(enemyID) = true;
					m_enemyMuzzleFlashStartTimes.at(enemyID) = enemyMuzzleCurrentTime;
				}

				if (m_renderEnemyMuzzleFlash.at(enemyID) && isMainPass)
				{
					m_enemyMuzzleTimesSinceStart.at(enemyID) = enemyMuzzleCurrentTime - m_enemyMuzzleFlashStartTimes.at(enemyID);
					m_enemyMuzzleAlphas.at(enemyID) = glm::max(0.0f, 1.0f - (m_enemyMuzzleTimesSinceStart.at(enemyID) / m_enemyMuzzleFlashDurations.at(enemyID)));
					m_enemyMuzzleFlashScales.at(enemyID) = 1.0f + (0.5f * m_enemyMuzzleAlphas.at(enemyID));

					m_enemyMuzzleModelMatrices.at(enemyID) = glm::mat4(1.0f);

					m_enemyMuzzleModelMatrices.at(enemyID) = glm::translate(m_enemyMuzzleModelMatrices.at(enemyID), enem->GetEnemyShootPos(m_muzzleOffset));
					m_enemyMuzzleModelMatrices.at(enemyID) = glm::rotate(m_enemyMuzzleModelMatrices.at(enemyID), enem->GetYaw(), glm::vec3(0.0f, 1.0f, 0.0f));
					m_enemyMuzzleModelMatrices.at(enemyID) = glm::scale(m_enemyMuzzleModelMatrices.at(enemyID), glm::vec3(m_enemyMuzzleFlashScales.at(enemyID), m_enemyMuzzleFlashScales.at(enemyID), 1.0f));
					m_enemyMuzzleFlashQuad->Draw3D(m_enemyMuzzleFlashTints.at(enemyID), m_enemyMuzzleAlphas.at(enemyID), m_projection, m_view, m_enemyMuzzleModelMatrices.at(enemyID));
				}

				if (enem->GetEnemyHasShot() && enem->GetEnemyDebugRayRenderTimer() > 0.0f)
				{
					if (enem->GetEnemyHasHit())
					{
						enemyTracerEnd = enem->GetEnemyHitPoint();
					}
					else
					{
						enemyTracerEnd = enem->GetEnemyShootPos(m_muzzleOffset) + enem->GetEnemyShootDir() * enem->GetEnemyShootDistance();
					}

					float enemyTracerCurrentTime = (float)glfwGetTime();

					if (m_renderEnemyTracer.at(enemyID) && m_enemyTracerStartTimes.at(enemyID) + m_enemyTracerDurations.at(enemyID) > enemyTracerCurrentTime)
					{
						m_renderEnemyTracer.at(enemyID) = false;
					}
					else
					{
						m_renderEnemyTracer.at(enemyID) = true;
						m_enemyTracerStartTimes.at(enemyID) = enemyTracerCurrentTime;
					}

					if (m_renderEnemyTracer.at(enemyID) && isMainPass)
					{
						m_enemyTracerTimesSinceStart.at(enemyID) = enemyTracerCurrentTime - m_enemyTracerStartTimes.at(enemyID);
						m_enemyTracerAlphas.at(enemyID) = glm::max(0.0f, 1.0f - (m_enemyTracerTimesSinceStart.at(enemyID) / m_enemyTracerDurations.at(enemyID)));
						m_enemyTracerScales.at(enemyID) = 1.0f + (0.5f * m_enemyTracerAlphas.at(enemyID));

						m_enemyTracerModelMatrices.at(enemyID) = glm::mat4(1.0f);

						glm::vec3 tracerDir = glm::normalize(enemyTracerEnd - enem->GetEnemyShootPos(m_tracerOffset));

						m_enemyTracerModelMatrices.at(enemyID) = glm::translate(m_enemyTracerModelMatrices.at(enemyID), enem->GetEnemyShootPos(m_tracerOffset));
						glm::quat tracerRotation = glm::rotation(glm::vec3(0.0f, 0.0f, 1.0f), tracerDir);
						m_enemyTracerModelMatrices.at(enemyID) *= glm::toMat4(tracerRotation);
						//m_enemyTracerModelMatrices.at(enemyID) = glm::rotate(m_enemyTracerModelMatrices.at(enemyID), (-enem->GetYaw() + 90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
						m_enemyTracerModelMatrices.at(enemyID) = glm::scale(m_enemyTracerModelMatrices.at(enemyID), glm::vec3(m_enemyTracerScales.at(enemyID), m_enemyTracerScales.at(enemyID), length(enemyTracerEnd - enem->GetEnemyShootPos(m_tracerOffset))));
						m_enemyTracerModelMatrices.at(enemyID) = glm::translate(m_enemyTracerModelMatrices.at(enemyID), glm::vec3(0.0f, 0.0f, 0.27f));

						m_enemyTracerQuad->Draw3D(m_enemyTracerTints.at(enemyID), m_enemyTracerAlphas.at(enemyID) - m_dt, m_projection, m_view, m_enemyTracerModelMatrices.at(enemyID));
					}
				}

			}
		}
	}
}

void GameManager::RenderPlayerCrosshairAndMuzzleFlash(bool isMainPass)
{
	if ((m_player->GetPlayerState() == AIMING || m_player->GetPlayerState() == SHOOTING) && m_camSwitchedToAim == false && isMainPass)
	{
		m_renderer->RemoveDepthAndSetBlending();

		glm::vec3 rayO = m_player->GetShootPos();
		glm::vec3 rayD = glm::normalize(m_player->GetPlayerAimFront());
		float dist = m_player->GetShootDistance();

		glm::vec3 rayEnd = rayO + rayD * dist;

		glm::vec3 lineColor = glm::vec3(1.0f, 0.0f, 0.0f);


		if (m_player->GetPlayerState() == SHOOTING)
		{
			lineColor = glm::vec3(0.0f, 1.0f, 0.0f);

			float currentTime = (float)glfwGetTime();

			if (m_renderPlayerMuzzleFlash && m_playerMuzzleFlashStartTime + m_playerMuzzleFlashDuration > currentTime)
			{
				m_renderPlayerMuzzleFlash = false;
			}
			else
			{
				m_renderPlayerMuzzleFlash = true;
				m_playerMuzzleFlashStartTime = currentTime;
			}


			if (m_renderPlayerMuzzleFlash)

			{
				m_playerMuzzleTimeSinceStart = currentTime - m_playerMuzzleFlashStartTime;
				m_playerMuzzleAlpha = glm::max(0.0f, 1.0f - (m_playerMuzzleTimeSinceStart / m_playerMuzzleFlashDuration));
				m_playerMuzzleFlashScale = 1.0f + (0.5f * m_playerMuzzleAlpha);

				m_playerMuzzleModel = glm::mat4(1.0f);

				m_playerMuzzleModel = glm::translate(m_playerMuzzleModel, m_player->GetShootPos());
				m_playerMuzzleModel = glm::rotate(m_playerMuzzleModel, (-m_player->GetYaw() + 90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
				m_playerMuzzleModel = glm::scale(m_playerMuzzleModel, glm::vec3(m_playerMuzzleFlashScale, m_playerMuzzleFlashScale, 1.0f));
				m_playerMuzzleFlashQuad->Draw3D(m_playerMuzzleTint, m_playerMuzzleAlpha, m_projection, m_view, m_playerMuzzleModel);
			}
		}


		glm::vec4 rayEndWorldSpace = glm::vec4(rayEnd, 1.0f);
		glm::vec4 rayEndCameraSpace = m_view * rayEndWorldSpace;
		glm::vec4 rayEndNDC = m_projection * rayEndCameraSpace;

		glm::vec4 targetNDC(0.0f, 0.5f, rayEndNDC.z / rayEndNDC.w, 1.0f);
		glm::vec4 targetCameraSpace = glm::inverse(m_projection) * targetNDC;
		glm::vec4 targetWorldSpace = glm::inverse(m_view) * targetCameraSpace;

		rayEnd = glm::vec3(targetWorldSpace) / targetWorldSpace.w;

		glm::vec3 crosshairHitpoint;
		glm::vec3 crosshairCol;

		auto clipCoords = glm::vec4(0.15f, 0.5f, 1.0f, 1.0f);

		glm::vec4 cameraCoords = inverse(m_projection) * clipCoords;
		cameraCoords /= cameraCoords.w;

		glm::vec4 worldCoords = inverse(m_view) * cameraCoords;
		rayEnd = glm::vec3(worldCoords) / worldCoords.w;

		rayD = normalize(rayEnd - rayO);

		if (m_physicsWorld->RayEnemyCrosshairIntersect(rayO, rayD, crosshairHitpoint))
		{
			crosshairCol = glm::vec3(1.0f, 0.0f, 0.0f);
		}
		else
		{
			crosshairCol = glm::vec3(1.0f, 1.0f, 1.0f);
		}

		glm::vec2 ndcPos = m_crosshair->CalculateCrosshairPosition(rayEnd, m_window->GetWidth(), m_window->GetHeight(), m_projection, m_view);

		float ndcX = (ndcPos.x / m_window->GetWidth()) * 2.0f - 1.0f;
		float ndcY = (ndcPos.y / m_window->GetHeight()) * 2.0f - 1.0f;

		if (isMainPass)
			m_crosshair->DrawCrosshair(glm::vec2(0.0f, 0.5f), crosshairCol);

		m_renderer->ResetRenderStates();
	}
}


void GameManager::Update(float deltaTime)
{
	m_inputManager->ProcessInput(m_window->GetWindow(), deltaTime);
	float pauseFactor = m_inputManager->GetPauseFactor();
	float timeScaleFactor = m_inputManager->GetTimeScaleFacotr();

	float scaledDeltaTime = deltaTime;


	bool isPaused = m_inputManager->GetIsPaused();
	bool isTimeScaled = m_inputManager->GetIsTimeScaled();

	if (isPaused)
		scaledDeltaTime *= pauseFactor;

	if (isTimeScaled)
		scaledDeltaTime *= timeScaleFactor;

	GetActiveScene()->OnUpdate(deltaTime);

	m_player->UpdatePlayerVectors();
	m_player->UpdatePlayerAimVectors();

	m_player->Update(scaledDeltaTime, isPaused, isTimeScaled);

	m_dt = scaledDeltaTime;

	float targetPos[3] = { m_player->GetPosition().x, m_player->GetPosition().y, m_player->GetPosition().z };

	int enemyID = 0;
	for (Enemy* e : m_enemies)
	{
		if (e == nullptr || e->IsDead())
			continue;

		e->SetDeltaTime(deltaTime);

		//if (!useEDBT)
		//{
		//	if (training)
		//	{
		//		e->EnemyDecision(enemyStates[e->GetID()], e->GetID(), squadActions, deltaTime, mEnemyStateQTable);
		//	}
		//	else
		//	{
		//		e->EnemyDecisionPrecomputedQ(enemyStates[e->GetID()], e->GetID(), squadActions, deltaTime, mEnemyStateQTable);
		//	}
		//}
//		e->Update(useEDBT, speedDivider, blendFac);


	/*	targetPos[0] = 0.0f;
		targetPos[1] = 0.0f;
		targetPos[2] = .0f;*/

		//float offset = 5.0f;
		//targetPos[0] += (e->GetID() % 3 - 1) * offset;
		//targetPos[2] += ((e->GetID() / 3) % 3 - 1) * offset;
		//float halfExtents2[3] = { 50.0f, 10.0f, 50.0f };
		//dtPolyRef playerPoly;
		//float targetPlayerPosOnNavMesh[3];
		//
		//navMeshQuery->findNearestPoly(targetPos, halfExtents2, &filter, &playerPoly, targetPlayerPosOnNavMesh);
		////	Logger::log(1, "Player position: %f %f %f\n", player->getPosition().x, player->getPosition().y, player->getPosition().z);
		//
		//std::vector<float* [3]> enemPos;
		//
		//float enemyPosition[3] = { e->GetPosition().x, e->GetPosition().y, e->GetPosition().z };
		//
		////DebugNavmeshConnectivity(navMeshQuery, navMesh, filter, targetPos, enemyPosition);
		//
		//float randPos[3];
		//dtPolyRef randRef;
		//
		////		findRandomNavMeshPoint(navMeshQuery, &filter, randPos, &randRef);
		//
		//dtPolyRef targetPoly;
		//float targetPosOnNavMesh[3];
		//
		//float jitterX = ((rand() % 100) / 100.0f - 0.5f) * 10.0f;
		//float jitterZ = ((rand() % 100) / 100.0f - 0.5f) * 10.0f;
		//
		//float target[3] = {
		//	targetPlayerPosOnNavMesh[0] + jitterX,
		//	targetPlayerPosOnNavMesh[1],
		//	targetPlayerPosOnNavMesh[2] + jitterZ
		//};
		//
		//if (!navMeshQuery)
		//{
		//}
		//
		//dtStatus status = navMeshQuery->findNearestPoly(
		//	targetPos, halfExtents2, &filter, &targetPoly, targetPlayerPosOnNavMesh
		//);
		//if (dtStatusFailed(status)) {
		//	continue;
		//}
		//
		//bool moveStatus = crowd->requestMoveTarget(e->GetID(), targetPoly, targetPlayerPosOnNavMesh);
		//
		//if (!moveStatus) {
		//	continue;
		//}


	}
	//crowd->update(deltaTime, nullptr);


	//for (Enemy* e : m_enemies)
	//{
	//
	//	const dtCrowdAgent* agent = crowd->getAgent(e->GetID());
	//	float agentPos[3];
	//	dtVcopy(agentPos, agent->npos);
	//	//	e->Update(false, speedDivider, blendFac);
	//	if ((agent->npos - targetPos) < glm::abs(32.0f)) {
	//		// stop steering and switch to idle/attack state
	//		crowd->resetMoveTarget(e->GetID());
	//	}
	//	e->SetPosition(glm::vec3(agentPos[0], agentPos[1], agentPos[2]));
	//}
	m_audioManager->Update(scaledDeltaTime);
	m_audioSystem->Update(scaledDeltaTime);

	CalculatePerformance(deltaTime);
}

void GameManager::Render(bool isMinimapRenderPass, bool isShadowMapRenderPass, bool isMainRenderPass)
{
	glEnable(GL_DEPTH_TEST);
	glDisable(GL_BLEND);

	m_renderer->ResetRenderStates();


	//bufferCreateInfo.initialData = matrices.data();

	//m_renderBackend->UpdateBuffer(h, 0, matrices.data(), 3 * sizeof(glm::mat4));
	//m_renderBackend->UploadCameraMatrices(h, matrices, 0);
	//m_renderBackend->CreateBuffer(bufferCreateInfo);

	if (!isShadowMapRenderPass)
	{
		m_renderer->ResetViewport(m_screenWidth, m_screenHeight);
		m_renderer->Clear();
	}

	if (isMinimapRenderPass)
		m_renderer->BindMinimapFbo(m_screenWidth, m_screenHeight);

	if (isShadowMapRenderPass)
		m_renderer->BindShadowMapFbo(SHADOW_WIDTH, SHADOW_HEIGHT);

	for (auto obj : m_gameObjects) {
		if (obj->IsDestroyed())
			continue;
		if (isMinimapRenderPass)
		{
			m_renderer->Draw(obj, m_minimapView, m_minimapProjection, m_camera->GetPosition(), false, m_lightSpaceMatrix);
		}
		else if (isShadowMapRenderPass)
		{
			m_renderer->Draw(obj, m_lightSpaceView, m_lightSpaceProjection, m_camera->GetPosition(), true, m_lightSpaceMatrix);
		}
		else
		{
			m_renderer->Draw(obj, m_view, m_projection, m_camera->GetPosition(), false, m_lightSpaceMatrix);
			
		}
	}

	glm::mat4 modelMat = glm::mat4(1.0f);

	std::vector<glm::mat4> matrixData;
	matrixData.push_back(m_view);
	matrixData.push_back(m_projection);
	modelMat = glm::translate(modelMat, glm::vec3(0.0f));
	modelMat = glm::scale(modelMat, glm::vec3(5.0f));

	matrixData.push_back(modelMat);
	m_renderBackend->UpdateBuffer(h, 0, matrixData.data(), 3 * sizeof(glm::mat4));
	m_renderBackend->BindUniformBuffer(h, 0);
		
	if (!isMinimapRenderPass && !isShadowMapRenderPass)
		RenderStaticModels(m_activeScene->GetRegistry(), *m_renderBackend, *uploader, pipes);




	//navMeshShader.Use();
	//navMeshShader.SetMat4("view", m_view);
	//navMeshShader.SetMat4("projection", m_projection);
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

	//glDisable(GL_CULL_FACE);
	////glEnable(GL_CULL_FACE);
	////glCullFace(GL_BACK);
	//glBindVertexArray(vao);
	//glDisable(GL_CULL_FACE);
	////glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	//glDrawElements(GL_TRIANGLES, navRenderMeshIndices.size(), GL_UNSIGNED_INT, 0);
	////glDrawArrays(GL_TRIANGLES, 0, navRenderMeshVertices.size() / 3);
//	//glDrawElements(GL_TRIANGLES, navMesh.size(), GL_UNSIGNED_INT, 0);
	////glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	////glDisable(GL_POLYGON_OFFSET_FILL);
	//glBindVertexArray(0);
	//glDisable(GL_CULL_FACE);
	//
	//
	//hfnavMeshShader.Use();
	//hfnavMeshShader.SetMat4("view", m_view);
	//hfnavMeshShader.SetMat4("projection", m_projection);
	//
	////	glDisable(GL_CULL_FACE);
	////	//glEnable(GL_CULL_FACE);
	////	//glCullFace(GL_BACK);
	////	glBindVertexArray(hfvao);
	////	glDisable(GL_CULL_FACE);
	////	//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	////	glDrawElements(GL_TRIANGLES, hfnavRenderMeshIndices.size(), GL_UNSIGNED_INT, 0);
	////	//glDrawArrays(GL_TRIANGLES, 0, navMeshVertices.size() / 3);
	//////	glDrawElements(GL_TRIANGLES, navMesh.size(), GL_UNSIGNED_INT, 0);
	////	//glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	////	//glDisable(GL_POLYGON_OFFSET_FILL);
	////	glBindVertexArray(0);
	////	glDisable(GL_CULL_FACE);


	if (m_camSwitchedToAim)
		m_camSwitchedToAim = false;

	RenderEnemyLineAndMuzzleFlash(isMainRenderPass, isMinimapRenderPass, isShadowMapRenderPass);

	m_renderer->DrawCubemap(m_cubemap);

	RenderPlayerCrosshairAndMuzzleFlash(isMainRenderPass);

	if (isMainRenderPass)
	{
		m_renderer->DrawMinimap(m_minimapQuad, &m_minimapShader);
	}

#ifdef _DEBUG
	if (isMainRenderPass)
	{
		m_renderer->DrawShadowMap(m_shadowMapQuad, &m_shadowMapQuadShader);
	}
#endif

	if (isMinimapRenderPass)
	{
		m_renderer->UnbindMinimapFbo();
	}
	else if (isShadowMapRenderPass)
	{
		m_renderer->UnbindShadowMapFbo();
	}

}