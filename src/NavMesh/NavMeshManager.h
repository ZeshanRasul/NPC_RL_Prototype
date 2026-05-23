#pragma once
#include <vector>
#include <string>
#include <glad/glad.h>
#include <glm/glm.hpp>

#include "Recast.h"
#include "DetourNavMeshBuilder.h"
#include "DetourNavMeshQuery.h"
#include "DetourCrowd.h"
#include "DetourCommon.h"
#include "DetourNavMesh.h"

#include "src/OpenGL/Shader.h"

class Enemy;

class NavMeshManager {
public:
    NavMeshManager();
    ~NavMeshManager();

    // Build (or load from cache) the navmesh from a world-space triangle soup
    // extracted from the ground mesh. Uploads the debug render VAO.
    void Build(const std::vector<float>& vertices,
               const std::vector<unsigned int>& indices,
               const std::string& shaderVert,
               const std::string& shaderFrag);

    // Spawn dtCrowd agents for each enemy at their current positions.
    void InitCrowd(const std::vector<Enemy*>& enemies);

    // Add a new crowd agent at pos (snapped to navmesh) and return the slot index.
    // outSnappedPos receives the snapped world position.
    int RegisterCrowdAgent(const glm::vec3& pos, glm::vec3& outSnappedPos);

    // Advance crowd simulation and push new positions back to enemies.
    void Update(float deltaTime,
                const std::vector<Enemy*>& enemies,
                const glm::vec3& playerPos);

    // Draw navmesh debug overlay.
    void RenderDebug(const glm::mat4& view, const glm::mat4& projection);

    // Snap a world-space position to the nearest navmesh polygon.
    // Returns true and writes snapped position if a polygon is found.
    bool SnapToNavMesh(const glm::vec3& pos, glm::vec3& outPos,
                       float xzExtent = 2.0f, float yExtent = 10.0f) const;

    // Remove the crowd agent at agentIdx and re-add it at newPos (snapped to navmesh).
    // Used in edit mode to keep the agent in sync with gizmo-dragged positions.
    // Returns the snapped world position written back to the enemy.
    glm::vec3 TeleportAgent(int agentIdx, const glm::vec3& newPos);

    dtNavMeshQuery*    GetQuery()       const { return m_navMeshQuery; }
    dtCrowd*           GetCrowd()       const { return m_crowd; }
    const dtQueryFilter& GetFilter()    const { return m_filter; }
    const float*       GetHalfExtents() const { return m_halfExtents; }

private:
    bool BuildTile(int tx, int ty, float* bmin, float* bmax,
                   rcConfig cfg, unsigned char*& navData, int* navDataSize,
                   dtNavMeshParams parameters);

    void UploadRenderMesh();

    // Ground geometry (retained for save-path tile rebuild)
    std::vector<float>        m_vertices;
    std::vector<unsigned int> m_indices;
    int*          m_triIndices = nullptr;
    unsigned char* m_triAreas  = nullptr;

    // Recast context and per-tile intermediate meshes
    rcContext m_ctx;
    std::vector<rcHeightfield*>       m_heightFields;
    std::vector<rcCompactHeightfield*> m_compactHeightFields;
    std::vector<rcContourSet*>         m_contourSets;
    std::vector<rcPolyMesh*>           m_polyMeshes;
    std::vector<rcPolyMeshDetail*>     m_polyMeshDetails;

    // Detour runtime
    dtNavMesh*      m_navMesh      = nullptr;
    dtNavMeshQuery* m_navMeshQuery = nullptr;
    dtCrowd*        m_crowd        = nullptr;
    dtQueryFilter   m_filter;
    float           m_halfExtents[3] = { 500.0f, 50.0f, 500.0f };

    // Debug render
    GLuint m_vao = 0, m_vbo = 0, m_ebo = 0;
    Shader m_shader{};
    std::vector<float>        m_renderVertices;
    std::vector<unsigned int> m_renderIndices;

    // Tile dimensions
    float m_tileWorldSize = 0.0f;
    int   m_tileCountX    = 0;
    int   m_tileCountY    = 0;
    int   m_tileCount     = 0;

    bool m_saveNavMesh = false;
    bool m_loadNavMesh = true;
};
