#version 460 core
out vec4 FragColor;

in vec2 TexCoords;
in vec3 WorldPos;
in vec3 Normal;

// Texture slots — matched to what Enemy::DrawGLTFModel binds
uniform sampler2D albedoMap;    // slot 0  BaseColor
uniform sampler2D metallicMap;  // slot 1  Metallic   (.r channel)
uniform sampler2D roughnessMap; // slot 2  Roughness  (.r channel)
uniform sampler2D normalMap;    // slot 3  Normal
uniform sampler2D emissiveMap;  // slot 4  Emissive
uniform sampler2D occlusionTex; // slot 5  (optional, usually absent)

// Feature flags
uniform bool useAlbedo;
uniform bool useMetallicMap;
uniform bool useRoughnessMap;
uniform bool useNormalMap;
uniform bool useEmissiveMap;
uniform bool useOcclusionMap;

// Fallbacks when textures are absent
uniform vec3  baseColour;
uniform float metallicFactor;
uniform float roughnessFactor;
uniform float u_metallicScale; // multiplied into sampled/fallback metallic (set < 1 to recover diffuse without IBL)

struct DirLight {
    vec3 direction;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular; // reserved — PBR computes specular from material parameters
};

#define MAX_POINT_LIGHTS 16
#define MAX_SPOT_LIGHTS  8

struct ScenePointLight {
    vec3  position;
    vec3  color;
    float intensity;
};

struct SceneSpotLight {
    vec3  position;
    vec3  direction;
    vec3  color;
    float intensity;
    float innerCutoff;
    float outerCutoff;
};

uniform ScenePointLight u_pointLights[MAX_POINT_LIGHTS];
uniform int             u_numPointLights;
uniform SceneSpotLight  u_spotLights[MAX_SPOT_LIGHTS];
uniform int             u_numSpotLights;

uniform DirLight dirLight;
uniform vec3     cameraPos;

const float PI = 3.14159265359;

// Derivative-based TBN — no tangent vertex attribute required
vec3 getNormalFromMap()
{
    vec3 tangentNormal = texture(normalMap, TexCoords).xyz * 2.0 - 1.0;
    vec3 Q1  = dFdx(WorldPos);
    vec3 Q2  = dFdy(WorldPos);
    vec2 st1 = dFdx(TexCoords);
    vec2 st2 = dFdy(TexCoords);
    vec3 N   = normalize(Normal);
    vec3 T   = normalize(Q1 * st2.t - Q2 * st1.t);
    vec3 B   = -normalize(cross(N, T));
    mat3 TBN = mat3(T, B, N);
    return normalize(TBN * tangentNormal);
}

float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a      = roughness * roughness;
    float a2     = a * a;
    float NdotH  = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;
    float denom  = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;
    return a2 / denom;
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = roughness + 1.0;
    float k = (r * r) / 8.0;
    return NdotV / (NdotV * (1.0 - k) + k);
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    return GeometrySchlickGGX(max(dot(N, V), 0.0), roughness)
         * GeometrySchlickGGX(max(dot(N, L), 0.0), roughness);
}

vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

vec3 CalcPointLight(vec3 lightPos, vec3 lightColor, vec3 fragPos,
                    vec3 N, vec3 V, float metallic, float roughness, vec3 albedo)
{
    vec3  L           = normalize(lightPos - fragPos);
    vec3  H           = normalize(V + L);
    float dist        = length(lightPos - fragPos);
    vec3  radiance    = lightColor / (dist * dist);
    vec3  F0          = mix(vec3(0.04), albedo, metallic);
    float NDF = DistributionGGX(N, H, roughness);
    float G   = GeometrySmith(N, V, L, roughness);
    vec3  F   = fresnelSchlick(max(dot(H, V), 0.0), F0);
    vec3  spec = (NDF * G * F) / (4.0 * max(dot(N,V),0.0) * max(dot(N,L),0.0) + 0.001);
    vec3  kD   = (vec3(1.0) - F) * (1.0 - metallic);
    return (kD * albedo / PI + spec) * radiance * max(dot(N, L), 0.0);
}

vec3 CalcSpotLight(vec3 lightPos, vec3 lightDir, float innerCutoff, float outerCutoff,
                   vec3 lightColor, vec3 fragPos,
                   vec3 N, vec3 V, float metallic, float roughness, vec3 albedo)
{
    vec3  L         = normalize(lightPos - fragPos);
    float theta     = dot(L, normalize(-lightDir));
    float epsilon   = innerCutoff - outerCutoff;
    float spotI     = clamp((theta - outerCutoff) / epsilon, 0.0, 1.0);
    float dist      = length(lightPos - fragPos);
    vec3  radiance  = lightColor / (dist * dist) * spotI;
    vec3  F0        = mix(vec3(0.04), albedo, metallic);
    vec3  H         = normalize(V + L);
    float NDF = DistributionGGX(N, H, roughness);
    float G   = GeometrySmith(N, V, L, roughness);
    vec3  F   = fresnelSchlick(max(dot(H, V), 0.0), F0);
    vec3  spec = (NDF * G * F) / (4.0 * max(dot(N,V),0.0) * max(dot(N,L),0.0) + 0.001);
    vec3  kD   = (vec3(1.0) - F) * (1.0 - metallic);
    return (kD * albedo / PI + spec) * radiance * max(dot(N, L), 0.0);
}

void main()
{
    // --- Material inputs ---
    vec3 albedo = useAlbedo
        ? pow(texture(albedoMap, TexCoords).rgb, vec3(2.2))
        : pow(baseColour, vec3(2.2));

    float metallic  = useMetallicMap  ? texture(metallicMap,  TexCoords).r : metallicFactor;
    float roughness = useRoughnessMap ? texture(roughnessMap, TexCoords).r : roughnessFactor;
    metallic  = clamp(metallic * u_metallicScale, 0.0, 1.0);
    roughness = clamp(roughness, 0.04, 1.0); // prevent GGX singularity

    float ao = useOcclusionMap ? texture(occlusionTex, TexCoords).r : 1.0;

    vec3 emissive = useEmissiveMap
        ? pow(texture(emissiveMap, TexCoords).rgb, vec3(2.2))
        : vec3(0.0);

    // Normal — only perturb when a normal map is loaded (avoids garbage from unbound slot)
    vec3 N = useNormalMap ? getNormalFromMap() : normalize(Normal);

    vec3 V  = normalize(cameraPos - WorldPos);
    vec3 F0 = mix(vec3(0.04), albedo, metallic);

    // --- Lighting ---
    vec3 Lo = vec3(0.0);

    // Directional light
    // dirLight.diffuse is the sun colour × intensity (HDR ok, e.g. vec3(1.5–3.0))
    {
        vec3  L   = normalize(-dirLight.direction);
        vec3  H   = normalize(V + L);
        float NDF = DistributionGGX(N, H, roughness);
        float G   = GeometrySmith(N, V, L, roughness);
        vec3  F   = fresnelSchlick(max(dot(H, V), 0.0), F0);
        vec3  spec = (NDF * G * F) / (4.0 * max(dot(N,V),0.0) * max(dot(N,L),0.0) + 0.0001);
        vec3  kD   = (vec3(1.0) - F) * (1.0 - metallic);
        Lo += (kD * albedo / PI + spec) * dirLight.diffuse * max(dot(N, L), 0.0);
    }

    // Point lights
    for (int i = 0; i < u_numPointLights; ++i)
        Lo += CalcPointLight(u_pointLights[i].position, u_pointLights[i].color,
                             WorldPos, N, V, metallic, roughness, albedo)
              * u_pointLights[i].intensity;

    // Spot lights
    for (int i = 0; i < u_numSpotLights; ++i)
        Lo += CalcSpotLight(u_spotLights[i].position, u_spotLights[i].direction,
                            u_spotLights[i].innerCutoff, u_spotLights[i].outerCutoff,
                            u_spotLights[i].color,
                            WorldPos, N, V, metallic, roughness, albedo)
              * u_spotLights[i].intensity;

    // dirLight.ambient is your fill light — keep small, e.g. vec3(0.03–0.1)
    vec3 ambient = dirLight.ambient * albedo * ao;
    vec3 color   = ambient + Lo + emissive;

    // Reinhard tone mapping + gamma correction
    color = color / (color + vec3(1.0));
    color = pow(color, vec3(1.0 / 2.2));

    FragColor = vec4(color, 1.0);
}
