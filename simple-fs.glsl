#version 330 core
//----------------------------------------------------------------------------------------
/**
 * \file    simple-fs.glsl
 * \author  galuszde
 * \note    Code comments generated with AI assistance (Claude, Anthropic).
 * \brief   Fragment shader implementing Phong lighting model.
 *          Combines ambient, diffuse, and specular components with a diffuse texture.
 */
//----------------------------------------------------------------------------------------

// uniforms -- sent from CPU every frame via glUniform*
uniform vec3 vDiffuse;          // material base color
uniform vec3 vLightDir;         // normalized light direction (world space)
uniform vec3 vCameraPos;        // camera position (world space), used for specular
uniform sampler2D uTexture;     // diffuse texture bound to unit 0

// inputs interpolated per-pixel from vertex shader outputs
in vec3 vNormal;                // surface normal (world space)
in vec3 vWorldPos;              // fragment position (world space)
in vec2 vTexCoord;              // UV texture coordinates

out vec4 fragmentColor;

void main() {
    // Normalize after interpolation -- rasterizer interpolates linearly,
    // which shortens unit vectors between vertices
    vec3 N = normalize(vNormal);
    vec3 L = normalize(vLightDir);

    // Vector from fragment to camera -- needed for specular highlight
    vec3 V = normalize(vCameraPos - vWorldPos);

    // Reflected light direction around the surface normal
    // -L because reflect() expects the vector pointing away from the surface
    vec3 R = reflect(-L, N);

    // Ambient -- constant base light, prevents fully black shadows
    float ambient  = 0.35;

    // Diffuse -- dot product gives cosine of angle between normal and light.
    // max() clamps to 0 so back-facing surfaces receive no direct light.
    float diffuse  = max(dot(N, L), 0.0);

    // Specular -- how much reflected light aims directly at the camera.
    // pow(..., 64.0) sharpens the highlight to a small glossy spot.
    float specular = pow(max(dot(R, V), 0.0), 64.0) * 0.4;

    // Sample diffuse texture at interpolated UV coordinates
    vec3 texColor = texture(uTexture, vTexCoord).rgb;

    // Combine: texture modulated by lighting, specular added as white highlight
    vec3 color = texColor * (ambient + diffuse) + vec3(specular);

    fragmentColor = vec4(color, 1.0);   // alpha = 1.0 -- fully opaque
}