//----------------------------------------------------------------------------------------
/**
 * \file    callbacks.cpp
 * \author  vaclaon3
 * \brief   Implementace GLUT callbacku.
 */
//----------------------------------------------------------------------------------------

#include "callbacks.h"
#include "camera.h"

// ================================================================================
// onDisplay() — prekreslit okno
// ================================================================================

void onDisplay() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glUseProgram(shdr.program);

    // --- View matice ---
    glm::mat4 view = glm::lookAt(
        g_camPos,
        g_camPos + getCamFront(),
        glm::vec3(0.0f, 1.0f, 0.0f)
    );

    // --- Projekcni matice ---
    glm::mat4 proj = glm::perspective(
        glm::radians(45.0f),
        (float)WIN_WIDTH / WIN_HEIGHT,
        0.1f,
        200.0f
    );

    // --- Model matice lode ---
    glm::mat4 model = glm::scale(glm::mat4(1.0f), glm::vec3(0.5f));

    // --- PVM = proj * view * model ---
    glm::mat4 PVM = proj * view * model;

    // Odeslat matice a svetelne parametry do shaderu
    glUniformMatrix4fv(shdr.mPVM, 1, GL_FALSE, glm::value_ptr(PVM));
    glUniformMatrix4fv(shdr.mModel, 1, GL_FALSE, glm::value_ptr(model));
    glUniform3fv(shdr.vLightDir, 1, glm::value_ptr(LIGHT_DIR));
    glUniform3fv(shdr.vCameraPos, 1, glm::value_ptr(g_camPos));

    // --- Kreslit vsechny sub-mese lode ---
    for (const auto& m : g_meshes) {
        glUniform3fv(shdr.vDiffuse, 1, glm::value_ptr(m.diffuseColor));
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, m.texture);
        glBindVertexArray(m.vao);
        glDrawElements(GL_TRIANGLES, m.numTriangles * 3, GL_UNSIGNED_INT, nullptr);
    }

    glm::mat4 modelWater = glm::scale(glm::mat4(1.0f), glm::vec3(4.0f));
    glm::mat4 PVMwater = proj * view * modelWater;
    glUniformMatrix4fv(shdr.mPVM, 1, GL_FALSE, glm::value_ptr(PVMwater));
    glUniformMatrix4fv(shdr.mModel, 1, GL_FALSE, glm::value_ptr(modelWater));
    // --- Kreslit vodu ---
    // Prepnout na world-space UV rezim (uWaterUVScale > 0).
    // 0.02f = textura se opakuje kazdych 50 world units — doladis vizualne.
    glUniform1f(shdr.fWaterUVScale, 0.02f);
    glUniform3fv(shdr.vDiffuse, 1, glm::value_ptr(glm::vec3(0.1f, 0.3f, 0.5f)));
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, g_water.texture);
    glBindVertexArray(g_water.vao);
    glDrawElements(GL_TRIANGLES, g_water.numIndices, GL_UNSIGNED_INT, nullptr);

    // Vratit zpet na VBO UV rezim pro dalsi objekty
    glUniform1f(shdr.fWaterUVScale, 0.0f);
    // ---- Voda dokreslena -----


    glBindVertexArray(0);
    glUseProgram(0);
    glutSwapBuffers();
}

// ================================================================================
// onReshape() — zmena velikosti okna
// ================================================================================

void onReshape(int w, int h) {
    glViewport(0, 0, w, h);
}

// ================================================================================
// onKeyDown() / onKeyUp() — klavesnice
// ================================================================================

void onKeyDown(unsigned char key, int /*x*/, int /*y*/) {
    if (key == 27) glutLeaveMainLoop();   // ESC = korektne ukoncit program
    g_keys[key] = true;
    // Pohyb se nepocita tady — pocita ho onTimer() jednou za snimek.
}

void onKeyUp(unsigned char key, int /*x*/, int /*y*/) {
    g_keys[key] = false;
}

// ================================================================================
// onMouseMotion() — pohyb mysi (bez stisku tlacitka)
// ================================================================================

void onMouseMotion(int x, int y) {
    int cx = WIN_WIDTH  / 2;
    int cy = WIN_HEIGHT / 2;

    // Ignoruj event ktery vznikl samotnym glutWarpPointer nize.
    if (x == cx && y == cy) return;

    // dx/dy = posun mysi od stredu okna za posledni snimek
    // Yaw  += dx * citlivost  (pohyb doprava zvetsuje yaw)
    // Pitch -= dy * citlivost (pohyb mysi dolu = zmenseni pitch = koukani dolu)
    g_camYaw   += CAM_SENS * (x - cx);
    g_camPitch -= CAM_SENS * (y - cy);

    // Omez pitch aby kamera nepresla pres zenith/nadir (jinak by se flip otocila)
    g_camPitch = glm::clamp(g_camPitch, -89.0f, 89.0f);

    // Vrat mys na stred okna — dalsi pohyb se zase meri od stredu
    glutWarpPointer(cx, cy);
    glutPostRedisplay();
}

// ================================================================================
// onMouse() — klik nebo scroll koleckem
// ================================================================================

void onMouse(int button, int state, int /*x*/, int /*y*/) {
    // Zajimaji nas jen stisky (GLUT_DOWN), ne uvolneni
    if (state != GLUT_DOWN) return;

    glm::vec3 front = getCamFront();
    if      (button == 3) g_camPos += front * ZOOM_SPEED;   // scroll up   = priblizit
    else if (button == 4) g_camPos -= front * ZOOM_SPEED;   // scroll down = oddálit

    // TODO: levy klik (GLUT_LEFT_BUTTON) = color picking pokladu (faze 6)

    glutPostRedisplay();
}

// ================================================================================
// onTimer() — herni smycka
// ================================================================================

void onTimer(int /*value*/) {
    // Aktualizuj pozici kamery podle stisknutych klaves
    updateCamera();

    // Jinak by se timer zavolal jen jednou a smycka by se zastavila.
    glutTimerFunc(16, onTimer, 0);   // 16ms = ~60 FPS

    // Pozadej GLUT o překreslení — zavola onDisplay() v hlavni smycce
    glutPostRedisplay();
}
