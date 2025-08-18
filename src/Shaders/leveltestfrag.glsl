#version 460 core
out vec4 FragColor;

in vec2 TexCoords;
in vec3 WorldPos;
in vec3 Normal;

uniform vec4 uBaseColorFactor;
uniform vec2 uMetallicRoughness;
uniform int useTex;

uniform sampler2D uBaseColorTexture;

void main() {
    vec4 texColor = texture(uBaseColorTexture, TexCoords);

    vec4 baseColor;
    if (useTex == 1) {
        baseColor = texColor;
    } else {
        baseColor = uBaseColorFactor;
    }
    FragColor = baseColor;
}
