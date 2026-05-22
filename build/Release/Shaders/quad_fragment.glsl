#version 460 core

out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D minimapTex;

void main()
{
	FragColor = texture(minimapTex, TexCoords);

	float borderThickness = 0.01;
	vec3 borderColour = vec3(0.541, 0.196, 0.176);

	if (TexCoords.x < borderThickness || TexCoords.x > 1.0 - borderThickness ||
		TexCoords.y < borderThickness || TexCoords.y > 1.0 - borderThickness)
	{
		FragColor = vec4(borderColour, 1.0);
	} else {
	
		FragColor = texture(minimapTex, TexCoords);
	}
}