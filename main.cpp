//----------------------------------------------------------------------------------------
/**
 * \file    main.cpp
 * \author  galuszde
 * \note    Code comments generated with AI assistance (Claude, Anthropic).
 * \brief   Program entry point. Initializes GLUT, OpenGL context, and scene.
 */
 //----------------------------------------------------------------------------------------

#include "globals.h"
#include "callbacks.h"
#include "mesh_utils.h"
#include "volcano.h"
#include "chest.h"
#include "skybox/skybox.h"

namespace galuszde {

    /// @brief  Releases all GPU resources before the OpenGL context is destroyed.
    void finalize() {
        pgr::deleteProgramAndShaders(g_shaderLocation.program);

        for (auto& mesh : g_meshes) {
            glDeleteVertexArrays(1, &mesh.vao);
            glDeleteBuffers(1, &mesh.vbo);
            glDeleteBuffers(1, &mesh.ibo);
            glDeleteTextures(1, &mesh.texture);
        }

        glDeleteVertexArrays(1, &g_water.vao);
        glDeleteBuffers(1, &g_water.vbo);
        glDeleteBuffers(1, &g_water.ibo);
        glDeleteTextures(1, &g_water.texture);

        glDeleteVertexArrays(1, &g_volcano.vao);
        glDeleteBuffers(1, &g_volcano.vbo);
        glDeleteBuffers(1, &g_volcano.ibo);
        glDeleteTextures(1, &g_volcano.texture);

        // Release chest GPU resources
        glDeleteVertexArrays(1, &g_chestGeom.vao);
        glDeleteBuffers(1, &g_chestGeom.vbo);
        glDeleteBuffers(1, &g_chestGeom.ibo);
        glDeleteTextures(1, &g_chestGeom.texDiffuse);
        glDeleteTextures(1, &g_chestGeom.texNormal);
        pgr::deleteProgramAndShaders(g_chestShader.program);

        glDeleteVertexArrays(1, &g_skybox.vao);
        glDeleteBuffers(1, &g_skybox.vbo);
        glDeleteTextures(1, &g_skybox.cubemapTexture);
        pgr::deleteProgramAndShaders(g_skyboxShader.program);
    }

    /// @brief  Initializes all scene subsystems in dependency order.
    /// @return true on success, false if any subsystem fails.
    bool init() {
        std::cout << "Initializing scene..." << std::endl;

        if (!loadShipModel())    return false;
        if (!initWater())        return false;
        if (!initVolcano())      return false;
        if (!initShaders())      return false;
        if (!initSkyboxShader()) return false;
        if (!initSkybox())       return false;

        // Chest shader must be compiled before initChests() because initChests()
        // only loads geometry/textures -- the shader is a separate step.
        if (!initChestShader())  return false;

        // initChests() reads config.txt, loads OBJ, uploads geometry, loads textures,
        // generates random positions.  Must be after pgr::initialize() (GPU context).
        if (!initChests())       return false;

        glClearColor(0.1f, 0.18f, 0.28f, 1.0f);
        glEnable(GL_DEPTH_TEST);

        std::cout << "Scene initialized successfully." << std::endl;
        return true;
    }

} // namespace galuszde

int main(int argc, char* argv[]) {
    glutInit(&argc, argv);
    glutInitContextVersion(pgr::OGL_VER_MAJOR, pgr::OGL_VER_MINOR);
    glutInitContextFlags(GLUT_FORWARD_COMPATIBLE);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(WIN_WIDTH, WIN_HEIGHT);
    glutCreateWindow(WIN_TITLE);
    glutSetCursor(GLUT_CURSOR_NONE);

    glutDisplayFunc(galuszde::onDisplay);
    glutReshapeFunc(galuszde::onReshape);
    glutKeyboardFunc(galuszde::onKeyDown);
    glutKeyboardUpFunc(galuszde::onKeyUp);
    glutPassiveMotionFunc(galuszde::onMouseMotion);
    glutMouseFunc(galuszde::onMouse);
    glutSpecialFunc(galuszde::onSpecialKeyDown);   // arrow keys, F1-F12
    glutTimerFunc(16, galuszde::onTimer, 0);

    if (!pgr::initialize(pgr::OGL_VER_MAJOR, pgr::OGL_VER_MINOR))
        pgr::dieWithError("pgr::initialize() failed");

    if (!galuszde::init())
        pgr::dieWithError("init() failed -- see error messages above.");

    glutCloseFunc(galuszde::finalize);
    glutMainLoop();
    return 0;
}