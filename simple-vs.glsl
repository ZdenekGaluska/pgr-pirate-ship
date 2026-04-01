#version 330 core

uniform mat4 mPVM;
uniform mat4 mModel;
uniform float uWaterUVScale;

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 texCoord;

out vec2 vTexCoord;
out vec3 vNormal;
out vec3 vWorldPos;

void main() {
    // Transformace pozice do world space — potrebujeme ji pro UV i pro osvetleni
    vWorldPos = vec3(mModel * vec4(position, 1.0));

    // UV: bud z VBO (lod), nebo z world XZ (voda)
    if (uWaterUVScale > 0.0)
        vTexCoord = vWorldPos.xz * uWaterUVScale;   // world coords -> UV, mierka = kolikrat se textura opakuje
    else
        vTexCoord = texCoord;                        // normalni UV z VBO

    mat3 normalMatrix = transpose(inverse(mat3(mModel)));
    vNormal   = normalize(normalMatrix * normal);
    gl_Position = mPVM * vec4(position, 1.0);
}