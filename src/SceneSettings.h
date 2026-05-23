#pragma once
#include <string>
#include <vector>
#include <fstream>
#include <glm/glm.hpp>

// Use the nlohmann json bundled with tinygltf
#include "src/tinygltf/json.hpp"
#include "LightingSystem.h"

using json = nlohmann::json;

// ------------------------------------------------------------------
// Helpers: glm <-> json
// ------------------------------------------------------------------
inline json vec3ToJson(const glm::vec3& v)  { return { v.x, v.y, v.z }; }
inline json vec2ToJson(const glm::vec2& v)  { return { v.x, v.y }; }

inline glm::vec3 jsonToVec3(const json& j, glm::vec3 fallback = glm::vec3(0.0f))
{
    if (j.is_array() && j.size() == 3)
        return glm::vec3(j[0].get<float>(), j[1].get<float>(), j[2].get<float>());
    return fallback;
}

inline glm::vec2 jsonToVec2(const json& j, glm::vec2 fallback = glm::vec2(0.0f))
{
    if (j.is_array() && j.size() == 2)
        return glm::vec2(j[0].get<float>(), j[1].get<float>());
    return fallback;
}

// ------------------------------------------------------------------
// Enemy spawn record
// ------------------------------------------------------------------
struct EnemySpawn
{
    int       id;
    glm::vec3 position;
};

// ------------------------------------------------------------------
// Full scene settings blob
// ------------------------------------------------------------------
struct SceneSettings
{
    // Lighting
    glm::vec3 lightDirection  = glm::vec3(-0.5f, -1.0f, -0.3f);
    glm::vec3 lightAmbient    = glm::vec3(1.0f);
    glm::vec3 lightDiffuse    = glm::vec3(0.8f);
    glm::vec3 lightSpecular   = glm::vec3(0.8f, 0.9f, 1.0f);
    glm::vec3 pbrColor        = glm::vec3(300.0f);

    // Shadow framing
    glm::vec3 sceneCenter    = glm::vec3(27.0f, -20.0f, 130.0f);
    float     lightDistance  = 400.0f;

    // Shadow frustum
    float orthoLeft   = -200.0f;
    float orthoRight  =  200.0f;
    float orthoBottom = -200.0f;
    float orthoTop    =  200.0f;
    float nearPlane   =    1.0f;
    float farPlane    =  800.0f;

    // Minimap
    glm::vec2 minimapCenter = glm::vec2(27.0f, 130.0f);
    float     minimapHeight = 400.0f;
    float     minimapExtent = 250.0f;

    // Enemy spawns (populated from live enemy list)
    std::vector<EnemySpawn> enemySpawns;

    // ------------------------------------------------------------------
    void Save(const std::string& path) const
    {
        json j;

        j["lighting"]["direction"] = vec3ToJson(lightDirection);
        j["lighting"]["ambient"]   = vec3ToJson(lightAmbient);
        j["lighting"]["diffuse"]   = vec3ToJson(lightDiffuse);
        j["lighting"]["specular"]  = vec3ToJson(lightSpecular);
        j["lighting"]["pbrColor"]  = vec3ToJson(pbrColor);

        j["shadow"]["sceneCenter"]  = vec3ToJson(sceneCenter);
        j["shadow"]["lightDistance"] = lightDistance;
        j["shadow"]["orthoLeft"]    = orthoLeft;
        j["shadow"]["orthoRight"]   = orthoRight;
        j["shadow"]["orthoBottom"]  = orthoBottom;
        j["shadow"]["orthoTop"]     = orthoTop;
        j["shadow"]["nearPlane"]    = nearPlane;
        j["shadow"]["farPlane"]     = farPlane;

        j["minimap"]["center"]  = vec2ToJson(minimapCenter);
        j["minimap"]["height"]  = minimapHeight;
        j["minimap"]["extent"]  = minimapExtent;

        json spawns = json::array();
        for (const auto& s : enemySpawns)
        {
            json entry;
            entry["id"]  = s.id;
            entry["pos"] = vec3ToJson(s.position);
            spawns.push_back(entry);
        }
        j["enemies"] = spawns;

        std::ofstream f(path);
        f << j.dump(4);
    }

    // Returns false if the file doesn't exist or is malformed (caller keeps defaults).
    bool Load(const std::string& path)
    {
        std::ifstream f(path);
        if (!f.is_open()) return false;

        json j;
        try { f >> j; }
        catch (...) { return false; }

        if (j.contains("lighting"))
        {
            auto& l = j["lighting"];
            if (l.contains("direction")) lightDirection = jsonToVec3(l["direction"], lightDirection);
            if (l.contains("ambient"))   lightAmbient   = jsonToVec3(l["ambient"],   lightAmbient);
            if (l.contains("diffuse"))   lightDiffuse   = jsonToVec3(l["diffuse"],   lightDiffuse);
            if (l.contains("specular"))  lightSpecular  = jsonToVec3(l["specular"],  lightSpecular);
            if (l.contains("pbrColor"))  pbrColor       = jsonToVec3(l["pbrColor"],  pbrColor);
        }

        if (j.contains("shadow"))
        {
            auto& s = j["shadow"];
            if (s.contains("sceneCenter"))   sceneCenter   = jsonToVec3(s["sceneCenter"], sceneCenter);
            if (s.contains("lightDistance")) lightDistance = s["lightDistance"].get<float>();
            if (s.contains("orthoLeft"))     orthoLeft     = s["orthoLeft"].get<float>();
            if (s.contains("orthoRight"))    orthoRight    = s["orthoRight"].get<float>();
            if (s.contains("orthoBottom"))   orthoBottom   = s["orthoBottom"].get<float>();
            if (s.contains("orthoTop"))      orthoTop      = s["orthoTop"].get<float>();
            if (s.contains("nearPlane"))     nearPlane     = s["nearPlane"].get<float>();
            if (s.contains("farPlane"))      farPlane      = s["farPlane"].get<float>();
        }

        if (j.contains("minimap"))
        {
            auto& m = j["minimap"];
            if (m.contains("center"))  minimapCenter = jsonToVec2(m["center"], minimapCenter);
            if (m.contains("height"))  minimapHeight = m["height"].get<float>();
            if (m.contains("extent"))  minimapExtent = m["extent"].get<float>();
        }

        if (j.contains("enemies") && j["enemies"].is_array())
        {
            enemySpawns.clear();
            for (auto& e : j["enemies"])
            {
                EnemySpawn es;
                es.id       = e.value("id", -1);
                es.position = jsonToVec3(e["pos"]);
                enemySpawns.push_back(es);
            }
        }

        return true;
    }
};
