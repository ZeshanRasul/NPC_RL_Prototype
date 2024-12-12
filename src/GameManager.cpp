#include "GameManager.h"
#include "Components/AudioComponent.h"

#include "imgui/imgui.h"
#include "imgui/backend/imgui_impl_glfw.h"
#include "imgui/backend/imgui_impl_opengl3.h"

DirLight dirLight = {
		glm::vec3(-3.0f, -2.0f, -3.0f),

		glm::vec3(0.15f, 0.2f, 0.25f),
		glm::vec3(7.0f),
		glm::vec3(0.8f, 0.9f, 1.0f)
};

glm::vec3 dirLightPBRColour = glm::vec3(10.f, 10.0f, 10.0f);

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
	enemyShadowMapShader.loadShaders("C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Shaders/shadow_map_enemy_vertex.glsl", "C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Shaders/shadow_map_fragment.glsl");
	shadowMapQuadShader.loadShaders("C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Shaders/shadow_map_quad_vertex.glsl", "C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Shaders/shadow_map_quad_fragment.glsl");
	playerMuzzleFlashShader.loadShaders("C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Shaders/muzzle_flash_vertex.glsl", "C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Shaders/muzzle_flash_fragment.glsl");
	navMeshShader.loadShaders("C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Shaders/navmesh_vert.glsl", "C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Shaders/navmesh_frag.glsl");

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
		Cube* cover = new Cube(gameGrid->snapToGrid(coverPos), glm::vec3((float)gameGrid->GetCellSize()), &cubeShader, &shadowMapShader, false, this, cubeTexFilename);
		cover->SetAABBShader(&aabbShader);
		cover->LoadMesh();
		coverSpots.push_back(cover);
	}

	size_t vertexOffset = 0;

	for (Cube* coverSpot : coverSpots)
	{
		for (glm::vec3 coverPosVerts : coverSpot->GetPositionVertices())
		{
			navMeshVertices.push_back(coverPosVerts.x);
			navMeshVertices.push_back(coverPosVerts.y);
			navMeshVertices.push_back(coverPosVerts.z);
		}

		GLuint* indices = coverSpot->GetIndices();

		int numIndices = sizeof(indices) / sizeof(GLuint);

		for (int i = 0; i < numIndices; i++)
		{
			navMeshIndices.push_back(indices[i] + vertexOffset);
		}

		vertexOffset += coverSpot->GetPositionVertices().size();
	}

	gameGrid->initializeGrid();

	for (glm::vec3 gridCellPosVerts : gameGrid->GetWSVertices())
	{
		navMeshVertices.push_back(gridCellPosVerts.x);
		navMeshVertices.push_back(gridCellPosVerts.y);
		navMeshVertices.push_back(gridCellPosVerts.z);
	}

	for (int index : gameGrid->GetIndices())
	{
		navMeshIndices.push_back(index + vertexOffset);
	}

	int indexCount = (int)navMeshIndices.size();
	triIndices = new int[indexCount];

	for (int i = 0; i < indexCount; i++)
	{
		triIndices[i] = navMeshIndices[i];
	}

	triAreas = new unsigned char[indexCount / 3];

	Logger::log(1, "Tri Areas: %s", triAreas);

	ctx = new rcContext();

	rcConfig cfg{};

	cfg.cs = 0.3f;                      // Cell size
	cfg.ch = 0.2f;                      // Cell height
	cfg.walkableSlopeAngle = 45.0f;     // Allow steeper slopes
	cfg.walkableHeight = 7.0f;          // Agent height (in grid units)
	cfg.walkableClimb = 3.0f;           // Max climb height
	cfg.walkableRadius = 3.0f;          // Agent radius
	cfg.maxEdgeLen = 24;                // Larger edge length
	cfg.minRegionArea = 8;  // Keep smaller regions
	cfg.maxSimplificationError = 0.5f;  // Fine simplification
	cfg.detailSampleDist = cfg.cs * 6;  // Balance detail
	cfg.maxVertsPerPoly = 6;			// Fewer verts per poly
	cfg.tileSize = 32;					// Tile size
	cfg.borderSize = (int)(cfg.walkableRadius / cfg.cs + 0.5f);
	cfg.width = cfg.tileSize + cfg.borderSize * 2;
	cfg.height = cfg.tileSize + cfg.borderSize * 2;

	float minBounds[3] = { 0.0f, 0.0f, 0.0f };
	float maxBounds[3] = { gameGrid->GetCellSize() * gameGrid->GetGridSize() * 1000.0f, 2.0f, gameGrid->GetCellSize() * gameGrid->GetGridSize() * 1000.0f};

	heightField = rcAllocHeightfield();
	if (!rcCreateHeightfield(ctx, *heightField, gameGrid->GetCellSize() * gameGrid->GetGridSize() * 100.0f, gameGrid->GetCellSize() * gameGrid->GetGridSize() * 100.0f, minBounds, maxBounds, cfg.cs, cfg.ch))
	{
		Logger::log(1, "%s error: Could not create heightfield\n", __FUNCTION__);
	}
	else
	{
		Logger::log(1, "%s: Heightfield successfully created\n", __FUNCTION__);
	};

	int numTris = indexCount / 3;

	Logger::log(1, "Triangle indices: %d\n", indexCount);
	Logger::log(1, "Triangle count: %d\n", numTris);

	rcMarkWalkableTriangles(ctx, cfg.walkableSlopeAngle, navMeshVertices.data(), navMeshVertices.size(), triIndices, numTris, triAreas);
	if (!rcRasterizeTriangles(ctx, navMeshVertices.data(), navMeshVertices.size(), triIndices, triAreas, numTris, *heightField, cfg.walkableClimb))
	{
		Logger::log(1, "%s error: Could not rasterize triangles\n", __FUNCTION__);
	}
	else
	{
		Logger::log(1, "%s: rasterize triangles successfully created\n", __FUNCTION__);
	};

	rcFilterLowHangingWalkableObstacles(ctx, cfg.walkableClimb, *heightField);
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

	dtNavMeshCreateParams params;
	params.verts = polyMesh->verts;
	params.vertCount = polyMesh->nverts;
	params.polys = polyMesh->polys;
	params.polyAreas = polyMesh->areas;
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

	dtStatus status = navMeshQuery->init(navMesh, 2048);
	if (dtStatusFailed(status))
	{
		Logger::log(1, "%s error: Could not init Detour navMeshQuery\n", __FUNCTION__);
	} 
	else
	{
		Logger::log(1, "%s: Detour navMeshQuery successfully initialized\n", __FUNCTION__);

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


	player = new Player(gameGrid->snapToGrid(glm::vec3(90.0f, 0.0f, 25.0f)), glm::vec3(3.0f), &playerShader, &playerShadowMapShader, true, this);
	//	player = new Player(gameGrid->snapToGrid(glm::vec3(23.0f, 0.0f, 37.0f)), glm::vec3(3.0f), &playerShader, &playerShadowMapShader, true, this);

	player->aabbShader = &aabbShader;

	std::string texture = "C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Assets/Models/GLTF/Enemies/Ely/EnemyEly_ely_vanguardsoldier_kerwinatienza_M2_BaseColor.png";
	std::string texture2 = "C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Assets/Models/GLTF/Enemies/Ely/ely-vanguardsoldier-kerwinatienza_diffuse_2.png";
	std::string texture3 = "C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Assets/Models/GLTF/Enemies/Ely/ely-vanguardsoldier-kerwinatienza_diffuse_3.png";
	std::string texture4 = "C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Assets/Models/GLTF/Enemies/Ely/ely-vanguardsoldier-kerwinatienza_diffuse_4.png";

	enemy = new Enemy(gameGrid->snapToGrid(glm::vec3(33.0f, 0.0f, 23.0f)), glm::vec3(3.0f), &enemyShader, &enemyShadowMapShader, true, this, gameGrid, texture, 0, GetEventManager(), *player);
	enemy->SetAABBShader(&aabbShader);
	enemy->SetUpAABB();

	enemy2 = new Enemy(gameGrid->snapToGrid(glm::vec3(3.0f, 0.0f, 53.0f)), glm::vec3(3.0f), &enemyShader, &enemyShadowMapShader, true, this, gameGrid, texture2, 1, GetEventManager(), *player);
	enemy2->SetAABBShader(&aabbShader);
	enemy2->SetUpAABB();

	enemy3 = new Enemy(gameGrid->snapToGrid(glm::vec3(43.0f, 0.0f, 53.0f)), glm::vec3(3.0f), &enemyShader, &enemyShadowMapShader, true, this, gameGrid, texture3, 2, GetEventManager(), *player);
	enemy3->SetAABBShader(&aabbShader);
	enemy3->SetUpAABB();

	enemy4 = new Enemy(gameGrid->snapToGrid(glm::vec3(11.0f, 0.0f, 23.0f)), glm::vec3(3.0f), &enemyShader, &enemyShadowMapShader, true, this, gameGrid, texture4, 3, GetEventManager(), *player);
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

	for (Cube* coverSpot : coverSpots)
	{
		gameObjects.push_back(coverSpot);
	}

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

	mMusicEvent = audioSystem->PlayEvent("event:/bgm");

	for (int i = 0; i < polyMesh->nverts; ++i) {
		const unsigned short* v = &polyMesh->verts[i * 3];
		navmeshVertices.push_back(v[0] * cfg.cs); // X
		navmeshVertices.push_back(v[1] * cfg.ch); // Y
		navmeshVertices.push_back(v[2] * cfg.cs); // Z
	}
	for (int i = 0; i < polyMesh->npolys; ++i) {
		for (int j = 0; j < polyMesh->nvp; ++j) {
			unsigned short index = polyMesh->polys[i * polyMesh->nvp + j];
			if (index == RC_MESH_NULL_IDX) break;
			navmeshIndices.push_back(static_cast<unsigned int>(index));
		}
	}
	// Create VAO
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	// Create VBO
	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, navmeshVertices.size() * sizeof(float), navmeshVertices.data(), GL_STATIC_DRAW);

	// Create EBO
	glGenBuffers(1, &ebo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, navmeshIndices.size() * sizeof(int), navmeshIndices.data(), GL_STATIC_DRAW);

	// Enable vertex attribute (e.g., position at location 0)
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	glBindVertexArray(0);

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

	projection = glm::perspective(glm::radians(camera->Zoom), (float)width / (float)height, 0.1f, 500.0f);

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

	ImGui::InputFloat3("Position", &player->getPosition()[0]);
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

		e->Update(useEDBT, speedDivider, blendFac);
	}

	mAudioManager->Update(deltaTime);
	audioSystem->Update(deltaTime);

	calculatePerformance(deltaTime);
}

void GameManager::render(bool isMinimapRenderPass, bool isShadowMapRenderPass, bool isMainRenderPass)
{
	glEnable(GL_DEPTH_TEST);
	glDisable(GL_BLEND);

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
		gameGrid->drawGrid(gridShader, minimapView, minimapProjection, camera->Position, false, lightSpaceMatrix, renderer->GetShadowMapTexture());
	}
	else if (isShadowMapRenderPass)
	{
		gameGrid->drawGrid(shadowMapShader, lightSpaceView, lightSpaceProjection, camera->Position, true, lightSpaceMatrix, renderer->GetShadowMapTexture());
		navMeshShader.use();
		navMeshShader.setMat4("view", view);
		navMeshShader.setMat4("projection", projection);
		glBindVertexArray(vao);
		glDrawElements(GL_TRIANGLES, navmeshIndices.size(), GL_UNSIGNED_INT, 0);
		glBindVertexArray(0);
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		glDrawElements(GL_TRIANGLES, navmeshIndices.size(), GL_UNSIGNED_INT, 0);
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}
	else
	{
		gameGrid->drawGrid(gridShader, view, projection, camera->Position, false, lightSpaceMatrix, renderer->GetShadowMapTexture());
		navMeshShader.use();
		navMeshShader.setMat4("view", view);
		navMeshShader.setMat4("projection", projection);
		glBindVertexArray(vao);
		glDrawElements(GL_TRIANGLES, navmeshIndices.size(), GL_UNSIGNED_INT, 0);
		glBindVertexArray(0);
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		glDrawElements(GL_TRIANGLES, navmeshIndices.size(), GL_UNSIGNED_INT, 0);
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}

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