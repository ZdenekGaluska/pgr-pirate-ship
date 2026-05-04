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

        // right = sideways vector = cross product of front and world up.
        // Normalized because front is not always perpendicular to (0,1,0) when pitch != 0.
        glm::vec3 right = glm::normalize(glm::cross(front, glm::vec3(0.0f, 1.0f, 0.0f)));

        if (g_cameraMode == CAM_FREE) {
            // Move along view direction (W/S) and sideways (A/D)
            if (g_keys['w']) g_camPos += CAM_SPEED * front;
            if (g_keys['s']) g_camPos -= CAM_SPEED * front;
            if (g_keys['a']) g_camPos -= CAM_SPEED * right;
            if (g_keys['d']) g_camPos += CAM_SPEED * right;

            // Vertical movement independent of view direction (fly mode)
            if (g_keys[' ']) g_camPos.y += CAM_SPEED;   // Space = up
            if (g_keys['e']) g_camPos.y -= CAM_SPEED;   // E     = down
        }
        else if (g_cameraMode == CAM_SHIP) {
            // Ship forward vector in XZ -- ship has no pitch
            glm::vec3 shipFront = glm::normalize(glm::vec3(
                cos(glm::radians(g_shipYaw)),
                0.0f,
                sin(glm::radians(g_shipYaw))
            ));

            if (g_keys['w']) g_shipPos += SHIP_SPEED * shipFront;
            if (g_keys['a']) g_shipYaw -= SHIP_TURN_SPEED;
            if (g_keys['d']) g_shipYaw += SHIP_TURN_SPEED;
        }
    }

} // namespace galuszde