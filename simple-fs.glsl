#version 330 core
//----------------------------------------------------------------------------------------
/**
 * \file    simple-fs.glsl
 * \author  galuszde
 * \note    Code comments generated with AI assistance (Claude, Anthropic).
 * \brief   Fragment shader implementing Phong lighting model with per-object materials.
 *
 * Material properties (u_ambient, u_specularStr, u_shininess) are passed as uniforms
 * so each object (ship, volcano, water) can have visually distinct material behaviour
 * without separate shader programs (rubric 9b -- materials via uniform).
 *
 * Example material values set in drawXxx() functions:
 *   Ship:    ambient=0.30, specularStr=0.25, shininess=32   (painted wood, mild gloss)
 *   Volcano: ambient=0.45, specularStr=0.05, shininess=4    (rough porous rock, nearly matte)
 *   Water:   ambient=0.50, specularStr=0.90, shininess=256  (highly reflective surface)
 */
//----------------------------------------------------------------------------------------

uniform vec3      vDiffuse;      // material base color
uniform vec3      vLightDir;     // normalized light direction (world space)
uniform vec3      vCameraPos;    // camera position (world space), for specular
uniform sampler2D uTexture;      // diffuse texture bound to unit 0

// Per-object material parameters (rubric 9b)
uniform float u_ambient;     // constant ambient factor (prevents fully black shadows)
uniform float u_specularStr; // specular intensity multiplier (0 = matte, 1 = shiny)
uniform float u_shininess;   // Phong exponent (higher = smaller, sharper highlight)

in vec3 vNormal;
in vec3 vWorldPos;
in vec2 vTexCoord;

out vec4 fragmentColor;

void main() {
    // Normalize after interpolation -- rasterizer linearly interpolates normals,
    // which shortens unit vectors between vertices; re-normalizing fixes this.
    vec3 N = normalize(vNormal);
    vec3 L = normalize(vLightDir);
    vec3 V = normalize(vCameraPos - vWorldPos);
    vec3 R = reflect(-L, N);

    // Diffuse: dot(N, L) = cosine of angle between light and surface normal.
    // max() = no negative light contribution from back-facing geometry.
    float diffuse  = max(dot(N, L), 0.0);

    // Specular: how much reflected light points directly at the camera.
    // pow() sharpens the highlight according to the per-object shininess value.
    float specular = pow(max(dot(R, V), 0.0), u_shininess) * u_specularStr;

    vec3 texColor = texture(uTexture, vTexCoord).rgb;

    // Combine diffuse texture with Phong components.
    // u_ambient sets the minimum brightness so shadowed areas remain visible.
    vec3 color = texColor * (u_ambient + diffuse) + vec3(specular);

    fragmentColor = vec4(color, 1.0);
}
