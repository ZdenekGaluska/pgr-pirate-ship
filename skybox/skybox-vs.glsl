#version 330 core
// Skybox vertex shader.
// Trick 1: mat3(u_view) strips translation so skybox never moves with camera.
// Trick 2: pos.xyww sets depth = 1.0 after perspective divide,
//          so skybox is always drawn behind every other object.

layout(location = 0) in vec3 a_position;

uniform mat4 u_projection;
uniform mat4 u_view;

out vec3 v_texCoord;

void main() {
    // The position vector doubles as the cubemap lookup direction
    v_texCoord = a_position;

    // Remove translation component from view matrix
    mat4 viewNoTranslation = mat4(mat3(u_view));
    vec4 pos = u_projection * viewNoTranslation * vec4(a_position, 1.0);

    // Force z/w = 1.0 (maximum depth) so skybox loses every depth test
    gl_Position = pos.xyww;
}