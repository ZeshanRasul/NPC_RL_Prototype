#version 460 core

layout (location = 0) in vec2 TexCoord;

out vec4 FragColor;

uniform sampler2D tex;

void main() {

    vec3 result = vec3(texture(tex, TexCoord));
    FragColor = vec4(result, 1.0); 
}