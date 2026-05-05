#version 330 core
//----------------------------------------------------------------------------------------
/**
 * \file    chest/chest-vs.glsl
 * \author  galuszde
 * \note    Code comments generated with AI assistance (Claude, Anthropic).
 * \brief   Vertex shader for treasure chests.
 *
 * Computes the TBN (Tangent-Bitangent-Normal) matrix in world space and passes
 * it to the fragment shader so normal map vectors can be transformed from
 * tangent space into world space for correct Phong lighting.
 *
 * Input layout (must match obj_loader.cpp interleaved buffer):
 *   location 0 = vec3 position
 *   location 1 = vec3 normal
 *   location 2 = vec2 uv
 *   location 3 = vec3 tangent
 */
//----------------------------------------------------------------------------------------

uniform mat4 mPVM;    // projection * view * model -- transforms to clip space
uniform mat4 mModel;  // model matrix -- transforms to world space

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 uv;
layout(location = 3) in vec3 tangent;

out vec2 vUV;          // UV coordinates for texture sampling
out vec3 vWorldPos;    // world-space position for lighting distance/direction
out mat3 vTBN;         // world-space TBN matrix: transforms tangent-space to world-space

void main() {
    // Normal matrix: transpose(inverse(mat3(model))) corrects normals when the
    // model matrix contains non-uniform scale.  mat3() strips the translation.
    mat3 normalMatrix = transpose(inverse(mat3(mModel)));

    // Transform T and N into world space using the normal matrix.
    vec3 T = normalize(normalMatrix * tangent);
    vec3 N = normalize(normalMatrix * normal);

    // Re-orthogonalise T against N (Gram-Schmidt).
    // Floating-point errors and interpolation can make T and N slightly non-perpendicular.
    // Subtracting the component of T along N projects T onto the plane perpendicular to N.
    T = normalize(T - dot(T, N) * N);

    // Bitangent = N x T (cross product gives the remaining orthogonal axis).
    // World-space bitangent needed to form a complete orthonormal frame.
    vec3 B = cross(N, T);

    // Assemble TBN: columns are the three world-space basis vectors of tangent space.
    // Multiplying a tangent-space normal by vTBN rotates it into world space.
    vTBN     = mat3(T, B, N);
    vUV      = uv;
    vWorldPos = vec3(mModel * vec4(position, 1.0));
    gl_Position = mPVM * vec4(position, 1.0);
}
