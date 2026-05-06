#version 330 core
//----------------------------------------------------------------------------------------
/**
 * \file    cloud-vs.glsl
 * \author  galuszde
 * \note    Code comments generated with AI assistance (Claude, Anthropic).
 * \brief   Cloud plane vertex shader -- passes UV with spritesheet frame offset.
 */
//----------------------------------------------------------------------------------------

uniform mat4  mPVM;
uniform vec2  uFrameOffset;   // UV offset of the current spritesheet frame

layout(location = 0) in vec3 position;
layout(location = 1) in vec2 uv;

out vec2 vUV;

void main() {
    // Scale UV to one frame cell (2 cols x 3 rows) then shift to current frame
    vUV = uv * vec2(0.5, 0.3333) + uFrameOffset;
    gl_Position = mPVM * vec4(position, 1.0);
}