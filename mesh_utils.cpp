//----------------------------------------------------------------------------------------
/**
 * \file    mesh_utils.cpp
 * \author  vaclaon3
 * \brief   Implementace utility funkci pro mese a textury.
 */
//----------------------------------------------------------------------------------------

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "mesh_utils.h"

// ================================================================================
// loadTexture()
// ================================================================================

GLuint loadTexture(const char* path) {
    // stb_image nacte obrazek do RAM jako pole bytu.
    // flip = true, protoze OpenGL ma (0,0) vlevo dole, ale PNG vlevo nahore.
    int w, h, channels;
    stbi_set_flip_vertically_on_load(true);
    unsigned char* data = stbi_load(path, &w, &h, &channels, 0);

    std::cerr << "Nacitam texturu: " << path
              << " -> " << (data ? "OK" : "FAIL") << std::endl;

    if (!data) {
        std::cerr << "  Chyba: " << stbi_failure_reason() << std::endl;
        return 0;
    }

    // Vytvorit OpenGL texturu a nahrat data
    GLuint texID;
    glGenTextures(1, &texID);
    glBindTexture(GL_TEXTURE_2D, texID);

    // Format: RGBA pokud ma obrazek alfa kanal, jinak RGB
    GLenum fmt = (channels == 4) ? GL_RGBA : GL_RGB;
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, fmt, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);   // automaticky vygenerovat mipmaps

    // Nastaveni filtrovani a opakovani
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,     GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,     GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    stbi_image_free(data);   // uvolnit RAM, GPU ma uz svoji kopii
    return texID;
}

// ================================================================================
// uploadMesh()
// ================================================================================

Mesh uploadMesh(const aiMesh* mesh, const aiMaterial* mat) {
    Mesh m;
    const unsigned int N = mesh->mNumVertices;

    // -----------------------------------------------------------------------
    // VBO — nahraje pozice, normaly a UV souradnice na GPU.
    // -----------------------------------------------------------------------
    glGenBuffers(1, &m.vbo);
    glBindBuffer(GL_ARRAY_BUFFER, m.vbo);
    glBufferData(GL_ARRAY_BUFFER, 8 * sizeof(float) * N, nullptr, GL_STATIC_DRAW);

    // Pozice (offset 0)
    glBufferSubData(GL_ARRAY_BUFFER,
        0,
        3 * sizeof(float) * N,
        mesh->mVertices);   // aiVector3D[] je primo 3x float, kompatibilni

    // Normaly (offset za pozicemi)
    glBufferSubData(GL_ARRAY_BUFFER,
        3 * sizeof(float) * N,
        3 * sizeof(float) * N,
        mesh->mNormals);

    // UV souradnice — Assimp je ma jako aiVector3D (x, y, ignorujeme z),
    // musime je zkopirovat rucne do pole 2D vektoru.
    std::vector<float> uvs(2 * N, 0.0f);
    if (mesh->HasTextureCoords(0)) {
        for (unsigned i = 0; i < N; ++i) {
            uvs[2 * i]     = mesh->mTextureCoords[0][i].x;
            uvs[2 * i + 1] = mesh->mTextureCoords[0][i].y;
        }
    }
    glBufferSubData(GL_ARRAY_BUFFER,
        6 * sizeof(float) * N,
        2 * sizeof(float) * N,
        uvs.data());

    // -----------------------------------------------------------------------
    // IBO — indexy trojuhelniku.
    // -----------------------------------------------------------------------
    std::vector<unsigned int> idx(mesh->mNumFaces * 3);
    for (unsigned f = 0; f < mesh->mNumFaces; ++f) {
        idx[f * 3 + 0] = mesh->mFaces[f].mIndices[0];
        idx[f * 3 + 1] = mesh->mFaces[f].mIndices[1];
        idx[f * 3 + 2] = mesh->mFaces[f].mIndices[2];
    }
    glGenBuffers(1, &m.ibo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m.ibo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
        3 * sizeof(unsigned int) * mesh->mNumFaces,
        idx.data(),
        GL_STATIC_DRAW);

    // -----------------------------------------------------------------------
    // VAO — "recept" jak cist VBO.
    // -----------------------------------------------------------------------
    glGenVertexArrays(1, &m.vao);
    glBindVertexArray(m.vao);

    // IBO se ukla DA do VAO — kazde glDrawElements pak ví odkud brat indexy
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m.ibo);
    glBindBuffer(GL_ARRAY_BUFFER, m.vbo);

    // Atribut 0: pozice (3 floaty, offset 0)
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

    // Atribut 1: normala (3 floaty, offset za pozicemi)
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0,
        (void*)(3 * sizeof(float) * N));

    // Atribut 2: UV (2 floaty, offset za pozicemi + normalami)
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0,
        (void*)(6 * sizeof(float) * N));

    glBindVertexArray(0);   // odpojit VAO — bezpecnost, aby nasledujici volani ho nenarusila

    m.numTriangles = mesh->mNumFaces;

    // -----------------------------------------------------------------------
    // Material — diffuse textura a barva.
    // GetTexture() vrati cestu k texturovemu souboru jak je ulozena v GLTF.
    // -----------------------------------------------------------------------
    aiString texPath;
    if (mat->GetTexture(aiTextureType_DIFFUSE, 0, &texPath) == AI_SUCCESS) {
        std::string fullPath = "pirate_ship/";
        fullPath += texPath.C_Str();
        m.texture = loadTexture(fullPath.c_str());
    }

    // Diffuse barva z materialu (pouzije se kdyz neni textura, nebo jako tint)
    aiColor4D c(0.7f, 0.6f, 0.5f, 1.0f);   // vychozi barva pokud material neuvadi jinou
    aiGetMaterialColor(mat, AI_MATKEY_COLOR_DIFFUSE, &c);
    m.diffuseColor = glm::vec3(c.r, c.g, c.b);

    return m;
}

// ================================================================================
// generateWaterGrid()
// ================================================================================

void generateWaterGrid(WaterGrid& grid) {
    // N = pocet vrcholu na jeden bok mrizky.
    // Vysledna mrizka ma N*N vrcholu a (N-1)*(N-1) ctvercu = 2*(N-1)^2 trojuhelniku.
    const int   N    = 64;
    const float SIZE = 100.0f;   // celkova velikost mrizky ve world units
    const float step = SIZE / (N - 1);   // vzdalenost mezi sousednimi vrcholy
    const float half = SIZE / 2.0f;      // posunout mrizku tak aby byla centrovana na (0,0)

    // -----------------------------------------------------------------------
    // Vrcholy — jednoduchá dvojitá smycka, (i = radek Z, j = sloupec X)
    // Y = 0 v klidu; shader ji bude animovat (Gerstner waves).
    // -----------------------------------------------------------------------
    std::vector<glm::vec3> verts;
    verts.reserve(N * N);
    for (int i = 0; i < N; i++)
        for (int j = 0; j < N; j++)
            verts.push_back({ j * step - half, 0.0f, i * step - half });

    // -----------------------------------------------------------------------
    // Indexy — kazdy ctvercovy patch se deli na 2 trojuhelniky.
    //
    //   tl --- tr
    //   |  \   |     tl = top-left    = i*N + j
    //   |   \  |     tr = top-right   = i*N + j + 1
    //   bl --- br    bl = bottom-left = (i+1)*N + j
    //                br = bottom-right= (i+1)*N + j + 1
    //
    // Trojuhelniky: (tl, bl, tr) a (bl, br, tr) — counter-clockwise poradi.
    // -----------------------------------------------------------------------
    std::vector<unsigned int> idx;
    idx.reserve((N - 1) * (N - 1) * 6);
    for (int i = 0; i < N - 1; i++) {
        for (int j = 0; j < N - 1; j++) {
            unsigned int tl = i * N + j;
            unsigned int tr = tl + 1;
            unsigned int bl = (i + 1) * N + j;
            unsigned int br = bl + 1;
            idx.insert(idx.end(), { tl, bl, tr, bl, br, tr });
        }
    }

    grid.numIndices = (unsigned int)idx.size();

    // --- VBO: nahraje vrcholy na GPU ---
    glGenBuffers(1, &grid.vbo);
    glBindBuffer(GL_ARRAY_BUFFER, grid.vbo);
    glBufferData(GL_ARRAY_BUFFER,
        verts.size() * sizeof(glm::vec3),
        verts.data(),
        GL_STATIC_DRAW);

    // --- IBO: nahraje indexy na GPU ---
    glGenBuffers(1, &grid.ibo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, grid.ibo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
        idx.size() * sizeof(unsigned int),
        idx.data(),
        GL_STATIC_DRAW);

    // --- VAO: zapamatuje si jak cist VBO ---
    // Bez VAO bys musel pred kazdym draw callem znovu volat glVertexAttribPointer.
    // VAO si toto nastaveni ulozi — pri draw callu staci jen glBindVertexArray.
    glGenVertexArrays(1, &grid.vao);
    glBindVertexArray(grid.vao);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, grid.ibo);   // IBO se ukla DA do VAO
    glBindBuffer(GL_ARRAY_BUFFER, grid.vbo);

    // Atribut 0: pozice (3 floaty)
    // Water grid ma jen pozice — normaly a UV se spocitaji v shaderu (Gerstner)
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

    glBindVertexArray(0);
}
