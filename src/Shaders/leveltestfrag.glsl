#version 460 core
out vec4 FragColor;

in vec2 TexCoords;
//in vec3 WorldPos;
in vec3 Normal;

uniform vec4 uBaseColorFactor;
uniform vec2 uMetallicRoughness;

void main() {
    FragColor = vec4(Normal, 1.0);
}
