//----------------------------------------------------------------------------------------
/**
 * \file    obj_loader.cpp
 * \author  galuszde
 * \note    Code comments generated with AI assistance (Claude, Anthropic).
 * \brief   Custom Wavefront OBJ loader implementation.
 *
 * Two-pass approach:
 *   Pass 1 -- collect all raw v / vt / vn lists from file lines.
 *   Pass 2 -- iterate face lines; for each (v/vt/vn) triple, look up or create
 *             a unique vertex in the final interleaved buffer.
 *
 * After both passes, tangent vectors are computed per-triangle and accumulated
 * per-vertex, then normalised.  This gives correct tangent-space normal mapping
 * without needing a separate preprocessing step.
 */
 //----------------------------------------------------------------------------------------

#include "obj_loader.h"
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <array>
#include <cmath>

namespace galuszde {

    // -----------------------------------------------------------------------
    // Internal types used only during parsing
    // -----------------------------------------------------------------------

    /// @brief  One raw face vertex as read from an OBJ 'f' line.
    ///         Indices are 1-based (OBJ convention); 0 means "not present".
    struct FaceVertex {
        int v = 0;   ///< Index into raw position array (1-based).
        int vt = 0;   ///< Index into raw UV array (1-based), 0 = absent.
        int vn = 0;   ///< Index into raw normal array (1-based), 0 = absent.
    };

    // -----------------------------------------------------------------------
    // parseFaceVertex()
    // -----------------------------------------------------------------------

    /// @brief  Parses one face token such as "12/5/3", "12//3", "12/5", or "12".
    ///
    /// OBJ indices are 1-based and may be negative (relative from end of list).
    /// Negative indices are not resolved here -- the caller resolves them against
    /// the list sizes after all 'v'/'vt'/'vn' lines are read.
    ///
    /// @param  token  Single whitespace-separated token from the 'f' line.
    /// @return Parsed FaceVertex with 1-based indices (0 = missing component).
    static FaceVertex parseFaceVertex(const std::string& token) {
        FaceVertex fv;
        // Split on '/' -- OBJ format: v, v/vt, v//vn, v/vt/vn
        std::istringstream ss(token);
        std::string part;
        int idx = 0;
        while (std::getline(ss, part, '/')) {
            if (!part.empty()) {
                int val = std::stoi(part);
                if (idx == 0) fv.v = val;
                else if (idx == 1) fv.vt = val;
                else if (idx == 2) fv.vn = val;
            }
            ++idx;
        }
        return fv;
    }

    // -----------------------------------------------------------------------
    // resolveIndex()
    // -----------------------------------------------------------------------

    /// @brief  Converts a 1-based (possibly negative) OBJ index to a 0-based C++ index.
    ///
    /// OBJ negative indices count back from the end of the current list,
    /// e.g. -1 refers to the last element added.
    ///
    /// @param  raw   The raw index from the OBJ file (1-based or negative).
    /// @param  size  Current size of the target list.
    /// @return Zero-based index, or -1 if raw == 0 (component absent).
    static int resolveIndex(int raw, int size) {
        if (raw == 0) return -1;
        return (raw > 0) ? (raw - 1) : (size + raw);
    }

    // -----------------------------------------------------------------------
    // computeTangents()
    // -----------------------------------------------------------------------

    /// @brief  Computes tangent vectors for all vertices and writes them into the
    ///         interleaved buffer at the tangent slot (offset 8, stride 11).
    ///
    /// Main idea:
    ///   A tangent vector points along the U axis of the UV space, expressed in
    ///   world space.  For a triangle with positions p0,p1,p2 and UVs uv0,uv1,uv2:
    ///
    ///     edge1 = p1 - p0,   duv1 = uv1 - uv0
    ///     edge2 = p2 - p0,   duv2 = uv2 - uv0
    ///
    ///     r = 1 / (duv1.x * duv2.y - duv2.x * duv1.y)
    ///     tangent = (edge1 * duv2.y - edge2 * duv1.y) * r
    ///
    ///   Each vertex accumulates tangents from all triangles that share it.
    ///   The final tangent is normalised and then re-orthogonalised against the
    ///   normal (Gram-Schmidt) in the vertex shader.
    ///
    /// @param verts    Interleaved float array (11 floats per vertex), modified in place.
    /// @param indices  Triangle index list (3 per triangle).
    static void computeTangents(std::vector<float>& verts,
        const std::vector<unsigned int>& indices)
    {
        const int STRIDE = 11;   // floats per vertex: pos(3)+nrm(3)+uv(2)+tan(3)

        // Accumulator: one vec3 tangent sum per vertex, initially zero.
        std::vector<glm::vec3> tangentAcc(verts.size() / STRIDE, glm::vec3(0.0f));

        const unsigned int numTri = static_cast<unsigned int>(indices.size() / 3);

        for (unsigned int t = 0; t < numTri; ++t) {
            // Retrieve the three vertex indices for this triangle.
            unsigned int i0 = indices[t * 3 + 0];
            unsigned int i1 = indices[t * 3 + 1];
            unsigned int i2 = indices[t * 3 + 2];

            // Read positions from the interleaved buffer (offset 0 in each vertex block).
            glm::vec3 p0(verts[i0 * STRIDE + 0], verts[i0 * STRIDE + 1], verts[i0 * STRIDE + 2]);
            glm::vec3 p1(verts[i1 * STRIDE + 0], verts[i1 * STRIDE + 1], verts[i1 * STRIDE + 2]);
            glm::vec3 p2(verts[i2 * STRIDE + 0], verts[i2 * STRIDE + 1], verts[i2 * STRIDE + 2]);

            // Read UVs from the interleaved buffer (offset 6 in each vertex block).
            glm::vec2 uv0(verts[i0 * STRIDE + 6], verts[i0 * STRIDE + 7]);
            glm::vec2 uv1(verts[i1 * STRIDE + 6], verts[i1 * STRIDE + 7]);
            glm::vec2 uv2(verts[i2 * STRIDE + 6], verts[i2 * STRIDE + 7]);

            // Edges in position space and UV space.
            glm::vec3 edge1 = p1 - p0;
            glm::vec3 edge2 = p2 - p0;
            glm::vec2 duv1 = uv1 - uv0;
            glm::vec2 duv2 = uv2 - uv0;

            // Denominator of the UV-space inverse.
            float det = duv1.x * duv2.y - duv2.x * duv1.y;

            glm::vec3 tangent(1.0f, 0.0f, 0.0f);   // default if UV mapping is degenerate
            if (std::fabs(det) > 1e-6f) {
                float r = 1.0f / det;
                tangent = (edge1 * duv2.y - edge2 * duv1.y) * r;
            }

            // Accumulate this triangle's tangent into all three of its vertices.
            tangentAcc[i0] += tangent;
            tangentAcc[i1] += tangent;
            tangentAcc[i2] += tangent;
        }

        // Normalise accumulated tangents and write back into the interleaved buffer.
        // Offset 8 within each 11-float vertex block is where the tangent lives.
        for (unsigned int i = 0; i < tangentAcc.size(); ++i) {
            glm::vec3 T = tangentAcc[i];
            float len = glm::length(T);
            if (len > 1e-6f) T /= len;
            else              T = glm::vec3(1.0f, 0.0f, 0.0f);   // fallback for isolated verts

            verts[i * STRIDE + 8] = T.x;
            verts[i * STRIDE + 9] = T.y;
            verts[i * STRIDE + 10] = T.z;
        }
    }

    // -----------------------------------------------------------------------
    // loadOBJ()
    // -----------------------------------------------------------------------

    ObjData loadOBJ(const std::string& path) {
        ObjData result;

        std::ifstream file(path);
        if (!file.is_open()) {
            std::cerr << "OBJ loader: cannot open file: " << path << std::endl;
            return result;
        }

        // ---- Pass 1: collect raw v / vt / vn arrays -------------------------

        std::vector<glm::vec3> rawPos;    // all 'v' lines
        std::vector<glm::vec2> rawUV;     // all 'vt' lines
        std::vector<glm::vec3> rawNorm;   // all 'vn' lines

        // Faces are stored temporarily until all raw arrays are complete,
        // because OBJ negative indices need the final array sizes to resolve.
        // Each face is a list of FaceVertex triples (may be 3 or 4 vertices).
        std::vector<std::vector<FaceVertex>> rawFaces;

        std::string line;
        while (std::getline(file, line)) {
            // Skip empty lines and comments
            if (line.empty() || line[0] == '#') continue;

            std::istringstream ss(line);
            std::string keyword;
            ss >> keyword;

            if (keyword == "v") {
                // Vertex position: v x y z
                glm::vec3 p;
                ss >> p.x >> p.y >> p.z;
                rawPos.push_back(p);
            }
            else if (keyword == "vt") {
                // Texture coordinate: vt u v  (third component w is optional, ignored)
                glm::vec2 uv;
                ss >> uv.x >> uv.y;
                rawUV.push_back(uv);
            }
            else if (keyword == "vn") {
                // Vertex normal: vn nx ny nz
                glm::vec3 n;
                ss >> n.x >> n.y >> n.z;
                rawNorm.push_back(n);
            }
            else if (keyword == "f") {
                // Face: f v/vt/vn ... (3 or 4 tokens for triangle / quad)
                std::vector<FaceVertex> face;
                std::string token;
                while (ss >> token)
                    face.push_back(parseFaceVertex(token));
                if (face.size() >= 3)
                    rawFaces.push_back(face);
            }
            // All other keywords (o, g, s, mtllib, usemtl, ...) are ignored.
        }
        file.close();

        if (rawPos.empty()) {
            std::cerr << "OBJ loader: no vertices found in: " << path << std::endl;
            return result;
        }

        // ---- Pass 2: build indexed interleaved buffer -----------------------
        //
        // OBJ can have different (v, vt, vn) combinations that share the same
        // position -- e.g. a seam vertex needs two UV coordinates.
        // We deduplicate by mapping each unique triple to one output vertex.
        //
        // Key format: "posIdx/uvIdx/nrmIdx" (all 0-based after resolution).
        std::unordered_map<std::string, unsigned int> vertexCache;

        const int STRIDE = 11;   // floats per output vertex

        for (const auto& face : rawFaces) {
            // Fan-triangulate: for a quad ABCD -> triangles ABC + ACD.
            // For a triangle the loop runs once.
            unsigned int fanA = 0;   // index in the face vector, not in the output buffer

            // We need to first resolve all face vertices to output indices,
            // then fan-triangulate using the resolved output indices.
            std::vector<unsigned int> resolvedIndices;

            for (const FaceVertex& fv : face) {
                // Resolve 1-based / negative OBJ indices to 0-based indices.
                int pi = resolveIndex(fv.v, static_cast<int>(rawPos.size()));
                int uvi = resolveIndex(fv.vt, static_cast<int>(rawUV.size()));
                int ni = resolveIndex(fv.vn, static_cast<int>(rawNorm.size()));

                // Build a string key for deduplication.
                std::string key = std::to_string(pi) + "/" +
                    std::to_string(uvi) + "/" +
                    std::to_string(ni);

                auto it = vertexCache.find(key);
                if (it != vertexCache.end()) {
                    // Already created this vertex -- reuse its index.
                    resolvedIndices.push_back(it->second);
                }
                else {
                    // New unique vertex -- append to the interleaved buffer.
                    unsigned int newIdx = static_cast<unsigned int>(
                        result.vertices.size() / STRIDE);

                    // Position (defaults to origin if index out of range)
                    glm::vec3 p(0.0f);
                    if (pi >= 0 && pi < static_cast<int>(rawPos.size()))
                        p = rawPos[pi];

                    // Normal (defaults to up if absent or out of range)
                    glm::vec3 n(0.0f, 1.0f, 0.0f);
                    if (ni >= 0 && ni < static_cast<int>(rawNorm.size()))
                        n = glm::normalize(rawNorm[ni]);

                    // UV (defaults to (0,0) if absent or out of range)
                    glm::vec2 uv(0.0f);
                    if (uvi >= 0 && uvi < static_cast<int>(rawUV.size()))
                        uv = rawUV[uvi];

                    // Tangent is zeroed here -- filled in by computeTangents() below.
                    result.vertices.insert(result.vertices.end(), {
                        p.x,  p.y,  p.z,
                        n.x,  n.y,  n.z,
                        uv.x, uv.y,
                        0.0f, 0.0f, 0.0f   // tangent placeholder
                        });

                    vertexCache[key] = newIdx;
                    resolvedIndices.push_back(newIdx);
                }
            }

            // Fan-triangulate the face into triangles using resolved output indices.
            // Triangle fan: (0,1,2), (0,2,3), (0,3,4), ...
            for (size_t i = 2; i < resolvedIndices.size(); ++i) {
                result.indices.push_back(resolvedIndices[0]);
                result.indices.push_back(resolvedIndices[i - 1]);
                result.indices.push_back(resolvedIndices[i]);
            }
        }

        // ---- Post-pass: compute tangent vectors for all vertices ------------
        computeTangents(result.vertices, result.indices);

        result.numTriangles = static_cast<unsigned int>(result.indices.size() / 3);

        std::cout << "OBJ loaded: " << path
            << "  vertices=" << (result.vertices.size() / STRIDE)
            << "  triangles=" << result.numTriangles << std::endl;

        return result;
    }

} // namespace galuszde