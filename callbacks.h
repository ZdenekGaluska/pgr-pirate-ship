#pragma once
//----------------------------------------------------------------------------------------
/**
 * \file    callbacks.h
 * \author  vaclaon3
 * \brief   Deklarace GLUT callbacku.
 *
 * Tady jsou jen vstupni body z GLUT — kazda funkce zachyti event
 * a zavola prislusny modul (kamera, logika, kresleni...).
 *
 * Registrace techto callbacku se dela v main.cpp pres glutXxxFunc().
 *
 * Signatury jsou pevne dane GLUT API — nelze je menit.
 */
//----------------------------------------------------------------------------------------

#include "globals.h"

/// Vola GLUT kdyz je treba prekreslit okno.
/// Pocita view/proj matice, posila uniformy, kreslí vsechny mese.
void onDisplay();

/// Vola GLUT pri zmene velikosti okna.
/// Nastavi viewport (oblast kresleni) na novou velikost.
void onReshape(int w, int h);

/// Vola GLUT pri stisku klavesy (ASCII znaky, mezera, ESC...).
/// Nastavi g_keys[key] = true; ESC ukonci program.
void onKeyDown(unsigned char key, int x, int y);

/// Vola GLUT pri uvolneni klavesy.
/// Nastavi g_keys[key] = false.
void onKeyUp(unsigned char key, int x, int y);

/// Vola GLUT pri pohybu mysi BEZ stisknuti tlacitka.
/// Aktualizuje g_camYaw a g_camPitch, vraci mys na stred okna.
void onMouseMotion(int x, int y);

/// Vola GLUT pri kliknuti mysi nebo scrollovani koleckem.
/// Scroll (button 3/4) = zoom (posun kamery dopredu/dozadu).
/// Levy klik = v budoucnu color picking pokladu.
void onMouse(int button, int state, int x, int y);

/// Vola se periodicky (16ms = ~60 FPS) z glutTimerFunc.
/// Pohybuje kamerou podle g_keys[], pak znovu registruje sam sebe.
void onTimer(int value);
