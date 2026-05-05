#pragma once
//----------------------------------------------------------------------------------------
/**
 * \file    obj_loader.h
 * \author  galuszde
 * \note    Code comments generated with AI assistance (Claude, Anthropic).
 * \brief   Custom Wavefront OBJ file loader -- no external library (satisfies rubric 2.3).
 *
 * Parses the OBJ text format and builds a fully interleaved vertex buffer ready for
 * direct GPU upload.  Tangent vectors are computed analytically from positions and UVs
 * so the chest fragment shader can perform tangent-space normal mapping.
 *
 * Supported OBJ syntax:
 *   v  x y z          -- vertex position
 *   vt u v            -- texture coordinate
 *   vn x y z          -- vertex normal
 *   f  v/vt/vn  ...   -- triangle face (and quads, fan-triangulated)
 *   #  ...            -- comment (ignored)
 *   o, g, mtllib, ... -- object/group/material lines (ignored, geometry only)
 *
 * Interleaved VBO layout per vertex (11 floats, stride = 44 bytes):
 *   [ px py pz | nx ny nz | u v | tx ty tz ]
 *     pos (12B)  normal(12B) UV(8B) tangent(12B)
 *
 * Shader attribute locations must match:
 *   location 0 = vec3 position
 *   location 1 = vec3 normal
 *   location 2 = vec2 uv
 *   location 3 = vec3 tangent
 */
 //----------------------------------------------------------------------------------------

#include "globals.h"
#include <string>
#include <vector>

namespace galuszde {

    /// @brief  Parsed and GPU-ready mesh data from a single OBJ file.
    ///
    /// Feed vertices directly into glBufferData(GL_ARRAY_BUFFER, ...).
    /// Feed indices directly into glBufferData(GL_ELEMENT_ARRAY_BUFFER, ...).
    struct ObjData {
        std::vector<float>        vertices;   ///< Interleaved: pos(3)|nrm(3)|uv(2)|tangent(3)
        std::vector<unsigned int> indices;    ///< Triangle indices, 3 per triangle.
        unsigned int              numTriangles = 0;
    };

    /// @brief  Loads a Wavefront OBJ file and returns the parsed mesh data.
    ///
    /// All face types are supported (pos-only, pos/uv, pos/uv/nrm, quads).
    /// Missing UVs default to (0,0), missing normals default to (0,1,0).
    /// Tangent vectors are computed analytically after all geometry is assembled.
    ///
    /// @param  path  Path to the .obj file, relative to the working directory.
    /// @return ObjData with filled vertices/indices, or empty ObjData on failure.
    ObjData loadOBJ(const std::string& path);

} // namespace galuszde