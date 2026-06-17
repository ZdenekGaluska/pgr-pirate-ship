#version 330 core
//----------------------------------------------------------------------------------------
/**
 * \file    simple-fs.glsl
 * \author  galuszde
 * \note    Code comments generated with AI assistance (Claude, Anthropic).
 * \brief   Fragment shader -- Phong lighting with directional light, hardcoded
 *          point light (volcano lava) and hardcoded spotlight (camera torch).
 *
 * Light sources:
 *   1. Directional -- sun, direction via uniform vLightDir
 *   2. Point       -- red lava glow at volcano crater, hardcoded position (rubric 8.2)
 *   3. Spotlight   -- camera torch, follows vCameraPos/direction (rubric 7a, 8.3)
 */
//----------------------------------------------------------------------------------------

uniform vec3      vDiffuse;
uniform vec3      vLightDir;
uniform vec3      vCameraPos;
uniform vec3      vCameraDir;   // normalized camera look direction (for spotlight cone)
uniform sampler2D uTexture;

uniform float u_ambient;
uniform float u_specularStr;
uniform float u_shininess;

uniform bool u_lavaActive;   // true = lava point light on

in vec3 vNormal;
in vec3 vWorldPos;
in vec2 vTexCoord;

out vec4 fragmentColor;

// -------------------------------------------------------------------------
// Hardcoded light parameters 
// -------------------------------------------------------------------------

// Point light: red lava glow at the volcano crater (rubric 8.2)
const vec3  LAVA_POS       = vec3(30.0, 10.0, -20.0);  // crater world position
const vec3  LAVA_COLOR     = vec3(24.0, 0.1, 0.0);
const float LAVA_CONSTANT  = 1.0;
const float LAVA_LINEAR    = 0.005;
const float LAVA_QUADRATIC = 0.0008;

// Spotlight: camera torch (rubric 7a, 8.3)
const vec3  SPOT_COLOR     = vec3(0.95, 0.9, 0.8);   // warm white torch
const float SPOT_CUTOFF    = 0.97;                    // cos(~14 degrees) -- narrow beam
const float SPOT_OUTER     = 0.94;                    // cos(~20 degrees) -- soft edge
const float SPOT_CONSTANT  = 1.0;
const float SPOT_LINEAR    = 0.027;
const float SPOT_QUADRATIC = 0.0028;

// -------------------------------------------------------------------------
// calcDirectional() -- sun
// -------------------------------------------------------------------------

/// @brief  Phong contribution from the directional sun light.
/// @param  N  Surface normal (normalized).
/// @param  V  View direction (fragment to camera, normalized).
/// @return    RGB light contribution (not yet multiplied by albedo).
vec3 calcDirectional(vec3 N, vec3 V) {
    vec3  L       = normalize(vLightDir);
    vec3  R       = reflect(-L, N);
    float diff    = max(dot(N, L), 0.0);
    float spec    = pow(max(dot(R, V), 0.0), u_shininess) * u_specularStr;
    return vec3(diff + spec);
}

// -------------------------------------------------------------------------
// calcPointLight() -- volcano lava glow
// -------------------------------------------------------------------------

/// @brief  Phong contribution from the hardcoded lava point light.
///         Attenuation: 1 / (c + l*d + q*d*d) keeps brightness physically plausible.
/// @param  N  Surface normal (normalized).
/// @param  V  View direction (normalized).
/// @return    RGB light contribution (tinted by LAVA_COLOR).
vec3 calcPointLight(vec3 N, vec3 V) {
    vec3  toLight = LAVA_POS - vWorldPos;
    float dist    = length(toLight);
    vec3  L       = toLight / dist;
    vec3  R       = reflect(-L, N);

    float diff    = max(dot(N, L), 0.0);
    float spec    = pow(max(dot(R, V), 0.0), u_shininess) * u_specularStr;
    float atten   = 1.0 / (LAVA_CONSTANT + LAVA_LINEAR * dist + LAVA_QUADRATIC * dist * dist);

    return LAVA_COLOR * (diff + spec) * atten;
}

// -------------------------------------------------------------------------
// calcSpotlight() -- camera torch
// -------------------------------------------------------------------------

/// @brief  Phong contribution from the hardcoded camera spotlight (rubric 7a, 8.3).
///         Position  = vCameraPos (already a uniform).
///         Direction = normalize(vWorldPos - vCameraPos) -- points from camera to fragment.
///         Smooth edge via smoothstep between SPOT_CUTOFF and SPOT_OUTER.
/// @param  N  Surface normal (normalized).
/// @param  V  View direction (normalized).
/// @return    RGB light contribution (tinted by SPOT_COLOR).
vec3 calcSpotlight(vec3 N, vec3 V) {
    vec3  toLight  = vCameraPos - vWorldPos;
    float dist     = length(toLight);
    vec3  L        = toLight / dist;

    // Cone check: angle between camera look direction and direction to fragment.
    // vCameraDir points where the camera looks; -L points from camera to fragment.
    float cosTheta  = dot(normalize(vCameraDir), normalize(-toLight));
    float intensity = smoothstep(SPOT_OUTER, SPOT_CUTOFF, cosTheta);

    if (intensity <= 0.0) return vec3(0.0);   // outside cone -- skip calculation

    vec3  R    = reflect(-L, N);
    float diff = max(dot(N, L), 0.0);
    float spec = pow(max(dot(R, V), 0.0), u_shininess) * u_specularStr;
    float atten = 1.0 / (SPOT_CONSTANT + SPOT_LINEAR * dist + SPOT_QUADRATIC * dist * dist);

    return SPOT_COLOR * (diff + spec) * atten * intensity;
}

// -------------------------------------------------------------------------
// main()
// -------------------------------------------------------------------------

void main() {
    vec3 N = normalize(vNormal);
    vec3 V = normalize(vCameraPos - vWorldPos);

    vec3 texColor = texture(uTexture, vTexCoord).rgb;

    // Combine all three light contributions
    vec3 lighting = vec3(u_ambient)                                    // ambient baseline
                  + calcDirectional(N, V)                              // sun
                  + (u_lavaActive ? calcPointLight(N, V) : vec3(0.0)) // lava glow (toggled)
                  + calcSpotlight(N, V);                               // camera torch

    // Linear fog: blend scene colour toward fog colour based on camera distance (rubric 17a).
    // Fog starts at FOG_START units and is fully opaque at FOG_END units.
    const vec3  FOG_COLOR = vec3(0.55, 0.65, 0.75);   // pale blue-grey sea haze
    const float FOG_START = 30.0;
    const float FOG_END   = 120.0;

    float fragDist  = length(vCameraPos - vWorldPos);
    float fogFactor = clamp((FOG_END - fragDist) / (FOG_END - FOG_START), 0.0, 1.0);
    vec3  litColor  = texColor * lighting;
    fragmentColor   = vec4(mix(FOG_COLOR, litColor, fogFactor), 1.0);
}