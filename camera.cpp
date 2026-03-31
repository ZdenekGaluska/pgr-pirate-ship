//----------------------------------------------------------------------------------------
/**
 * \file    camera.cpp
 * \author  vaclaon3
 * \brief   Implementace FPS kamery.
 */
//----------------------------------------------------------------------------------------

#include "camera.h"

// ================================================================================
// getCamFront()
// ================================================================================

glm::vec3 getCamFront() {
    // Sfericke souradnice -> kartezske:
    //   yaw   = rotace v rovine XZ (doleva/doprava)
    //   pitch = elevace nad/pod horizontem (nahoru/dolu)
    //
    // Vysledek je vzdy normalizovany (delka = 1),
    // protoze cos^2 + sin^2 = 1 uz zajistuje jednotkovou delku.
    return glm::normalize(glm::vec3(
        cos(glm::radians(g_camYaw)) * cos(glm::radians(g_camPitch)),  // X
        sin(glm::radians(g_camPitch)),                                  // Y (vyska)
        sin(glm::radians(g_camYaw)) * cos(glm::radians(g_camPitch))   // Z
    ));
}

// ================================================================================
// updateCamera()
// ================================================================================

void updateCamera() {
    glm::vec3 front = getCamFront();

    // right = vektor doprava = krizovy soucin (front x globalUp)
    // Normalizujeme protoze front neni vzdy kolmy na (0,1,0) (pri pitch != 0).
    glm::vec3 right = glm::normalize(glm::cross(front, glm::vec3(0.0f, 1.0f, 0.0f)));

    // Pohyb ve smeru pohledu (W/S) a do strany (A/D)
    if (g_keys['w']) g_camPos += CAM_SPEED * front;
    if (g_keys['s']) g_camPos -= CAM_SPEED * front;
    if (g_keys['a']) g_camPos -= CAM_SPEED * right;
    if (g_keys['d']) g_camPos += CAM_SPEED * right;

    // Pohyb po ose Y — nezavisle na smeru pohledu (jako "fly mode")
    if (g_keys[' ']) g_camPos.y += CAM_SPEED;   // Space = nahoru
    if (g_keys['e']) g_camPos.y -= CAM_SPEED;   // E     = dolu
}
