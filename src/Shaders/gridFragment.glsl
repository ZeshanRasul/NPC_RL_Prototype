#version 460 core

in vec2 TexCoord;

out vec4 FragColor;

uniform vec3 color;

uniform sampler2D tex;

void main() {

    vec4 texColor = texture(tex, TexCoord);
 //   FragColor = texColor * vec4(color, 0.0);
    FragColor = texColor;
}
