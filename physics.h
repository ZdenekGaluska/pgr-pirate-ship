#pragma once
//----------------------------------------------------------------------------------------
/**
 * \file    physics.h
 * \author  galuszde
 * \note    Code comments generated with AI assistance (Claude, Anthropic).
 * \brief   Ship physics — movement, wave response, speed modifier.
 */
 //----------------------------------------------------------------------------------------

#include "globals.h"

namespace galuszde {

    /// @brief Updates ship state every frame (call from onTimer).
    ///
    /// Delegates to internal helpers: normal sampling, rotation precompute,
    /// height tracking and horizontal movement. Writes results into globals:
    /// g_shipPos, g_shipY, g_shipNormal, g_shipRotAxis, g_shipRotAngle, g_shipSpeedMod.
    void updateShip();

} // namespace galuszde