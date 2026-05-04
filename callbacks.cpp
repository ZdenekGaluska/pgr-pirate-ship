//----------------------------------------------------------------------------------------
/**
 * \file    callbacks.cpp
 * \author  galuszde
 * \note    Code comments generated with AI assistance (Claude, Anthropic).
 * \brief   GLUT callback implementations and scene rendering functions.
 *          Rendering is split into drawShip(), drawWater(), drawVolcano() for readability.
 *          initShaders() handles shader compilation and uniform location caching.
 */
 //----------------------------------------------------------------------------------------

#include "callbacks.h"
#include "camera.h"
#include "algorithms.h"
#include "physics.h"

namespace galuszde {

    // initShaders()

    /// @brief  Compiles vertex and fragment shaders, links them into a program,
    ///         and caches all uniform locations for use in draw calls.
    ///         Uses pgr::createShaderFromFile() and pgr::createProgram() from the
    ///         PGR framework -- error messages are printed to stderr automatically.
    /// @return true on success, false if compilation or linking fails.
    bool initShaders() {
        GLuint shaderList[] = {
            pgr::createShaderFromFile(GL_VERTEX_SHADER,   "simple-vs.glsl"),
            pgr::createShaderFromFile(GL_FRAGMENT_SHADER, "simple-fs.glsl"),
            0   // sentinel -- pgr::createProgram reads the array up to this null
        };
        g_shaderLocation.program = pgr::createProgram(shaderList);
        if (!g_shaderLocation.program) {
            std::cerr << "Shader compilation or linking failed." << std::endl;
            return false;
        }

        // glGetUniformLocation() is slow -- call once at startup and store IDs
        glUseProgram(g_shaderLocation.program);

        // Bind texture sampler to unit 0 -- does not change at runtime
        GLint textureLocation = glGetUniformLocation(g_shaderLocation.program, "uTexture");
        glUniform1i(textureLocation, 0);

        g_shaderLocation.mPVM = glGetUniformLocation(g_shaderLocation.program, "mPVM");
        g_shaderLocation.mModel = glGetUniformLocation(g_shaderLocation.program, "mModel");
        g_shaderLocation.vDiffuse = glGetUniformLocation(g_shaderLocation.program, "vDiffuse");
        g_shaderLocation.vLightDir = glGetUniformLocation(g_shaderLocation.program, "vLightDir");
        g_shaderLocation.vCameraPos = glGetUniformLocation(g_shaderLocation.program, "vCameraPos");
        g_shaderLocation.fWaterUVScale = glGetUniformLocation(g_shaderLocation.program, "uWaterUVScale");
        g_shaderLocation.fTime = glGetUniformLocation(g_shaderLocation.program, "u_time");

        glUniform1f(g_shaderLocation.fTime, 0.0f);
        glUniform1f(g_shaderLocation.fWaterUVScale, 0.0f);

        glUseProgram(0);
        return true;
    }

    // drawShip()

    /// @brief  Builds the ship model matrix and draws all sub-meshes.
    ///         Ship transform (position, tilt, yaw) is read from globals pre-computed
    ///         by updateShip() -- no physics runs here.
    /// @param  view  View matrix computed in onDisplay().
    /// @param  proj  Projection matrix computed in onDisplay().
    static void drawShip(const glm::mat4& view, const glm::mat4& proj) {
        // Translate to buoyancy height, apply wave tilt, rotate by yaw, scale to scene
        glm::mat4 model = glm::translate(glm::mat4(1.0f),
            glm::vec3(g_shipPos.x, g_shipY, g_shipPos.z));
        model = glm::rotate(model, g_shipRotAngle, g_shipRotAxis);
        model = glm::rotate(model, -glm::radians(g_shipYaw + 180.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::scale(model, glm::vec3(0.5f));

        glm::mat4 PVM = proj * view * model;
        glUniformMatrix4fv(g_shaderLocation.mPVM, 1, GL_FALSE, glm::value_ptr(PVM));
        glUniformMatrix4fv(g_shaderLocation.mModel, 1, GL_FALSE, glm::value_ptr(model));

        for (const auto& mesh : g_meshes) {
            glUniform3fv(g_shaderLocation.vDiffuse, 1, glm::value_ptr(mesh.diffuseColor));
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, mesh.texture);
            glBindVertexArray(mesh.vao);
            glDrawElements(GL_TRIANGLES, mesh.numTriangles * 3, GL_UNSIGNED_INT, nullptr);
        }
    }

    // drawWater()

    /// @brief  Draws the ocean water grid.
    ///         The grid follows the camera in XZ so the ocean appears infinite.
    ///         uWaterUVScale > 0 switches the shader to world-space UV tiling mode.
    /// @param  view  View matrix computed in onDisplay().
    /// @param  proj  Projection matrix computed in onDisplay().
    /// @param  time  Elapsed time passed to the shader for Gerstner wave animation.
    static void drawWater(const glm::mat4& view, const glm::mat4& proj, float time) {
        // Translate grid to camera XZ position -- creates the illusion of infinite ocean
        glm::mat4 model = glm::translate(glm::mat4(1.0f),
            glm::vec3(g_camPos.x, 0.0f, g_camPos.z));

        glm::mat4 PVM = proj * view * model;
        glUniformMatrix4fv(g_shaderLocation.mPVM, 1, GL_FALSE, glm::value_ptr(PVM));
        glUniformMatrix4fv(g_shaderLocation.mModel, 1, GL_FALSE, glm::value_ptr(model));

        // 0.02f tiles the texture every 50 world units
        glUniform1f(g_shaderLocation.fWaterUVScale, 0.02f);
        glUniform1f(g_shaderLocation.fTime, time);
        glUniform3fv(g_shaderLocation.vDiffuse, 1, glm::value_ptr(glm::vec3(0.1f, 0.3f, 0.5f)));

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, g_water.texture);
        glBindVertexArray(g_water.vao);
        glDrawElements(GL_TRIANGLES, g_water.numIndices, GL_UNSIGNED_INT, nullptr);

        // Reset to VBO UV mode so subsequent objects are not affected
        glUniform1f(g_shaderLocation.fWaterUVScale, 0.0f);
    }

    // drawVolcano()

    /// @brief  Draws the hardcoded volcano mesh at its fixed world position (VOLCANO_POS).
    ///         No animation -- the volcano is a static scene object.
    ///         uWaterUVScale stays at 0 so the vertex shader uses standard (non-Gerstner) path.
    /// @param  view  View matrix computed in onDisplay().
    /// @param  proj  Projection matrix computed in onDisplay().
    static void drawVolcano(const glm::mat4& view, const glm::mat4& proj) {
        // Static transform: translate to fixed world position, no rotation, no scale
        glm::mat4 model = glm::translate(glm::mat4(1.0f), VOLCANO_POS);

        glm::mat4 PVM = proj * view * model;
        glUniformMatrix4fv(g_shaderLocation.mPVM, 1, GL_FALSE, glm::value_ptr(PVM));
        glUniformMatrix4fv(g_shaderLocation.mModel, 1, GL_FALSE, glm::value_ptr(model));

        glUniform3fv(g_shaderLocation.vDiffuse, 1, glm::value_ptr(g_volcano.diffuseColor));
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, g_volcano.texture);
        glBindVertexArray(g_volcano.vao);
        glDrawElements(GL_TRIANGLES, g_volcano.numTriangles * 3, GL_UNSIGNED_INT, nullptr);
    }

    // onDisplay()

    void onDisplay() {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glUseProgram(g_shaderLocation.program);

        // Dividing by 800.0f must match the identical constant in simple-vs.glsl
        float time = glutGet(GLUT_ELAPSED_TIME) / 800.0f;

        // View matrix -- depends on active camera mode
        glm::mat4 view;
        if (g_cameraMode == CAM_SHIP) {
            glm::vec3 shipFront = glm::normalize(glm::vec3(
                cos(glm::radians(g_shipYaw)), 0.0f, sin(glm::radians(g_shipYaw))
            ));
            g_camPos = g_shipPos - shipFront * 9.0f + glm::vec3(0.0f, 4.0f, 0.0f);
            view = glm::lookAt(g_camPos,
                g_shipPos + glm::vec3(0.0f, 2.2f, 0.0f),
                glm::vec3(0.0f, 1.0f, 0.0f));
        }
        if (g_cameraMode == CAM_FREE) {
            view = glm::lookAt(g_camPos,
                g_camPos + getCamFront(),
                glm::vec3(0.0f, 1.0f, 0.0f));
        }

        // Projection matrix -- shared by all objects in the scene
        glm::mat4 proj = glm::perspective(
            glm::radians(45.0f),
            (float)WIN_WIDTH / WIN_HEIGHT,
            0.1f,
            200.0f
        );

        glUniform3fv(g_shaderLocation.vLightDir, 1, glm::value_ptr(LIGHT_DIR));
        glUniform3fv(g_shaderLocation.vCameraPos, 1, glm::value_ptr(g_camPos));
        glUniform1f(g_shaderLocation.fTime, time);

        drawShip(view, proj);
        drawVolcano(view, proj);
        drawWater(view, proj, time);   // water last -- opaque but drawn after solid objects

        glBindVertexArray(0);
        glUseProgram(0);
        glutSwapBuffers();
    }

    // onReshape()

    void onReshape(int newWidth, int newHeight) {
        glViewport(0, 0, newWidth, newHeight);
    }

    // onKeyDown() / onKeyUp()

    void onKeyDown(unsigned char key, int /*x*/, int /*y*/) {
        if (key == 27) glutLeaveMainLoop();   // ESC -- exit cleanly via glutCloseFunc

        g_keys[key] = true;

        if (key == 9)   // TAB -- cycle through camera modes
            g_cameraMode = (CameraMode)((g_cameraMode + 1) % 2);
    }

    void onKeyUp(unsigned char key, int /*x*/, int /*y*/) {
        g_keys[key] = false;
    }

    // onMouseMotion()

    void onMouseMotion(int x, int y) {
        if (g_cameraMode == CAM_FREE) {
            int centerX = WIN_WIDTH / 2;
            int centerY = WIN_HEIGHT / 2;

            // Ignore the synthetic event triggered by glutWarpPointer itself
            if (x == centerX && y == centerY) return;

            g_camYaw += CAM_SENS * (x - centerX);
            g_camPitch -= CAM_SENS * (y - centerY);
            g_camPitch = glm::clamp(g_camPitch, -89.0f, 89.0f);

            glutWarpPointer(centerX, centerY);
            glutPostRedisplay();
        }
    }

    // onMouse()

    void onMouse(int button, int state, int /*x*/, int /*y*/) {
        if (state != GLUT_DOWN) return;

        glm::vec3 front = getCamFront();
        if (button == 3)      g_camPos += front * ZOOM_SPEED;
        else if (button == 4) g_camPos -= front * ZOOM_SPEED;

        glutPostRedisplay();
    }

    // onTimer()

    void onTimer(int /*value*/) {
        updateCamera();
        updateShip();

        // Re-register -- glutTimerFunc is one-shot, does not repeat automatically
        glutTimerFunc(16, onTimer, 0);
        glutPostRedisplay();
    }

} // namespace galuszde