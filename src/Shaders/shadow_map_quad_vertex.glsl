#version 460 core

layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aTexCoords;

out vec2 TexCoords;

uniform vec2 ndcOffset;

void main() {
	vec2 ndcPosition = aPos.xy + ndcOffset;
	gl_Position = vec4(ndcPosition.x, ndcPosition.y, 0.0, 1.0);
	TexCoords = aTexCoords;
}