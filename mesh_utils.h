#pragma once
//----------------------------------------------------------------------------------------
/**
 * \file    mesh_utils.h
 * \author  galuszde
 * \note    Code comments generated with AI assistance (Claude, Anthropic).
 * \brief   GPU upload utilities for meshes and textures.
 *
 * Contents:
 *   loadTexture()       -- loads PNG/JPG from disk, uploads to GPU, returns texture ID
 *   uploadMesh()        -- takes an Assimp aiMesh and uploads vertices/normals/UVs/indices to GPU
 *   generateWaterGrid() -- generates a regular triangle grid representing the ocean surface
 *   loadShipModel()     -- loads the pirate ship GLTF via Assimp, uploads all sub-meshes to GPU
 *   initWater()         -- generates water grid and loads water texture
 *
 * All functions use OpenGL objects (GLuint) -- must be called after pgr::initialize().
 */
 //----------------------------------------------------------------------------------------

#include "globals.h"

namespace galuszde {

    /// @brief  Loads an image from disk and uploads it to the GPU as a 2D texture.
    ///         Delegates to pgr::createTexture() which uses DevIL internally.
    ///         Generates mipmaps automatically. Supports PNG, JPG, BMP, TGA, DDS.
    /// @param  path  Path to the image file, relative to the working directory.
    /// @return OpenGL texture ID on success, 0 on failure.
    GLuint loadTexture(const char* path);

    /// @brief  Uploads a single Assimp mesh to the GPU (VBO + IBO + VAO).
    ///         Extracts diffuse texture path and fallback color from the material.
    ///
    /// VBO layout (non-interleaved -- three contiguous blocks):
    ///   [positions * N][normals * N][UVs * N]   where N = vertex count
    ///
    /// Shader attribute locations:
    ///   location 0 = vec3 position
    ///   location 1 = vec3 normal
    ///   location 2 = vec2 UV coordinates
    ///
    /// @param  mesh  Pointer to the Assimp mesh (must not be null).
    /// @param  mat   Pointer to the Assimp material for this mesh (must not be null).
    /// @return Mesh struct with valid VAO, VBO, IBO, texture ID, and triangle count.
    Mesh uploadMesh(const aiMesh* mesh, const aiMaterial* mat);

    /// @brief  Generates a regular NxN triangle grid for the ocean surface and uploads
    ///         it to the GPU. Grid is centered on (0,0,0) in the XZ plane (Y = 0 at rest).
    ///         Gerstner wave animation is applied per-vertex in the vertex shader.
    /// @param  grid  Output WaterGrid struct filled with VAO, VBO, IBO, and index count.
    void generateWaterGrid(WaterGrid& grid);

    /// @brief  Loads the pirate ship GLTF model via Assimp and uploads every
    ///         sub-mesh to the GPU. Results are appended to g_meshes.
    /// @return true on success, false if the file cannot be loaded.
    bool loadShipModel();

    /// @brief  Generates the water grid geometry and loads the water texture.
    ///         A missing texture is non-fatal -- water will render black.
    /// @return true always (grid generation cannot fail).
    bool initWater();

} // namespace galuszde