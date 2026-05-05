//----------------------------------------------------------------------------------------
/**
 * \file    chest.cpp
 * \author  galuszde
 * \note    Code comments generated with AI assistance (Claude, Anthropic).
 * \brief   Treasure chest initialisation and random placement.
 *
 * IMPORTANT -- model conversion note:
 *   The treasure chest is provided as a GLTF file (treasure/scene.gltf).
 *   The OBJ loader in this project reads the Wavefront OBJ format, not GLTF.
 *   Export the model to OBJ first, e.g. in Blender:
 *     File -> Export -> Wavefront (.obj) -> uncheck "Write Materials", check "Triangulate"
 *   Save as:  treasure/chest.obj
 *
 * Random placement:
 *   Chests are placed in a circle of radius g_chestConfig.area around the origin.
 *   Positions are deterministic for the same seed, reproducible across reloads.
 *   Y = 0 (sea level); chests appear partially submerged, consistent with pirate theme.
 */
 //----------------------------------------------------------------------------------------

#include "chest.h"
#include "obj_loader.h"
#include "mesh_utils.h"   // for loadTexture()
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <cmath>
#include <ctime>

namespace galuszde {

    // -----------------------------------------------------------------------
    // loadChestConfig()
    // -----------------------------------------------------------------------

    /// @brief  Parses config.txt and fills g_chestConfig.
    ///
    ///         File format: one "key=value" pair per line.
    ///         Lines starting with // are treated as comments and skipped.
    ///         Unknown keys are silently ignored so unrelated config entries
    ///         can coexist in the same file without errors.
    ///
    ///         Recognised keys:
    ///           chest_count   -- integer, number of chests to spawn
    ///           chest_area    -- float,   spawn circle radius in world units
    ///           seed          -- integer, RNG seed for reproducible layouts
    static void loadChestConfig() {
        std::ifstream file("config.txt");
        if (!file.is_open()) {
            std::cerr << "config.txt not found -- using defaults." << std::endl;
            return;
        }

        std::string line;
        while (std::getline(file, line)) {
            // Skip blank lines and comment lines starting with //
            if (line.empty() || line.substr(0, 2) == "//") continue;

            // Split on '='
            auto sep = line.find('=');
            if (sep == std::string::npos) continue;   // no '=' on this line

            std::string key = line.substr(0, sep);
            std::string value = line.substr(sep + 1);

            // Trim leading/trailing whitespace from key and value
            auto trim = [](std::string& s) {
                while (!s.empty() && (s.front() == ' ' || s.front() == '\t')) s.erase(s.begin());
                while (!s.empty() && (s.back() == ' ' || s.back() == '\t' ||
                    s.back() == '\r')) s.pop_back();
                };
            trim(key);
            trim(value);

            if (key == "chest_count") g_chestConfig.count = std::stoi(value);
            else if (key == "chest_area")  g_chestConfig.area = std::stof(value);
            else if (key == "seed")        g_chestConfig.seed = std::stoi(value);
            else if (key == "chest_pickup_radius") g_chestConfig.pickupRadius = std::stof(value);
            else if (key == "chest_scale")         g_chestConfig.scale = std::stof(value);
        }

        std::cout << "Config loaded: chests=" << g_chestConfig.count
            << "  area=" << g_chestConfig.area
            << "  seed=" << g_chestConfig.seed << std::endl;
    }

    // -----------------------------------------------------------------------
    // generateChestPositions()
    // -----------------------------------------------------------------------

    /// @brief  Fills g_chests with random chest instance positions.
    ///
    ///         Uses a uniform-disk distribution: random angle + sqrt(random) radius
    ///         so density is uniform across the disk area (not concentrated at center).
    ///         The seed from g_chestConfig ensures the same layout on every run with
    ///         the same config values; pressing R with an edited seed gives a new layout.
    ///
    ///         Y = 0 (sea level) -- chests sit at the water surface at t=0.
    static void generateChestPositions() {
        g_chests.clear();

        // Seed the standard library RNG.  Using srand() is intentional here:
        // the RNG does not need to be cryptographically strong, just reproducible.
        srand(static_cast<unsigned int>(g_chestConfig.seed));

        const float PI = 3.14159265f;

        for (int i = 0; i < g_chestConfig.count; ++i) {
            // Uniform disk: angle in [0, 2*pi], radius scaled by sqrt to avoid
            // concentration near the center (a naive rand % area clusters there).
            float angle = (static_cast<float>(rand()) / RAND_MAX) * 2.0f * PI;
            float rFrac = static_cast<float>(rand()) / RAND_MAX;
            float radius = g_chestConfig.area * std::sqrt(rFrac);   // sqrt = uniform area density

            ChestInstance inst;
            inst.pos.x = std::cos(angle) * radius;
            inst.pos.y = 0.0f;   // sea level; partially submerged look is intentional
            inst.pos.z = std::sin(angle) * radius;
            inst.collected = false;

            g_chests.push_back(inst);
        }

        std::cout << "Generated " << g_chests.size() << " chest positions." << std::endl;
    }

    // -----------------------------------------------------------------------
    // uploadChestGeometry()
    // -----------------------------------------------------------------------

    /// @brief  Uploads ObjData to the GPU as VAO + VBO + IBO.
    ///
    ///         VBO layout (interleaved, stride 44 bytes = 11 floats):
    ///           pos(3) | normal(3) | uv(2) | tangent(3)
    ///
    ///         Attribute locations must match chest-vs.glsl:
    ///           0 = position, 1 = normal, 2 = uv, 3 = tangent
    ///
    /// @param  data  Parsed OBJ data from loadOBJ().
    /// @return true if data is non-empty and upload succeeded.
    static bool uploadChestGeometry(const ObjData& data) {
        if (data.vertices.empty() || data.indices.empty()) {
            std::cerr << "uploadChestGeometry: empty OBJ data." << std::endl;
            return false;
        }

        const int STRIDE = 11 * sizeof(float);   // 44 bytes per vertex

        // VBO: upload full interleaved vertex buffer
        glGenBuffers(1, &g_chestGeom.vbo);
        glBindBuffer(GL_ARRAY_BUFFER, g_chestGeom.vbo);
        glBufferData(GL_ARRAY_BUFFER,
            data.vertices.size() * sizeof(float),
            data.vertices.data(),
            GL_STATIC_DRAW);

        // IBO: upload triangle index buffer
        glGenBuffers(1, &g_chestGeom.ibo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_chestGeom.ibo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER,
            data.indices.size() * sizeof(unsigned int),
            data.indices.data(),
            GL_STATIC_DRAW);

        // VAO: record attribute layout so draw calls only need glBindVertexArray
        glGenVertexArrays(1, &g_chestGeom.vao);
        glBindVertexArray(g_chestGeom.vao);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_chestGeom.ibo);   // IBO stored inside VAO
        glBindBuffer(GL_ARRAY_BUFFER, g_chestGeom.vbo);

        // Attribute 0: position (3 floats, byte offset 0)
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, STRIDE, (void*)0);

        // Attribute 1: normal (3 floats, byte offset 12)
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, STRIDE, (void*)(3 * sizeof(float)));

        // Attribute 2: UV (2 floats, byte offset 24)
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, STRIDE, (void*)(6 * sizeof(float)));

        // Attribute 3: tangent (3 floats, byte offset 32)
        glEnableVertexAttribArray(3);
        glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, STRIDE, (void*)(8 * sizeof(float)));

        glBindVertexArray(0);

        g_chestGeom.numIndices = static_cast<unsigned int>(data.indices.size());
        return true;
    }

    // -----------------------------------------------------------------------
    // Public API
    // -----------------------------------------------------------------------

    bool initChests() {
        // 1. Load config to get count, area, seed
        loadChestConfig();

        // 2. Load OBJ geometry using the custom loader (no Assimp -- rubric 2.3)
        ObjData obj = loadOBJ("treasure/chest.obj");
        if (obj.vertices.empty()) {
            std::cerr << "initChests: failed to load treasure/chest.obj" << std::endl;
            return false;
        }

        // 3. Upload geometry to GPU
        if (!uploadChestGeometry(obj)) return false;

        // 4. Load textures (loadTexture from mesh_utils uses pgr::createTexture / DevIL)
        g_chestGeom.texDiffuse = loadTexture("treasure/textures/material_baseColor.png");
        g_chestGeom.texNormal = loadTexture("treasure/textures/material_normal.png");

        if (!g_chestGeom.texDiffuse)
            std::cerr << "Warning: chest diffuse texture missing -- rendering dark." << std::endl;
        if (!g_chestGeom.texNormal)
            std::cerr << "Warning: chest normal map missing -- shading will be flat." << std::endl;

        // 5. Generate initial chest positions
        generateChestPositions();

        std::cout << "Chests initialised: " << g_chests.size()
            << " instances, " << obj.numTriangles << " triangles each." << std::endl;
        return true;
    }

    void reloadChests() {
        loadChestConfig();

        // Apply multipliers on top of the freshly loaded base values
        g_chestConfig.count *= 2;
        g_chestConfig.area *= 1.2f;
        g_chestConfig.pickupRadius *= 2.0f;
        g_chestConfig.scale *= 0.9f;
        // New time-based seed so the layout changes every reload
        g_chestConfig.seed = static_cast<int>(time(nullptr));

        generateChestPositions();
        std::cout << "Config reloaded -- count=" << g_chestConfig.count
            << "  area=" << g_chestConfig.area
            << "  radius=" << g_chestConfig.pickupRadius
            << "  scale=" << g_chestConfig.scale << std::endl;
    }

    void resetChestsAction() {
        // New seed from wall-clock time -- every F1 press produces a different layout
        g_chestConfig.seed = static_cast<int>(time(nullptr));
        // Double the count -- each F1 press adds another wave of chests to collect
        g_chestConfig.count *= 2;
        generateChestPositions();
        std::cout << "F1: chests reset -- count=" << g_chestConfig.count
            << "  seed=" << g_chestConfig.seed << std::endl;
    }

} // namespace galuszde