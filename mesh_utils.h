#pragma once
//----------------------------------------------------------------------------------------
/**
 * \file    mesh_utils.h
 * \author  vaclaon3
 * \brief   Utility funkce pro praci s meshi a texturami.
 *
 * Obsah:
 *   loadTexture()      — nacte PNG/JPG ze souboru, nahraje do GPU, vrati ID textury
 *   uploadMesh()       — vezme Assimp aiMesh a nahraj data (vrcholy, normaly, UV, indexy) na GPU
 *   generateWaterGrid()— vygeneruje pravidelnou mrizku trojuhelniku reprezentujici vodu
 *
 * Vsechny funkce pracuji s OpenGL objekty (GLuint) — musi se volat az po pgr::initialize().
 */
//----------------------------------------------------------------------------------------

#include "globals.h"

/// @brief Nacte obrazek ze souboru a nahraje ho jako OpenGL texturu.
/// @param path  Cesta k souboru (relativne k Working Directory), napr. "pirate_ship/sail.png"
/// @return      GLuint ID textury, 0 pokud se nacitani nezdarilo.
///
/// Textura ma nastaveno:
///   - WRAP:   GL_REPEAT (textura se opakuje)
///   - FILTER: GL_LINEAR_MIPMAP_LINEAR (trilinearni filtrovani)
///   - Mipmaps: automaticky vygenerovane (glGenerateMipmap)
GLuint loadTexture(const char* path);

/// @brief Vezme jeden Assimp mesh a nahraj ho na GPU (do VAO/VBO/IBO).
/// @param mesh  Ukazatel na Assimp mesh (nesmí byt NULL).
/// @param mat   Ukazatel na Assimp material pro tento mesh (nesmí byt NULL).
/// @return      Naplneny Mesh objekt s platnym VAO, VBO, IBO.
///
/// Layout VBO (non-interleaved — data jsou za sebou v blocich):
///   [pozice * N][normaly * N][UV * N]
///   kde N = pocet vrcholu
///
/// Atributy shaderu:
///   location 0 = vec3 pozice
///   location 1 = vec3 normala
///   location 2 = vec2 UV souradnice
Mesh uploadMesh(const aiMesh* mesh, const aiMaterial* mat);

/// @brief Vygeneruje pravidelnou NxN mrizku trojuhelniku pro plochu vody.
/// @param grid  [out] WaterGrid objekt ktery bude naplnen VAO/VBO/IBO.
///
/// Mrizka je centrována na bod (0, 0, 0) v rovine XZ (Y = 0).
/// Velikost mrizky a hustota vrcholu jsou nastaveny jako lokalni konstanty uvnitr funkce.
/// V budoucnu shader pohne vrcholy podle Gerstner wave rovnic.
void generateWaterGrid(WaterGrid& grid);
