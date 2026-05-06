#version 330 core
//----------------------------------------------------------------------------------------
/**
 * \file    cloud-fs.glsl
 * \author  galuszde
 * \note    Code comments generated with AI assistance (Claude, Anthropic).
 * \brief   Cloud plane fragment shader -- samples spritesheet, discards dark pixels.
 *
 * Black background in the spritesheet is discarded via alpha threshold so the
 * plane itself is invisible and only the cloud shape remains (rubric 15b).
 */
//----------------------------------------------------------------------------------------

uniform sampler2D uTexture;
uniform float     uAlpha;   // global opacity for fade effects

in  vec2 vUV;
out vec4 fragColor;

void main() {
    vec4 tex = texture(uTexture, vUV);

    // Derive alpha from brightness -- black background becomes transparent,
    // white cloud becomes opaque.  Threshold 0.05 eliminates near-black fringe.
    float alpha = dot(tex.rgb, vec3(0.333)) * uAlpha;
    if (alpha < 0.05) discard;

    fragColor = vec4(tex.rgb, alpha);
}