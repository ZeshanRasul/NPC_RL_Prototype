#version 460 core

layout (location = 0) in vec2 TexCoord;

out vec4 FragColor;

uniform sampler2D tex;

void main() {

    vec4 result = texture(tex, TexCoord);
    FragColor = result; 
}