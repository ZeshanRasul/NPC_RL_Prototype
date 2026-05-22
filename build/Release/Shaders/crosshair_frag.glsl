#version 460 core

layout (location = 0) in vec2 TexCoord;

out vec4 FragColor;

uniform sampler2D tex;
uniform vec3 color;

void main() {

    vec4 result = texture(tex, TexCoord);
    FragColor = result * vec4(color, 1.0);
}