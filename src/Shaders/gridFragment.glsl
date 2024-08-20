#version 460 core
out vec4 FragColor;

layout (std140, binding = 1) uniform Colors {
    vec3 color;
};

void main() {
    FragColor = vec4(color, 1.0);
}
