#version 460 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;
layout (location = 3) in vec4 aJointNum;
layout (location = 4) in vec4 aJointWeight;

layout (location = 0) out vec3 normal;
layout (location = 1) out vec2 texCoord;

layout (std140, binding = 0) uniform Matrices {
    mat4 view;
    mat4 projection;
	mat4 model;
};

layout (std430, binding = 5) readonly buffer JointMatrices {
    mat4 jointMat[];
};

void main() {
	  mat4 skinMat =
		aJointWeight.x * jointMat[int(aJointNum.x)] +
		aJointWeight.y * jointMat[int(aJointNum.y)] +
		aJointWeight.z * jointMat[int(aJointNum.z)] +
		aJointWeight.w * jointMat[int(aJointNum.w)];
	
	gl_Position = projection * view * model * skinMat * vec4(aPos, 1.0);
	normal = aNormal;
	texCoord = aTexCoord;
}