#pragma once
//----------------------------------------------------------------------------------------
/**
 * \file    chest.h
 * \author  galuszde
 * \note    Code comments generated with AI assistance (Claude, Anthropic).
 * \brief   Treasure chest management -- loading, random placement, and pickup logic.
 *
 * The chest system consists of three layers:
 *   1. Shared GPU geometry (one VAO/VBO/IBO for all instances) -- loaded from OBJ once.
 *   2. Per-instance data (ChestInstance in g_chests) -- position + collected flag.
 *   3. Config file (config.txt) -- chest count, spawn area, seed -- reloadable at runtime.
 *
 * All chests share one draw call per instance, each with a translated model matrix.
 * No geometry duplication happens at runtime.
 *
 * Rubric connections:
 *   2.3  -- own OBJ file loader (no Assimp) used by initChests()
 *   10c  -- config.txt reloaded on keypress via reloadChests()
 *   19.2 -- distance-based picking in onMouse() (callbacks.cpp)
 */
 //----------------------------------------------------------------------------------------

#include "globals.h"

namespace galuszde {

    /// @brief  Loads the chest OBJ model from disk (own loader), uploads geometry to GPU,
    ///         loads diffuse and normal map textures, reads config.txt, and generates
    ///         random instance positions.
    ///
    ///         Expected file paths (relative to working directory):
    ///           treasure/chest.obj                       -- exported OBJ from GLTF
    ///           treasure/textures/material_baseColor.png -- diffuse texture
    ///           treasure/textures/material_normal.png    -- tangent-space normal map
    ///           config.txt                               -- chest configuration
    ///
    ///         Must be called after pgr::initialize() (GPU context must exist).
    /// @return true on success; false if OBJ or a texture cannot be loaded.
    bool initChests();

    /// @brief  Re-reads config.txt and regenerates all chest instance positions.
    ///
    ///         GPU geometry and textures are NOT re-uploaded (no GPU cost).
    ///         Only g_chests is rebuilt -- existing collected state is discarded.
    ///         Called from onKeyDown() when the user presses 'R'.
    void reloadChests();

    /// @brief  Doubles the chest count, assigns a new time-based seed, and regenerates positions.
    ///         Called on F1 keypress (rubric -- function keys).
    void resetChestsAction();

} // namespace galuszde