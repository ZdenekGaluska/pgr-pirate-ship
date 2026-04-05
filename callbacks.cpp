//----------------------------------------------------------------------------------------
/**
 * \file    callbacks.cpp
 * \author  vaclaon3
 * \brief   Implementace GLUT callbacku.
 */
 //----------------------------------------------------------------------------------------

#include "callbacks.h"
#include "camera.h"
#include "algorithms.h"

// ================================================================================
// onDisplay() — prekreslit okno
// ================================================================================

void onDisplay() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glUseProgram(shdr.program);

    glm::mat4 view;
    if (g_cameraMode == CAM_SHIP) {

        glm::vec3 shipFront = glm::normalize(glm::vec3(
            cos(glm::radians(g_shipYaw)),
            0.0f,
            sin(glm::radians(g_shipYaw))
        ));

        g_camPos = g_shipPos - shipFront * 9.0f + glm::vec3(0.0f, 4.0f, 0.0f);
        view = glm::lookAt(
            g_camPos ,
            g_shipPos  + glm::vec3(0.0f, 2.2f, 0.0f),
            glm::vec3(0.0f, 1.0f, 0.0f)
        );
    }

    if(g_cameraMode == CAM_FREE){
        view = glm::lookAt(
            g_camPos,
            g_camPos + getCamFront(),
            glm::vec3(0.0f, 1.0f, 0.0f)
        );
    }

    // --- Projekcni matice ---
    glm::mat4 proj = glm::perspective(
        glm::radians(45.0f),
        (float)WIN_WIDTH / WIN_HEIGHT,
        0.1f,
        200.0f
    );

    float t = glutGet(GLUT_ELAPSED_TIME) / 800.0f;

    const float SHIP_PROBE = 1.5f;

    // Stred lode — pouzijeme pro vertikalni vysku
    vaclaon3::GerstnerResult wCenter = vaclaon3::evaluateGerstner(
        glm::vec2(g_shipPos.x, g_shipPos.z), t);

    // 4 body kolem stredu — prid, zad, levy bok, pravy bok
    // Z kazdeho bodu bereme jen normalu (vyska zajima jen stred)
    glm::vec3 normalSum = glm::vec3(0.0f);
    normalSum += vaclaon3::evaluateGerstner(
        glm::vec2(g_shipPos.x, g_shipPos.z + SHIP_PROBE), t).normal;
    normalSum += vaclaon3::evaluateGerstner(
        glm::vec2(g_shipPos.x, g_shipPos.z - SHIP_PROBE), t).normal;
    normalSum += vaclaon3::evaluateGerstner(
        glm::vec2(g_shipPos.x + SHIP_PROBE, g_shipPos.z), t).normal;
    normalSum += vaclaon3::evaluateGerstner(
        glm::vec2(g_shipPos.x - SHIP_PROBE, g_shipPos.z), t).normal;

    // Prumer normaly = normalize(soucet) — jeden vektor reprezentujici
    // prumernou orientaci povrchu vody pod lodi
    glm::vec3 targetNormal = glm::normalize(normalSum);

    // --- Plynule sledovani vysky a naklonu (lerp) ---
    // glm::mix(a, b, t) = a + (b-a)*t — priblizi se o t*100 % k cilove hodnote
    // 0.05f pro vysku, 0.03f pro naklon — naklon reaguje trochu pomaleji
    float targetY = wCenter.displacement.y - 0.45f;
    g_shipY = glm::mix(g_shipY, targetY, 0.1f);
    g_shipNormal = glm::normalize(glm::mix(g_shipNormal, targetNormal, 0.06f));

    // --- Model matice lode ---
    glm::mat4 model = glm::translate(glm::mat4(1.0f),
        glm::vec3(g_shipPos.x, g_shipY, g_shipPos.z));

    // Rotace lode podle zprumerovane normaly vlny:
    // Chceme otocit vektor (0,1,0) na g_shipNormal.
    // cross(up, normal) = osa kolem ktere se lod natoci
    // atan2(sinA, cosA) = uhel o kolik — pouzivame atan2 misto acos
    //                     kvuli numericke stabilite pri malych uhlech
    glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
    glm::vec3 axis = glm::cross(up, g_shipNormal);
    float     sinA = glm::length(axis);
    float     cosA = glm::dot(up, g_shipNormal);
    if (sinA > 0.0001f)   // ochrana: kdyz je lod temer rovne, axis je nulovy vektor
        model = glm::rotate(model, std::atan2(sinA, cosA), glm::normalize(axis));

    model = glm::rotate(model, -glm::radians(g_shipYaw + 180.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    model = glm::scale(model, glm::vec3(0.5f));

    // --- PVM = proj * view * model ---
    glm::mat4 PVM = proj * view * model;

    // Odeslat matice a svetelne parametry do shaderu
    glUniformMatrix4fv(shdr.mPVM, 1, GL_FALSE, glm::value_ptr(PVM));
    glUniformMatrix4fv(shdr.mModel, 1, GL_FALSE, glm::value_ptr(model));
    glUniform3fv(shdr.vLightDir, 1, glm::value_ptr(LIGHT_DIR));
    glUniform3fv(shdr.vCameraPos, 1, glm::value_ptr(g_camPos));
    glUniform1f(shdr.fTime, t);

    // --- Kreslit vsechny sub-mese lode ---
    for (const auto& m : g_meshes) {
        glUniform3fv(shdr.vDiffuse, 1, glm::value_ptr(m.diffuseColor));
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, m.texture);
        glBindVertexArray(m.vao);
        glDrawElements(GL_TRIANGLES, m.numTriangles * 3, GL_UNSIGNED_INT, nullptr);
    }

    // --- Kreslit vodu ---
    glm::mat4 modelWater = glm::translate(glm::mat4(1.0f), glm::vec3(g_camPos.x, 0.0f, g_camPos.z));
    glm::mat4 PVMwater = proj * view * modelWater;
    glUniformMatrix4fv(shdr.mPVM, 1, GL_FALSE, glm::value_ptr(PVMwater));
    glUniformMatrix4fv(shdr.mModel, 1, GL_FALSE, glm::value_ptr(modelWater));

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

    if (key == 9) g_cameraMode = (CameraMode)((g_cameraMode + 1) % 2);
}

void onKeyUp(unsigned char key, int /*x*/, int /*y*/) {
    g_keys[key] = false;
}

// ================================================================================
// onMouseMotion() — pohyb mysi (bez stisku tlacitka)
// ================================================================================

void onMouseMotion(int x, int y) {
    if(g_cameraMode == CAM_FREE) {
        int cx = WIN_WIDTH / 2;
        int cy = WIN_HEIGHT / 2;

        // Ignoruj event ktery vznikl samotnym glutWarpPointer nize.
        if (x == cx && y == cy) return;

        // dx/dy = posun mysi od stredu okna za posledni snimek
        // Yaw  += dx * citlivost  (pohyb doprava zvetsuje yaw)
        // Pitch -= dy * citlivost (pohyb mysi dolu = zmenseni pitch = koukani dolu)
        g_camYaw += CAM_SENS * (x - cx);
        g_camPitch -= CAM_SENS * (y - cy);

        // Omez pitch aby kamera nepresla pres zenith/nadir (jinak by se flip otocila)
        g_camPitch = glm::clamp(g_camPitch, -89.0f, 89.0f);

        // Vrat mys na stred okna — dalsi pohyb se zase meri od stredu
        glutWarpPointer(cx, cy);
        glutPostRedisplay();
    }
}

// ================================================================================
// onMouse() — klik nebo scroll koleckem
// ================================================================================

void onMouse(int button, int state, int /*x*/, int /*y*/) {
    // Zajimaji nas jen stisky (GLUT_DOWN), ne uvolneni
    if (state != GLUT_DOWN) return;

    glm::vec3 front = getCamFront();
    if (button == 3) g_camPos += front * ZOOM_SPEED;   // scroll up   = priblizit
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

    // Pozadej GLUT o prekresleni — zavola onDisplay() v hlavni smycce
    glutPostRedisplay();
}