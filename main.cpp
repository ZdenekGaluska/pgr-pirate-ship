//----------------------------------------------------------------------------------------
/**
 * \file    main.cpp
 * \author  galuszde
 * \note    Code comments generated with AI assistance (Claude, Anthropic).
 * \brief   Program entry point. Initializes GLUT, OpenGL context, and scene.
 *          Registers GLUT callbacks and starts the main event loop.
 *
 *   1. glutInit()           -- initializes the windowing system
 *   2. glutCreateWindow()   -- creates the OpenGL context on the GPU
 *   3. pgr::initialize()    -- loads OpenGL function pointers via gl_core_4_4 loader,
 *                              calls ilInit() for DevIL texture loading
 *                              (MUST be after glutCreateWindow)
 *   4. init()               -- loads models, shaders, scene
 *                              (MUST be after pgr::initialize)
 *   5. glutMainLoop()       -- starts the event loop (never returns)
 */
 //----------------------------------------------------------------------------------------

#include "globals.h"
#include "callbacks.h"
#include "mesh_utils.h"
#include "volcano.h"
#include "skybox/skybox.h"

namespace galuszde {

    /// @brief  Releases all GPU resources before the OpenGL context is destroyed.
    ///         Called automatically by GLUT when the window is closed.
    void finalize() {
        // Release shader program and attached shaders
        pgr::deleteProgramAndShaders(g_shaderLocation.program);

        // Release ship mesh GPU resources
        for (auto& mesh : g_meshes) {
            glDeleteVertexArrays(1, &mesh.vao);
            glDeleteBuffers(1, &mesh.vbo);
            glDeleteBuffers(1, &mesh.ibo);
            glDeleteTextures(1, &mesh.texture);
        }

        // Release water grid GPU resources
        glDeleteVertexArrays(1, &g_water.vao);
        glDeleteBuffers(1, &g_water.vbo);
        glDeleteBuffers(1, &g_water.ibo);
        glDeleteTextures(1, &g_water.texture);

        // Release volcano GPU resources
        glDeleteVertexArrays(1, &g_volcano.vao);
        glDeleteBuffers(1, &g_volcano.vbo);
        glDeleteBuffers(1, &g_volcano.ibo);
        glDeleteTextures(1, &g_volcano.texture);

        // Release skybox GPU resources
        glDeleteVertexArrays(1, &g_skybox.vao);
        glDeleteBuffers(1, &g_skybox.vbo);
        glDeleteTextures(1, &g_skybox.cubemapTexture);
        pgr::deleteProgramAndShaders(g_skyboxShader.program);
    }

    /// @brief  Initializes the scene by delegating to subsystem init functions.
    ///         Each subsystem (model, water, volcano, shaders) owns its own initialization logic.
    ///         Sets base OpenGL render state after all subsystems are ready.
    /// @return true on success, false if any subsystem initialization fails.
    bool init() {
        std::cout << "Initializing scene..." << std::endl;

        if (!loadShipModel())  return false;   // Assimp load + GPU upload
        if (!initWater())      return false;   // grid generation + texture
        if (!initVolcano())    return false;   // hardcoded mesh + GPU upload
        if (!initShaders())    return false;   // compile, link, cache uniforms
        if (!initSkyboxShader())  return false;
        if (!initSkybox())        return false;

        glClearColor(0.1f, 0.18f, 0.28f, 1.0f);   // background color (dark ocean blue)
        glEnable(GL_DEPTH_TEST);                    // Z-buffer: only draw what is in front

        std::cout << "Scene initialized successfully." << std::endl;
        return true;
    }

} // namespace galuszde

/// @brief  Program entry point. Creates GLUT window, registers all callbacks,
///         initializes PGR framework and scene, then enters the GLUT event loop.
/// @param  argc  Argument count passed by the operating system.
/// @param  argv  Argument values passed by the operating system.
int main(int argc, char* argv[]) {
    // --- GLUT windowing system init ---
    glutInit(&argc, argv);
    glutInitContextVersion(pgr::OGL_VER_MAJOR, pgr::OGL_VER_MINOR);
    glutInitContextFlags(GLUT_FORWARD_COMPATIBLE);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(WIN_WIDTH, WIN_HEIGHT);
    glutCreateWindow(WIN_TITLE);
    glutSetCursor(GLUT_CURSOR_NONE);   // hide cursor

    // --- Register GLUT callbacks ---
    glutDisplayFunc(galuszde::onDisplay);
    glutReshapeFunc(galuszde::onReshape);
    glutKeyboardFunc(galuszde::onKeyDown);
    glutKeyboardUpFunc(galuszde::onKeyUp);
    glutPassiveMotionFunc(galuszde::onMouseMotion);
    glutMouseFunc(galuszde::onMouse);
    glutTimerFunc(16, galuszde::onTimer, 0);   // 16ms ~= 60 FPS game loop

    // --- PGR framework init ---
    if (!pgr::initialize(pgr::OGL_VER_MAJOR, pgr::OGL_VER_MINOR))
        pgr::dieWithError("pgr::initialize() failed");

    // --- Scene init: models, water, volcano, shaders ---
    if (!galuszde::init())
        pgr::dieWithError("init() failed -- see error messages above.");

    // --- Cleanup callback ---
    glutCloseFunc(galuszde::finalize);

    // --- GLUT main loop ---
    glutMainLoop();
    return 0;
}