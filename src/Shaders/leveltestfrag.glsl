#version 460 core
out vec4 FragColor;

in vec2 TexCoords;
in vec3 Normal;

uniform sampler2D uBaseColorTexture;

uniform vec3 uBaseColorFactor;
uniform vec2 uMetallicRoughness;
uniform bool useTex;


struct DirLight {
	vec3 direction;

	vec3 ambient;
	vec3 diffuse;
	vec3 specular;
};

DirLight dirLight = DirLight(vec3(-0.5, -1.0, 2.0), vec3(0.5, 0.5, 0.5), vec3(0.5, 0.1, 0.3), vec3(0.1, 0.2, 0.3));


vec3 CalcDirLight(DirLight light, vec3 normal)
{
	vec3 lightDir = normalize(-light.direction);

	// diffuse
	float diff = max(dot(normal, lightDir), 0.0);

	// combine results
	vec3 ambient;
	vec3 diffuse;

 	if (useTex) {
		ambient = light.ambient * vec3(texture(uBaseColorTexture, TexCoords)) * uBaseColorFactor.xyz;
		diffuse = light.diffuse * diff * vec3(texture(uBaseColorTexture, TexCoords)) * uBaseColorFactor.xyz;

	} else {
		ambient = light.ambient * uBaseColorFactor.xyz;
		diffuse = light.diffuse * diff * uBaseColorFactor.xyz;
	}


	return (ambient + diffuse);
}

void main() {
	vec4 texColor;

	vec3 norm = normalize(Normal);
	vec3 result = CalcDirLight(dirLight, norm);

	if (useTex) {
		texColor = texture(uBaseColorTexture, TexCoords) * vec4(uBaseColorFactor.xyz, 1.0);
	} else {
		texColor = vec4(uBaseColorFactor.xyz, 1.0);
	}
	FragColor = vec4(result, 1.0);
}
//	vec4 col = texture(uBaseColorTexture, TexCoords);
//	
//    FragColor = col`
//}
