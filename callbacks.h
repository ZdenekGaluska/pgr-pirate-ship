#pragma once
//----------------------------------------------------------------------------------------
/**
 * \file    callbacks.h
 * \author  galuszde
 * \note    Code comments generated with AI assistance (Claude, Anthropic).
 * \brief   GLUT callback declarations and scene rendering interface.
 *
 * GLUT callbacks are the entry points driven by the event loop in glutMainLoop().
 * Each callback handles one event type and delegates to the appropriate module
 * (camera, physics, rendering).
 *
 * Callback signatures are fixed by the GLUT API -- do not modify parameters.
 * Registration happens in main.cpp via glutXxxFunc().
 */
 //----------------------------------------------------------------------------------------
#include "globals.h"

namespace galuszde {

    // Scene initialization -- called once from main.cpp::init()

    /// @brief  Compiles and links the shader program using pgr::createShaderFromFile()
    ///         and pgr::createProgram(), then caches all uniform locations.
    ///         Must be called after pgr::initialize() and before any draw call.
    /// @return true on success, false if shader compilation or linking fails.
    bool initShaders();

    /// @brief  Compiles and links the skybox shader program, caches uniform locations.
    ///         Must be called after pgr::initialize() and before any draw call.
    /// @return true on success, false if compilation or linking fails.
    bool initSkyboxShader();

    // GLUT callbacks -- registered in main.cpp, called by glutMainLoop()

    /// @brief  Called by GLUT whenever the window needs to be redrawn.
    ///         Computes view/projection matrices, draws ship and water, swaps buffers.
    void onDisplay();

    /// @brief  Called by GLUT when the window is resized.
    ///         Updates the OpenGL viewport to match the new window dimensions.
    /// @param  newWidth   New window width in pixels.
    /// @param  newHeight  New window height in pixels.
    void onReshape(int newWidth, int newHeight);

    /// @brief  Called by GLUT on printable key press (ASCII, space, ESC).
    ///         Sets g_keys[key] = true. ESC exits the program cleanly.
    /// @param  key  ASCII key code of the pressed key.
    /// @param  x    Mouse X position in pixels at the time of the key press.
    /// @param  y    Mouse Y position in pixels at the time of the key press.
    void onKeyDown(unsigned char key, int x, int y);

    /// @brief  Called by GLUT on key release.
    ///         Sets g_keys[key] = false to stop movement on the next timer tick.
    /// @param  key  ASCII key code of the released key.
    /// @param  x    Mouse X position in pixels at the time of the key release.
    /// @param  y    Mouse Y position in pixels at the time of the key release.
    void onKeyUp(unsigned char key, int x, int y);

    /// @brief  Called by GLUT on passive mouse movement (no button held).
    ///         Updates g_camYaw and g_camPitch, warps cursor back to window center.
    /// @param  x  Current mouse X position in pixels (GLUT origin = top-left).
    /// @param  y  Current mouse Y position in pixels (GLUT origin = top-left).
    void onMouseMotion(int x, int y);

    /// @brief  Called by GLUT on mouse button press or scroll wheel event.
    ///         Scroll wheel (buttons 3/4) zooms the camera forward/backward.
    /// @param  button  GLUT button ID (GLUT_LEFT_BUTTON, 3 = scroll up, 4 = scroll down).
    /// @param  state   GLUT_DOWN or GLUT_UP.
    /// @param  x       Mouse X position in pixels at the time of the click.
    /// @param  y       Mouse Y position in pixels at the time of the click.
    void onMouse(int button, int state, int x, int y);

    /// @brief  Periodic game loop tick fired every 16 ms (~60 FPS) via glutTimerFunc.
    ///         Updates camera and ship state, re-registers itself, requests redraw.
    /// @param  value  Timer ID passed by glutTimerFunc (unused -- only one timer).
    void onTimer(int value);

} // namespace galuszde