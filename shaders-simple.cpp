//----------------------------------------------------------------------------------------
/**
 * \file    shaders-simple.cpp
 * \author  vaclaon3
 * \brief   Piratska lod (GLTF via Assimp) s volnou FPS kamerou
 *
 * STRUKTURA SOUBORU:
 *   1. Konstanty a datove typy
 *   2. Globalni stav (kamera, klaves, mese)
 *   3. Pomocne funkce (kamera)
 *   4. GPU upload (Assimp mesh -> VAO/VBO/IBO)
 *   5. Inicializace (model + shadery + OpenGL nastaveni)
 *   6. GLUT Callbacky (display, reshape, klavesy, mys, timer)
 *   7. main()
 */
 //----------------------------------------------------------------------------------------

#include <iostream>
#include <vector>
#include "pgr.h"   // zabaluje: GLEW + freeglut + GLM + Assimp + pgr utility

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

// ================================================================================
// 1. KONSTANTY
// ================================================================================

const int   WIN_WIDTH = 1280;
const int   WIN_HEIGHT = 720;
const char* WIN_TITLE = "Piratska lod - vaclaon3";
const char* MODEL_PATH = "pirate_ship/scene.gltf";  // relativne k Working Directory

const float CAM_SPEED = 0.15f;   // rychlost pohybu kamery (jednotky/frame)
const float CAM_SENS = 0.1f;    // citlivost mysi (stupne/pixel)
const float ZOOM_SPEED = 0.8f;    // rychlost zoomu pri scrollovani

// ================================================================================
// 2. DATOVE TYPY
// ================================================================================

/*
 * Model lode se sklada z vice meshu (trup, stezne, vlajka, ...),
 * kazdy ma vlastni VAO/VBO/IBO a materialovou barvu.
 *
 * VAO (Vertex Array Object)  = zapamatuje si layout atributu (kde jsou pozice, normaly, ...)
 * VBO (Vertex Buffer Object) = buffer s daty vrcholu (pozice, normaly, UV) na GPU
 * IBO (Index Buffer Object)  = buffer s indexy trojuhelniku na GPU
 */
struct Mesh {
    GLuint vao = 0;
    GLuint vbo = 0;
    GLuint ibo = 0;
    unsigned int numTriangles = 0;
    glm::vec3 diffuseColor = glm::vec3(0.8f);
    GLuint texture = 0;   // ID textury, 0 = žádná textura
};

struct WaterGrid {
    GLuint vao = 0;
    GLuint vbo = 0;
    GLuint ibo = 0;
    unsigned int numIndices = 0;
};

/**
 * ShaderLocations = ulozene lokace uniform promennych shaderu.
 * glGetUniformLocation() je pomale volani — zavolame ho jednou pri init()
 * a vysledek si ulozime sem. Pri kazdem draw callu pak pouzijeme ulozene hodnoty.
 */
struct {
    GLuint program = 0;
    GLint  mPVM = -1;   // uniform "mPVM"      = projekce * view * model matice
    GLint  mModel = -1;   // uniform "mModel"    = model matice (pro transformaci normal)
    GLint  vDiffuse = -1;   // uniform "vDiffuse"  = barva materialu
    GLint  vLightDir = -1; // uniform "vLightDir"  = smer svetla
    GLint  vCameraPos = -1; // uniform "vCameraPos" = pozice kamery (pro specular)
} shdr;

// ================================================================================
// 3. GLOBALNI STAV
//    Tyto promenne ziji po celou dobu behu programu.
//    Modifikuji je callbacky, cte je display callback.
// ================================================================================

std::vector<Mesh> g_meshes;  // vsechny sub-mese nacteneho modelu
WaterGrid g_water;

// --- Kamera (FPS model) ---
glm::vec3 g_camPos = glm::vec3(0.0f, 1.5f, 5.0f);
float     g_camYaw = -90.0f;  // rotace doleva/doprava (kolem osy Y)
float     g_camPitch = 0.0f;  // rotace nahoru/dolu   (kolem osy X)

// --- Stav klaves ---
bool g_keys[256] = {};

// --- Svetlo ---
// Pevny smer slunce (normalizovany unit vektor).
const glm::vec3 LIGHT_DIR = glm::normalize(glm::vec3(1.0f, 2.0f, 1.0f));

// ================================================================================
// 4. POMOCNE FUNKCE
// ================================================================================

/**
 * Prevod sfericke souradnice na kartezske:
 *   x = cos(yaw) * cos(pitch)
 *   y = sin(pitch)
 *   z = sin(yaw) * cos(pitch)
 *
 * Vysledek je normalizovany unit vektor (delka = 1).
 * Pouziva se pro:
 *   - lookAt() pri vypoctu view matice
 *   - pohyb kamery WASD (posun ve smeru front/right)
 */
glm::vec3 getCamFront() { // vypocita smerovy vektor kamery z yaw a pitch.
    return glm::normalize(glm::vec3(
        cos(glm::radians(g_camYaw)) * cos(glm::radians(g_camPitch)),
        sin(glm::radians(g_camPitch)),
        sin(glm::radians(g_camYaw)) * cos(glm::radians(g_camPitch))
    ));
}


GLuint loadTexture(const char* path) {
    int w, h, channels;
    stbi_set_flip_vertically_on_load(true);
    unsigned char* data = stbi_load(path, &w, &h, &channels, 0);
    std::cerr << "Nacitam: " << path << " result: " << (data ? "OK" : "FAIL") << std::endl;
    if (!data) {
        std::cerr << "Nelze nacist texturu: " << path << std::endl;
        return 0;
    }

    GLuint texID;
    glGenTextures(1, &texID);
    glBindTexture(GL_TEXTURE_2D, texID);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0,
        (channels == 4 ? GL_RGBA : GL_RGB), GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    stbi_image_free(data);

    return texID;
}


void generateWaterGrid(WaterGrid& grid) {
    const int   N = 64;
    const float SIZE = 100.0f;
    const float step = SIZE / (N - 1);  // vzdálenost mezi vrcholy
    const float half = SIZE / 2.0f;     // aby byl grid centrovaný na (0,0)

    // --- Vrcholy ---
    // Jednoduchá dvojitá smyèka, i = øádek (Z), j = sloupec (X)
    std::vector<glm::vec3> verts;
    for (int i = 0; i < N; i++)
        for (int j = 0; j < N; j++)
            verts.push_back({ j * step - half, 0.0f, i * step - half });

    // --- Indexy ---
    // Grid je N×N vrcholù ale (N-1)×(N-1) ètvercù.
    // Každý ètverec = 2 trojúhelníky = 6 indexù.
    // tl/tr/bl/br = top-left/top-right/bottom-left/bottom-right rohy ètverce.
    std::vector<unsigned int> idx;
    for (int i = 0; i < N - 1; i++)
        for (int j = 0; j < N - 1; j++) {
            unsigned int tl = i * N + j, tr = tl + 1, bl = (i + 1) * N + j, br = bl + 1;
            idx.insert(idx.end(), { tl, bl, tr, bl, br, tr });
        }

    grid.numIndices = idx.size();

    // --- VBO: nahraje vrcholy na GPU ---
    glGenBuffers(1, &grid.vbo);
    glBindBuffer(GL_ARRAY_BUFFER, grid.vbo);
    glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(glm::vec3), verts.data(), GL_STATIC_DRAW);

    // --- IBO: nahraje indexy na GPU ---
    glGenBuffers(1, &grid.ibo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, grid.ibo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, idx.size() * sizeof(unsigned int), idx.data(), GL_STATIC_DRAW);

    // --- VAO: zapamatuje si "jak èíst VBO" ---
    // Bez VAO bys musel pøed každým draw callem znovu volat glVertexAttribPointer.
    // VAO si toto nastavení uloží — pøi draw callu staèí jen glBindVertexArray.
    glGenVertexArrays(1, &grid.vao);
    glBindVertexArray(grid.vao);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, grid.ibo);  // IBO se ukládá DO VAO
    glBindBuffer(GL_ARRAY_BUFFER, grid.vbo);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);  // location 0 = vec3
    glBindVertexArray(0);
}

// ================================================================================
// 5. GPU UPLOAD — ASSIMP MESH -> VAO/VBO/IBO
// ================================================================================


static Mesh uploadMesh(const aiMesh* mesh, const aiMaterial* mat) {
    Mesh m;
    const unsigned int N = mesh->mNumVertices;

    // VBO
    glGenBuffers(1, &m.vbo);
    glBindBuffer(GL_ARRAY_BUFFER, m.vbo);
    glBufferData(GL_ARRAY_BUFFER, 8 * sizeof(float) * N, nullptr, GL_STATIC_DRAW);

    glBufferSubData(GL_ARRAY_BUFFER, 0,
        3 * sizeof(float) * N, mesh->mVertices);

    glBufferSubData(GL_ARRAY_BUFFER, 3 * sizeof(float) * N,
        3 * sizeof(float) * N, mesh->mNormals);

    std::vector<float> uvs(2 * N, 0.0f);
    if (mesh->HasTextureCoords(0))
        for (unsigned i = 0; i < N; ++i) {
            uvs[2 * i] = mesh->mTextureCoords[0][i].x;
            uvs[2 * i + 1] = mesh->mTextureCoords[0][i].y;
        }
    glBufferSubData(GL_ARRAY_BUFFER, 6 * sizeof(float) * N,
        2 * sizeof(float) * N, uvs.data());

    // IBO
    std::vector<unsigned int> idx(mesh->mNumFaces * 3);
    for (unsigned f = 0; f < mesh->mNumFaces; ++f) {
        idx[f * 3 + 0] = mesh->mFaces[f].mIndices[0];
        idx[f * 3 + 1] = mesh->mFaces[f].mIndices[1];
        idx[f * 3 + 2] = mesh->mFaces[f].mIndices[2];
    }
    glGenBuffers(1, &m.ibo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m.ibo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
        3 * sizeof(unsigned int) * mesh->mNumFaces, idx.data(), GL_STATIC_DRAW);

    // VAO — vše uvnitø
    glGenVertexArrays(1, &m.vao);
    glBindVertexArray(m.vao);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m.ibo);
    glBindBuffer(GL_ARRAY_BUFFER, m.vbo);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, (void*)(3 * sizeof(float) * N));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, (void*)(6 * sizeof(float) * N));
    glBindVertexArray(0);

    m.numTriangles = mesh->mNumFaces;

    aiString texPath;
    if (mat->GetTexture(aiTextureType_DIFFUSE, 0, &texPath) == AI_SUCCESS) {
        std::string fullPath = "pirate_ship/";
        fullPath += texPath.C_Str();
        m.texture = loadTexture(fullPath.c_str());
    }

    aiColor4D c(0.7f, 0.6f, 0.5f, 1.0f);
    aiGetMaterialColor(mat, AI_MATKEY_COLOR_DIFFUSE, &c);
    m.diffuseColor = glm::vec3(c.r, c.g, c.b);

    return m;
}

// ================================================================================
// 6. INICIALIZACE
// ================================================================================

bool init() {
    std::cerr << "START" << std::endl;
    Assimp::Importer imp;
    const aiScene* scn = imp.ReadFile(MODEL_PATH,
        aiProcess_Triangulate |  // rozloz polygony na trojuhelniky
        aiProcess_PreTransformVertices |  // sjednotit hierarchii nodu do jednoho meshe
        aiProcess_GenSmoothNormals |  // vygenerovat normaly
        aiProcess_JoinIdenticalVertices); // spojit duplicitni vrcholy

    if (!scn) {
        std::cerr << "Assimp error: " << imp.GetErrorString() << std::endl;
        return false;
    }
    std::cout << "Nacteno " << scn->mNumMeshes << " meshu." << std::endl;

    generateWaterGrid(g_water);

    for (unsigned i = 0; i < scn->mNumMeshes; ++i)
        g_meshes.push_back(uploadMesh(
            scn->mMeshes[i],
            scn->mMaterials[scn->mMeshes[i]->mMaterialIndex]));

    // --- Shadery ---
    GLuint shaders[] = {
        pgr::createShaderFromFile(GL_VERTEX_SHADER,   "simple-vs.glsl"),
        pgr::createShaderFromFile(GL_FRAGMENT_SHADER, "simple-fs.glsl"),
        0
    };
    shdr.program = pgr::createProgram(shaders);
    if (!shdr.program) return false;

    GLint uTexture = glGetUniformLocation(shdr.program, "uTexture");
    glUseProgram(shdr.program);
    glUniform1i(uTexture, 0);  // texture unit 0

    shdr.mPVM = glGetUniformLocation(shdr.program, "mPVM");
    shdr.mModel = glGetUniformLocation(shdr.program, "mModel");
    shdr.vDiffuse = glGetUniformLocation(shdr.program, "vDiffuse");
    shdr.vLightDir = glGetUniformLocation(shdr.program, "vLightDir");
    shdr.vCameraPos = glGetUniformLocation(shdr.program, "vCameraPos");

    glClearColor(0.1f, 0.18f, 0.28f, 1.0f);
    glEnable(GL_DEPTH_TEST);
    return true;
}

// ================================================================================
// 7. GLUT CALLBACKY
// ================================================================================

// --------------------------------------------------------------------------------
// onDisplay() — vola GLUT kdyz je potreba prekreslit okno
// --------------------------------------------------------------------------------
void onDisplay() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glUseProgram(shdr.program);

    // View matice: transformuje world space -> camera space
    //   lookAt(pozice kamery, cilovy bod, up vektor)
    glm::mat4 view = glm::lookAt(g_camPos, g_camPos + getCamFront(), glm::vec3(0, 1, 0));

    // Projekcni matice: perspektiva
    //   perspective(FOV, aspect ratio, near clip, far clip)
    glm::mat4 proj = glm::perspective(glm::radians(45.0f),
        (float)WIN_WIDTH / WIN_HEIGHT, 0.1f, 200.0f);

    // Model matice: umisteni objektu ve world space (tady jen scale 2x)
    glm::mat4 model = glm::scale(glm::mat4(1.0f), glm::vec3(2.0f));

    // PVM = projekce * view * model (kombinovana matice pro vertex shader)
    glm::mat4 PVM = proj * view * model;

    glUniformMatrix4fv(shdr.mPVM, 1, GL_FALSE, glm::value_ptr(PVM));
    glUniformMatrix4fv(shdr.mModel, 1, GL_FALSE, glm::value_ptr(model));
    glUniform3fv(shdr.vLightDir, 1, glm::value_ptr(LIGHT_DIR));
    glUniform3fv(shdr.vCameraPos, 1, glm::value_ptr(g_camPos));

    // Vykreslit vsechny sub-mese
    for (const auto& m : g_meshes) {
        glUniform3fv(shdr.vDiffuse, 1, glm::value_ptr(m.diffuseColor));
        glBindVertexArray(m.vao);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, m.texture);
        glDrawElements(GL_TRIANGLES, m.numTriangles * 3, GL_UNSIGNED_INT, nullptr);
    }

    // Vykresli grid vody
    glBindVertexArray(g_water.vao);
    glDrawElements(GL_TRIANGLES, g_water.numIndices, GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);

    glBindVertexArray(0);
    glUseProgram(0);
    glutSwapBuffers();  // double buffering: prohod front/back buffer
}

// --------------------------------------------------------------------------------
// onReshape() — vola GLUT pri zmene velikosti okna
// --------------------------------------------------------------------------------
void onReshape(int w, int h) {
    glViewport(0, 0, w, h);
}

// --------------------------------------------------------------------------------
// onKeyDown() / onKeyUp() — stisk/uvolneni klavesy
//
// Misto priameho pohybu kamery jen nastavime priznak g_keys[].
// Pohyb resi onTimer() — tak je pohyb plynuly a nezavisi na
// rychlosti opakovani klavesy v OS.
// --------------------------------------------------------------------------------
void onKeyDown(unsigned char key, int, int) {
    if (key == 27) glutLeaveMainLoop();  // ESC = ukoncit
    g_keys[key] = true;
}

void onKeyUp(unsigned char key, int, int) {
    g_keys[key] = false;
}

// --------------------------------------------------------------------------------
// onMouseMotion() — pohyb mysi bez stisku tlacitka
//
// Jak funguje rotace:
//   dx = posun mysi v x od posledniho snimku
//   dy = posun mysi v y od posledniho snimku
//   yaw   += dx * citlivost    (doprava = vetsi yaw)
//   pitch -= dy * citlivost    (dolu = mensi pitch, proto minus)
//   pitch je omezeno na <-89, 89> aby kamera nepresla pres zenith
// --------------------------------------------------------------------------------
void onMouseMotion(int x, int y) {
    int cx = WIN_WIDTH / 2, cy = WIN_HEIGHT / 2;

    // ignoruj event ktery vznikl samotnym glutWarpPointer (jinak infinite loop)
    if (x == cx && y == cy) return;

    g_camYaw += CAM_SENS * (x - cx);
    g_camPitch -= CAM_SENS * (y - cy);
    g_camPitch = glm::clamp(g_camPitch, -89.0f, 89.0f);

    glutWarpPointer(cx, cy);  // vrat mys na stred
    glutPostRedisplay();
}

// --------------------------------------------------------------------------------
// onMouse() — klik nebo scroll koleckem
//
// GLUT koduje scroll jako button 3 (up) a 4 (down).
// --------------------------------------------------------------------------------
void onMouse(int button, int state, int x, int y) {
    if (state != GLUT_DOWN) return;
    glm::vec3 front = getCamFront();
    if (button == 3) g_camPos += front * ZOOM_SPEED;
    else if (button == 4) g_camPos -= front * ZOOM_SPEED;
    glutPostRedisplay();
}

// --------------------------------------------------------------------------------
// onTimer() — herní smycka, vola se kazdych 16ms (~60 FPS)
//
// Pohyb kamery:
//   front = smer pohledu kamery
//   right = krizovy soucin front x up = vektor doprava
//   Pohyb = zmena pozice o (rychlost * smer)
//
// POZOR: glutTimerFunc() je jednorázové — musíme ho vzdy znovu zaregistrovat!
// --------------------------------------------------------------------------------
void onTimer(int) {
    glm::vec3 front = getCamFront();
    glm::vec3 right = glm::normalize(glm::cross(front, glm::vec3(0, 1, 0)));

    if (g_keys['w']) g_camPos += CAM_SPEED * front;   // dopredu
    if (g_keys['s']) g_camPos -= CAM_SPEED * front;   // dozadu
    if (g_keys['a']) g_camPos -= CAM_SPEED * right;   // doleva
    if (g_keys['d']) g_camPos += CAM_SPEED * right;   // doprava
    if (g_keys[' ']) g_camPos.y += CAM_SPEED;         // nahoru (Space)
    if (g_keys['e']) g_camPos.y -= CAM_SPEED;         // dolu (E)

    glutTimerFunc(16, onTimer, 0);  // znovu zaregistrovat na 16ms
    glutPostRedisplay();
}

// ================================================================================
// 8. MAIN
//    Inicializuje GLUT, vytvori okno, zaregistruje callbacky, spusti smycku.
//    POZOR: pgr::initialize() MUSI byt az PO glutCreateWindow()!
// ================================================================================
int main(int argc, char* argv[]) {
    glutInit(&argc, argv);
    glutInitContextVersion(pgr::OGL_VER_MAJOR, pgr::OGL_VER_MINOR);
    glutInitContextFlags(GLUT_FORWARD_COMPATIBLE);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(WIN_WIDTH, WIN_HEIGHT);
    glutCreateWindow(WIN_TITLE);
    glutSetCursor(GLUT_CURSOR_NONE);

    glutDisplayFunc(onDisplay);
    glutReshapeFunc(onReshape);
    glutKeyboardFunc(onKeyDown);
    glutKeyboardUpFunc(onKeyUp);
    glutPassiveMotionFunc(onMouseMotion);
    glutMouseFunc(onMouse);
    glutTimerFunc(16, onTimer, 0);

    if (!pgr::initialize(pgr::OGL_VER_MAJOR, pgr::OGL_VER_MINOR))
        pgr::dieWithError("pgr init failed");

    if (!init())
        pgr::dieWithError("Inicializace selhala");

    glutMainLoop();
    return 0;
}