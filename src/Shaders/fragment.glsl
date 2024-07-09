#version 330 core

out vec4 FragColor;

uniform float playerColorR;
uniform float playerColorG;
uniform float playerColorB;

void main()
{
	FragColor = vec4(playerColorR, playerColorG, playerColorB, 1.0);
}