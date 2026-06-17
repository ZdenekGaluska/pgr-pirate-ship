#version 330 core
//----------------------------------------------------------------------------------------
/**
 * \file    simple-vs.glsl
 * \author  galuszde
 * \note    Code comments generated with AI assistance (Claude, Anthropic).
 * \brief   Vertex shader with Gerstner wave displacement for water and standard
 *          transform for all other objects (ship, islands).
 *          uWaterUVScale > 0 switches to water mode, 0 = standard object mode.
 */
//----------------------------------------------------------------------------------------

uniform mat4  mPVM;             // projection * view * model -- transforms vertex to screen space
uniform mat4  mModel;           // model matrix -- transforms vertex to world space
uniform float uWaterUVScale;    // > 0 = water mode (Gerstner displacement), 0 = standard object
uniform float u_time;           // elapsed time for wave animation, must match CPU divisor (/ 800.0f)

layout(location = 0) in vec3 position;  // vertex position from VAO (attribute 0)
layout(location = 1) in vec3 normal;    // vertex normal from VAO (attribute 1)
layout(location = 2) in vec2 texCoord;  // UV coordinates from VAO (attribute 2)

out vec2 vTexCoord;     // passed to fragment shader -- UV coordinates
out vec3 vNormal;       // passed to fragment shader -- surface normal (world space)
out vec3 vWorldPos;     // passed to fragment shader -- position in world space (for lighting)

// gerstnerWave()
//
// Adds one wave component to displacement and normal accumulator.
// Calling this multiple times with different parameters and summing results
// produces a realistic irregular ocean surface (wave superposition).
//
// Each wave is defined by:
//   dir        = propagation direction in XZ plane (normalized 2D vector)
//   amplitude  = wave height
//   wavelength = distance between two crests
//   speed      = propagation speed
//   steepness  = crest sharpness (0 = pure sine, higher = sharper crest)
//   phase      = phase offset, shifts wave pattern in space
//
// inout parameters are accumulated across multiple calls -- do not reset between calls.
void gerstnerWave(
    vec2  xzPos,
    vec2  dir,
    float amplitude,
    float wavelength,
    float speed,
    float steepness,
    float phase,
    inout vec3 displacement,
    inout vec3 nrm
) {
    // Wave number -- how many full cycles per unit length (higher = shorter wave)
    float k     = 2.0 * 3.14159265 / wavelength;

    // Angular frequency -- how fast the wave moves in time
    float omega = speed * k;

    // Phase at this vertex -- where on the sine curve this vertex currently sits.
    // dot(dir, xzPos) projects the vertex onto the wave direction axis.
    float phi   = k * dot(dir, xzPos) - omega * u_time + phase;

    // Normalized steepness -- divided by k*amplitude so steepness=1 is the true maximum
    // before geometry folds over itself
    float Q     = steepness / (k * amplitude);

    // XZ displacement -- vertices move sideways as well as up/down.
    // This creates the characteristic sharp crests of Gerstner waves
    // (pure sine waves only displace vertically).
    displacement.x += Q * amplitude * dir.x * cos(phi);
    displacement.z += Q * amplitude * dir.y * cos(phi);

    // Vertical displacement
    displacement.y += amplitude * sin(phi);

    // Analytical normal -- exact derivative of the displacement function.
    // More accurate than numerical estimation (comparing neighbor vertices)
    // and avoids artifacts at grid edges.
    nrm.x -= dir.x * k * amplitude * cos(phi);
    nrm.z -= dir.y * k * amplitude * cos(phi);
    nrm.y -= Q     * k * amplitude * sin(phi);
}

void main() {
    vWorldPos = vec3(mModel * vec4(position, 1.0));

    if (uWaterUVScale > 0.0) {
        // Water vertex -- apply Gerstner wave superposition.
        // Ten waves with varying direction, amplitude, wavelength, and speed
        // produce a visually irregular ocean surface.
        vec3 disp   = vec3(0.0);              // start from zero displacement
        vec3 nAccum = vec3(0.0, 1.0, 0.0);   // start from flat upward normal

        gerstnerWave(vWorldPos.xz, normalize(vec2( 1.0,  0.2)), 0.800, 25.0, 1.0, 0.040, 0.0, disp, nAccum);
        gerstnerWave(vWorldPos.xz, normalize(vec2( 0.8,  0.5)), 0.600, 20.0, 0.9, 0.035, 2.3, disp, nAccum);
        gerstnerWave(vWorldPos.xz, normalize(vec2(-0.4,  1.0)), 0.140, 15.0, 1.3, 0.015, 1.1, disp, nAccum);
        gerstnerWave(vWorldPos.xz, normalize(vec2( 0.3, -0.8)), 0.120, 13.0, 1.4, 0.012, 3.7, disp, nAccum);
        gerstnerWave(vWorldPos.xz, normalize(vec2(-0.7,  0.4)), 0.120, 10.0, 1.7, 0.010, 0.8, disp, nAccum);
        gerstnerWave(vWorldPos.xz, normalize(vec2( 0.6, -0.3)), 0.068,  9.0, 1.6, 0.010, 2.9, disp, nAccum);
        gerstnerWave(vWorldPos.xz, normalize(vec2(-0.2, -0.9)), 0.060,  7.0, 2.0, 0.008, 4.2, disp, nAccum);
        gerstnerWave(vWorldPos.xz, normalize(vec2( 0.9,  0.1)), 0.040,  6.0, 2.1, 0.007, 1.6, disp, nAccum);
        gerstnerWave(vWorldPos.xz, normalize(vec2(-0.5,  0.7)), 0.029,  4.5, 2.4, 0.005, 3.3, disp, nAccum);
        gerstnerWave(vWorldPos.xz, normalize(vec2( 0.4,  0.6)), 0.020,  3.0, 2.8, 0.004, 5.1, disp, nAccum);

        // Apply accumulated displacement to vertex position
        vec3 displacedPos = position + disp;

        // Normal is already in world space -- Gerstner computes it in world coords directly
        vNormal   = normalize(nAccum);

        // UV offset driven by time -- creates the illusion of flowing water (rubric 14a)
        vTexCoord = (vWorldPos.xz + disp.xz) * uWaterUVScale + vec2(u_time * 0.005, u_time * 0.007);

        vWorldPos = vec3(mModel * vec4(displacedPos, 1.0));
        gl_Position = mPVM * vec4(displacedPos, 1.0);

    } else {
        // Standard object (ship, islands) -- no displacement, standard transform.
        // Normal matrix corrects normal direction under non-uniform scaling.
        mat3 normalMatrix = transpose(inverse(mat3(mModel)));
        vNormal     = normalize(normalMatrix * normal);
        vTexCoord   = texCoord;
        gl_Position = mPVM * vec4(position, 1.0);
    }
}