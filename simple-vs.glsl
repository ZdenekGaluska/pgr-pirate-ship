#version 330 core

uniform mat4 mPVM;
uniform mat4 mModel;

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;

layout(location = 2) in vec2 texCoord;
out vec2 vTexCoord;

out vec3 vNormal;
out vec3 vWorldPos;

void main() {
    vTexCoord = texCoord;
    mat3 normalMatrix = transpose(inverse(mat3(mModel)));
    vNormal   = normalize(normalMatrix * normal);
    vWorldPos = vec3(mModel * vec4(position, 1.0));
    gl_Position = mPVM * vec4(position, 1.0);
}