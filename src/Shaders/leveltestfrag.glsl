#version 460 core

in vec2 TexCoords;
in vec3 WorldPos;
in vec3 Normal;
out vec4 FragColor;

void main() {
    FragColor = vec4(Normal, 0.0);
}
