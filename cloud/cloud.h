#pragma once
//----------------------------------------------------------------------------------------
/**
 * \file    cloud.h
 * \author  galuszde
 * \note    Code comments generated with AI assistance (Claude, Anthropic).
 * \brief   Animated cloud plane above the volcano using a spritesheet texture (rubric 15b).
 *
 * A single horizontal quad is placed above VOLCANO_POS.
 * The spritesheet (2 cols x 3 rows = 6 frames) is cycled at CLOUD_FPS.
 * Alpha blending discards the black background so only the cloud shape renders.
 */
 //----------------------------------------------------------------------------------------

#include "globals.h"

namespace galuszde {

    /// @brief  Compiles cloud shader, uploads plane geometry, loads spritesheet texture.
    ///         Must be called after pgr::initialize().
    /// @return true on success, false if shader or texture fails to load.
    bool initCloud();

    /// @brief  Advances cloud animation timer by dt seconds.
    ///         Call once per frame from onTimer().
    /// @param  dt  Delta time in seconds.
    void updateCloud(float dt);

    /// @brief  Draws the cloud plane with current spritesheet frame.
    /// @param  view  View matrix from buildViewMatrix().
    /// @param  proj  Projection matrix from onDisplay().
    void drawCloud(const glm::mat4& view, const glm::mat4& proj);

    /// @brief  Releases cloud GPU resources. Call from finalize().
    void destroyCloud();

} // namespace galuszde