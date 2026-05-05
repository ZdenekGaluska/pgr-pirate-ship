//----------------------------------------------------------------------------------------
/**
 * \file    camera.cpp
 * \author  galuszde
 * \note    Code comments generated with AI assistance (Claude, Anthropic).
 * \brief   FPS camera implementation -- view direction and movement.
 */
 //----------------------------------------------------------------------------------------
#include "camera.h"

namespace galuszde {

    glm::vec3 getCamFront() {
        // Spherical to Cartesian conversion:
        //   yaw   = rotation in the XZ plane (left/right)
        //   pitch = elevation above/below the horizon (up/down)
 
        // The result is always normalized (length = 1)
        return glm::normalize(glm::vec3(
            cos(glm::radians(g_camYaw)) * cos(glm::radians(g_camPitch)),   // X
            sin(glm::radians(g_camPitch)),                                   // Y (elevation)
            sin(glm::radians(g_camYaw)) * cos(glm::radians(g_camPitch))    // Z
        ));
    }

    void updateCamera() {
        glm::vec3 front = getCamFront();
        glm::vec3 right = glm::normalize(glm::cross(front, glm::vec3(0.0f, 1.0f, 0.0f)));

        float speed = g_sprint ? CAM_SPEED * SPRINT_MULT : CAM_SPEED;

        if (g_cameraMode == CAM_FREE) {
            if (g_keys['w'] || g_arrowKeys[0]) g_camPos += speed * front;
            if (g_keys['s'] || g_arrowKeys[1]) g_camPos -= speed * front;
            if (g_keys['a'] || g_arrowKeys[2]) g_camPos -= speed * right;
            if (g_keys['d'] || g_arrowKeys[3]) g_camPos += speed * right;
            if (g_keys[' '])  g_camPos.y += speed;
            if (g_keys['e'])  g_camPos.y -= speed;
        }
        else if (g_cameraMode == CAM_SHIP) {
            glm::vec3 shipFront = glm::normalize(glm::vec3(
                cos(glm::radians(g_shipYaw)), 0.0f, sin(glm::radians(g_shipYaw))
            ));
            float shipSpeed = g_sprint ? SHIP_SPEED * SPRINT_MULT : SHIP_SPEED;
            if (g_keys['w'] || g_arrowKeys[0]) g_shipPos += shipSpeed * shipFront;
            if (g_keys['a'] || g_arrowKeys[2]) g_shipYaw -= SHIP_TURN_SPEED;
            if (g_keys['d'] || g_arrowKeys[3]) g_shipYaw += SHIP_TURN_SPEED;
        }
        // CAM_STATIC1 / CAM_STATIC2: camera is fixed, no movement
    }

} // namespace galuszde