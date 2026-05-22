#version 460 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec2 aTexCoords;

out vec2 TexCoords;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main() {
    TexCoords = aTexCoords;

    vec3 camRight = vec3(view[0][0], view[1][0], view[2][0]); 
    vec3 camUp = vec3(view[0][1], view[1][1], view[2][1]);

    vec3 worldPos = mat3(model) * aPos;
    worldPos = worldPos + ((aPos.x * 0.05) * camRight) + ((aPos.y * 0.05) * camUp);

    gl_Position = projection * view * model * vec4(worldPos, 1.0);
}
