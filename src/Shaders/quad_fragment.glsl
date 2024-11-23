#version 460 core

out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D minimapTex;

void main()
{
	FragColor = texture(minimapTex, TexCoords);
}