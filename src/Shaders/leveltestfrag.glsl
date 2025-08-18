#version 460 core
out vec4 FragColor;

in vec2 TexCoords;
in vec3 WorldPos;
in vec3 Normal;

uniform vec4 uBaseColorFactor;
uniform vec2 uMetallicRoughness;

uniform sampler2D uBaseColorTexture;

void main() {
    vec4 texColor = texture(uBaseColorTexture, TexCoords);

    vec4 baseColor = texColor;
    FragColor = baseColor;
}
