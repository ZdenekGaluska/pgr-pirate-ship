#pragma once
//----------------------------------------------------------------------------------------
/**
 * \file    callbacks.h
 * \author  galuszde
 * \note    Code comments generated with AI assistance (Claude, Anthropic).
 * \brief   GLUT callback declarations and scene rendering interface.
 */
 //----------------------------------------------------------------------------------------
#include "globals.h"

namespace galuszde {

    // --- Shader initialization (called once from main.cpp::init()) ---

    /// @brief  Compiles the main scene shader, links it, caches all uniform locations.
    /// @return true on success, false if compilation or linking fails.
    bool initShaders();

    /// @brief  Compiles the skybox shader, links it, caches uniform locations.
    /// @return true on success, false if compilation or linking fails.
    bool initSkyboxShader();

    /// @brief  Compiles the chest shader (chest-vs.glsl / chest-fs.glsl),
    ///         links it, and caches all uniform locations into g_chestShader.
    ///         Must be called after pgr::initialize() and before any chest draw call.
    /// @return true on success, false if compilation or linking fails.
    bool initChestShader();

    // --- GLUT callbacks (registered in main.cpp, called by glutMainLoop()) ---

    /// @brief  Called by GLUT when the window needs to be redrawn.
    void onDisplay();

    /// @brief  Called by GLUT when the window is resized.
    /// @param  newWidth   New window width in pixels.
    /// @param  newHeight  New window height in pixels.
    void onReshape(int newWidth, int newHeight);

    /// @brief  Called by GLUT on printable key press.
    ///         ESC = exit, TAB = cycle camera mode, R = reload config and chests.
    /// @param  key  ASCII key code of the pressed key.
    /// @param  x    Mouse X position at time of press.
    /// @param  y    Mouse Y position at time of press.
    void onKeyDown(unsigned char key, int x, int y);

    /// @brief  Called by GLUT on key release.
    /// @param  key  ASCII key code of the released key.
    /// @param  x    Mouse X position.
    /// @param  y    Mouse Y position.
    void onKeyUp(unsigned char key, int x, int y);

    /// @brief  Called by GLUT on passive mouse movement (no button held).
    ///         Updates g_camYaw and g_camPitch, warps cursor back to window center.
    /// @param  x  Current mouse X in pixels (GLUT origin = top-left).
    /// @param  y  Current mouse Y in pixels (GLUT origin = top-left).
    void onMouseMotion(int x, int y);

    /// @brief  Called by GLUT on mouse button press or scroll wheel event.
    ///         Left click (CAM_SHIP mode): collect nearby treasure chests.
    ///         Scroll wheel: zoom camera forward/backward.
    /// @param  button  GLUT button ID (GLUT_LEFT_BUTTON, 3 = scroll up, 4 = scroll down).
    /// @param  state   GLUT_DOWN or GLUT_UP.
    /// @param  x       Mouse X in pixels.
    /// @param  y       Mouse Y in pixels.
    void onMouse(int button, int state, int x, int y);

    /// @brief  Periodic game loop tick (~60 FPS) via glutTimerFunc.
    ///         Updates camera and ship state, re-registers itself, requests redraw.
    /// @param  value  Timer ID (unused).
    void onTimer(int value);

    /// @brief  Called by GLUT on special key press (arrow keys, F1-F12).
    /// @param  key  GLUT key constant (GLUT_KEY_F1 .. GLUT_KEY_F12, GLUT_KEY_UP, etc.).
    /// @param  x    Mouse X position in pixels at the time of the key press.
    /// @param  y    Mouse Y position in pixels at the time of the key press.
    void onSpecialKeyDown(int key, int x, int y);

    /// @brief  Called by GLUT on special key release (arrow keys, F1-F12).
    ///         Clears the corresponding arrow key state and updates sprint flag.
    /// @param  key  GLUT key constant (GLUT_KEY_UP, GLUT_KEY_DOWN, ...).
    /// @param  x    Mouse X position in pixels.
    /// @param  y    Mouse Y position in pixels.
    void onSpecialKeyUp(int key, int x, int y);

} // namespace galuszde