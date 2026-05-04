//----------------------------------------------------------------------------------------
/**
 * \file    globals.cpp
 * \author  galuszde
 * \note    Code comments generated with AI assistance (Claude, Anthropic).
 * \brief   Definitions of global variables declared in globals.h.
 *
 * Every "extern" variable from globals.h must have its definition exactly here.
 * Other translation units see the same instance via "extern" -- not a copy, shared memory.
 */
 //----------------------------------------------------------------------------------------
#include "globals.h"

std::vector<Mesh> g_meshes;          // populated during init() via Assimp
WaterGrid         g_water;           // populated during init() via generateWaterGrid()
Mesh              g_volcano;         // populated during init() via initVolcano()
ShaderLocations   g_shaderLocation;  // populated during init() via glGetUniformLocation()

SkyboxShaderLocations g_skyboxShader;   // populated during init() via initSkyboxShader()
SkyboxData            g_skybox;         // populated during init() via initSkybox()

// --- Camera ---
// Default position: slightly above water, pulled back from the ship
glm::vec3 g_camPos = glm::vec3(0.0f, 1.5f, 5.0f);
float     g_camYaw = -90.0f;  // -90 = camera faces forward (into -Z)
float     g_camPitch = 0.0f;  //   0 = camera looks straight ahead (not up/down)

// --- Key state ---
bool g_keys[256] = {};  // all false at startup = no key is pressed

// --- Ship state ---
glm::vec3 g_shipPos = glm::vec3(0.0f, 0.0f, 0.0f);  // ship starts at world origin
float     g_shipY = 0.0f;                          // lerps toward wave height each frame
glm::vec3 g_shipNormal = glm::vec3(0.0f, 1.0f, 0.0f);  // starts upright, lerp will tilt it
float     g_shipYaw = 180.0f;
float     g_shipSpeedMod = 0.0f;

// Default rotation: none (axis is arbitrary when angle is 0)
glm::vec3 g_shipRotAxis = glm::vec3(1.0f, 0.0f, 0.0f);
float     g_shipRotAngle = 0.0f;

// --- Camera mode ---
CameraMode g_cameraMode = CAM_FREE;

// --- Lighting ---
// Sun direction: slightly right, mostly from above, slightly forward.
// normalize() ensures length == 1.0 as expected by the shader.
const glm::vec3 LIGHT_DIR = glm::normalize(glm::vec3(1.0f, 2.0f, 1.0f));

// Fixed world position for the hardcoded volcano mesh.
// 30 units forward, -1 below sea level (base sits in water), -20 to the side.
const glm::vec3 VOLCANO_POS = glm::vec3(30.0f, -2.0f, -20.0f);