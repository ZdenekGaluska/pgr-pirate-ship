#pragma once
//----------------------------------------------------------------------------------------
/**
 * \file    algorithms.h
 * \author  galuszde
 * \note    Code comments generated with AI assistance (Claude, Anthropic).
 * \brief   CPU mirror of the GPU Gerstner wave algorithm.
 *
 * Provides the same wave math as simple-vs.glsl so the CPU can query water
 * height and surface normals without reading back data from the GPU.
 * Used by physics.cpp to track ship buoyancy and tilt.
 */
 //----------------------------------------------------------------------------------------

#include "globals.h"

namespace galuszde {

    /// @brief Result of evaluating all Gerstner wave contributions at one world-space point.
    struct GerstnerResult {
        glm::vec3 displacement;   ///< XYZ offset from the flat rest surface caused by all waves.
        glm::vec3 normal;         ///< Unit surface normal at this point (points away from water).
    };

    /// @brief  Evaluates the full set of Gerstner waves at a given world-space XZ position.
    ///
    /// Sums the contributions of all 10 wave components (identical parameters to the
    /// vertex shader) and returns the resulting displacement and surface normal.
    /// The caller must supply the same time value used in the shader uniform u_time
    /// (i.e. glutGet(GLUT_ELAPSED_TIME) / 800.0f) to keep CPU and GPU in sync.
    ///
    /// @param worldXZ  World-space XZ position to sample (ship position, probe point, etc.).
    /// @param time     Simulation time in seconds, scaled identically to the vertex shader.
    /// @return         Displacement vector and normalised surface normal at worldXZ.
    GerstnerResult evaluateGerstner(glm::vec2 worldXZ, float time);

} // namespace galuszde