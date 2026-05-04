//----------------------------------------------------------------------------------------
/**
 * \file    mesh_utils.cpp
 * \author  galuszde
 * \note    Code comments generated with AI assistance (Claude, Anthropic).
 * \brief   GPU upload utilities for meshes, textures, and the water grid.
 *
 * Texture loading uses pgr::createTexture() which internally uses DevIL.
 * DevIL is initialized by pgr::initialize() in main.cpp -- all functions here
 * must be called after pgr::initialize().
 */
 //----------------------------------------------------------------------------------------

#include "mesh_utils.h"

namespace galuszde {

    GLuint loadTexture(const char* path) {
        // pgr::createTexture handles: DevIL load, glGenTextures, glTexImage2D,
        // glGenerateMipmap, and GL_LINEAR_MIPMAP_LINEAR filter setup.
        // Returns 0 and prints error to stderr on failure.
        GLuint textureID = pgr::createTexture(path);
        if (!textureID)
            std::cerr << "Warning: failed to load texture: " << path << std::endl;
        return textureID;
    }

    // -----------------------------------------------------------------------
    // uploadMesh() helpers -- static
    // -----------------------------------------------------------------------

    /// @brief  Allocates a VBO and uploads positions, normals, and UV coordinates.
    ///
    /// Layout (non-interleaved, three contiguous blocks in one buffer):
    ///   [pos0..posN][norm0..normN][uv0..uvN]   where N = vertexCount
    ///
    /// @param mesh         Source Assimp mesh.
    /// @param vertexCount  Number of vertices in the mesh.
    /// @return             OpenGL VBO ID.
    static GLuint uploadVBO(const aiMesh* mesh, unsigned int vertexCount) {
        GLuint vbo;
        glGenBuffers(1, &vbo);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);

        // Allocate space for all three blocks at once (8 floats per vertex: 3+3+2)
        glBufferData(GL_ARRAY_BUFFER, 8 * sizeof(float) * vertexCount, nullptr, GL_STATIC_DRAW);

        // Block 1: positions (offset 0) -- aiVector3D[] is directly 3x float, no copy needed
        glBufferSubData(GL_ARRAY_BUFFER,
            0,
            3 * sizeof(float) * vertexCount,
            mesh->mVertices);

        // Block 2: normals (offset after positions)
        glBufferSubData(GL_ARRAY_BUFFER,
            3 * sizeof(float) * vertexCount,
            3 * sizeof(float) * vertexCount,
            mesh->mNormals);

        // Block 3: UV coordinates -- Assimp stores them as aiVector3D (x, y, z ignored),
        // so we manually copy only x and y into a flat float array.
        std::vector<float> uvCoords(2 * vertexCount, 0.0f);
        if (mesh->HasTextureCoords(0)) {
            for (unsigned int i = 0; i < vertexCount; ++i) {
                uvCoords[2 * i] = mesh->mTextureCoords[0][i].x;
                uvCoords[2 * i + 1] = mesh->mTextureCoords[0][i].y;
            }
        }
        glBufferSubData(GL_ARRAY_BUFFER,
            6 * sizeof(float) * vertexCount,
            2 * sizeof(float) * vertexCount,
            uvCoords.data());

        return vbo;
    }

    /// @brief  Allocates an IBO and uploads triangle indices.
    ///
    /// @param mesh  Source Assimp mesh (faces must be triangulated).
    /// @return      OpenGL IBO ID.
    static GLuint uploadIBO(const aiMesh* mesh) {
        // 3 indices per face, guaranteed by aiProcess_Triangulate
        std::vector<unsigned int> indices(mesh->mNumFaces * 3);
        for (unsigned int f = 0; f < mesh->mNumFaces; ++f) {
            indices[f * 3 + 0] = mesh->mFaces[f].mIndices[0];
            indices[f * 3 + 1] = mesh->mFaces[f].mIndices[1];
            indices[f * 3 + 2] = mesh->mFaces[f].mIndices[2];
        }

        GLuint ibo;
        glGenBuffers(1, &ibo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER,
            indices.size() * sizeof(unsigned int),
            indices.data(),
            GL_STATIC_DRAW);

        return ibo;
    }

    /// @brief  Creates a VAO that records how to read the given VBO and IBO.
    ///
    /// After this call, binding the returned VAO is all that is needed before
    /// a draw call -- no manual attribute setup required per frame.
    ///
    /// @param vbo          VBO created by uploadVBO().
    /// @param ibo          IBO created by uploadIBO().
    /// @param vertexCount  Needed to compute byte offsets into the non-interleaved VBO.
    /// @return             OpenGL VAO ID.
    static GLuint setupVAO(GLuint vbo, GLuint ibo, unsigned int vertexCount) {
        GLuint vao;
        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);   // IBO is stored inside the VAO
        glBindBuffer(GL_ARRAY_BUFFER, vbo);

        // Attribute 0: position (3 floats, offset 0)
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

        // Attribute 1: normal (3 floats, starts after all positions)
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0,
            (void*)(3 * sizeof(float) * vertexCount));

        // Attribute 2: UV (2 floats, starts after all positions + normals)
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0,
            (void*)(6 * sizeof(float) * vertexCount));

        glBindVertexArray(0);   // unbind -- prevents accidental modification by later calls

        return vao;
    }

    /// @brief  Extracts diffuse texture and fallback color from an Assimp material.
    ///
    /// @param mat     Assimp material to read from.
    /// @param result  Mesh struct whose texture and diffuseColor fields are filled in.
    static void loadMaterial(const aiMaterial* mat, Mesh& result) {
        // Diffuse texture -- path is relative to the model directory
        aiString texturePath;
        if (mat->GetTexture(aiTextureType_DIFFUSE, 0, &texturePath) == AI_SUCCESS) {
            std::string fullPath = "pirate_ship/";
            fullPath += texturePath.C_Str();
            result.texture = loadTexture(fullPath.c_str());
        }

        // Fallback diffuse color (used when no texture is present, or as a tint)
        aiColor4D diffuseColor(0.7f, 0.6f, 0.5f, 1.0f);
        aiGetMaterialColor(mat, AI_MATKEY_COLOR_DIFFUSE, &diffuseColor);
        result.diffuseColor = glm::vec3(diffuseColor.r, diffuseColor.g, diffuseColor.b);
    }

    Mesh uploadMesh(const aiMesh* mesh, const aiMaterial* mat) {
        Mesh result;
        const unsigned int vertexCount = mesh->mNumVertices;

        result.vbo = uploadVBO(mesh, vertexCount);
        result.ibo = uploadIBO(mesh);
        result.vao = setupVAO(result.vbo, result.ibo, vertexCount);
        result.numTriangles = mesh->mNumFaces;

        loadMaterial(mat, result);

        return result;
    }

    void generateWaterGrid(WaterGrid& grid) {

        const float step = OCEAN_SIZE / (OCEAN_GRID_DENSITY - 1);   // world-space distance between adjacent vertices
        const float half = OCEAN_SIZE / 2.0f;      // offset to center the grid on (0, 0)

        // Vertices -- flat grid in the XZ plane, Y = 0 at rest.
        // The vertex shader animates Y via Gerstner waves each frame.
        std::vector<glm::vec3> vertices;
        vertices.reserve(OCEAN_GRID_DENSITY * OCEAN_GRID_DENSITY);
        for (int row = 0; row < OCEAN_GRID_DENSITY; row++)
            for (int col = 0; col < OCEAN_GRID_DENSITY; col++)
                vertices.push_back({ col * step - half, 0.0f, row * step - half });

        // Indices -- each square patch is split into 2 triangles.
        //
        //   tl --- tr
        //   |  \   |     tl = top-left     = row*OCEAN_GRID_DENSITY + col
        //   |   \  |     tr = top-right    = row*OCEAN_GRID_DENSITY + col + 1
        //   bl --- br    bl = bottom-left  = (row+1)*OCEAN_GRID_DENSITY + col
        //                br = bottom-right = (row+1)*OCEAN_GRID_DENSITY + col + 1
        //
        // Winding: counter-clockwise -- matches OpenGL front-face default.
        std::vector<unsigned int> indices;
        indices.reserve((OCEAN_GRID_DENSITY - 1) * (OCEAN_GRID_DENSITY - 1) * 6);
        for (int row = 0; row < OCEAN_GRID_DENSITY - 1; row++) {
            for (int col = 0; col < OCEAN_GRID_DENSITY - 1; col++) {
                unsigned int topLeft = row * OCEAN_GRID_DENSITY + col;
                unsigned int topRight = topLeft + 1;
                unsigned int bottomLeft = (row + 1) * OCEAN_GRID_DENSITY + col;
                unsigned int bottomRight = bottomLeft + 1;
                indices.insert(indices.end(), { topLeft, bottomLeft, topRight,
                                                bottomLeft, bottomRight, topRight });
            }
        }

        grid.numIndices = (unsigned int)indices.size();

        // VBO: upload vertex positions to GPU
        glGenBuffers(1, &grid.vbo);
        glBindBuffer(GL_ARRAY_BUFFER, grid.vbo);
        glBufferData(GL_ARRAY_BUFFER,
            vertices.size() * sizeof(glm::vec3),
            vertices.data(),
            GL_STATIC_DRAW);

        // IBO: upload triangle indices to GPU
        glGenBuffers(1, &grid.ibo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, grid.ibo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER,
            indices.size() * sizeof(unsigned int),
            indices.data(),
            GL_STATIC_DRAW);

        // VAO: records attribute layout so draw calls need only glBindVertexArray
        glGenVertexArrays(1, &grid.vao);
        glBindVertexArray(grid.vao);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, grid.ibo);   // IBO stored inside VAO
        glBindBuffer(GL_ARRAY_BUFFER, grid.vbo);

        // Attribute 0: position (3 floats).
        // Water grid has positions only -- normals and UV are computed in the shader.
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

        glBindVertexArray(0);
    }

    bool loadShipModel() {
        Assimp::Importer importer;
        const aiScene* scene = importer.ReadFile(MODEL_PATH,
            aiProcess_Triangulate |             // decompose polygons into triangles
            aiProcess_PreTransformVertices |    // flatten node hierarchy into one coordinate system
            aiProcess_GenSmoothNormals |        // generate smooth normals for Phong shading
            aiProcess_JoinIdenticalVertices     // merge duplicate vertices to reduce VBO size
        );

        if (!scene) {
            std::cerr << "Assimp error: " << importer.GetErrorString() << std::endl;
            return false;
        }
        std::cout << "Model loaded: " << scene->mNumMeshes << " meshes." << std::endl;

        // Upload each sub-mesh (hull, sails, mast, ...) to the GPU as its own VAO
        for (unsigned int i = 0; i < scene->mNumMeshes; ++i) {
            g_meshes.push_back(uploadMesh(
                scene->mMeshes[i],
                scene->mMaterials[scene->mMeshes[i]->mMaterialIndex]
            ));
        }

        return true;
    }

    bool initWater() {
        generateWaterGrid(g_water);

        // Path is relative to working directory (project root, next to .sln)
        g_water.texture = loadTexture("water.png");
        if (!g_water.texture)
            std::cerr << "Warning: water texture not found, water will render black." << std::endl;

        return true;
    }

} // namespace galuszde