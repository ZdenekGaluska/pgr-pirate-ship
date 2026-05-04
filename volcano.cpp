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

    static const int   SEG = 12;           // radial segments (more = rounder silhouette)
    static const float PI = 3.14159265f;

    /// Ring definition: radius from center axis and height on Y.
    struct Ring { float r, y; };

    static const Ring RINGS[] = {
        { 9.0f,  0.0f },   // Ring 0 -- base (beach ring)
        { 6.0f,  5.0f },   // Ring 1 -- lower slope
        { 3.5f,  9.5f },   // Ring 2 -- upper slope
        { 2.0f, 12.0f },   // Ring 3 -- crater rim
    };
    static const int NUM_RINGS = 4;

    /// Per-band outward normal components (radial factor and vertical factor).
    struct BandNorm { float radial, ny; };

    // -----------------------------------------------------------------------
    // buildGeometry() helpers -- static
    // -----------------------------------------------------------------------

    /// @brief  Computes the outward surface normal direction for each band.
    ///
    /// Surface tangent along the slope = (dr, dy).
    /// Outward normal = rotate 90 degrees = (dy, -dr), then normalize.
    /// radial > 0 and ny > 0 for an outward-and-upward normal on a shrinking cone.
    ///
    /// @param out  Output array of size NUM_RINGS-1, one entry per band.
    static void computeBandNormals(BandNorm out[NUM_RINGS - 1]) {
        for (int b = 0; b < NUM_RINGS - 1; b++) {
            float dr = RINGS[b + 1].r - RINGS[b].r;   // negative (cone shrinks upward)
            float dy = RINGS[b + 1].y - RINGS[b].y;   // positive (going up)
            float len = std::sqrt(dy * dy + dr * dr);
            out[b] = { dy / len, -dr / len };
        }
    }

    /// @brief  Appends one interleaved vertex per ring-row position to verts.
    ///
    /// Each ring emits SEG+1 vertices (last = first in position, different U for clean seam).
    /// Smooth normals are computed by averaging adjacent band normals at shared ring edges.
    ///
    /// @param verts     Output interleaved float array (8 floats per vertex appended).
    /// @param bandNorm  Precomputed per-band normals from computeBandNormals().
    static void emitRingVertices(std::vector<float>& verts, const BandNorm bandNorm[NUM_RINGS - 1]) {
        for (int r = 0; r < NUM_RINGS; r++) {
            for (int i = 0; i <= SEG; i++) {
                float angle = 2.0f * PI * i / SEG;
                float c = std::cos(angle);
                float s = std::sin(angle);

                float x = RINGS[r].r * c;
                float y = RINGS[r].y;
                float z = RINGS[r].r * s;

                // Average adjacent band normals for smooth shading at shared ring edges
                float radSum = 0.0f, nySum = 0.0f;
                int   count = 0;
                if (r > 0) { radSum += bandNorm[r - 1].radial; nySum += bandNorm[r - 1].ny; count++; }
                if (r < NUM_RINGS - 1) { radSum += bandNorm[r].radial;     nySum += bandNorm[r].ny;     count++; }
                radSum /= count; nySum /= count;

                float nx = radSum * c;
                float ny = nySum;
                float nz = radSum * s;
                float nlen = std::sqrt(nx * nx + ny * ny + nz * nz);
                nx /= nlen; ny /= nlen; nz /= nlen;

                float u = static_cast<float>(i) / SEG;
                float v = static_cast<float>(r) / (NUM_RINGS - 1);

                verts.insert(verts.end(), { x, y, z, nx, ny, nz, u, v });
            }
        }
    }

    /// @brief  Appends bottom disk center and crater disk center vertices to verts.
    /// @param  verts  Output array; two vertices appended at indices NUM_RINGS*(SEG+1) and +1.
    /// @return        Index of the bottom center vertex (crater center = return value + 1).
    static int emitCenterVertices(std::vector<float>& verts) {
        int centerBottomIdx = NUM_RINGS * (SEG + 1);
        // Bottom center: normal faces down (-Y)
        verts.insert(verts.end(), { 0.0f, RINGS[0].y,             0.0f,  0.0f, -1.0f, 0.0f,  0.5f, 0.5f });
        // Crater center: normal faces up (+Y)
        verts.insert(verts.end(), { 0.0f, RINGS[NUM_RINGS - 1].y, 0.0f,  0.0f,  1.0f, 0.0f,  0.5f, 0.5f });
        return centerBottomIdx;
    }

    /// @brief  Appends side band triangle indices to idx.
    ///
    /// Each band between adjacent rings is tessellated as quads split into 2 CCW triangles.
    ///
    /// @param idx  Output index array; 3 * 2 * SEG * (NUM_RINGS-1) indices appended.
    static void emitSideIndices(std::vector<unsigned int>& idx) {
        for (int r = 0; r < NUM_RINGS - 1; r++) {
            int rowA = r * (SEG + 1);   // current ring row start
            int rowB = (r + 1) * (SEG + 1);   // next ring row start
            for (int i = 0; i < SEG; i++) {
                unsigned int tl = rowA + i;       // current ring, current segment
                unsigned int tr = rowA + i + 1;   // current ring, next segment
                unsigned int bl = rowB + i;       // next ring,    current segment
                unsigned int br = rowB + i + 1;   // next ring,    next segment
                idx.insert(idx.end(), { tl, bl, tr,    // CCW triangle 1
                                        tr, bl, br });  // CCW triangle 2
            }
        }
    }

    /// @brief  Appends bottom disk and crater disk cap indices to idx.
    ///
    /// @param idx              Output index array.
    /// @param centerBottomIdx  Index of the bottom center vertex (from emitCenterVertices).
    static void emitCapIndices(std::vector<unsigned int>& idx, int centerBottomIdx) {
        int centerTopIdx = centerBottomIdx + 1;

        // Bottom disk cap -- CCW when viewed from below (normal faces -Y)
        int baseRow = 0;
        for (int i = 0; i < SEG; i++)
            idx.insert(idx.end(), { (unsigned)centerBottomIdx, (unsigned)(baseRow + i + 1), (unsigned)(baseRow + i) });

        // Crater disk cap -- CCW when viewed from above (normal faces +Y)
        int rimRow = (NUM_RINGS - 1) * (SEG + 1);
        for (int i = 0; i < SEG; i++)
            idx.insert(idx.end(), { (unsigned)centerTopIdx, (unsigned)(rimRow + i), (unsigned)(rimRow + i + 1) });
    }

    /// @brief  Builds complete interleaved vertex and index arrays for the volcano.
    ///
    /// @param verts  Output interleaved float array: [ x y z | nx ny nz | u v ] per vertex.
    /// @param idx    Output index array: 3 indices per triangle, CCW winding.
    static void buildGeometry(std::vector<float>& verts, std::vector<unsigned int>& idx) {
        BandNorm bandNorm[NUM_RINGS - 1];
        computeBandNormals(bandNorm);
        emitRingVertices(verts, bandNorm);
        int centerBottomIdx = emitCenterVertices(verts);
        emitSideIndices(idx);
        emitCapIndices(idx, centerBottomIdx);
    }

    // -----------------------------------------------------------------------
    // initVolcano() helpers -- static
    // -----------------------------------------------------------------------

    /// @brief  Uploads vertex and index data to the GPU as VBO and IBO.
    ///
    /// @param verts    Interleaved float array produced by buildGeometry().
    /// @param indices  Triangle index array produced by buildGeometry().
    static void uploadVolcanoBuffers(const std::vector<float>& verts,
        const std::vector<unsigned int>& indices) {
        // VBO: interleaved vertex data (pos + normal + uv)
        glGenBuffers(1, &g_volcano.vbo);
        glBindBuffer(GL_ARRAY_BUFFER, g_volcano.vbo);
        glBufferData(GL_ARRAY_BUFFER,
            verts.size() * sizeof(float), verts.data(), GL_STATIC_DRAW);

        // IBO: triangle indices
        glGenBuffers(1, &g_volcano.ibo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_volcano.ibo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER,
            indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);
    }

    /// @brief  Creates the VAO and records the interleaved attribute layout.
    ///
    /// Must be called after uploadVolcanoBuffers() so g_volcano.vbo/ibo are valid.
    /// stride = 32 B: pos(12 B) + normal(12 B) + uv(8 B).
    static void setupVolcanoVAO() {
        const int stride = 8 * sizeof(float);   // interleaved: pos(3) + normal(3) + uv(2)

        glGenVertexArrays(1, &g_volcano.vao);
        glBindVertexArray(g_volcano.vao);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_volcano.ibo);   // IBO stored inside VAO
        glBindBuffer(GL_ARRAY_BUFFER, g_volcano.vbo);

        // Attribute 0: position (3 floats, offset 0 B)
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);

        // Attribute 1: normal (3 floats, offset 12 B)
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));

        // Attribute 2: UV (2 floats, offset 24 B)
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (void*)(6 * sizeof(float)));

        glBindVertexArray(0);
    }

    // -----------------------------------------------------------------------
    // initVolcano()
    // -----------------------------------------------------------------------

    bool initVolcano() {
        std::vector<float>        verts;
        std::vector<unsigned int> indices;
        buildGeometry(verts, indices);

        uploadVolcanoBuffers(verts, indices);
        setupVolcanoVAO();

        g_volcano.numTriangles = static_cast<unsigned int>(indices.size() / 3);
        g_volcano.diffuseColor = glm::vec3(0.35f, 0.25f, 0.18f);   // dark volcanic rock brown

        // Texture is optional -- renders with diffuseColor fallback if file is missing
        g_volcano.texture = loadTexture("volcano.png");

        std::cout << "Volcano initialized: " << g_volcano.numTriangles << " triangles." << std::endl;
        return true;
    }

} // namespace galuszde