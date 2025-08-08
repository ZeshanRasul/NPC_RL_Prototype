#version 460 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;
layout (location = 3) in vec2 aTexCoord_1;
layout (location = 4) in vec2 aTexCoord_2;


layout (location = 0) out vec3 normal;
layout (location = 1) out vec2 texCoord;

layout (std140, binding = 0) uniform Matrices {
    mat4 view;
    mat4 projection;
	  mat4 model;
};

void main() {
  gl_Position = projection * view * model * vec4(aPos, 1.0);
  normal = vec3(transpose(inverse(model)) * vec4(aNormal, 1.0));
  texCoord = aTexCoord;

}