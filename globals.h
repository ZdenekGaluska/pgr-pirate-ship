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
const float SHIP_MOD_FACTOR = 0.05f;    // how fast g_shipSpeedMod lerps toward the target speed
const float SHIP_SPEED_SCALE = 0.04f;   // global speed scalar -- keeps all ship movement values small
const float SPRINT_MULT = 4.0f; // speed modifier while holding Shift

// Static camera 1 -- top view on volcano
const glm::vec3 CAM_STATIC1_POS = glm::vec3(15.0f, 26.0f, -5.0f);
const glm::vec3 CAM_STATIC1_TARGET = glm::vec3(30.0f, 2.0f, -20.0f);

// Static camera -- ocean panorama 
const glm::vec3 CAM_STATIC2_POS = glm::vec3(-55.0f, 14.0f, 55.0f);
const glm::vec3 CAM_STATIC2_TARGET = glm::vec3(0.0f, 0.0f, 0.0f);

// --- Ocean constants ---
const int   OCEAN_GRID_DENSITY = 1500;   // Number of vertices per grid side.
const float OCEAN_SIZE = 400.0f; // World-space extent of the ocean grid (XZ).

// --- Model constants ---

/// Path to the ship GLTF model, relative to the project Working Directory.
/// Set Working Directory in VS: Project -> Properties -> Debugging -> Working Directory = $(ProjectDir)
const char* const MODEL_PATH = "pirate_ship/scene.gltf";

// --- Volcano world position ---
extern const glm::vec3 VOLCANO_POS;

// Bird animation constants
const float BIRD_RADIUS = 15.0f;   // circle radius around volcano
const float BIRD_HEIGHT = 18.0f;   // height above volcano base
const float BIRD_SPEED = 0.4f;    // angular velocity (radians per second)

// Cloud animation constants
const float CLOUD_HEIGHT = 40.0f;   // height above volcano base
const float CLOUD_SIZE = 15.0f;   // half-extent of the plane in world units
const float CLOUD_FPS = 2.0f;    // frames per second (slow drifting clouds)
const int   CLOUD_FRAME_COLS = 2;       // spritesheet columns
const int   CLOUD_FRAME_ROWS = 3;       // spritesheet rows

// --- Data types ---

/**
 * Mesh = one sub-mesh of a loaded 3D model, resident on the GPU.
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
    glm::vec3    diffuseColor = glm::vec3(0.8f);
    GLuint       texture = 0;
};

/// Active camera mode.
enum CameraMode {
    CAM_FREE = 0,   // free-fly FPS camera
    CAM_SHIP = 1,   // third  person boat view
    CAM_STATIC1 = 2,   // static view on volcano
    CAM_STATIC2 = 3     // static view on ocean
        
};

/**
 * WaterGrid = ocean surface represented as a regular triangle grid.
 */
struct WaterGrid {
    GLuint       vao = 0;
    GLuint       vbo = 0;
    GLuint       ibo = 0;
    unsigned int numIndices = 0;
    GLuint       texture = 0;
};

/**
 * ShaderLocations = cached uniform variable locations for the main shader program.
 *
 * glGetUniformLocation() is a slow call -- we call it once at startup and store
 * the integer slot IDs here.  u_ambient / u_specularStr / u_shininess enable
 * per-object material variation (rubric 9b).
 */
struct ShaderLocations {
    GLuint program = 0;
    GLint  mPVM = -1;   ///< uniform "mPVM"          = projection * view * model
    GLint  mModel = -1;   ///< uniform "mModel"        = model matrix
    GLint  vDiffuse = -1;   ///< uniform "vDiffuse"      = material base color
    GLint  vLightDir = -1;   ///< uniform "vLightDir"     = light direction
    GLint  vCameraPos = -1;   ///< uniform "vCameraPos"    = camera position
    GLint  fWaterUVScale = -1;   ///< uniform "uWaterUVScale" = water UV scale
    GLint  fTime = -1;   ///< uniform "u_time"        = elapsed time

    // Per-object material parameters (rubric 9b -- materials via uniform)
    GLint  uAmbient = -1;   ///< uniform "u_ambient"     = ambient light factor
    GLint  uSpecularStr = -1;   ///< uniform "u_specularStr" = specular intensity multiplier
    GLint  uShininess = -1;   ///< uniform "u_shininess"   = Phong shininess exponent
    GLint  uLavaActive = -1;   ///< uniform "u_lavaActive" = lava point light on/off
    GLint vCameraDir = -1;   ///< uniform "vCameraDir" = normalized camera look direction
};

/// Cached uniform locations for the skybox shader program.
struct SkyboxShaderLocations {
    GLuint program = 0;
    GLint  mProjection = -1;
    GLint  mView = -1;
    GLint  uSkybox = -1;
};

/// GPU resources for the skybox unit cube and its cubemap texture.
struct SkyboxData {
    GLuint vao = 0;
    GLuint vbo = 0;
    GLuint cubemapTexture = 0;
};

/// @brief  Runtime state of the animated seagull (rubric 16a).
struct BirdState {
    Mesh  mesh;           ///< GPU geometry loaded from seagull OBJ/GLTF.
    float angle = 0.0f;   ///< Current angle on the circle (radians).
};

/// @brief  Cloud plane state -- spritesheet animation above volcano (rubric 15b).
struct CloudData {
    GLuint vao = 0;
    GLuint vbo = 0;
    GLuint ibo = 0;
    GLuint texture = 0;

    // Shader locations
    GLuint program = 0;
    GLint  locPVM = -1;
    GLint  locFrameOff = -1;
    GLint  locAlpha = -1;
    GLint  locTexture = -1;

    float  timer = 0.0f;   ///< Accumulated time for frame selection.
};

// ---------------------------------------------------------------------------
// Chest types
// ---------------------------------------------------------------------------

/// @brief  Values read from config.txt (reloaded at runtime with 'R').
struct ChestConfig {
    int   count = 15;     ///< Number of chests to place in the world.
    float area = 120.0f; ///< Radius of the spawn circle around origin (world units).
    int   seed = 42;     ///< RNG seed -- change for a different layout.
    float pickupRadius = 8.0f;   ///< Distance for left-click chest collection.
    float scale = 1.5f;   ///< Uniform scale applied to each chest instance.
};

/// @brief  Per-instance data for one chest in the world.
///
/// All instances share the same VAO/VBO/IBO (g_chestGeom).
/// Each instance is drawn with a different translate model matrix.
struct ChestInstance {
    glm::vec3 pos;
    bool      collected = false;   ///< true = has been picked up, skip draw and pickup test
};

/// @brief  Shared GPU geometry and textures for all chest instances.
///
/// Loaded once by initChests(); not re-uploaded on reloadChests().
struct ChestGeom {
    GLuint       vao = 0;
    GLuint       vbo = 0;
    GLuint       ibo = 0;
    unsigned int numIndices = 0;
    GLuint       texDiffuse = 0;   ///< Diffuse / base color texture (unit 0)
    GLuint       texNormal = 0;   ///< Tangent-space normal map     (unit 1)
};

/// @brief  Cached uniform locations for the chest shader program.
struct ChestShaderLocations {
    GLuint program = 0;
    GLint  mPVM = -1;
    GLint  mModel = -1;
    GLint  uDiffuseMap = -1;   ///< sampler2D  bound to texture unit 0
    GLint  uNormalMap = -1;   ///< sampler2D  bound to texture unit 1
    GLint  uEnvMap = -1;   ///< samplerCube bound to texture unit 2
    GLint  uEnvStrength = -1;
    GLint  vLightDir = -1;
    GLint  vCameraPos = -1;
    GLint  uAmbient = -1;
    GLint  uSpecular = -1;
    GLint  uShininess = -1;
};

// --- Global variable declarations ---

extern std::vector<Mesh>  g_meshes;
extern WaterGrid           g_water;
extern Mesh                g_volcano;
extern ShaderLocations     g_shaderLocation;

extern SkyboxShaderLocations g_skyboxShader;
extern SkyboxData            g_skybox;

// Chest globals
extern ChestConfig              g_chestConfig;
extern std::vector<ChestInstance> g_chests;
extern ChestGeom                g_chestGeom;
extern ChestShaderLocations     g_chestShader;

// --- Camera ---
extern glm::vec3 g_camPos;
extern float     g_camYaw;
extern float     g_camPitch;

// --- Key state ---
extern bool g_keys[256];

// --- Ship state ---
extern glm::vec3 g_shipPos;
extern float     g_shipY;
extern glm::vec3 g_shipNormal;
extern float     g_shipYaw;
extern float     g_shipSpeedMod;
extern glm::vec3 g_shipRotAxis;
extern float     g_shipRotAngle;

// --- Camera mode ---
extern CameraMode g_cameraMode;

// --- Lighting ---
extern const glm::vec3 LIGHT_DIR;

extern bool g_sprint;          ///< true while Shift is held -- multiplies movement speed
extern bool g_arrowKeys[4];    ///< arrow key state: [0]=up [1]=down [2]=left [3]=right

extern BirdState g_bird;

extern CloudData g_cloud;

extern bool g_lavaActive;   ///< true = lava point light enabled
extern bool g_birdPaused;   ///< true = seagull animation paused