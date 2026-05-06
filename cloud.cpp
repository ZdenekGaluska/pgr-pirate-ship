//----------------------------------------------------------------------------------------
/**
 * \file    cloud.cpp
 * \author  galuszde
 * \note    Code comments generated with AI assistance (Claude, Anthropic).
 * \brief   Animated cloud plane -- shader setup, plane geometry, spritesheet playback.
 */
//----------------------------------------------------------------------------------------

#include "cloud/cloud.h"
#include "mesh_utils.h"

namespace galuszde {

    // -----------------------------------------------------------------------
    // initCloudShader()
    // -----------------------------------------------------------------------

    /// @brief  Compiles and links the cloud shader pair, caches uniform locations.
    /// @return true on success.
    static bool initCloudShader() {
        GLuint shaderList[] = {
            pgr::createShaderFromFile(GL_VERTEX_SHADER,   "cloud/cloud-vs.glsl"),
            pgr::createShaderFromFile(GL_FRAGMENT_SHADER, "cloud/cloud-fs.glsl"),
            0
        };
        g_cloud.program = pgr::createProgram(shaderList);
        if (!g_cloud.program) {
            std::cerr << "Cloud shader failed." << std::endl;
            return false;
        }

        glUseProgram(g_cloud.program);
        g_cloud.locPVM      = glGetUniformLocation(g_cloud.program, "mPVM");
        g_cloud.locFrameOff = glGetUniformLocation(g_cloud.program, "uFrameOffset");
        g_cloud.locAlpha    = glGetUniformLocation(g_cloud.program, "uAlpha");
        g_cloud.locTexture  = glGetUniformLocation(g_cloud.program, "uTexture");
        glUniform1i(g_cloud.locTexture, 0);   // texture unit 0
        glUseProgram(0);
        return true;
    }

    // -----------------------------------------------------------------------
    // initCloudPlane()
    // -----------------------------------------------------------------------

    /// @brief  Uploads a horizontal quad (two triangles) centred at origin to the GPU.
    ///
    ///         Layout per vertex: pos(3) + uv(2) = 5 floats, stride 20 bytes.
    ///         UV (0,0)-(1,1) covers one full spritesheet -- the shader scales it
    ///         down to one frame cell.
    static void initCloudPlane() {
        const float S = CLOUD_SIZE;

        // Four corners of the horizontal plane, Y = 0 (translated in drawCloud)
        // Interleaved: x y z u v
        const float verts[] = {
            -S, 0.0f, -S,   0.0f, 0.0f,
             S, 0.0f, -S,   1.0f, 0.0f,
             S, 0.0f,  S,   1.0f, 1.0f,
            -S, 0.0f,  S,   0.0f, 1.0f,
        };
        const unsigned int idx[] = { 0, 1, 2,   0, 2, 3 };

        glGenBuffers(1, &g_cloud.vbo);
        glBindBuffer(GL_ARRAY_BUFFER, g_cloud.vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);

        glGenBuffers(1, &g_cloud.ibo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_cloud.ibo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(idx), idx, GL_STATIC_DRAW);

        glGenVertexArrays(1, &g_cloud.vao);
        glBindVertexArray(g_cloud.vao);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_cloud.ibo);
        glBindBuffer(GL_ARRAY_BUFFER, g_cloud.vbo);

        const int stride = 5 * sizeof(float);

        // Attribute 0: position (3 floats, offset 0)
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);

        // Attribute 1: UV (2 floats, offset 12 B)
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));

        glBindVertexArray(0);
    }

    // -----------------------------------------------------------------------
    // Public API
    // -----------------------------------------------------------------------

    bool initCloud() {
        if (!initCloudShader()) return false;
        initCloudPlane();

        g_cloud.texture = loadTexture("cloud/cloud.png");
        if (!g_cloud.texture)
            std::cerr << "Cloud: texture cloud.png not found." << std::endl;

        std::cout << "Cloud initialized." << std::endl;
        return true;
    }

    void updateCloud(float dt) {
        g_cloud.timer += dt;
    }

    void drawCloud(const glm::mat4& view, const glm::mat4& proj) {
        if (!g_cloud.vao || !g_cloud.texture) return;

        // Determine current frame from timer
        int totalFrames = CLOUD_FRAME_COLS * CLOUD_FRAME_ROWS;
        int frame       = static_cast<int>(g_cloud.timer * CLOUD_FPS) % totalFrames;
        int col         = frame % CLOUD_FRAME_COLS;
        int row         = frame / CLOUD_FRAME_COLS;

        // UV offset: top-left corner of this frame in the spritesheet
        glm::vec2 frameOffset(
            col * (1.0f / CLOUD_FRAME_COLS),
            row * (1.0f / CLOUD_FRAME_ROWS)
        );

        // Position plane above volcano
        glm::mat4 model = glm::translate(glm::mat4(1.0f),
            glm::vec3(VOLCANO_POS.x, VOLCANO_POS.y + CLOUD_HEIGHT, VOLCANO_POS.z));
        glm::mat4 PVM = proj * view * model;

        glUseProgram(g_cloud.program);
        glUniformMatrix4fv(g_cloud.locPVM, 1, GL_FALSE, glm::value_ptr(PVM));
        glUniform2fv(g_cloud.locFrameOff, 1, glm::value_ptr(frameOffset));
        glUniform1f(g_cloud.locAlpha, 0.85f);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, g_cloud.texture);

        // Alpha blending: cloud pixels blend over sky, black pixels disappear
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDepthMask(GL_FALSE);   // transparent objects don't write to depth buffer

        glBindVertexArray(g_cloud.vao);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
        glBindVertexArray(0);

        // Restore GL state for subsequent opaque draws
        glDepthMask(GL_TRUE);
        glDisable(GL_BLEND);

        glUseProgram(0);
    }

    void destroyCloud() {
        glDeleteVertexArrays(1, &g_cloud.vao);
        glDeleteBuffers(1, &g_cloud.vbo);
        glDeleteBuffers(1, &g_cloud.ibo);
        glDeleteTextures(1, &g_cloud.texture);
        pgr::deleteProgramAndShaders(g_cloud.program);
    }

} // namespace galuszde