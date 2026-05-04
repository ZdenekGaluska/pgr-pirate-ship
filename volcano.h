#pragma once
//----------------------------------------------------------------------------------------
/**
 * \file    volcano.h
 * \author  galuszde
 * \note    Code comments generated with AI assistance (Claude, Anthropic).
 * \brief   Hardcoded volcano mesh -- geometry defined directly in C++ source.
 *
 * Geometry is defined as a vertex attribute array in volcano.cpp (not loaded from file,
 * not a generic grid).  This satisfies the "hardcode v src" rubric requirement.
 *
 * VBO layout: interleaved  pos(3) | normal(3) | uv(2)  per vertex
 * Shader attribute locations: 0=position, 1=normal, 2=uv  (same as ship/water)
 */
 //----------------------------------------------------------------------------------------

#include "globals.h"

namespace galuszde {

    /// @brief  Builds volcano geometry, uploads VBO/IBO/VAO to GPU, loads texture.
    ///         Writes result into g_volcano (defined in globals.cpp).
    ///         Must be called after pgr::initialize().
    /// @return true always (missing texture is non-fatal, renders with fallback color).
    bool initVolcano();

} // namespace galuszde#pragma once
