#pragma once
//----------------------------------------------------------------------------------------
/**
 * \file    bird.h
 * \author  galuszde
 * \note    Code comments generated with AI assistance (Claude, Anthropic).
 * \brief   Seagull animated on a circle above the volcano (rubric 16a).
 *
 * The bird follows a horizontal circle centred on VOLCANO_POS.
 * Position and yaw are derived analytically from the current angle so
 * the bird always faces its direction of travel.
 */
 //----------------------------------------------------------------------------------------

#include "globals.h"

namespace galuszde {

    /// @brief  Loads the seagull model and uploads geometry to GPU.
    ///         Must be called after pgr::initialize().
    /// @return true on success, false if the model file cannot be loaded.
    bool initBird();

    /// @brief  Advances the bird angle by dt and updates g_bird.angle.
    ///         Call once per frame from onTimer().
    /// @param  dt  Delta time in seconds since last frame.
    void updateBird(float dt);

    /// @brief  Draws the seagull at its current circle position.
    /// @param  view  View matrix from buildViewMatrix().
    /// @param  proj  Projection matrix from onDisplay().
    void drawBird(const glm::mat4& view, const glm::mat4& proj);

} // namespace galuszde