#version 460 core

layout(location = 0) in vec3 aPos;

out vec4 FragPosLightSpace;
out float vGradient;

uniform mat4 view;
uniform mat4 projection;
uniform mat4 lightSpaceMatrix;

void main()
{
    FragPosLightSpace = lightSpaceMatrix * vec4(aPos, 1.0);

    vGradient = (gl_VertexID == 0) ? 0.0 : 1.0;
    gl_Position = projection * view * vec4(aPos, 1.0);
}