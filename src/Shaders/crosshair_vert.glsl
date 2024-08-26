#version 460 core

layout(location=0) in vec3 aPos;
layout(location=1) in vec2 aTexCoord;

uniform vec2 ndcPos;

layout (location = 0) out vec2 TexCoord;

void main()
{
	vec2 ndcPosition = aPos.xy + ndcPos;
	gl_Position = vec4(ndcPosition, 0.0, 1.0);
	TexCoord = aTexCoord;
}