//----------------------------------------------------------------------------------------
/**
 * \file    callbacks.cpp
 * \author  galuszde
 * \note    Code comments generated with AI assistance (Claude, Anthropic).
 * \brief   GLUT callback implementations and scene rendering functions.
 */
 //----------------------------------------------------------------------------------------

#include "callbacks.h"
#include "camera.h"
#include "algorithms.h"
#include "physics.h"
#include "chest.h"
#include <cctype>

namespace galuszde {

    // =========================================================================
    // initShaders()
    // =========================================================================

    /// @brief  Compiles vertex and fragment shaders, links them into a program,
    ///         and caches all uniform locations for use in draw calls.
    /// @return true on success, false if compilation or linking fails.
    bool initShaders() {
        GLuint shaderList[] = {
            pgr::createShaderFromFile(GL_VERTEX_SHADER,   "simple-vs.glsl"),
            pgr::createShaderFromFile(GL_FRAGMENT_SHADER, "simple-fs.glsl"),
            0
        };
        g_shaderLocation.program = pgr::createProgram(shaderList);
        if (!g_shaderLocation.program) {
            std::cerr << "Shader compilation or linking failed." << std::endl;
            return false;
        }

        glUseProgram(g_shaderLocation.program);

        // Bind texture sampler to unit 0 -- does not change at runtime
        GLint textureLocation = glGetUniformLocation(g_shaderLocation.program, "uTexture");
        glUniform1i(textureLocation, 0);

        // Cache all uniform locations -- calling glGetUniformLocation every frame is slow
        g_shaderLocation.mPVM = glGetUniformLocation(g_shaderLocation.program, "mPVM");
        g_shaderLocation.mModel = glGetUniformLocation(g_shaderLocation.program, "mModel");
        g_shaderLocation.vDiffuse = glGetUniformLocation(g_shaderLocation.program, "vDiffuse");
        g_shaderLocation.vLightDir = glGetUniformLocation(g_shaderLocation.program, "vLightDir");
        g_shaderLocation.vCameraPos = glGetUniformLocation(g_shaderLocation.program, "vCameraPos");
        g_shaderLocation.fWaterUVScale = glGetUniformLocation(g_shaderLocation.program, "uWaterUVScale");
        g_shaderLocation.fTime = glGetUniformLocation(g_shaderLocation.program, "u_time");

        // Material uniforms (rubric 9b) -- set per object in draw calls
        g_shaderLocation.uAmbient = glGetUniformLocation(g_shaderLocation.program, "u_ambient");
        g_shaderLocation.uSpecularStr = glGetUniformLocation(g_shaderLocation.program, "u_specularStr");
        g_shaderLocation.uShininess = glGetUniformLocation(g_shaderLocation.program, "u_shininess");

        glUniform1f(g_shaderLocation.fTime, 0.0f);
        glUniform1f(g_shaderLocation.fWaterUVScale, 0.0f);

        glUseProgram(0);
        return true;
    }

    // =========================================================================
    // initSkyboxShader()
    // =========================================================================

    bool initSkyboxShader() {
        GLuint shaderList[] = {
            pgr::createShaderFromFile(GL_VERTEX_SHADER,   "skybox/skybox-vs.glsl"),
            pgr::createShaderFromFile(GL_FRAGMENT_SHADER, "skybox/skybox-fs.glsl"),
            0
        };
        g_skyboxShader.program = pgr::createProgram(shaderList);
        if (!g_skyboxShader.program) {
            std::cerr << "Skybox shader compilation or linking failed." << std::endl;
            return false;
        }

        glUseProgram(g_skyboxShader.program);

        g_skyboxShader.mProjection = glGetUniformLocation(g_skyboxShader.program, "u_projection");
        g_skyboxShader.mView = glGetUniformLocation(g_skyboxShader.program, "u_view");
        g_skyboxShader.uSkybox = glGetUniformLocation(g_skyboxShader.program, "u_skybox");

        glUniform1i(g_skyboxShader.uSkybox, 0);

        glUseProgram(0);
        return true;
    }

    // =========================================================================
    // initChestShader()
    // =========================================================================

    /// @brief  Compiles and links the chest shader pair, caches all uniform locations.
    ///
    ///         Sampler bindings set here at init time (static, never change):
    ///           u_diffuseMap = texture unit 0
    ///           u_normalMap  = texture unit 1
    ///           u_envMap     = texture unit 2
    /// @return true on success, false if compilation or linking fails.
    bool initChestShader() {
        GLuint shaderList[] = {
            pgr::createShaderFromFile(GL_VERTEX_SHADER,   "chest/chest-vs.glsl"),
            pgr::createShaderFromFile(GL_FRAGMENT_SHADER, "chest/chest-fs.glsl"),
            0
        };
        g_chestShader.program = pgr::createProgram(shaderList);
        if (!g_chestShader.program) {
            std::cerr << "Chest shader compilation or linking failed." << std::endl;
            return false;
        }

        glUseProgram(g_chestShader.program);

        // Bind samplers to their texture units -- static, set once at init
        GLint locDiff = glGetUniformLocation(g_chestShader.program, "u_diffuseMap");
        GLint locNorm = glGetUniformLocation(g_chestShader.program, "u_normalMap");
        GLint locEnv = glGetUniformLocation(g_chestShader.program, "u_envMap");
        glUniform1i(locDiff, 0);   // unit 0 = diffuse texture
        glUniform1i(locNorm, 1);   // unit 1 = normal map
        glUniform1i(locEnv, 2);   // unit 2 = skybox cubemap (env reflections)

        // Cache remaining locations for per-frame use
        g_chestShader.mPVM = glGetUniformLocation(g_chestShader.program, "mPVM");
        g_chestShader.mModel = glGetUniformLocation(g_chestShader.program, "mModel");
        g_chestShader.uEnvStrength = glGetUniformLocation(g_chestShader.program, "u_envStrength");
        g_chestShader.vLightDir = glGetUniformLocation(g_chestShader.program, "vLightDir");
        g_chestShader.vCameraPos = glGetUniformLocation(g_chestShader.program, "vCameraPos");
        g_chestShader.uAmbient = glGetUniformLocation(g_chestShader.program, "u_ambient");
        g_chestShader.uSpecular = glGetUniformLocation(g_chestShader.program, "u_specular");
        g_chestShader.uShininess = glGetUniformLocation(g_chestShader.program, "u_shininess");

        glUseProgram(0);
        return true;
    }

    // =========================================================================
    // Draw helpers (static -- not exported)
    // =========================================================================

    // -------------------------------------------------------------------------
    // drawShip()
    // -------------------------------------------------------------------------

    /// @brief  Builds the ship model matrix and draws all sub-meshes.
    ///         Material: painted wood -- mild ambient, low specular, shininess 32.
    /// @param  view  View matrix computed in onDisplay().
    /// @param  proj  Projection matrix computed in onDisplay().
    static void drawShip(const glm::mat4& view, const glm::mat4& proj) {
        glm::mat4 model = glm::translate(glm::mat4(1.0f),
            glm::vec3(g_shipPos.x, g_shipY, g_shipPos.z));
        model = glm::rotate(model, g_shipRotAngle, g_shipRotAxis);
        model = glm::rotate(model, -glm::radians(g_shipYaw + 180.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::scale(model, glm::vec3(0.5f));

        glm::mat4 PVM = proj * view * model;
        glUniformMatrix4fv(g_shaderLocation.mPVM, 1, GL_FALSE, glm::value_ptr(PVM));
        glUniformMatrix4fv(g_shaderLocation.mModel, 1, GL_FALSE, glm::value_ptr(model));

        // Ship material: painted/aged wood -- medium ambient, low-moderate specular
        glUniform1f(g_shaderLocation.uAmbient, 0.30f);
        glUniform1f(g_shaderLocation.uSpecularStr, 0.25f);
        glUniform1f(g_shaderLocation.uShininess, 32.0f);

        for (const auto& mesh : g_meshes) {
            glUniform3fv(g_shaderLocation.vDiffuse, 1, glm::value_ptr(mesh.diffuseColor));
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, mesh.texture);
            glBindVertexArray(mesh.vao);
            glDrawElements(GL_TRIANGLES, mesh.numTriangles * 3, GL_UNSIGNED_INT, nullptr);
        }
    }

    // -------------------------------------------------------------------------
    // drawWater()
    // -------------------------------------------------------------------------

    /// @brief  Draws the ocean water grid.
    ///         Material: water surface -- high ambient (sky reflection), strong specular.
    /// @param  view  View matrix from onDisplay().
    /// @param  proj  Projection matrix from onDisplay().
    /// @param  time  Elapsed time for Gerstner wave animation.
    static void drawWater(const glm::mat4& view, const glm::mat4& proj, float time) {
        glm::mat4 model = glm::translate(glm::mat4(1.0f),
            glm::vec3(g_camPos.x, 0.0f, g_camPos.z));

        glm::mat4 PVM = proj * view * model;
        glUniformMatrix4fv(g_shaderLocation.mPVM, 1, GL_FALSE, glm::value_ptr(PVM));
        glUniformMatrix4fv(g_shaderLocation.mModel, 1, GL_FALSE, glm::value_ptr(model));

        // Water material: highly reflective surface -- strong specular, high shininess
        glUniform1f(g_shaderLocation.uAmbient, 0.50f);
        glUniform1f(g_shaderLocation.uSpecularStr, 0.90f);
        glUniform1f(g_shaderLocation.uShininess, 256.0f);

        glUniform1f(g_shaderLocation.fWaterUVScale, 0.02f);
        glUniform1f(g_shaderLocation.fTime, time);
        glUniform3fv(g_shaderLocation.vDiffuse, 1, glm::value_ptr(glm::vec3(0.1f, 0.3f, 0.5f)));

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, g_water.texture);
        glBindVertexArray(g_water.vao);
        glDrawElements(GL_TRIANGLES, g_water.numIndices, GL_UNSIGNED_INT, nullptr);

        glUniform1f(g_shaderLocation.fWaterUVScale, 0.0f);
    }

    // -------------------------------------------------------------------------
    // drawVolcano()
    // -------------------------------------------------------------------------

    /// @brief  Draws the hardcoded volcano mesh at its fixed world position.
    ///         Material: rough volcanic rock -- high ambient (self-lit look), almost matte.
    /// @param  view  View matrix from onDisplay().
    /// @param  proj  Projection matrix from onDisplay().
    static void drawVolcano(const glm::mat4& view, const glm::mat4& proj) {
        glm::mat4 model = glm::translate(glm::mat4(1.0f), VOLCANO_POS);

        glm::mat4 PVM = proj * view * model;
        glUniformMatrix4fv(g_shaderLocation.mPVM, 1, GL_FALSE, glm::value_ptr(PVM));
        glUniformMatrix4fv(g_shaderLocation.mModel, 1, GL_FALSE, glm::value_ptr(model));

        // Volcano material: porous volcanic rock -- high ambient, near-zero specular
        glUniform1f(g_shaderLocation.uAmbient, 0.45f);
        glUniform1f(g_shaderLocation.uSpecularStr, 0.05f);
        glUniform1f(g_shaderLocation.uShininess, 4.0f);

        glUniform3fv(g_shaderLocation.vDiffuse, 1, glm::value_ptr(g_volcano.diffuseColor));
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, g_volcano.texture);
        glBindVertexArray(g_volcano.vao);
        glDrawElements(GL_TRIANGLES, g_volcano.numTriangles * 3, GL_UNSIGNED_INT, nullptr);
    }

    // -------------------------------------------------------------------------
    // drawSkybox()
    // -------------------------------------------------------------------------

    /// @brief  Draws the skybox cube behind all other geometry.
    ///         Depth function is relaxed to GL_LEQUAL because the vertex shader
    ///         forces z/w = 1.0 (maximum depth), which would fail GL_LESS.
    /// @param  view  View matrix from onDisplay().
    /// @param  proj  Projection matrix from onDisplay().
    static void drawSkybox(const glm::mat4& view, const glm::mat4& proj) {
        glUseProgram(g_skyboxShader.program);
        glUniformMatrix4fv(g_skyboxShader.mProjection, 1, GL_FALSE, glm::value_ptr(proj));
        glUniformMatrix4fv(g_skyboxShader.mView, 1, GL_FALSE, glm::value_ptr(view));

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, g_skybox.cubemapTexture);

        glDepthFunc(GL_LEQUAL);
        glBindVertexArray(g_skybox.vao);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glBindVertexArray(0);
        glDepthFunc(GL_LESS);

        glUseProgram(0);
    }

    // -------------------------------------------------------------------------
    // drawChests()
    // -------------------------------------------------------------------------

    /// @brief  Draws all uncollected treasure chest instances using the chest shader.
    ///
    ///         All instances share one VAO (g_chestGeom.vao) and two 2D textures.
    ///         The skybox cubemap is reused as the environment map on unit 2 --
    ///         no extra texture load needed (rubric 12d -- env map).
    ///
    ///         Material: polished wood with metal fittings -- moderate ambient,
    ///         warm specular tint, shininess 64 (rubric 9b -- distinct material).
    ///
    ///         Each instance is drawn with a Y-axis rotation derived from its X position
    ///         to avoid all chests looking identical.
    ///
    /// @param  view  View matrix from onDisplay().
    /// @param  proj  Projection matrix from onDisplay().
    static void drawChests(const glm::mat4& view, const glm::mat4& proj) {
        if (g_chests.empty() || !g_chestGeom.vao) return;

        glUseProgram(g_chestShader.program);

        // Set lighting uniforms -- same light source as the main scene shader
        glUniform3fv(g_chestShader.vLightDir, 1, glm::value_ptr(LIGHT_DIR));
        glUniform3fv(g_chestShader.vCameraPos, 1, glm::value_ptr(g_camPos));

        // Chest material: dark polished wood + brass fittings.
        // ambient=0.35 keeps shadowed faces visible, shininess=64 gives a moderate gloss,
        // warm specular tint (gold/brass) distinguishes it from ship (white specular).
        glUniform1f(g_chestShader.uAmbient, 0.35f);
        glUniform3f(g_chestShader.uSpecular, 0.8f, 0.65f, 0.2f);   // warm gold tint
        glUniform1f(g_chestShader.uShininess, 64.0f);

        // env reflection strength: 0.3 gives subtle metallic gleam on the lock/hinges
        glUniform1f(g_chestShader.uEnvStrength, 0.3f);

        // Bind diffuse texture to unit 0
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, g_chestGeom.texDiffuse);

        // Bind normal map to unit 1
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, g_chestGeom.texNormal);

        // Bind the skybox cubemap to unit 2 as the environment map.
        // This reuses the already-loaded skybox texture (no extra texture) and gives
        // the chest's metallic parts reflections of the actual sky surrounding the scene.
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_CUBE_MAP, g_skybox.cubemapTexture);

        glBindVertexArray(g_chestGeom.vao);

        for (const auto& chest : g_chests) {
            // Skip already collected chests -- they are invisible and not interactable
            if (chest.collected) continue;

            // Build model matrix: translate to world position.
            // Small Y-axis rotation based on X position varies the look without extra data.
            float rotY = glm::radians(chest.pos.x * 37.0f);   // deterministic per-instance angle
            glm::mat4 model = glm::translate(glm::mat4(1.0f), chest.pos);
            model = glm::rotate(model, rotY, glm::vec3(0.0f, 1.0f, 0.0f));
            model = glm::scale(model, glm::vec3(g_chestConfig.scale));

            glm::mat4 PVM = proj * view * model;
            glUniformMatrix4fv(g_chestShader.mPVM, 1, GL_FALSE, glm::value_ptr(PVM));
            glUniformMatrix4fv(g_chestShader.mModel, 1, GL_FALSE, glm::value_ptr(model));

            glDrawElements(GL_TRIANGLES, g_chestGeom.numIndices, GL_UNSIGNED_INT, nullptr);
        }

        // Restore texture unit state so subsequent draw calls are not affected
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, 0);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, 0);

        glBindVertexArray(0);
        glUseProgram(0);
    }

    // =========================================================================
    // buildViewMatrix()
    // =========================================================================

    /// @brief  Computes the view matrix for the currently active camera mode.
    ///         CAM_SHIP also updates g_camPos as a side effect (third-person offset).
    /// @return View matrix ready to pass to draw calls.
    static glm::mat4 buildViewMatrix() {
        if (g_cameraMode == CAM_SHIP) {
            glm::vec3 shipFront = glm::normalize(glm::vec3(
                cos(glm::radians(g_shipYaw)), 0.0f, sin(glm::radians(g_shipYaw))
            ));
            g_camPos = g_shipPos - shipFront * 9.0f + glm::vec3(0.0f, 4.0f, 0.0f);
            return glm::lookAt(g_camPos,
                g_shipPos + glm::vec3(0.0f, 2.2f, 0.0f),
                glm::vec3(0.0f, 1.0f, 0.0f));
        }
        if (g_cameraMode == CAM_FREE)
            return glm::lookAt(g_camPos, g_camPos + getCamFront(), glm::vec3(0.0f, 1.0f, 0.0f));
        if (g_cameraMode == CAM_STATIC1)
            return glm::lookAt(CAM_STATIC1_POS, CAM_STATIC1_TARGET, glm::vec3(0.0f, 1.0f, 0.0f));
        return glm::lookAt(CAM_STATIC2_POS, CAM_STATIC2_TARGET, glm::vec3(0.0f, 1.0f, 0.0f));
    }

    // =========================================================================
    // drawScene()
    // =========================================================================

    /// @brief  Renders all scene objects in correct draw order.
    ///
    ///         Order: opaque objects (ship, volcano, chests) -> skybox -> water.
    ///         Skybox switches shader programs internally; the main shader is
    ///         re-bound before water so lighting uniforms are active again.
    ///
    /// @param  view  View matrix from buildViewMatrix().
    /// @param  proj  Projection matrix from onDisplay().
    /// @param  time  Elapsed time for Gerstner wave animation.
    static void drawScene(const glm::mat4& view, const glm::mat4& proj, float time) {
        glUseProgram(g_shaderLocation.program);
        glUniform3fv(g_shaderLocation.vLightDir, 1, glm::value_ptr(LIGHT_DIR));
        glUniform3fv(g_shaderLocation.vCameraPos, 1, glm::value_ptr(g_camPos));
        glUniform1f(g_shaderLocation.fTime, time);

        drawShip(view, proj);
        drawVolcano(view, proj);
        drawChests(view, proj);   // switches to chest shader internally
        drawSkybox(view, proj);   // switches to skybox shader internally

        // Re-bind main shader: drawSkybox left program = 0
        glUseProgram(g_shaderLocation.program);
        glUniform3fv(g_shaderLocation.vLightDir, 1, glm::value_ptr(LIGHT_DIR));
        glUniform3fv(g_shaderLocation.vCameraPos, 1, glm::value_ptr(g_camPos));
        drawWater(view, proj, time);

        glBindVertexArray(0);
        glUseProgram(0);
    }

    // =========================================================================
    // onDisplay()
    // =========================================================================

    void onDisplay() {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        float time = glutGet(GLUT_ELAPSED_TIME) / 800.0f;

        glm::mat4 view = buildViewMatrix();
        glm::mat4 proj = glm::perspective(
            glm::radians(45.0f),
            (float)WIN_WIDTH / WIN_HEIGHT,
            0.1f, 200.0f
        );

        drawScene(view, proj, time);

        glutSwapBuffers();
    }

    // =========================================================================
    // onReshape()
    // =========================================================================

    void onReshape(int newWidth, int newHeight) {
        glViewport(0, 0, newWidth, newHeight);
    }

    // =========================================================================
    // onKeyDown() / onKeyUp()
    // =========================================================================

    void onKeyUp(unsigned char key, int /*x*/, int /*y*/) {
        g_keys[key] = false;
    }

    void onKeyDown(unsigned char key, int /*x*/, int /*y*/) {
        if (key == 27) glutLeaveMainLoop();   // ESC -- exit

        // Normalize to lowercase so g_keys['w'] works even with Shift held
        g_keys[key] = true;
        g_keys[(unsigned char)tolower(key)] = true;

        // Sprint state: Shift held during any key event
        g_sprint = (glutGetModifiers() & GLUT_ACTIVE_SHIFT) != 0;

        if (key == 9)  // TAB -- cycle all camera modes
            g_cameraMode = (CameraMode)((g_cameraMode + 1) % 4);
        if (key == 'r' || key == 'R')
            reloadChests();
    }

    // =========================================================================
    // onMouseMotion()
    // =========================================================================

    void onMouseMotion(int x, int y) {
        if (g_cameraMode == CAM_FREE) {
            int centerX = WIN_WIDTH / 2;
            int centerY = WIN_HEIGHT / 2;

            if (x == centerX && y == centerY) return;

            g_camYaw += CAM_SENS * (x - centerX);
            g_camPitch -= CAM_SENS * (y - centerY);
            g_camPitch = glm::clamp(g_camPitch, -89.0f, 89.0f);

            glutWarpPointer(centerX, centerY);
            glutPostRedisplay();
        }
    }

    // =========================================================================
    // onMouse()
    // =========================================================================

    /// @brief  Left click in CAM_SHIP mode: collect all chests within CHEST_PICKUP_RADIUS.
    ///
    ///         Distance is measured from g_shipPos (the ship hull origin) to each
    ///         chest position.  All chests within range are collected at once --
    ///         simpler than picking just one and avoids the player having to click
    ///         multiple times when chests are close together.
    ///         Scroll buttons 3/4 move the camera forward/backward as before.
    void onMouse(int button, int state, int /*x*/, int /*y*/) {
        if (state != GLUT_DOWN) return;

        if (button == GLUT_LEFT_BUTTON && g_cameraMode == CAM_SHIP) {
            // Collect all uncollected chests within pickup distance of the ship.
            int collected = 0;
            for (auto& chest : g_chests) {
                if (chest.collected) continue;

                // 2D distance in XZ plane (ignore Y differences -- chest is at sea level)
                float dx = chest.pos.x - g_shipPos.x;
                float dz = chest.pos.z - g_shipPos.z;
                float dist = std::sqrt(dx * dx + dz * dz);

                if (dist < g_chestConfig.pickupRadius) {
                    chest.collected = true;
                    ++collected;
                }
            }

            if (collected > 0)
                std::cout << "Picked up " << collected << " treasure chest(s)!" << std::endl;
            else
                std::cout << "No chests nearby (sail closer and try again)." << std::endl;

            glutPostRedisplay();
            return;
        }

        // Scroll wheel: zoom camera forward/backward
        glm::vec3 front = getCamFront();
        if (button == 3) g_camPos += front * ZOOM_SPEED;
        else if (button == 4) g_camPos -= front * ZOOM_SPEED;

        glutPostRedisplay();
    }

    // =========================================================================
    // onTimer()
    // =========================================================================

    void onTimer(int /*value*/) {

        float t = glutGet(GLUT_ELAPSED_TIME) / 800.0f;
        for (auto& chest : g_chests) {
            if (chest.collected) continue;
            // evaluateGerstner vrátí displacement v daném XZ bodì -- stejná funkce
            // co používá loï, takže truhly a loï sdílí stejné vlny
            GerstnerResult w = evaluateGerstner(
                glm::vec2(chest.pos.x, chest.pos.z), t);
            chest.pos.y = w.displacement.y - 0.2f;  // -0.2 = mírnì ponoøené
        }

        updateCamera();
        updateShip();

        glutTimerFunc(16, onTimer, 0);
        glutPostRedisplay();
    }

    void onSpecialKeyUp(int key, int /*x*/, int /*y*/) {
        if (key == GLUT_KEY_UP)    g_arrowKeys[0] = false;
        if (key == GLUT_KEY_DOWN)  g_arrowKeys[1] = false;
        if (key == GLUT_KEY_LEFT)  g_arrowKeys[2] = false;
        if (key == GLUT_KEY_RIGHT) g_arrowKeys[3] = false;

        g_sprint = (glutGetModifiers() & GLUT_ACTIVE_SHIFT) != 0;
    }

    void onSpecialKeyDown(int key, int /*x*/, int /*y*/) {
        // F1-F4: pøímý výbìr kamery
        if (key == GLUT_KEY_F1) { g_cameraMode = CAM_STATIC1; glutPostRedisplay(); return; }
        if (key == GLUT_KEY_F2) { g_cameraMode = CAM_STATIC2; glutPostRedisplay(); return; }
        if (key == GLUT_KEY_F3) { g_cameraMode = CAM_FREE;    glutPostRedisplay(); return; }
        if (key == GLUT_KEY_F4) { g_cameraMode = CAM_SHIP;    glutPostRedisplay(); return; }

        // Šipky: pohyb (stejná logika jako WASD, zpracuje updateCamera())
        if (key == GLUT_KEY_UP)    g_arrowKeys[0] = true;
        if (key == GLUT_KEY_DOWN)  g_arrowKeys[1] = true;
        if (key == GLUT_KEY_LEFT)  g_arrowKeys[2] = true;
        if (key == GLUT_KEY_RIGHT) g_arrowKeys[3] = true;

        g_sprint = (glutGetModifiers() & GLUT_ACTIVE_SHIFT) != 0;
    }

} // namespace galuszde