#pragma once
//----------------------------------------------------------------------------------------
/**
 * \file    camera.h
 * \author  vaclaon3
 * \brief   FPS kamera — vypocet smeru pohledu a pohyb na zaklade vstupu.
 *
 * Kamera je reprezentovana tremi globalnimi promennymi (globals.h):
 *   g_camPos   — pozice ve world space
 *   g_camYaw   — rotace doleva/doprava (stupne)
 *   g_camPitch — rotace nahoru/dolu    (stupne)
 *
 * Tento soubor poskytuje funkce pro praci s kamerou.
 * Vstupni callbacky (onKeyDown, onMouseMotion) meni g_keys / yaw / pitch.
 * Pohyb kamery pocita updateCamera() — vola se z onTimer() jednou za snimek.
 */
//----------------------------------------------------------------------------------------

#include "globals.h"

/// @brief Vypocita normalizovany smerovy vektor kamery z g_camYaw a g_camPitch.
/// @return Unit vektor smeru pohledu (delka = 1).
///
/// Prevod sfericke souradnice na kartezske:
///   x = cos(yaw) * cos(pitch)
///   y = sin(pitch)
///   z = sin(yaw) * cos(pitch)
///
/// Pouziva se pro:
///   - glm::lookAt() pri vypoctu view matice v onDisplay()
///   - pohyb WASD (posun ve smeru front a right)
///   - scroll zoom (posun ve smeru front)
glm::vec3 getCamFront();

/// @brief Aktualizuje pozici kamery podle aktualne stisknutych klaves (g_keys[]).
///
/// Pocita se z g_keys[] jednou za snimek v onTimer().
/// Pohyb je tedy plynuly a nezavisi na rychlosti opakovani klavesy v OS.
///
/// Klávesy:
///   W / S — dopredu / dozadu (ve smeru pohledu)
///   A / D — doleva / doprava (perpendikularne k pohledu)
///   Space — nahoru (osa Y)
///   E     — dolu   (osa Y)
void updateCamera();
