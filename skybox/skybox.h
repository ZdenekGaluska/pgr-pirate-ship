#pragma once
#pragma once
//----------------------------------------------------------------------------------------
/**
 * \file    skybox.h
 * \author  galuszde
 * \note    Code comments generated with AI assistance (Claude, Anthropic).
 * \brief   Skybox cubemap -- loading 6 face textures and drawing an infinite sky.
 *
 * The skybox is a unit cube rendered around the camera.
 * Translation is stripped from the view matrix so the sky never moves.
 * Depth is forced to 1.0 so the skybox always renders behind all other objects.
 *
 * Face files expected in skybox/ directory (relative to working directory):
 *   right.jpg  left.jpg  top.jpg  bottom.jpg  front.jpg  back.jpg
 */
 //----------------------------------------------------------------------------------------
#include "../globals.h"

namespace galuszde {

    /// @brief  Loads 6 cubemap face textures, builds a unit-cube VAO, and uploads
    ///         everything to the GPU. Writes result into g_skybox (globals.cpp).
    ///         Must be called after pgr::initialize().
    /// @return true on success, false if any face texture fails to load.
    bool initSkybox();

} // namespace galuszde