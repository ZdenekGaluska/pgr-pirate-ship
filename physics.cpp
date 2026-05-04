//----------------------------------------------------------------------------------------
/**
 * \file    physics.cpp
 * \author  galuszde
 * \note    Code comments generated with AI assistance (Claude, Anthropic).
 * \brief   Ship physics — movement, tilt, rotation driven by Gerstner waves.
 */
 //----------------------------------------------------------------------------------------

#include "physics.h"
#include "algorithms.h"
#include "globals.h"
#include "camera.h"

namespace galuszde {

    // ================================================================================
    // Internal helpers — static, not exposed in physics.h
    // ================================================================================

    /// @brief Samples wave normals at four hull probe points and lerps g_shipNormal.
    ///
    /// Four probes instead of one so the normal represents the average tilt of the
    /// whole hull area, not just a single center point.
    /// Probe offset 1.5f ≈ half the hull width at ship scale 0.5.
    ///
    /// @param t  Current simulation time (same scale as vertex shader).
    static void sampleShipNormal(float t) {
        const float SHIP_PROBE = 1.5f;

        glm::vec3 normalSum = glm::vec3(0.0f);
        normalSum += galuszde::evaluateGerstner(
            glm::vec2(g_shipPos.x, g_shipPos.z + SHIP_PROBE), t).normal;
        normalSum += galuszde::evaluateGerstner(
            glm::vec2(g_shipPos.x, g_shipPos.z - SHIP_PROBE), t).normal;
        normalSum += galuszde::evaluateGerstner(
            glm::vec2(g_shipPos.x + SHIP_PROBE, g_shipPos.z), t).normal;
        normalSum += galuszde::evaluateGerstner(
            glm::vec2(g_shipPos.x - SHIP_PROBE, g_shipPos.z), t).normal;

        // Lerp — 0.06f gives slow, smooth tilt response (no instant snapping)
        glm::vec3 targetNormal = glm::normalize(normalSum);
        g_shipNormal = glm::normalize(glm::mix(g_shipNormal, targetNormal, 0.06f));
    }

    /// @brief Precomputes ship rotation axis and angle from g_shipNormal.
    ///
    /// Stored into g_shipRotAxis / g_shipRotAngle so that onDisplay() can apply
    /// them without modifying any state (onDisplay must be side-effect-free).
    ///
    /// Rotates world-up (0,1,0) onto g_shipNormal:
    ///   cross(up, normal) = rotation axis
    ///   atan2(sinA, cosA) = rotation angle (more stable than acos at small angles)
    static void computeShipRotation() {
        glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
        glm::vec3 axis = glm::cross(up, g_shipNormal);
        float     sinA = glm::length(axis);
        float     cosA = glm::dot(up, g_shipNormal);

        if (sinA > 0.0001f) {
            // Normal case — ship is tilted, rotation axis is valid
            g_shipRotAxis = glm::normalize(axis);
            g_shipRotAngle = std::atan2(sinA, cosA);
        }
        else {
            // Ship is nearly flat — axis would be a zero vector, normalize() would explode.
            // Set an arbitrary axis with zero angle = no rotation.
            g_shipRotAxis = glm::vec3(1.0f, 0.0f, 0.0f);
            g_shipRotAngle = 0.0f;
        }
    }

    /// @brief Lerps g_shipY toward the wave surface height at the ship's center.
    ///
    /// -0.45f = vertical offset so the hull sits in the water, not above it.
    /// 0.1f   = lerp speed (faster than tilt; height tracks waves more tightly).
    ///
    /// @param t  Current simulation time (same scale as vertex shader).
    static void trackShipHeight(float t) {
        galuszde::GerstnerResult wCenter = galuszde::evaluateGerstner(
            glm::vec2(g_shipPos.x, g_shipPos.z), t);

        float targetY = wCenter.displacement.y - 0.45f;
        g_shipY = glm::mix(g_shipY, targetY, 0.1f);
    }

    /// @brief Moves the ship horizontally — active (W held) or passive (wave-driven).
    ///
    /// Active:  player steers in g_shipYaw direction; wave slope adds/subtracts speed.
    ///   slopeBonus = dot(heading, slopeDir) * slopeLen * SHIP_SLOPE_MULT
    ///   +1 = heading downhill (accelerates), -1 = uphill (decelerates).
    ///
    /// Passive: ship slides along the wave slope independently of its yaw.
    ///   On flat water the speed modifier decays toward zero.
    static void moveShip() {
        // Forward direction in the XZ plane derived from current yaw
        glm::vec3 shipFront3D = glm::normalize(glm::vec3(
            cos(glm::radians(g_shipYaw)),
            0.0f,
            sin(glm::radians(g_shipYaw))
        ));

        // XZ component of wave normal = slope direction and steepness under the hull
        glm::vec2 slopeXZ = glm::vec2(g_shipNormal.x, g_shipNormal.z);
        float     slopeLen = glm::length(slopeXZ);

        if (g_keys['w'] && g_cameraMode == CAM_SHIP) {
            // --- ACTIVE movement ---
            float slopeBonus = 0.0f;
            if (slopeLen > 0.001f) {
                glm::vec2 slopeDir = slopeXZ / slopeLen;
                float     alignment = glm::dot(
                    glm::vec2(shipFront3D.x, shipFront3D.z), slopeDir);
                slopeBonus = alignment * slopeLen * SHIP_SLOPE_MULT;
            }

            float targetSpeed = glm::max(SHIP_SPEED + slopeBonus, 0.0f);
            g_shipSpeedMod += (targetSpeed - g_shipSpeedMod) * SHIP_MOD_FACTOR;
            g_shipPos += shipFront3D * g_shipSpeedMod;
        }
        else {
            // --- PASSIVE movement — slide along wave slope ---
            if (slopeLen > 0.001f) {
                float targetSpeed = slopeLen * SHIP_SLOPE_MULT;
                g_shipSpeedMod += (targetSpeed - g_shipSpeedMod) * SHIP_MOD_FACTOR;
                g_shipPos.x += slopeXZ.x * g_shipSpeedMod * SHIP_SPEED_SCALE;
                g_shipPos.z += slopeXZ.y * g_shipSpeedMod * SHIP_SPEED_SCALE;
            }
            else {
                // Flat surface — gradually decelerate to zero
                g_shipSpeedMod += (0.0f - g_shipSpeedMod) * SHIP_MOD_FACTOR * SHIP_SPEED_SCALE;
            }
        }
    }

    // ================================================================================
    // Public entry point
    // ================================================================================

    void updateShip() {
        // Time — same scale as in the vertex shader and onDisplay()
        float t = glutGet(GLUT_ELAPSED_TIME) / 800.0f;

        sampleShipNormal(t);   // 1. update g_shipNormal from wave probes
        computeShipRotation(); // 2. precompute g_shipRotAxis / g_shipRotAngle
        trackShipHeight(t);    // 3. lerp g_shipY to wave surface
        moveShip();            // 4. translate g_shipPos
    }

} // namespace galuszde