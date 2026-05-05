//----------------------------------------------------------------------------------------
/**
 * \file    globals.cpp
 * \author  galuszde
 * \note    Code comments generated with AI assistance (Claude, Anthropic).
 * \brief   Definitions of global variables declared in globals.h.
 */
 //----------------------------------------------------------------------------------------
#include "globals.h"

std::vector<Mesh> g_meshes;
WaterGrid         g_water;
Mesh              g_volcano;
ShaderLocations   g_shaderLocation;

SkyboxShaderLocations g_skyboxShader;
SkyboxData            g_skybox;

// Chest globals -- populated by initChests() / reloadChests()
ChestConfig              g_chestConfig;
std::vector<ChestInstance> g_chests;
ChestGeom                g_chestGeom;
ChestShaderLocations     g_chestShader;

// --- Camera ---
glm::vec3 g_camPos = glm::vec3(0.0f, 1.5f, 5.0f);
float     g_camYaw = -90.0f;
float     g_camPitch = 0.0f;

// --- Key state ---
bool g_keys[256] = {};

// --- Ship state ---
glm::vec3 g_shipPos = glm::vec3(0.0f, 0.0f, 0.0f);
float     g_shipY = 0.0f;
glm::vec3 g_shipNormal = glm::vec3(0.0f, 1.0f, 0.0f);
float     g_shipYaw = 180.0f;
float     g_shipSpeedMod = 0.0f;

glm::vec3 g_shipRotAxis = glm::vec3(1.0f, 0.0f, 0.0f);
float     g_shipRotAngle = 0.0f;

// --- Camera mode ---
CameraMode g_cameraMode = CAM_FREE;

// --- Lighting ---
const glm::vec3 LIGHT_DIR = glm::normalize(glm::vec3(1.0f, 2.0f, 1.0f));
const glm::vec3 VOLCANO_POS = glm::vec3(30.0f, -2.0f, -20.0f);

bool g_sprint = false;
bool g_arrowKeys[4] = {};