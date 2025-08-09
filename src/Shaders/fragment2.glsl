#version 460 core
out vec4 FragColor;

in vec2 TexCoords;
in vec3 WorldPos;
in vec3 Normal;

uniform sampler2D albedoMap;
uniform sampler2D metallicRoughnessMap;
uniform sampler2D normalMap;
uniform sampler2D aoMap;


struct DirLight {
	vec3 direction;

	vec3 ambient;
	vec3 diffuse;
	vec3 specular;
};

struct PointLight {
    vec3 position;
    vec3 color;
};

PointLight pointLight1 = PointLight(vec3(10.0, 503.0, -184.0), vec3(301.0, 10.1, 20.2));
PointLight pointLight2 = PointLight(vec3(16.0, 339.0, -155.0), vec3(301.0, 10.6, 20.2));
PointLight pointLight3 = PointLight(vec3(-382.0, 332.0, -153.0), vec3(301.0, 10.6, 20.2));
PointLight pointLight4 = PointLight(vec3(-83.0, 334.0, -8.0), vec3(300.0, 10.6, 20.2));


PointLight pointLights[] =  {
    pointLight1,
    pointLight2,
    pointLight3,
    pointLight4
};

uniform DirLight dirLight;

uniform bool useAlbedo;
uniform bool useMetallicRoughness;
uniform bool useNormalMap;
uniform bool useOcclusionMap;
uniform bool useEmissiveFactor;


vec3 CalcDirLight(DirLight light, vec3 normal);

uniform vec3 cameraPos;
uniform vec3 baseColour;
uniform float metallicFactor;
uniform float roughnessFactor;
uniform vec3 emissiveFactor;
uniform float emissiveStrength;

const float PI = 3.14159265359;

vec3 getNormalFromMap()
{
    vec3 tangentNormal = texture(normalMap, TexCoords).xyz * 2.0 - 1.0;

    vec3 Q1  = dFdx(WorldPos);
    vec3 Q2  = dFdy(WorldPos);
    vec2 st1 = dFdx(TexCoords);
    vec2 st2 = dFdy(TexCoords);

    vec3 N   = normalize(Normal);
    vec3 T  = normalize(Q1*st2.t - Q2*st1.t);
    vec3 B  = -normalize(cross(N, T));
    mat3 TBN = mat3(T, B, N);

    return normalize(TBN * tangentNormal);
}
// ----------------------------------------------------------------------------
float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a = roughness*roughness;
    float a2 = a*a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH*NdotH;

    float nom   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return nom / denom;
}
// ----------------------------------------------------------------------------
float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;

    float nom   = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return nom / denom;
}
// ----------------------------------------------------------------------------
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}
// ----------------------------------------------------------------------------
vec3 FresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}
// ----------------------------------------------------------------------------


vec3 CalcPointLight(
    vec3 lightPos,
    vec3 lightColor,
    vec3 fragPos,
    vec3 N,
    vec3 V,
    float metallic,
    float roughness,
    vec3 albedo
) {
    vec3 L = normalize(lightPos - fragPos);
    vec3 H = normalize(V + L);
    float distance = length(lightPos - fragPos);
    float attenuation = 1.0 / (distance * distance);
    vec3 radiance = lightColor * attenuation;

    vec3 F0 = vec3(0.04); 
    F0 = mix(F0, albedo, metallic);

    float NDF = DistributionGGX(N, H, roughness);
    float G   = GeometrySmith(N, V, L, roughness);
    vec3  F   = FresnelSchlick(max(dot(H, V), 0.0), F0);

    vec3 numerator    = NDF * G * F;
    float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.001;
    vec3 specular     = numerator / denominator;

    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= 1.0 - metallic;

    float NdotL = max(dot(N, L), 0.0);
    return (kD * albedo / PI + specular) * radiance * NdotL;
}

void main()
{		
    vec3 albedo;
    if (useAlbedo) {
        albedo =  pow(texture(albedoMap, TexCoords).rgb, vec3(2.2));
    } else {
        albedo = baseColour;
    }

    float metallic;
    if (useMetallicRoughness) {
        metallic  = texture(metallicRoughnessMap, TexCoords).b;
    } else {
        metallic = metallicFactor;
    }

    float roughness;
    if (useMetallicRoughness) {
        roughness = texture(metallicRoughnessMap, TexCoords).g;
    } else {
        roughness = roughnessFactor;
    }

    float ao;
    if (useOcclusionMap)
    {
        ao *= texture(aoMap, TexCoords).r;
    } else {
        ao = 1.0;
    }

    vec3 emissive;
    if (useEmissiveFactor) 
    {
        emissive = emissiveFactor * emissiveStrength;
    } else {
        emissive = vec3(0.0f, 0.0f, 0.0f);
   }

    vec3 N;

    if (useNormalMap) {
        N = getNormalFromMap();
    } else {
        N = normalize(Normal);
    }

    vec3 V = normalize(cameraPos - WorldPos);

    vec3 F0 = vec3(0.04); 
    F0 = mix(F0, albedo, metallic);

    // reflectance equation
    vec3 Lo = vec3(0.0);
    for(int i = 0; i < 4; ++i) 
    {
        // calculate per-light radiance
        vec3 L = normalize(-dirLight.direction);
        if (i % 2 == 0) {
            // skip every second light
           L = -L;
        }
        vec3 H = normalize(V + L);
        vec3 radiance = dirLight.diffuse;

        // Cook-Torrance BRDF
        float NDF = DistributionGGX(N, H, roughness);   
        float G   = GeometrySmith(N, V, L, roughness);      
        vec3 F    = FresnelSchlick(max(dot(H, V), 0.0), F0);
           
        vec3 numerator    = NDF * G * F; 
        float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001; // + 0.0001 to prevent divide by zero
        vec3 specular = numerator / denominator;
        
        // kS is equal to Fresnel
        vec3 kS = F;
        // for energy conservation, the diffuse and specular light can't
        // be above 1.0 (unless the surface emits light); to preserve this
        // relationship the diffuse component (kD) should equal 1.0 - kS.
        vec3 kD = vec3(1.0) - kS;
        // multiply kD by the inverse metalness such that only non-metals 
        // have diffuse lighting, or a linear blend if partly metal (pure metals
        // have no diffuse light).
        kD *= 1.0 - metallic;	  

        // scale light by NdotL
        float NdotL = max(dot(N, L), 0.0);        


        // add to outgoing radiance Lo
        Lo += (kD * albedo / PI + specular) * radiance * NdotL;
    }   
    
    for (int i = 0; i < 4; ++i) {
        Lo += CalcPointLight(
            pointLights[i].position,
            pointLights[i].color,
            WorldPos,
            N,
            V,
            metallic,
            roughness,
            albedo
        ) * 100.0f;
    }

    // ambient lighting
    vec3 ambient = vec3(0.03) * albedo * ao;
    
    vec3 color = ambient + Lo + emissive;

    // HDR tonemapping
    color = color / (color + vec3(1.0));
    // gamma correct
    color = pow(color, vec3(1.0/2.2)); 

    FragColor = vec4(color, 1.0);
}
