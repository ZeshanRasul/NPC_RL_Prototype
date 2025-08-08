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

uniform vec3 color;
uniform bool useTexture;

vec3 CalcDirLight(DirLight light, vec3 normal);

void main() {
	vec3 norm = normalize(normal);

	vec3 result = CalcDirLight(dirLight, norm);


	// HDR tonemapping
    result = result / (result + vec3(1.0));
    // gamma correct
    result = pow(result, vec3(1.0/2.2)); 

	FragColor = vec4(result, 1.0);
}

vec3 CalcDirLight(DirLight light, vec3 normal)
{
	vec3 lightDir = normalize(-light.direction);

	// diffuse
	float diff = max(dot(normal, lightDir), 0.0);

	vec3 ambient;
	vec3 diffuse;	

	// combine results
	if (useTexture == true) {
 		ambient = light.ambient * vec3(texture(tex, texCoord));
 		diffuse = light.diffuse * diff * vec3(texture(tex, texCoord));
	} 
	else {
		ambient = light.ambient * color;
		diffuse = light.diffuse * diff * color;
	}

	return (ambient + diffuse);
}
