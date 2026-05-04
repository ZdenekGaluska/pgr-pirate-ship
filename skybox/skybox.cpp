//----------------------------------------------------------------------------------------
/**
 * \file    skybox.cpp
 * \author  galuszde
 * \note    Code comments generated with AI assistance (Claude, Anthropic).
 * \brief   Skybox cubemap loading and GPU setup.
 */
 //----------------------------------------------------------------------------------------
#include "skybox.h"
#include "../globals.h"
#include <vector>
#include "IL/il.h"

namespace galuszde {

    // Unit cube vertices -- positions only, no normals or UVs.
    // The position vector is used directly as the cubemap lookup direction in the shader.
    // 36 vertices = 6 faces x 2 triangles x 3 vertices (no index buffer needed).
    static const float SKYBOX_VERTICES[] = {
        -1, 1,-1,  -1,-1,-1,   1,-1,-1,   1,-1,-1,   1, 1,-1,  -1, 1,-1,
        -1,-1, 1,  -1,-1,-1,  -1, 1,-1,  -1, 1,-1,  -1, 1, 1,  -1,-1, 1,
         1,-1,-1,   1,-1, 1,   1, 1, 1,   1, 1, 1,   1, 1,-1,   1,-1,-1,
        -1,-1, 1,  -1, 1, 1,   1, 1, 1,   1, 1, 1,   1,-1, 1,  -1,-1, 1,
        -1, 1,-1,   1, 1,-1,   1, 1, 1,   1, 1, 1,  -1, 1, 1,  -1, 1,-1,
        -1,-1,-1,  -1,-1, 1,   1,-1,-1,   1,-1,-1,  -1,-1, 1,   1,-1, 1
    };

    // loadCubemap()

    /// @brief  Loads one cubemap face without vertical flip.
    ///
    /// pgr::loadTexImage2D forces IL_ORIGIN_LOWER_LEFT which flips the image --
    /// correct for 2D textures but wrong for cubemap faces (they must be loaded as-is).
    /// This function overrides the origin to IL_ORIGIN_UPPER_LEFT before uploading.
    ///
    /// @param path    Path to the image file.
    /// @param target  GL_TEXTURE_CUBE_MAP_POSITIVE_X + face index.
    /// @return        true on success, false if the file cannot be loaded.
    static bool loadCubemapFace(const std::string& path, GLenum target) {
        ILuint imgId;
        ilGenImages(1, &imgId);
        ilBindImage(imgId);

        // Upper-left origin = no vertical flip -- cubemap convention differs from 2D textures
        ilEnable(IL_ORIGIN_SET);
        ilSetInteger(IL_ORIGIN_MODE, IL_ORIGIN_UPPER_LEFT);

        if (!ilLoadImage(path.c_str())) {
            std::cerr << "Skybox: cannot load face: " << path << std::endl;
            ilDeleteImages(1, &imgId);
            return false;
        }

        int w = ilGetInteger(IL_IMAGE_WIDTH);
        int h = ilGetInteger(IL_IMAGE_HEIGHT);
        int fmt = ilGetInteger(IL_IMAGE_FORMAT);
        int bpp = (fmt == IL_RGBA || fmt == IL_BGRA) ? 4 : 3;
        GLenum glFmt = (bpp == 4) ? GL_RGBA : GL_RGB;

        std::vector<unsigned char> data(w * h * bpp);
        ilCopyPixels(0, 0, 0, w, h, 1, glFmt, IL_UNSIGNED_BYTE, data.data());
        ilDeleteImages(1, &imgId);

        // Upload pixels to the currently bound cubemap face
        glTexImage2D(target, 0, glFmt, w, h, 0, glFmt, GL_UNSIGNED_BYTE, data.data());
        return true;
    }

    static GLuint loadCubemap(const std::string faces[6]) {
        GLuint texId;
        glGenTextures(1, &texId);
        glBindTexture(GL_TEXTURE_CUBE_MAP, texId);

        for (int i = 0; i < 6; i++) {
            if (!loadCubemapFace(faces[i], GL_TEXTURE_CUBE_MAP_POSITIVE_X + i)) {
                glDeleteTextures(1, &texId);
                return 0;
            }
        }

        // CLAMP_TO_EDGE on all 3 axes -- prevents visible seams at cube edges
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

        glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
        return texId;
    }

    // initSkybox()

    bool initSkybox() {
        // Load the 6 cubemap faces from the skybox/ directory
        std::string faces[6] = {
    "skybox/graycloud_ft.jpg",   // +X right
    "skybox/graycloud_bk.jpg",   // -X left
    "skybox/graycloud_up.jpg",   // +Y top
    "skybox/graycloud_dn.jpg",   // -Y bottom
    "skybox/graycloud_rt.jpg",    // -Z back
    "skybox/graycloud_lf.jpg"   // +Z front

        };

        g_skybox.cubemapTexture = loadCubemap(faces);
        if (!g_skybox.cubemapTexture) return false;

        // Upload unit cube geometry to GPU -- positions only (3 floats per vertex)
        glGenVertexArrays(1, &g_skybox.vao);
        glGenBuffers(1, &g_skybox.vbo);

        glBindVertexArray(g_skybox.vao);
        glBindBuffer(GL_ARRAY_BUFFER, g_skybox.vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(SKYBOX_VERTICES), SKYBOX_VERTICES, GL_STATIC_DRAW);

        // Attribute 0: position (3 floats, tightly packed -- no normals or UVs)
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

        glBindVertexArray(0);

        std::cout << "Skybox initialized." << std::endl;
        return true;
    }

} // namespace galuszde