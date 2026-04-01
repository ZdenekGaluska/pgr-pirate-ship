//----------------------------------------------------------------------------------------
/**
 * \file    main.cpp
 * \author  vaclaon3
 * \brief   Vstupni bod programu. Inicializace GLUT, OpenGL a sceny. Spusteni hlavni smycky.
 *
 * Poradi inicializace je dulezite:
 *   1. glutInit()           — inicializuje windowing system
 *   2. glutCreateWindow()   — vytvori OpenGL kontext
 *   3. pgr::initialize()    — inicializuje GLEW (extensions) — MUSI byt po CreateWindow!
 *   4. init()               — nase inicializace (shadery, modely, ...) — az po pgr::initialize!
 *   5. glutMainLoop()       — spusti smycku eventu (nikdy se nevrati)
 */
//----------------------------------------------------------------------------------------

#include "globals.h"
#include "callbacks.h"
#include "mesh_utils.h"

// ================================================================================
// init() — inicializace sceny
//
// Vola se jednou po pgr::initialize(). Nacte model, shadery, nastavi OpenGL stav.
// Vraci false pokud neco selze — program se pak ukonci s chybou.
// ================================================================================

bool init() {
    std::cerr << "Inicializace sceny..." << std::endl;

    // -----------------------------------------------------------------------
    // Nacist model lode pres Assimp
    // -----------------------------------------------------------------------
    Assimp::Importer imp;
    const aiScene* scn = imp.ReadFile(MODEL_PATH,
        aiProcess_Triangulate          |   // rozloz polygony na trojuhelniky
        aiProcess_PreTransformVertices |   // sjednotit hierarchii nodu do jednoho souradnicoveho systemu
        aiProcess_GenSmoothNormals     |   // vygenerovat hladke normaly (pro Phong osvelteni)
        aiProcess_JoinIdenticalVertices    // spojit duplicitni vrcholy (mensi VBO)
    );

    if (!scn) {
        std::cerr << "Assimp error: " << imp.GetErrorString() << std::endl;
        return false;
    }
    std::cout << "Model nacten: " << scn->mNumMeshes << " meshu." << std::endl;

    // Nahrat kazdy sub-mesh modelu na GPU
    for (unsigned i = 0; i < scn->mNumMeshes; ++i) {
        g_meshes.push_back(uploadMesh(
            scn->mMeshes[i],
            scn->mMaterials[scn->mMeshes[i]->mMaterialIndex]
        ));
    }

    // -----------------------------------------------------------------------
    // Vygenerovat mrizku vody
    // -----------------------------------------------------------------------
    generateWaterGrid(g_water);
    
    // Nacist texturu vody ze souboru do GPU
    // Cesta je relativni k Working Directory (koren projektu vedle .sln)
    g_water.texture = loadTexture("water.png");
    if (!g_water.texture)
        std::cerr << "Varovani: textura vody nenalezena, voda bude cerna." << std::endl;


    // -----------------------------------------------------------------------
    // Nacist a slinkovat shadery
    //
    // pgr::createShaderFromFile() zkompiluje GLSL shader ze souboru.
    // pgr::createProgram()        slinkuje vertex + fragment shader do programu.
    // Cesty jsou relativni k Working Directory (nastavit ve VS: Project -> Properties
    // -> Debugging -> Working Directory = $(ProjectDir)).
    // -----------------------------------------------------------------------
    GLuint shaders[] = {
        pgr::createShaderFromFile(GL_VERTEX_SHADER,   "simple-vs.glsl"),
        pgr::createShaderFromFile(GL_FRAGMENT_SHADER, "simple-fs.glsl"),
        0   // sentinel — pgr::createProgram() cte pole az po tento nulovy prvek
    };
    shdr.program = pgr::createProgram(shaders);
    if (!shdr.program) {
        std::cerr << "Chyba pri kompilaci shaderu." << std::endl;
        return false;
    }

    // -----------------------------------------------------------------------
    // Ulozit lokace uniform promennych shaderu
    //
    // glGetUniformLocation() je relativne pomale — volame ho jednou tady
    // a vysledky ulozime do shdr.*. Pri kazdem draw callu pak pouzijeme
    // ulozene hodnoty (viz callbacks.cpp / onDisplay).
    // -----------------------------------------------------------------------
    glUseProgram(shdr.program);

    // Texture unit 0 je vychozi — reckneme shaderu aby cetl z jednotky 0
    GLint uTexture = glGetUniformLocation(shdr.program, "uTexture");
    glUniform1i(uTexture, 0);

    shdr.mPVM       = glGetUniformLocation(shdr.program, "mPVM");
    shdr.mModel     = glGetUniformLocation(shdr.program, "mModel");
    shdr.vDiffuse   = glGetUniformLocation(shdr.program, "vDiffuse");
    shdr.vLightDir  = glGetUniformLocation(shdr.program, "vLightDir");
    shdr.vCameraPos = glGetUniformLocation(shdr.program, "vCameraPos");
    shdr.fWaterUVScale = glGetUniformLocation(shdr.program, "uWaterUVScale");

    glUniform1f(shdr.fWaterUVScale, 0.0f);
    glUseProgram(0);

    // -----------------------------------------------------------------------
    // Zakladni OpenGL nastaveni
    // -----------------------------------------------------------------------
    glClearColor(0.1f, 0.18f, 0.28f, 1.0f);   // barva pozadi (tmava modra = ocean)
    glEnable(GL_DEPTH_TEST);                    // Z-buffer: kreslit jen co je vpredu

    std::cerr << "Inicializace OK." << std::endl;
    return true;
}

// ================================================================================
// main() — vstupni bod programu
// ================================================================================

int main(int argc, char* argv[]) {
    // --- Inicializace GLUT (windowing system) ---
    glutInit(&argc, argv);
    glutInitContextVersion(pgr::OGL_VER_MAJOR, pgr::OGL_VER_MINOR);
    glutInitContextFlags(GLUT_FORWARD_COMPATIBLE);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(WIN_WIDTH, WIN_HEIGHT);
    glutCreateWindow(WIN_TITLE);
    glutSetCursor(GLUT_CURSOR_NONE);   // skryt kurzor mysi (FPS styl)

    // --- Registrace callbacku ---
    // MUSI byt pred pgr::initialize(), ale az po glutCreateWindow()
    glutDisplayFunc(onDisplay);
    glutReshapeFunc(onReshape);
    glutKeyboardFunc(onKeyDown);
    glutKeyboardUpFunc(onKeyUp);
    glutPassiveMotionFunc(onMouseMotion);
    glutMouseFunc(onMouse);
    glutTimerFunc(16, onTimer, 0);   // spustit herni smycku (16ms = ~60 FPS)

    // --- Inicializace PGR frameworku (GLEW) ---
    // MUSI byt az po glutCreateWindow() — GLEW potrebuje existujici OpenGL kontext!
    if (!pgr::initialize(pgr::OGL_VER_MAJOR, pgr::OGL_VER_MINOR))
        pgr::dieWithError("pgr::initialize() selhalo — nepodporovana verze OpenGL?");

    // --- Inicializace nasi sceny ---
    if (!init())
        pgr::dieWithError("init() selhalo — viz chybova hlaseni vyse.");

    // --- Hlavni smycka GLUT ---
    // Odtud program nikdy nevrati (az do zavreni okna nebo ESC).
    // GLUT vola callbacky registrovane vyse podle eventu.
    glutMainLoop();
    return 0;
}
