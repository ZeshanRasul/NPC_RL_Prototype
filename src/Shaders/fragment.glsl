#version 460 core
layout (location = 0) in vec3 normal;
layout (location = 1) in vec2 texCoord;

out vec4 FragColor;

struct DirLight {
	vec3 direction;

	vec3 ambient;
	vec3 diffuse;
	vec3 specular;
};

uniform DirLight dirLight;
uniform sampler2D tex;

vec3 CalcDirLight(DirLight light, vec3 normal);



void main() {
	vec3 norm = normalize(normal);

	vec3 result = CalcDirLight(dirLight, norm);

	FragColor = vec4(1.0, 1.0, 1.0, 1.0);
}

vec3 CalcDirLight(DirLight light, vec3 normal)
{
	vec3 lightDir = normalize(-light.direction);

	// diffuse
	float diff = max(dot(normal, lightDir), 0.0);

	// combine results
 	vec3 ambient = light.ambient * vec3(texture(tex, texCoord));
 	vec3 diffuse = light.diffuse * diff * vec3(texture(tex, texCoord));

	return (ambient + diffuse);
}
