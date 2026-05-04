//----------------------------------------------------------------------------------------
/**
 * \file    volcano.cpp
 * \author  galuszde
 * \note    Code comments generated with AI assistance (Claude, Anthropic).
 * \brief   Hardcoded volcano mesh -- VBO/VAO setup from a C++ vertex array.
 *
 * The geometry (ring radii, heights, segment count) is fully specified in this file.
 * Nothing is loaded from disk -- this is the "hardcode v src" object.
 *
 * Interleaved VBO layout per vertex: [ x y z | nx ny nz | u v ]  (8 floats, stride 32 B)
 * Triangle count: 3 side bands x 12 segments x 2 + 12 bottom + 12 crater = 96 triangles
 */
 //----------------------------------------------------------------------------------------

#include "volcano.h"
#include "mesh_utils.h"
#include <cmath>
#include <vector>

namespace galuszde {

    // -----------------------------------------------------------------------
    // Geometry parameters -- change these to reshape the volcano
    // -----------------------------------------------------------------------
    static const int   SEG = 12;    // number of radial segments (more = rounder silhouette)
    static const float PI = 3.14159265f;

    // Ring definitions: {radius from center, height Y}
    struct Ring { float r, y; };

    static const Ring RINGS[] = {
        { 12.0f, 0.0f },
        {  8.0f, 7.0f },
        {  5.0f, 13.0f },
        {  3.0f, 16.0f },
    };

    static const int NUM_RINGS = 4;

    // -----------------------------------------------------------------------
    // initVolcano() helpers -- static
    // -----------------------------------------------------------------------

    /// @brief  Fills a flat interleaved float array and an index array with volcano geometry.
    ///
    /// Vertex layout per row: (SEG+1) vertices, indexed row-major.
    ///   row r starts at index r * (SEG+1).
    ///   SEG+1 instead of SEG: first and last vertex share position but differ in U
    ///   coordinate so the texture wraps cleanly without a visible seam.
    ///
    /// Two extra vertices appended after the ring rows:
    ///   centerBottomIdx = NUM_RINGS * (SEG+1)  -- bottom disk center, n=(0,-1,0)
    ///   centerTopIdx    = centerBottomIdx + 1  -- crater disk center, n=(0,1,0)
    ///
    /// @param verts  Output interleaved float array: [ x y z | nx ny nz | u v ] per vertex.
    /// @param idx    Output index array: 3 indices per triangle, CCW winding.
    static void buildGeometry(std::vector<float>& verts, std::vector<unsigned int>& idx) {

        // --- Precompute one outward normal (radial_factor, ny) per band ---
        // Surface tangent along slope = (dr, dy); outward normal = rotate 90° = (dy, -dr)
        struct BandNorm { float radial, ny; };
        BandNorm bandNorm[NUM_RINGS - 1];
        for (int b = 0; b < NUM_RINGS - 1; b++) {
            float dr = RINGS[b + 1].r - RINGS[b].r;   // negative (cone shrinks upward)
            float dy = RINGS[b + 1].y - RINGS[b].y;   // positive (going up)
            float len = std::sqrt(dy * dy + dr * dr);
            bandNorm[b] = { dy / len, -dr / len };      // radial>0, ny>0 for outward+upward
        }

        // --- Emit ring vertices ---
        // For ring r, average adjacent band normals for smooth shading.
        for (int r = 0; r < NUM_RINGS; r++) {
            for (int i = 0; i <= SEG; i++) {
                float angle = 2.0f * PI * i / SEG;
                float c = std::cos(angle);
                float s = std::sin(angle);

                // Position
                float x = RINGS[r].r * c;
                float y = RINGS[r].y;
                float z = RINGS[r].r * s;

                // Smooth normal: average adjacent band normals
                float rad_sum = 0.0f, ny_sum = 0.0f;
                int   count = 0;
                if (r > 0) { rad_sum += bandNorm[r - 1].radial; ny_sum += bandNorm[r - 1].ny; count++; }
                if (r < NUM_RINGS - 1) { rad_sum += bandNorm[r].radial;     ny_sum += bandNorm[r].ny;     count++; }
                rad_sum /= count; ny_sum /= count;

                float nx = rad_sum * c;
                float ny = ny_sum;
                float nz = rad_sum * s;
                float nlen = std::sqrt(nx * nx + ny * ny + nz * nz);
                nx /= nlen; ny /= nlen; nz /= nlen;

                // UV: u wraps around (0..1), v goes from base(0) to rim(1)
                float u = static_cast<float>(i) / SEG;
                float v = static_cast<float>(r) / (NUM_RINGS - 1);

                verts.insert(verts.end(), { x, y, z, nx, ny, nz, u, v });
            }
        }

        // --- Bottom disk center ---
        int centerBottomIdx = NUM_RINGS * (SEG + 1);
        verts.insert(verts.end(), { 0.0f, RINGS[0].y, 0.0f,  0.0f, -1.0f, 0.0f,  0.5f, 0.5f });

        // --- Crater disk center ---
        int centerTopIdx = centerBottomIdx + 1;
        verts.insert(verts.end(), { 0.0f, RINGS[NUM_RINGS - 1].y, 0.0f,  0.0f, 1.0f, 0.0f,  0.5f, 0.5f });

        // --- Side band indices ---
        // Each band connects ring r to ring r+1 with quad pairs.
        for (int r = 0; r < NUM_RINGS - 1; r++) {
            int rowA = r * (SEG + 1);   // lower ring row start
            int rowB = (r + 1) * (SEG + 1);   // upper ring row start
            for (int i = 0; i < SEG; i++) {
                unsigned int tl = rowA + i;       // top-left  (lower ring, current segment)
                unsigned int tr = rowA + i + 1;   // top-right (lower ring, next segment)
                unsigned int bl = rowB + i;       // bot-left  (upper ring, current segment)
                unsigned int br = rowB + i + 1;   // bot-right (upper ring, next segment)
                // Two CCW triangles per quad
                idx.insert(idx.end(), { tl, bl, tr,
                                        tr, bl, br });
            }
        }

        // --- Bottom disk cap (facing down, CCW when viewed from below) ---
        int baseRow = 0;
        for (int i = 0; i < SEG; i++) {
            idx.insert(idx.end(), {
                static_cast<unsigned int>(centerBottomIdx),
                static_cast<unsigned int>(baseRow + i + 1),
                static_cast<unsigned int>(baseRow + i)
                });
        }

        // --- Crater disk cap (facing up, CCW when viewed from above) ---
        int rimRow = (NUM_RINGS - 1) * (SEG + 1);
        for (int i = 0; i < SEG; i++) {
            idx.insert(idx.end(), {
                static_cast<unsigned int>(centerTopIdx),
                static_cast<unsigned int>(rimRow + i),
                static_cast<unsigned int>(rimRow + i + 1)
                });
        }
    }

    // -----------------------------------------------------------------------
    // initVolcano()
    // -----------------------------------------------------------------------

    bool initVolcano() {
        std::vector<float>        verts;
        std::vector<unsigned int> indices;
        buildGeometry(verts, indices);

        const int stride = 8 * sizeof(float);   // interleaved: pos(3)+normal(3)+uv(2)

        // --- VBO: upload interleaved vertex data ---
        glGenBuffers(1, &g_volcano.vbo);
        glBindBuffer(GL_ARRAY_BUFFER, g_volcano.vbo);
        glBufferData(GL_ARRAY_BUFFER,
            verts.size() * sizeof(float),
            verts.data(),
            GL_STATIC_DRAW);

        // --- IBO: upload triangle indices ---
        glGenBuffers(1, &g_volcano.ibo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_volcano.ibo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER,
            indices.size() * sizeof(unsigned int),
            indices.data(),
            GL_STATIC_DRAW);

        // --- VAO: record attribute layout ---
        glGenVertexArrays(1, &g_volcano.vao);
        glBindVertexArray(g_volcano.vao);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_volcano.ibo);   // stored inside VAO
        glBindBuffer(GL_ARRAY_BUFFER, g_volcano.vbo);

        // Attribute 0: position (3 floats, offset 0)
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);

        // Attribute 1: normal (3 floats, offset 12 B)
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));

        // Attribute 2: UV (2 floats, offset 24 B)
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (void*)(6 * sizeof(float)));

        glBindVertexArray(0);

        // --- Mesh metadata ---
        g_volcano.numTriangles = static_cast<unsigned int>(indices.size() / 3);
        g_volcano.diffuseColor = glm::vec3(0.35f, 0.25f, 0.18f);   // dark volcanic rock brown

        // Texture is optional -- renders with diffuseColor fallback if file is missing
        g_volcano.texture = loadTexture("volcano.png");

        std::cout << "Volcano initialized: " << g_volcano.numTriangles << " triangles." << std::endl;
        return true;
    }

} // namespace galuszde