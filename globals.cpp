//----------------------------------------------------------------------------------------
/**
 * \file    globals.cpp
 * \author  vaclaon3
 * \brief   Definice globalnich promennych deklarovanych v globals.h.
 *
 * Pravidlo: kazda "extern" promenna z globals.h musi mit svoji definici prave zde.
 * Ostatni soubory pres "extern" vidi ty same instance — neni to kopie, je to sdilena pamet.
 */
 //----------------------------------------------------------------------------------------

#include "globals.h"

// ================================================================================
// DEFINICE GLOBALNICH PROMENNYCH
// ================================================================================

std::vector<Mesh> g_meshes;              // naplni se pri init() pres Assimp
WaterGrid         g_water;               // naplni se pri init() pres generateWaterGrid()
ShaderLocations   shdr;                  // naplni se pri init() pres glGetUniformLocation()

// --- Kamera ---
// Vychozi pozice: trochu nad vodou, oddalena od lode
glm::vec3 g_camPos = glm::vec3(0.0f, 1.5f, 5.0f);
float     g_camYaw = -90.0f;   // -90 = kamera kouka dopredu (do -Z)
float     g_camPitch = 0.0f;     // 0   = kamera kouka rovne (ne nahoru/dolu)

// --- Stav klaves ---
bool g_keys[256] = {};   // vsechno false na zacatku = zadna klavesa neni stisknuta

// --- Stav lode ---
glm::vec3 g_shipPos = glm::vec3(0.0f, 0.0f, 0.0f);   // lod zacina uprostred sveta
float     g_shipY = 0.0f;                            // zacina na hladine, lerp ji dorovná
glm::vec3 g_shipNormal = glm::vec3(0.0f, 1.0f, 0.0f);   // zacina rovne (flat), lerp ji natoci
float     g_shipYaw = 180.0f;

// --- Stav kammery ---
CameraMode g_cameraMode = CAM_FREE;

// --- Svetlo ---
// Smer slunce: trochu zprava, hodne shora, trochu dopredu.
// normalize() = delka vektoru bude 1 (shader to ocekava).
const glm::vec3 LIGHT_DIR = glm::normalize(glm::vec3(1.0f, 2.0f, 1.0f));