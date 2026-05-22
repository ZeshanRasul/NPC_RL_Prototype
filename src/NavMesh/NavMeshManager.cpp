#include "NavMeshManager.h"

#include <fstream>
#include <cstring>
#include <cstdlib>
#include <algorithm>

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "GameObjects/Enemy.h"
#include "Tools/Logger.h"

// ---------------------------------------------------------------------------
// File format constants and structs
// ---------------------------------------------------------------------------

static constexpr uint32_t NAVM_MAGIC   = 0x4D56414E; // 'NAVM'
static constexpr uint16_t NAVM_VERSION = 1;

static constexpr float AGENT_RADIUS = 0.6f;

#pragma pack(push, 1)
struct NavBinHeader {
    uint32_t        magic;
    uint16_t        version;
    uint16_t        flags;
    dtNavMeshParams params;
    uint32_t        tileCount;
    uint32_t        reserved;
};

struct TileRecordHeader {
    int32_t  x, y, layer;
    uint32_t dataSize;
};
#pragma pack(pop)

#pragma pack(push, 1)
struct PMagic  { uint32_t v = 0x53484D50; }; // 'PMHS'
struct PMHeader {
    uint32_t magic;
    uint16_t version;
    uint16_t reserved;
    uint32_t meshCount;
};
struct PMeshRecHeader {
    uint16_t nvp;
    uint16_t reserved;
    float    cs, ch;
    float    bmin[3];
    float    bmax[3];
    uint32_t nverts;
    uint32_t npolys;
    // Followed by: uint16_t verts[3*nverts], uint16_t polys[2*nvp*npolys]
};
#pragma pack(pop)

struct LoadedPolyMesh {
    uint16_t nvp;
    float    cs, ch;
    float    bmin[3], bmax[3];
    uint32_t nverts = 0, npolys = 0;
    std::vector<unsigned short> verts;
    std::vector<unsigned short> polys;
    std::vector<unsigned short> flags;
    std::vector<unsigned char>  areas;
};

// ---------------------------------------------------------------------------
// Internal helpers (file-local)
// ---------------------------------------------------------------------------

static bool SavePolyMeshes(const char* path, const std::vector<rcPolyMesh*>& meshes,
                            bool saveFlagsAreas = false)
{
    std::ofstream f(path, std::ios::binary);
    if (!f) return false;

    PMHeader hdr{};
    hdr.magic     = 0x53484D50;
    hdr.version   = 1;
    hdr.meshCount = (uint32_t)meshes.size();
    f.write((const char*)&hdr, sizeof(hdr));

    for (const rcPolyMesh* pm : meshes) {
        if (!pm) continue;
        PMeshRecHeader rh{};
        rh.nvp    = (uint16_t)pm->nvp;
        rh.cs     = pm->cs;
        rh.ch     = pm->ch;
        rh.bmin[0] = pm->bmin[0]; rh.bmin[1] = pm->bmin[1]; rh.bmin[2] = pm->bmin[2];
        rh.bmax[0] = pm->bmax[0]; rh.bmax[1] = pm->bmax[1]; rh.bmax[2] = pm->bmax[2];
        rh.nverts  = (uint32_t)pm->nverts;
        rh.npolys  = (uint32_t)pm->npolys;
        f.write((const char*)&rh, sizeof(rh));
        f.write((const char*)pm->verts, sizeof(unsigned short) * 3 * pm->nverts);
        f.write((const char*)pm->polys, sizeof(unsigned short) * 2 * pm->nvp * pm->npolys);
        if (saveFlagsAreas) {
            f.write((const char*)pm->flags, sizeof(unsigned short) * pm->npolys);
            f.write((const char*)pm->areas, sizeof(unsigned char) * pm->npolys);
        }
    }
    return true;
}

static bool LoadPolyMeshes(const char* path, std::vector<LoadedPolyMesh>& out,
                            bool hasFlagsAreas = false)
{
    std::ifstream f(path, std::ios::binary);
    if (!f) return false;

    PMHeader hdr{};
    f.read((char*)&hdr, sizeof(hdr));
    if (!f || hdr.magic != 0x53484D50 || hdr.version != 1) return false;

    out.clear();
    out.reserve(hdr.meshCount);

    for (uint32_t m = 0; m < hdr.meshCount; ++m) {
        PMeshRecHeader rh{};
        f.read((char*)&rh, sizeof(rh));
        if (!f) return false;

        LoadedPolyMesh pm{};
        pm.nvp    = rh.nvp;
        pm.cs     = rh.cs;
        pm.ch     = rh.ch;
        pm.bmin[0] = rh.bmin[0]; pm.bmin[1] = rh.bmin[1]; pm.bmin[2] = rh.bmin[2];
        pm.bmax[0] = rh.bmax[0]; pm.bmax[1] = rh.bmax[1]; pm.bmax[2] = rh.bmax[2];
        pm.nverts  = rh.nverts;
        pm.npolys  = rh.npolys;

        pm.verts.resize(3 * pm.nverts);
        pm.polys.resize(2 * pm.nvp * pm.npolys);
        f.read((char*)pm.verts.data(), sizeof(unsigned short) * pm.verts.size());
        f.read((char*)pm.polys.data(), sizeof(unsigned short) * pm.polys.size());
        if (!f) return false;

        if (hasFlagsAreas) {
            pm.flags.resize(pm.npolys);
            pm.areas.resize(pm.npolys);
            f.read((char*)pm.flags.data(), sizeof(unsigned short) * pm.npolys);
            f.read((char*)pm.areas.data(), sizeof(unsigned char)  * pm.npolys);
            if (!f) return false;
        }
        out.push_back(std::move(pm));
    }
    return true;
}

static bool SaveDetourNavMesh(dtNavMesh* nav, const char* path, int tileCountX, int tileCountY, int& tileCount)
{
    if (!nav) return false;
    std::ofstream f(path, std::ios::binary);
    if (!f) return false;

    tileCount = 0;
    for (int ty = 0; ty < tileCountY; ++ty)
        for (int tx = 0; tx < tileCountX; ++tx) {
            const dtMeshTile* t = nav->getTileAt(tx, ty, 0);
            if (t && t->header && t->data && t->dataSize) ++tileCount;
        }

    NavBinHeader hdr{};
    hdr.magic     = NAVM_MAGIC;
    hdr.version   = NAVM_VERSION;
    hdr.flags     = 0;
    hdr.params    = *nav->getParams();
    hdr.tileCount = (uint32_t)tileCount;
    f.write(reinterpret_cast<const char*>(&hdr), sizeof(hdr));

    for (int ty = 0; ty < tileCountY; ++ty)
        for (int tx = 0; tx < tileCountX; ++tx) {
            const dtMeshTile* t = nav->getTileAt(tx, ty, 0);
            if (!t || !t->header || !t->data || !t->dataSize) continue;
            TileRecordHeader th{};
            th.x        = t->header->x;
            th.y        = t->header->y;
            th.layer    = (int32_t)t->header->layer;
            th.dataSize = t->dataSize;
            f.write(reinterpret_cast<const char*>(&th), sizeof(th));
            f.write(reinterpret_cast<const char*>(t->data), th.dataSize);
        }
    return true;
}

static dtNavMesh* LoadDetourNavMesh(const char* path)
{
    Logger::Log(1, "Loading NavMesh from %s\n", path);
    std::ifstream f(path, std::ios::binary);
    if (!f) return nullptr;

    NavBinHeader hdr{};
    f.read(reinterpret_cast<char*>(&hdr), sizeof(hdr));
    if (!f || hdr.magic != NAVM_MAGIC || hdr.version != NAVM_VERSION) return nullptr;

    dtNavMesh* nav = dtAllocNavMesh();
    if (!nav) return nullptr;

    dtNavMeshParams params;
    memcpy(&params, &hdr.params, sizeof(dtNavMeshParams));
    nav->init(&params);

    for (uint32_t i = 0; i < hdr.tileCount; ++i) {
        Logger::Log(1, "Loading tile %d / %d\n", i + 1, hdr.tileCount);
        TileRecordHeader th{};
        f.read(reinterpret_cast<char*>(&th), sizeof(th));
        if (!f) { dtFree(nav); return nullptr; }
        if (th.dataSize == 0) continue;

        unsigned char* data = (unsigned char*)dtAlloc(th.dataSize, DT_ALLOC_PERM);
        if (!data) { dtFree(nav); return nullptr; }

        f.read(reinterpret_cast<char*>(data), th.dataSize);
        if (!f) { dtFree(data); dtFree(nav); return nullptr; }

        dtTileRef ref = 0;
        dtStatus st = nav->addTile(data, th.dataSize, DT_TILE_FREE_DATA, ref, nullptr);
        if (dtStatusFailed(st)) { dtFree(data); dtFree(nav); return nullptr; }
    }

    Logger::Log(1, "NavMesh loaded from %s\n", path);
    return nav;
}

// ---------------------------------------------------------------------------
// NavMeshManager
// ---------------------------------------------------------------------------

NavMeshManager::NavMeshManager() = default;

NavMeshManager::~NavMeshManager()
{
    delete[] m_triIndices;
    delete[] m_triAreas;

    for (auto* hf  : m_heightFields)       rcFreeHeightField(hf);
    for (auto* chf : m_compactHeightFields) rcFreeCompactHeightfield(chf);
    for (auto* cs  : m_contourSets)         rcFreeContourSet(cs);
    for (auto* pm  : m_polyMeshes)          rcFreePolyMesh(pm);
    for (auto* dm  : m_polyMeshDetails)     rcFreePolyMeshDetail(dm);

    dtFreeNavMeshQuery(m_navMeshQuery);
    dtFreeNavMesh(m_navMesh);
    dtFreeCrowd(m_crowd);

    if (m_vao) glDeleteVertexArrays(1, &m_vao);
    if (m_vbo) glDeleteBuffers(1, &m_vbo);
    if (m_ebo) glDeleteBuffers(1, &m_ebo);
}

void NavMeshManager::Build(const std::vector<float>& vertices,
                            const std::vector<unsigned int>& indices,
                            const std::string& shaderVert,
                            const std::string& shaderFrag)
{
    m_vertices = vertices;
    m_indices  = indices;

    m_shader.LoadShaders(shaderVert.c_str(), shaderFrag.c_str());

    // -----------------------------------------------------------------------
    // Build triIndices / triAreas arrays expected by Recast
    // -----------------------------------------------------------------------
    int indexCount   = (int)m_indices.size();
    int triangleCount = indexCount / 3;
    int vertexCount   = (int)m_vertices.size() / 3;

    m_triIndices = new int[indexCount];
    for (int i = 0; i < indexCount; ++i)
        m_triIndices[i] = (int)m_indices[i];

    m_triAreas = new unsigned char[triangleCount];

    m_filter.setIncludeFlags(0xFFFF);
    m_filter.setExcludeFlags(0);

    // Validate indices
    for (int i = 0; i < triangleCount * 3; ++i) {
        if (m_triIndices[i] < 0 || m_triIndices[i] >= vertexCount)
            Logger::Log(1, "Invalid triangle index %d: %d (vertexCount=%d)\n",
                        i, m_triIndices[i], vertexCount);
    }

    // -----------------------------------------------------------------------
    // rcConfig
    // -----------------------------------------------------------------------
    const float WALKABLE_SLOPE = 45.0f;

    rcConfig cfg{};
    cfg.cs                   = 0.3f;
    cfg.ch                   = 0.2f;
    cfg.walkableSlopeAngle   = WALKABLE_SLOPE;
    cfg.walkableHeight       = (int)ceilf(2.0f / cfg.ch);
    cfg.walkableClimb        = (int)floorf(0.4f / cfg.ch);
    cfg.walkableRadius       = (int)ceilf(AGENT_RADIUS / cfg.cs);
    cfg.maxEdgeLen           = (int)(12.0f / cfg.cs);
    cfg.minRegionArea        = rcSqr(12);
    cfg.mergeRegionArea      = rcSqr(30);
    cfg.maxSimplificationError = 0.1f;
    cfg.detailSampleDist     = cfg.cs * 6;
    cfg.maxVertsPerPoly      = 6;
    cfg.tileSize             = 248;

    rcMarkWalkableTriangles(&m_ctx, WALKABLE_SLOPE,
        m_vertices.data(), vertexCount,
        m_triIndices, triangleCount, m_triAreas);

    rcCalcBounds(m_vertices.data(), vertexCount, cfg.bmin, cfg.bmax);
    rcCalcGridSize(cfg.bmin, cfg.bmax, cfg.cs, &cfg.width, &cfg.height);
    cfg.borderSize = cfg.walkableRadius + 3;

    int mapVoxelsX = (int)((cfg.bmax[0] - cfg.bmin[0]) / cfg.cs + 0.5f);
    int mapVoxelsZ = (int)((cfg.bmax[2] - cfg.bmin[2]) / cfg.cs + 0.5f);

    m_tileCountX = (mapVoxelsX + cfg.tileSize - 1) / cfg.tileSize;
    m_tileCountY = (mapVoxelsZ + cfg.tileSize - 1) / cfg.tileSize;

    m_tileWorldSize = cfg.tileSize * cfg.cs;

    dtNavMeshParams params{};
    params.orig[0]    = floor(cfg.bmin[0] / m_tileWorldSize) * m_tileWorldSize;
    params.orig[1]    = cfg.bmin[1];
    params.orig[2]    = floor(cfg.bmin[2] / m_tileWorldSize) * m_tileWorldSize;
    params.tileWidth  = m_tileWorldSize;
    params.tileHeight = m_tileWorldSize;
    params.maxTiles   = 100 * 100;
    params.maxPolys   = 2048;

    // -----------------------------------------------------------------------
    // Build or load navmesh
    // -----------------------------------------------------------------------
    if (m_saveNavMesh) {
        m_navMesh = dtAllocNavMesh();
        m_navMesh->init(&params);

        for (int y = 0; y < m_tileCountY; ++y)
            for (int x = 0; x < m_tileCountX; ++x) {
                unsigned char* navData    = nullptr;
                int            navDataSz  = 0;
                if (BuildTile(x, y, cfg.bmin, cfg.bmax, cfg, navData, &navDataSz, params)) {
                    dtStatus st = m_navMesh->addTile(navData, navDataSz, DT_TILE_FREE_DATA, 0, nullptr);
                    if (dtStatusFailed(st))
                        Logger::Log(1, "addTile failed (%d,%d): status=%x\n", x, y, st);
                }
            }

        SaveDetourNavMesh(m_navMesh, "Level01.navbin", m_tileCountX, m_tileCountY, m_tileCount);
        SavePolyMeshes("Level01.polymesh", m_polyMeshes, false);
    }

    if (m_loadNavMesh) {
        m_navMesh = LoadDetourNavMesh("Level01.navbin");

        std::vector<LoadedPolyMesh> loaded;
        if (LoadPolyMeshes("Level01.polymesh", loaded)) {
            for (auto& lpm : loaded) {
                rcPolyMesh* polyMesh = rcAllocPolyMesh();
                polyMesh->nvp    = lpm.nvp;
                polyMesh->cs     = lpm.cs;
                polyMesh->ch     = lpm.ch;
                polyMesh->bmin[0] = lpm.bmin[0]; polyMesh->bmin[1] = lpm.bmin[1]; polyMesh->bmin[2] = lpm.bmin[2];
                polyMesh->bmax[0] = lpm.bmax[0]; polyMesh->bmax[1] = lpm.bmax[1]; polyMesh->bmax[2] = lpm.bmax[2];
                polyMesh->nverts  = lpm.nverts;
                polyMesh->npolys  = lpm.npolys;

                const float* orig = polyMesh->bmin;
                size_t baseIdx    = m_renderVertices.size() / 3;

                for (int i = 0; i < (int)polyMesh->nverts; ++i) {
                    const unsigned short* v = &lpm.verts[i * 3];
                    m_renderVertices.push_back(orig[0] + v[0] * polyMesh->cs);
                    m_renderVertices.push_back(orig[1] + v[1] * polyMesh->ch);
                    m_renderVertices.push_back(orig[2] + v[2] * polyMesh->cs);
                }

                for (int i = 0; i < (int)polyMesh->npolys; ++i) {
                    const unsigned short* p = &lpm.polys[i * lpm.nvp * 2];
                    for (int j = 2; j < polyMesh->nvp; ++j) {
                        if (p[j] == RC_MESH_NULL_IDX) break;
                        if (p[0] == p[j-1] || p[0] == p[j] || p[j-1] == p[j]) continue;
                        m_renderIndices.push_back((unsigned int)(baseIdx + p[0]));
                        m_renderIndices.push_back((unsigned int)(baseIdx + p[j-1]));
                        m_renderIndices.push_back((unsigned int)(baseIdx + p[j]));
                    }
                }

                m_polyMeshes.push_back(polyMesh);
            }
        }
    }

    // -----------------------------------------------------------------------
    // NavMesh query
    // -----------------------------------------------------------------------
    m_navMeshQuery = dtAllocNavMeshQuery();
    if (dtStatusFailed(m_navMeshQuery->init(m_navMesh, 4096)))
        Logger::Log(1, "NavMeshManager: NavMeshQuery init failed\n");
    else
        Logger::Log(1, "NavMeshManager: NavMeshQuery initialized\n");

    m_filter.setIncludeFlags(0x01);
    m_filter.setExcludeFlags(0);

    UploadRenderMesh();
}

void NavMeshManager::InitCrowd(const std::vector<Enemy*>& enemies)
{
    m_crowd = dtAllocCrowd();
    m_crowd->init((int)enemies.size(), AGENT_RADIUS, m_navMesh);

    for (Enemy* e : enemies) {
        dtCrowdAgentParams ap{};
        ap.radius                = AGENT_RADIUS;
        ap.height                = 1.0f;
        ap.maxSpeed              = 4.0f;
        ap.maxAcceleration       = 12.0f;
        ap.collisionQueryRange   = AGENT_RADIUS * 6.0f;
        ap.pathOptimizationRange = AGENT_RADIUS * 15.0f;
        ap.updateFlags           = DT_CROWD_ANTICIPATE_TURNS
                                 | DT_CROWD_OPTIMIZE_VIS
                                 | DT_CROWD_OPTIMIZE_TOPO
                                 | DT_CROWD_SEPARATION;
        ap.separationWeight = 0.5f;

        float startPos[3] = { e->GetPosition().x, e->GetPosition().y, e->GetPosition().z };
        float snapped[3]  = {};
        dtPolyRef startPoly = 0;

        dtStatus st = m_navMeshQuery->findNearestPoly(startPos, m_halfExtents, &m_filter, &startPoly, snapped);
        if (dtStatusFailed(st))
            Logger::Log(1, "[NavMesh] Enemy %d: no poly near spawn\n", e->GetID());

        if (e->GetID() == 6)
            e->SetPosition(glm::vec3(snapped[0], snapped[1] + 2.0f, snapped[2]));
        else
            e->SetPosition(glm::vec3(snapped[0], snapped[1], snapped[2]));

        int agentID = m_crowd->addAgent(snapped, &ap);
        Logger::Log(1, "[NavMesh] Enemy %d spawned as agent %d at (%.2f, %.2f, %.2f)\n",
                    e->GetID(), agentID, snapped[0], snapped[1], snapped[2]);
    }
}

void NavMeshManager::Update(float deltaTime,
                             const std::vector<Enemy*>& enemies,
                             const glm::vec3& playerPos)
{
    if (!m_navMeshQuery || !m_crowd) return;

    const float halfExtents[3] = { 50.0f, 10.0f, 50.0f };

    for (Enemy* e : enemies) {
        if (!e || e->IsDead()) continue;

        float offset = 5.0f;
        float targetPos[3] = {
            playerPos.x + (e->GetID() % 3 - 1) * offset,
            playerPos.y,
            playerPos.z + ((e->GetID() / 3) % 3 - 1) * offset
        };

        float jitterX = ((rand() % 100) / 100.0f - 0.5f) * 10.0f;
        float jitterZ = ((rand() % 100) / 100.0f - 0.5f) * 10.0f;

        dtPolyRef targetPoly = 0;
        float     targetOnMesh[3] = {};

        dtStatus st = m_navMeshQuery->findNearestPoly(
            targetPos, halfExtents, &m_filter, &targetPoly, targetOnMesh);

        if (dtStatusFailed(st)) continue;

        float jitteredTarget[3] = {
            targetOnMesh[0] + jitterX,
            targetOnMesh[1],
            targetOnMesh[2] + jitterZ
        };

        m_crowd->requestMoveTarget(e->GetID(), targetPoly, targetOnMesh);
    }

    m_crowd->update(deltaTime, nullptr);

    // Push crowd positions back to enemies
    for (Enemy* e : enemies) {
        const dtCrowdAgent* agent = m_crowd->getAgent(e->GetID());
        if (!agent) continue;

        float agentPos[3];
        dtVcopy(agentPos, agent->npos);

        float playerPosArr[3] = { playerPos.x, playerPos.y, playerPos.z };
        if ((agent->npos - playerPosArr) < glm::abs(5.0f))
            m_crowd->resetMoveTarget(e->GetID());

        if (e->GetID() == 6)
            e->SetPosition(glm::vec3(agentPos[0], agentPos[1] + 2.0f, agentPos[2]));
        else
            e->SetPosition(glm::vec3(agentPos[0], agentPos[1], agentPos[2]));
    }
}

void NavMeshManager::RenderDebug(const glm::mat4& view, const glm::mat4& projection)
{
    if (!m_vao) return;

    m_shader.Use();
    m_shader.SetMat4("view",       view);
    m_shader.SetMat4("projection", projection);

    glDisable(GL_CULL_FACE);
    glBindVertexArray(m_vao);
    glDrawElements(GL_TRIANGLES, (GLsizei)m_renderIndices.size(), GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

bool NavMeshManager::SnapToNavMesh(const glm::vec3& pos, glm::vec3& outPos,
                                    float xzExtent, float yExtent) const
{
    if (!m_navMeshQuery) return false;

    float searchPos[3]  = { pos.x, pos.y, pos.z };
    float halfExt[3]    = { xzExtent, yExtent, xzExtent };
    float snapped[3]    = {};
    dtPolyRef poly      = 0;

    dtStatus st = m_navMeshQuery->findNearestPoly(searchPos, halfExt, &m_filter, &poly, snapped);
    if (dtStatusFailed(st) || poly == 0) return false;

    outPos = glm::vec3(snapped[0], snapped[1], snapped[2]);
    return true;
}

// ---------------------------------------------------------------------------
// Private helpers
// ---------------------------------------------------------------------------

bool NavMeshManager::BuildTile(int tx, int ty, float* bmin, float* bmax,
                                rcConfig cfg, unsigned char*& navData, int* navDataSize,
                                dtNavMeshParams parameters)
{
    const float tileWS = cfg.tileSize * cfg.cs;

    float tileBMin[3] = {
        parameters.orig[0] + tx * tileWS,       bmin[1],
        parameters.orig[2] + ty * tileWS
    };
    float tileBMax[3] = {
        parameters.orig[0] + (tx + 1) * tileWS, bmax[1],
        parameters.orig[2] + (ty + 1) * tileWS
    };

    cfg.borderSize = cfg.walkableRadius + 3;
    const float border = cfg.borderSize * cfg.cs;
    cfg.width  = cfg.tileSize + cfg.borderSize * 2;
    cfg.height = cfg.tileSize + cfg.borderSize * 2;

    float tbmin[3] = { tileBMin[0] - border, tileBMin[1], tileBMin[2] - border };
    float tbmax[3] = { tileBMax[0] + border, tileBMax[1], tileBMax[2] + border };

    std::vector<float>        tileVerts;
    std::vector<int>          tileIndices;
    std::vector<unsigned char> tileAreas;
    std::unordered_map<int, int> globalToLocal;

    int triCount = (int)m_indices.size() / 3;
    for (int i = 0; i < triCount; ++i) {
        int ia = m_triIndices[i * 3 + 0];
        int ib = m_triIndices[i * 3 + 1];
        int ic = m_triIndices[i * 3 + 2];

        glm::vec3 a = glm::make_vec3(&m_vertices[ia * 3]);
        glm::vec3 b = glm::make_vec3(&m_vertices[ib * 3]);
        glm::vec3 c = glm::make_vec3(&m_vertices[ic * 3]);

        glm::vec3 triMin = glm::min(a, glm::min(b, c));
        glm::vec3 triMax = glm::max(a, glm::max(b, c));

        if (triMax.x < tbmin[0] || triMin.x > tbmax[0] ||
            triMax.y < tbmin[1] || triMin.y > tbmax[1] ||
            triMax.z < tbmin[2] || triMin.z > tbmax[2])
            continue;

        auto remap = [&](int g) -> int {
            auto it = globalToLocal.find(g);
            if (it != globalToLocal.end()) return it->second;
            int local = (int)tileVerts.size() / 3;
            tileVerts.push_back(m_vertices[g * 3 + 0]);
            tileVerts.push_back(m_vertices[g * 3 + 1]);
            tileVerts.push_back(m_vertices[g * 3 + 2]);
            globalToLocal[g] = local;
            return local;
        };

        tileIndices.push_back(remap(ia));
        tileIndices.push_back(remap(ib));
        tileIndices.push_back(remap(ic));
        tileAreas.push_back(RC_WALKABLE_AREA);
    }

    rcHeightfield* hf = rcAllocHeightfield();
    if (!rcCreateHeightfield(&m_ctx, *hf, cfg.width, cfg.height, tbmin, tbmax, cfg.cs, cfg.ch)) {
        Logger::Log(1, "BuildTile: rcCreateHeightfield failed\n");
        return false;
    }

    if (!rcRasterizeTriangles(&m_ctx, tileVerts.data(), (int)tileVerts.size() / 3,
            tileIndices.data(), tileAreas.data(), (int)tileIndices.size() / 3, *hf, cfg.walkableClimb))
        Logger::Log(1, "BuildTile: rcRasterizeTriangles failed\n");

    rcFilterWalkableLowHeightSpans(&m_ctx, cfg.walkableHeight, *hf);
    rcFilterLedgeSpans(&m_ctx, cfg.walkableHeight, cfg.walkableClimb, *hf);
    rcFilterLowHangingWalkableObstacles(&m_ctx, cfg.walkableHeight, *hf);

    rcCompactHeightfield* chf = rcAllocCompactHeightfield();
    if (!rcBuildCompactHeightfield(&m_ctx, cfg.walkableHeight, cfg.walkableClimb, *hf, *chf))
        Logger::Log(1, "BuildTile: rcBuildCompactHeightfield failed\n");

    rcErodeWalkableArea(&m_ctx, cfg.walkableRadius, *chf);
    rcBuildDistanceField(&m_ctx, *chf);
    rcBuildRegions(&m_ctx, *chf, cfg.borderSize, cfg.minRegionArea, cfg.mergeRegionArea);

    rcContourSet* cset = rcAllocContourSet();
    if (!rcBuildContours(&m_ctx, *chf, cfg.maxSimplificationError, cfg.maxEdgeLen, *cset)) {
        Logger::Log(1, "BuildTile: rcBuildContours failed\n");
        return false;
    }
    if (cset->nconts == 0) {
        Logger::Log(1, "BuildTile: no contours for tile (%d,%d)\n", tx, ty);
        return false;
    }

    rcPolyMesh* pmesh = rcAllocPolyMesh();
    if (!rcBuildPolyMesh(&m_ctx, *cset, cfg.maxVertsPerPoly, *pmesh))
        Logger::Log(1, "BuildTile: rcBuildPolyMesh failed\n");

    rcPolyMeshDetail* dmesh = rcAllocPolyMeshDetail();
    if (!rcBuildPolyMeshDetail(&m_ctx, *pmesh, *chf, cfg.detailSampleDist, cfg.detailSampleMaxError, *dmesh))
        Logger::Log(1, "BuildTile: rcBuildPolyMeshDetail failed\n");

    if (!pmesh->npolys || !pmesh || !dmesh || !chf || !cset || !hf) {
        navData     = nullptr;
        navDataSize = 0;
        return true;
    }

    static const unsigned short DT_POLYFLAGS_WALK = 0x01;
    for (int i = 0; i < pmesh->npolys; ++i)
        pmesh->flags[i] = (pmesh->areas[i] == RC_WALKABLE_AREA) ? DT_POLYFLAGS_WALK : 0;

    dtNavMeshCreateParams params{};
    params.verts             = pmesh->verts;
    params.vertCount         = pmesh->nverts;
    params.polys             = pmesh->polys;
    params.polyAreas         = pmesh->areas;
    params.polyFlags         = pmesh->flags;
    params.polyCount         = pmesh->npolys;
    params.nvp               = pmesh->nvp;
    params.walkableHeight    = cfg.walkableHeight * cfg.ch;
    params.walkableRadius    = cfg.walkableRadius * cfg.cs;
    params.walkableClimb     = cfg.walkableClimb  * cfg.ch;
    params.detailMeshes      = dmesh->meshes;
    params.detailVerts       = dmesh->verts;
    params.detailVertsCount  = dmesh->nverts;
    params.detailTris        = dmesh->tris;
    params.detailTriCount    = dmesh->ntris;
    params.cs                = cfg.cs;
    params.ch                = cfg.ch;
    params.buildBvTree       = true;
    params.tileX             = tx;
    params.tileY             = ty;
    params.tileLayer         = 0;
    rcVcopy(params.bmin, tileBMin);
    rcVcopy(params.bmax, tileBMax);

    if (dtStatusFailed(dtCreateNavMeshData(&params, &navData, navDataSize))) {
        Logger::Log(1, "BuildTile: dtCreateNavMeshData failed for (%d,%d)\n", tx, ty);
        return false;
    }

    m_heightFields.push_back(hf);
    m_compactHeightFields.push_back(chf);
    m_contourSets.push_back(cset);
    m_polyMeshes.push_back(pmesh);
    m_polyMeshDetails.push_back(dmesh);
    return true;
}

void NavMeshManager::UploadRenderMesh()
{
    Logger::Log(1, "NavMesh render verts: %zu, indices: %zu\n",
                m_renderVertices.size(), m_renderIndices.size());

    glGenVertexArrays(1, &m_vao);
    glBindVertexArray(m_vao);

    glGenBuffers(1, &m_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER,
                 m_renderVertices.size() * sizeof(float),
                 m_renderVertices.data(), GL_STATIC_DRAW);

    glGenBuffers(1, &m_ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 m_renderIndices.size() * sizeof(unsigned int),
                 m_renderIndices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);
}
