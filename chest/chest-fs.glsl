#version 330 core
//----------------------------------------------------------------------------------------
/**
 * \file    chest/chest-fs.glsl
 * \author  galuszde
 * \note    Code comments generated with AI assistance (Claude, Anthropic).
 * \brief   Fragment shader for treasure chests.
 *
 * Implements:
 *   - Tangent-space normal mapping (rubric 12d -- normal maps)
 *   - Environment map reflections via the existing skybox cubemap (rubric 12d -- env map)
 *   - Phong lighting with per-object material uniforms (rubric 9b -- materials via uniform)
 *
 * Texture units expected:
 *   unit 0 = u_diffuseMap  (GL_TEXTURE_2D  -- baseColor texture)
 *   unit 1 = u_normalMap   (GL_TEXTURE_2D  -- tangent-space normal map)
 *   unit 2 = u_envMap      (GL_TEXTURE_CUBE_MAP -- skybox cubemap for reflections)
 */
//----------------------------------------------------------------------------------------

// --- Texture samplers ---
uniform sampler2D   u_diffuseMap;   // albedo / base color texture
uniform sampler2D   u_normalMap;    // tangent-space normal map (RGB encoded in [0,1])
uniform samplerCube u_envMap;       // cubemap for environment reflections (reuses skybox)

// --- Lighting ---
uniform vec3 vLightDir;    // normalized directional light direction (world space)
uniform vec3 vCameraPos;   // camera/eye position (world space)

// --- Per-object material parameters (rubric 9b) ---
// Set differently for ship, volcano, and chest in the CPU draw calls.
uniform float u_ambient;     // base ambient light factor   (e.g. 0.3)
uniform vec3  u_specular;    // specular color tint         (e.g. vec3(0.8, 0.7, 0.3) for gold)
uniform float u_shininess;   // Phong exponent              (e.g. 64.0 for polished wood)
uniform float u_envStrength; // weight of environment reflection blended onto surface

// --- Inputs from vertex shader ---
in vec2 vUV;
in vec3 vWorldPos;
in mat3 vTBN;    // world-space TBN matrix (columns: tangent, bitangent, normal)

out vec4 fragColor;

void main() {
    // ---- Normal mapping -------------------------------------------------
    //
    // Normal maps store per-texel normals in tangent space.
    // RGB channels encode xyz in [0,1]; remap to [-1,1] by * 2 - 1.
    // Multiplying by vTBN rotates from tangent space to world space so the
    // Phong equations work in the same space as vWorldPos and vCameraPos.
    vec3 normalTangent = texture(u_normalMap, vUV).rgb * 2.0 - 1.0;
    vec3 N = normalize(vTBN * normalTangent);   // world-space shading normal

    // ---- Phong lighting -------------------------------------------------
    vec3 L = normalize(vLightDir);
    vec3 V = normalize(vCameraPos - vWorldPos);   // view direction (fragment to camera)
    vec3 R = reflect(-L, N);                       // reflected light direction

    // Diffuse component: angle between surface normal and light direction.
    // max() clamps to 0 so back-lit fragments receive no direct light.
    float diffuse = max(dot(N, L), 0.0);

    // Specular component: how much reflected light points toward the camera.
    // Higher u_shininess = smaller, sharper highlight (more polished surface).
    float specular = pow(max(dot(R, V), 0.0), u_shininess);

    // ---- Environment reflection (env map) --------------------------------
    //
    // Reflect the view direction around the surface normal to get the direction
    // that comes from the environment and hits the camera after reflecting.
    // textureCube lookup gives the sky/scene color from that direction.
    vec3 reflectDir = reflect(-V, N);
    vec3 envSample  = texture(u_envMap, reflectDir).rgb;

    // ---- Combine --------------------------------------------------------
    vec3 albedo = texture(u_diffuseMap, vUV).rgb;

    // Phong: ambient baseline + diffuse response + specular highlight
    vec3 color = albedo * (u_ambient + diffuse)
               + u_specular * specular;

    // Blend in environment reflection on top of the lit surface.
    // u_envStrength = 0 means no reflection, 1 means fully mirror-like.
    color = mix(color, envSample, u_envStrength * specular);

    fragColor = vec4(color, 1.0);
}
