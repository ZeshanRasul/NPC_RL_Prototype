#version 460 core
layout(location = 0) in vec3 aPos;
layout(location =1) in vec2 aTexCoord;

layout (std140, binding = 0) uniform Matrices {
    mat4 view;
    mat4 projection;
	mat4 model;
};

out vec2 TexCoord;

void main() {
    gl_Position = projection * view * model * vec4(aPos, 1.0);
    TexCoord = aTexCoord;
}
