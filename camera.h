#pragma once
//----------------------------------------------------------------------------------------
/**
 * \file    camera.h
 * \author  galuszde
 * \note    Code comments generated with AI assistance (Claude, Anthropic).
 * \brief   FPS camera -- view direction computation and input-driven movement.
 *
 * The camera is represented by three global variables (globals.h):
 *   g_camPos   -- position in world space
 *   g_camYaw   -- left/right rotation (degrees)
 *   g_camPitch -- up/down rotation (degrees)
 *
 * Input callbacks (onKeyDown, onMouseMotion) modify g_keys / yaw / pitch.
 * Camera movement is computed by updateCamera() -- called from onTimer() once per frame.
 */
 //----------------------------------------------------------------------------------------
#include "globals.h"

namespace galuszde {

    /// @brief  Computes a normalized view direction vector from g_camYaw and g_camPitch.
    /// @return Unit vector of the camera's look direction (length = 1).
    ///
    /// Spherical to Cartesian conversion:
    ///   x = cos(yaw) * cos(pitch)
    ///   y = sin(pitch)
    ///   z = sin(yaw) * cos(pitch)
    ///
    /// Used for:
    ///   - glm::lookAt() when computing the view matrix in onDisplay()
    ///   - WASD movement (translate along front and right vectors)
    ///   - scroll wheel zoom (translate along front vector)
    glm::vec3 getCamFront();

    /// @brief  Updates camera or ship position based on currently held keys (g_keys[]).
    ///
    /// Called once per frame from onTimer().
    /// Movement is therefore smooth and independent of OS key-repeat rate.
    ///
    /// CAM_FREE keys:
    ///   W / S   -- forward / backward (along view direction)
    ///   A / D   -- left / right (perpendicular to view direction)
    ///   Space   -- up (world Y axis)
    ///   E       -- down (world Y axis)
    ///
    /// CAM_SHIP keys:
    ///   W       -- move ship forward
    ///   A / D   -- rotate ship left / right
    void updateCamera();

} // namespace galuszde