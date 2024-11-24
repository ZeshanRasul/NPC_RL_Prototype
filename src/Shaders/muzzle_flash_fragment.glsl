#version 460 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D muzzleTexture;
uniform vec3 tint;  
uniform float alpha;

void main() {
    vec4 texColor = texture(muzzleTexture, TexCoords);
    //FragColor = vec4(texColor.rgb * tint, texColor.a * alpha);

    if (texColor.a < 0.1) {
		discard;
	}

    FragColor = texColor;
    //FragColor = vec4(texColor.rgb, texColor.a * alpha);
}
