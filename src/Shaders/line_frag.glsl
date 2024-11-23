#version 460 core

in vec4 FragPosLightSpace;

uniform vec3 lineColor;
uniform sampler2D shadowMap;

out vec4 FragColor;

float ShadowCalculation(vec4 fragPosLightSpace)
{
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;
    
    float closestDepth = texture(shadowMap, projCoords.xy).r;

    float currentDepth = projCoords.z;

    float shadow = currentDepth > closestDepth  ? 1.0 : 0.0;

    return shadow;
}

void main()
{
    float shadow = ShadowCalculation(FragPosLightSpace);

    FragColor = (1.0f - shadow) * vec4(lineColor, 1.0);
}