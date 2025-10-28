#version 460 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;


out vec2 TexCoords;
out vec3 Normal;

layout (std140, binding = 0) uniform Matrices {
    mat4 view;
    mat4 projection;
	mat4 model;
};

uniform mat4 Transform;

void main() {

    TexCoords = aTexCoord;
    vec3 WorldPos = vec3(model * Transform * vec4(aPos, 1.0));

    mat3 normalMatrix = transpose(inverse(mat3(model)));
    Normal = aNormal * normalMatrix;   

    gl_Position =  projection * view * vec4(WorldPos, 1.0);
}