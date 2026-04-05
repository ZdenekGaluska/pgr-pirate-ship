#pragma once
//----------------------------------------------------------------------------------------
/**
 * \file    globals.h
 * \author  vaclaon3
 * \brief   Sdilene datove typy, konstanty a deklarace globalnich promennych.
 *
 * Tento soubor includuje kazdy jiny .cpp soubor projektu.
 * Pravidla:
 *   - Definice promennych jsou v globals.cpp (tady jen "extern" deklarace).
 *   - Konstanty (const) jsou tady primo — kompilator je inlinuje, zadny ODR problem.
 *   - Struktury jsou tady — definice typu neni definice promenne, tedy OK.
 */
 //----------------------------------------------------------------------------------------

#include "pgr.h"          // GLEW + freeglut + GLM + Assimp + pgr utility
#include <vector>
#include <iostream>

// ================================================================================
// KONSTANTY OKNA A KAMERY
// ================================================================================

const int   WIN_WIDTH = 1920;
const int   WIN_HEIGHT = 1080;
const char* const WIN_TITLE = "Piratska lod - vaclaon3";

const float CAM_SPEED = 0.15f;   // rychlost pohybu kamery (jednotky/snimek)
const float CAM_SENS = 0.1f;    // citlivost mysi (stupne/pixel)
const float ZOOM_SPEED = 0.8f;    // rychlost zoomu pri scrollovani

const float SHIP_SPEED = 0.08f;
const float SHIP_TURN_SPEED = 0.30f;

// ================================================================================
// KONSTANTY MODELU
// ================================================================================

/// Cesta k GLTF modelu lode, relativne k Working Directory projektu.
/// Working Directory se nastavuje ve VS: Project -> Properties -> Debugging -> Working Directory = $(ProjectDir)
const char* const MODEL_PATH = "pirate_ship/scene.gltf";

// ================================================================================
// DATOVE TYPY
// ================================================================================

/**
 * Mesh = jeden sub-mesh nacteneho 3D modelu na GPU.
 *
 * Model lode se sklada z vice meshu (trup, stezne, vlajka, ...),
 * kazdy ma vlastni VAO/VBO/IBO a materialovou barvu.
 *
 * VAO (Vertex Array Object)  = zapamatuje si layout atributu (kde jsou pozice, normaly, UV)
 * VBO (Vertex Buffer Object) = buffer s daty vrcholu na GPU
 * IBO (Index Buffer Object)  = buffer s indexy trojuhelniku na GPU
 */
struct Mesh {
    GLuint       vao = 0;
    GLuint       vbo = 0;
    GLuint       ibo = 0;
    unsigned int numTriangles = 0;
    glm::vec3    diffuseColor = glm::vec3(0.8f);  // zakladni barva materialu
    GLuint       texture = 0;                 // ID diffuse textury, 0 = zadna textura
};

// 
enum CameraMode {
    CAM_FREE = 0,
    CAM_SHIP = 1
};

/**
 * WaterGrid = plocha vody reprezentovana pravidelnou mrizkou trojuhelniku.
 *
 * Vrcholy mrizky jsou generovany v CPU (generateWaterGrid), nahrane do GPU.
 * Shader pak vrcholy animuje (Gerstner waves).
 */
struct WaterGrid {
    GLuint       vao = 0;
    GLuint       vbo = 0;
    GLuint       ibo = 0;
    unsigned int numIndices = 0;
    GLuint       texture = 0;   // diffuse textura vody
};

/**
 * ShaderLocations = ulozene lokace uniform promennych shaderu.
 *
 * glGetUniformLocation() je pomale volani — zavolame ho jednou pri init()
 * a vysledky si ulozime sem. Pri kazdem draw callu pak pouzijeme ulozene hodnoty
 * misto opakovaneho vyhledavani.
 */
struct ShaderLocations {
    GLuint program = 0;
    GLint  mPVM = -1;   // uniform "mPVM"          = projekce * view * model
    GLint  mModel = -1;   // uniform "mModel"        = model matice (pro transformaci normal)
    GLint  vDiffuse = -1;   // uniform "vDiffuse"      = barva materialu
    GLint  vLightDir = -1;   // uniform "vLightDir"     = smer svetla (normalizovany vektor)
    GLint  vCameraPos = -1;   // uniform "vCameraPos"    = pozice kamery (pro specular highlight)
    GLint  fWaterUVScale = -1;  // uniform "uWaterUVScale" = mierka UV pro vodu z world coords
    GLint  fTime = -1;   // uniform "u_time"        = aktualni cas v sekundach
};

// ================================================================================
// DEKLARACE GLOBALNICH PROMENNYCH
// "extern" = promenna existuje, ale je definovana v globals.cpp.
// Kazdy soubor co includuje globals.h vidi tyto promenne.
// ================================================================================

extern std::vector<Mesh> g_meshes;   // vsechny sub-mese nacteneho modelu lode
extern WaterGrid          g_water;   // mrizka vody
extern ShaderLocations    shdr;      // lokace uniform promennych aktivniho shaderu

// --- Kamera (FPS model: pozice + yaw/pitch) ---
extern glm::vec3 g_camPos;    // pozice kamery ve world space
extern float     g_camYaw;    // rotace doleva/doprava (kolem osy Y), stupne
extern float     g_camPitch;  // rotace nahoru/dolu   (kolem osy X), stupne

// --- Stav klaves ---
// Pole 256 hodnot, index = ASCII kod klavesy, true = klavesa je stisknuta.
// Pohyb kamery se pocita v onTimer(), ne primo v onKeyDown() — tak je plynuly.
extern bool g_keys[256];

// --- Stav lode ---
extern glm::vec3 g_shipPos;      // XZ pozice lode ve world space (Y se pocita z vln)
extern float     g_shipY;        // aktualni vyska lode — pomalu lerp-uje k vysce vlny
extern glm::vec3 g_shipNormal;   // aktualni normala pod lodi — pomalu lerp-uje k normale vlny
extern float     g_shipYaw;

// --- Stav kamery
extern CameraMode g_cameraMode;

// --- Svetlo ---
// Pevny smer slunce (normalizovany unit vektor, definovano v globals.cpp).
extern const glm::vec3 LIGHT_DIR;