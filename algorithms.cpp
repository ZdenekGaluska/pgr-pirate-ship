//----------------------------------------------------------------------------------------
/**
 * \file    algorithms.cpp
 * \author  galuszde
 * \note    Code comments generated with AI assistance (Claude, Anthropic).
 * \brief   CPU Gerstner wave algorithm implementation.
 */
 //----------------------------------------------------------------------------------------

#include "algorithms.h"
#include <cmath>

namespace galuszde {

    /// @brief  Adds the displacement and normal contribution of one Gerstner wave component.
    ///
    /// Mirrors the per-wave math from simple-vs.glsl exactly.
    /// The results are accumulated into the inout references — the function never resets them,
    /// so the caller can call this repeatedly and the contributions sum up correctly.
    ///
    /// @param xzPos        World XZ coordinate of the point to evaluate (same input as the shader).
    /// @param dir          Normalised 2D propagation direction of this wave component.
    /// @param amplitude    Wave height (crest-to-rest distance in world units).
    /// @param wavelength   Distance between two consecutive crests in world units.
    /// @param speed        Propagation speed (world units per second).
    /// @param steepness    Sharpness of the crest (0 = smooth sine, higher = sharper peak).
    /// @param phase        Per-wave phase offset so different components start at different positions.
    /// @param time         Current simulation time (must match the shader's u_time value).
    /// @param displacement Inout — accumulated XYZ displacement; this function adds to it.
    /// @param nrm          Inout — accumulated surface normal; this function adds to it.
    static void gerstnerWaveContrib(
        glm::vec2  xzPos,
        glm::vec2  dir,
        float      amplitude,
        float      wavelength,
        float      speed,
        float      steepness,
        float      phase,
        float      time,
        glm::vec3& displacement,
        glm::vec3& nrm
    ) {
        const float PI = 3.14159265f;

        // Wave number — how many full oscillations fit into one unit of length.
        // Shorter wavelength = higher k = more oscillations per world unit.
        float k = 2.0f * PI / wavelength;

        // Angular frequency — how fast the phase changes over time.
        float omega = speed * k;

        // Phase angle for this point in this wave component.
        // dot(dir, xzPos)  = distance of the point along the propagation direction
        // - omega * time   = advances the wave forward over time
        // + phase          = individual offset so components don't all start at the same phase
        float phi = k * glm::dot(dir, xzPos) - omega * time + phase;

        // Normalised steepness coefficient.
        // Dividing by (k * amplitude) makes steepness = 1 the physical maximum
        // before the geometry would self-intersect (crest loops over).
        float Q = steepness / (k * amplitude);

        // XZ displacement — vertices move horizontally as well as vertically.
        // This lateral shift is what gives Gerstner waves their characteristic sharp crests,
        // unlike a pure sine wave which only moves vertices up and down.
        displacement.x += Q * amplitude * dir.x * std::cos(phi);
        displacement.z += Q * amplitude * dir.y * std::cos(phi);  // dir.y is the Z component of the 2D direction

        // Y displacement — vertical movement.
        displacement.y += amplitude * std::sin(phi);

        // Normal — analytical derivative of the displacement function.
        // Using the closed-form derivative is more accurate than a finite-difference
        // approximation (sampling two neighbouring points and taking the difference).
        nrm.x -= dir.x * k * amplitude * std::cos(phi);
        nrm.z -= dir.y * k * amplitude * std::cos(phi);
        nrm.y -= Q * k * amplitude * std::sin(phi);
    }

    GerstnerResult evaluateGerstner(glm::vec2 worldXZ, float time) {

        // Start from zero displacement and an upward-pointing normal (flat calm surface).
        glm::vec3 disp = glm::vec3(0.0f);
        glm::vec3 normal = glm::vec3(0.0f, 1.0f, 0.0f);

        // Ten wave components — identical parameters and order as in simple-vs.glsl.
        // Any change here must be mirrored in the shader, otherwise the ship will
        // appear to float above or sink below the visible water surface.
        gerstnerWaveContrib(worldXZ, glm::normalize(glm::vec2(1.0f, 0.2f)), 0.800f, 25.0f, 1.0f, 0.040f, 0.0f, time, disp, normal);
        gerstnerWaveContrib(worldXZ, glm::normalize(glm::vec2(0.8f, 0.5f)), 0.600f, 20.0f, 0.9f, 0.035f, 2.3f, time, disp, normal);
        gerstnerWaveContrib(worldXZ, glm::normalize(glm::vec2(-0.4f, 1.0f)), 0.140f, 15.0f, 1.3f, 0.015f, 1.1f, time, disp, normal);
        gerstnerWaveContrib(worldXZ, glm::normalize(glm::vec2(0.3f, -0.8f)), 0.120f, 13.0f, 1.4f, 0.012f, 3.7f, time, disp, normal);
        gerstnerWaveContrib(worldXZ, glm::normalize(glm::vec2(-0.7f, 0.4f)), 0.120f, 10.0f, 1.7f, 0.010f, 0.8f, time, disp, normal);
        gerstnerWaveContrib(worldXZ, glm::normalize(glm::vec2(0.6f, -0.3f)), 0.068f, 9.0f, 1.6f, 0.010f, 2.9f, time, disp, normal);
        gerstnerWaveContrib(worldXZ, glm::normalize(glm::vec2(-0.2f, -0.9f)), 0.060f, 7.0f, 2.0f, 0.008f, 4.2f, time, disp, normal);
        gerstnerWaveContrib(worldXZ, glm::normalize(glm::vec2(0.9f, 0.1f)), 0.040f, 6.0f, 2.1f, 0.007f, 1.6f, time, disp, normal);
        gerstnerWaveContrib(worldXZ, glm::normalize(glm::vec2(-0.5f, 0.7f)), 0.028f, 4.5f, 2.4f, 0.005f, 3.3f, time, disp, normal);
        gerstnerWaveContrib(worldXZ, glm::normalize(glm::vec2(0.4f, 0.6f)), 0.020f, 3.0f, 2.8f, 0.004f, 5.1f, time, disp, normal);

        // After summing all contributions the accumulated normal is no longer unit length.
        // normalize() converts it back to length 1, which shaders and rotation math require.
        return { disp, glm::normalize(normal) };
    }

} // namespace galuszde