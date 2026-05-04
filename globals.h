#pragma once
//----------------------------------------------------------------------------------------
/**
 * \file    globals.h
 * \author  galuszde
 * \note    Code comments generated with AI assistance (Claude, Anthropic).
 * \brief   Shared data types, constants, and global variable declarations.
 *
 * This file is included by every other .cpp file in the project.
 * Rules:
 *   - Variable definitions live in globals.cpp (only "extern" declarations here).
 *   - Constants (const) are defined here directly -- the compiler inlines them, no ODR issue.
 *   - Structs are defined here -- a type definition is not a variable definition, so OK.
 */
 //----------------------------------------------------------------------------------------

#include "pgr.h"          // GLEW + freeglut + GLM + Assimp + pgr utility
#include <vector>
#include <iostream>

// --- Window and camera constants ---

const int   WIN_WIDTH = 1920;
const int   WIN_HEIGHT = 1080;
const char* const WIN_TITLE = "Piratska lod - galuszde";

const float CAM_SPEED = 0.15f;   // camera movement speed (world units per frame)
const float CAM_SENS = 0.1f;    // mouse sensitivity (degrees per pixel)
const float ZOOM_SPEED = 0.8f;    // zoom speed on scroll wheel

const float SHIP_SPEED = 0.1f;
const float SHIP_TURN_SPEED = 0.30f;

const float SHIP_SLOPE_MULT = 1.0f;    // how much wave slope increases the target speed
const float SHIP_MOD_FACTOR = 0.5f;    // how fast g_shipSpeedMod lerps toward the target speed
const float SHIP_SPEED_SCALE = 0.04f;   // global speed scalar -- keeps all ship movement values small


// --- Ocean constants ---
const int   OCEAN_GRID_DENSITY = 1024;   // Number of vertices per grid side.
const float OCEAN_SIZE = 240.0f; // World-space extent of the ocean grid (XZ).

// --- Model constants ---

/// Path to the ship GLTF model, relative to the project Working Directory.
/// Set Working Directory in VS: Project -> Properties -> Debugging -> Working Directory = $(ProjectDir)
const char* const MODEL_PATH = "pirate_ship/scene.gltf";

// --- Volcano world position ---
// Placed 30 units ahead and 20 units to the right of spawn so it is
// visible from the default camera position but does not block the ship.
extern const glm::vec3 VOLCANO_POS;

// --- Data types ---

/**
 * Mesh = one sub-mesh of a loaded 3D model, resident on the GPU.
 *
 * The ship model consists of multiple meshes (hull, masts, sails, flag, ...),
 * each with its own VAO/VBO/IBO and material color.
 *
 * VAO (Vertex Array Object)  = remembers attribute layout (where positions, normals, UVs are)
 * VBO (Vertex Buffer Object) = buffer with vertex data on the GPU
 * IBO (Index Buffer Object)  = buffer with triangle indices on the GPU
 */
struct Mesh {
    GLuint       vao = 0;
    GLuint       vbo = 0;
    GLuint       ibo = 0;
    unsigned int numTriangles = 0;
    glm::vec3    diffuseColor = glm::vec3(0.8f);   // base material color
    GLuint       texture = 0;                       // diffuse texture ID, 0 = no texture
};

/// Active camera mode.
enum CameraMode {
    CAM_FREE = 0,   // free-fly FPS camera
    CAM_SHIP = 1    // third-person camera following the ship
};

/**
 * WaterGrid = ocean surface represented as a regular triangle grid.
 *
 * Grid vertices are generated on the CPU (generateWaterGrid) and uploaded to the GPU.
 * The vertex shader animates them each frame using Gerstner wave equations.
 */
struct WaterGrid {
    GLuint       vao = 0;
    GLuint       vbo = 0;
    GLuint       ibo = 0;
    unsigned int numIndices = 0;
    GLuint       texture = 0;   // diffuse water texture
};

/**
 * ShaderLocations = cached uniform variable locations for the active shader program.
 *
 * glGetUniformLocation() is a slow call -- we call it once at startup and store
 * the integer slot IDs here. Every draw call then uses the cached values
 * instead of looking them up again.
 */
struct ShaderLocations {
    GLuint program = 0;
    GLint  mPVM = -1;   // uniform "mPVM"          = projection * view * model
    GLint  mModel = -1;   // uniform "mModel"        = model matrix (for normal transformation)
    GLint  vDiffuse = -1;   // uniform "vDiffuse"      = material color
    GLint  vLightDir = -1;   // uniform "vLightDir"     = light direction (normalized vector)
    GLint  vCameraPos = -1;   // uniform "vCameraPos"    = camera position (for specular highlight)
    GLint  fWaterUVScale = -1;  // uniform "uWaterUVScale" = UV scale for world-space water tiling
    GLint  fTime = -1;   // uniform "u_time"        = current elapsed time in seconds
};

/// Cached uniform locations for the skybox shader program.
struct SkyboxShaderLocations {
    GLuint program = 0;
    GLint  mProjection = -1;   ///< uniform "u_projection" = projection matrix
    GLint  mView = -1;   ///< uniform "u_view"       = view matrix (translation stripped in VS)
    GLint  uSkybox = -1;   ///< uniform "u_skybox"     = cubemap sampler (texture unit 0)
};

/// GPU resources for the skybox unit cube and its cubemap texture.
struct SkyboxData {
    GLuint vao = 0;   ///< VAO recording the cube's vertex layout
    GLuint vbo = 0;   ///< VBO with 36 position-only vertices
    GLuint cubemapTexture = 0;   ///< OpenGL cubemap texture ID (6 faces)
};

// --- Global variable declarations ---
// "extern" = the variable exists but is defined in globals.cpp.
// Every file that includes globals.h shares the same instance.

extern std::vector<Mesh> g_meshes;          // all sub-meshes of the loaded ship model
extern WaterGrid          g_water;           // ocean surface grid
extern Mesh               g_volcano;         // hardcoded volcano mesh (volcano.cpp)
extern ShaderLocations    g_shaderLocation;  // cached uniform locations for the active shader

extern SkyboxShaderLocations g_skyboxShader;
extern SkyboxData            g_skybox;

// --- Camera (FPS model: position + yaw/pitch) ---
extern glm::vec3 g_camPos;    // camera position in world space
extern float     g_camYaw;    // left/right rotation around Y axis, degrees
extern float     g_camPitch;  // up/down rotation around X axis, degrees

// --- Key state ---
// Array of 256 booleans, index = ASCII key code, true = key is currently held.
// Camera movement is computed in onTimer(), not directly in onKeyDown() -- keeps it frame-rate independent.
extern bool g_keys[256];

// --- Ship state ---
extern glm::vec3 g_shipPos;      // ship XZ position in world space (Y is derived from waves)
extern float     g_shipY;        // current ship height -- lerps toward the wave height each frame
extern glm::vec3 g_shipNormal;   // current surface normal under the ship -- lerps toward the wave normal
extern float     g_shipYaw;      // ship heading in degrees (rotates around Y axis)
extern float     g_shipSpeedMod; // current ship speed -- lerps toward target speed each frame

// Pre-computed ship rotation -- written by updateShip(), read by onDisplay().
// Separation of physics and rendering: onDisplay() must never compute state, only read it.
extern glm::vec3 g_shipRotAxis;    // rotation axis derived from g_shipNormal via cross(up, normal)
extern float     g_shipRotAngle;   // rotation angle in radians derived from g_shipNormal via atan2

// --- Camera mode ---
extern CameraMode g_cameraMode;

// --- Lighting ---
// Fixed sun direction (normalized unit vector, defined in globals.cpp).
extern const glm::vec3 LIGHT_DIR;