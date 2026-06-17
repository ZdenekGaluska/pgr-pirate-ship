//----------------------------------------------------------------------------------------
/**
 * \file    bird.cpp
 * \author  galuszde
 * \note    Code comments generated with AI assistance (Claude, Anthropic).
 * \brief   Seagull circling the volcano -- model load, angle update, draw.
 */
 //----------------------------------------------------------------------------------------

#include "bird.h"
#include "../mesh_utils.h"
#include <cmath>

namespace galuszde {

    bool initBird() {
        Assimp::Importer importer;
        const aiScene* scene = importer.ReadFile("seagull/seagull.obj",
            aiProcess_Triangulate | aiProcess_PreTransformVertices | aiProcess_GenSmoothNormals
        );
        if (!scene) {
            std::cerr << "Bird: " << importer.GetErrorString() << std::endl;
            return false;
        }
        if (scene->mNumMeshes == 0) {
            std::cerr << "Bird: no meshes in model." << std::endl;
            return false;
        }

        // Use only the first mesh -- seagull is a single object
        g_bird.mesh = uploadMesh(
            scene->mMeshes[0],
            scene->mMaterials[scene->mMeshes[0]->mMaterialIndex]
        );

        std::cout << "Bird initialized." << std::endl;
        return true;
    }

    void updateBird(float dt) {
        // When paused the angle stays frozen -- bird hangs in place
        if (g_birdPaused) return;

        // Advance angle: full circle in (2*PI / BIRD_SPEED) seconds
        g_bird.angle += BIRD_SPEED * dt;
        if (g_bird.angle > 2.0f * 3.14159265f)
            g_bird.angle -= 2.0f * 3.14159265f;
    }

    void drawBird(const glm::mat4& view, const glm::mat4& proj) {
        if (!g_bird.mesh.vao) return;

        // Circle position centred on volcano
        float x = VOLCANO_POS.x - BIRD_RADIUS * std::cos(g_bird.angle);
        float y = VOLCANO_POS.y + BIRD_HEIGHT;
        float z = VOLCANO_POS.z - BIRD_RADIUS * std::sin(g_bird.angle);

        // Yaw: tangent of the circle = direction of travel.
        // Derivative of (cos, sin) = (-sin, cos), atan2 gives the heading angle.
        float yaw = std::atan2(-std::sin(g_bird.angle), std::cos(g_bird.angle)) + glm::radians(90.0f);

        glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(x, y, z));
        model = glm::rotate(model, yaw, glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::scale(model, glm::vec3(2.0f));

        glm::mat4 PVM = proj * view * model;
        glUniformMatrix4fv(g_shaderLocation.mPVM, 1, GL_FALSE, glm::value_ptr(PVM));
        glUniformMatrix4fv(g_shaderLocation.mModel, 1, GL_FALSE, glm::value_ptr(model));

        // Bird material: white feathers -- high ambient, low specular
        glUniform1f(g_shaderLocation.uAmbient, 0.6f);
        glUniform1f(g_shaderLocation.uSpecularStr, 0.1f);
        glUniform1f(g_shaderLocation.uShininess, 8.0f);

        glUniform3fv(g_shaderLocation.vDiffuse, 1, glm::value_ptr(g_bird.mesh.diffuseColor));
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, g_bird.mesh.texture);
        glBindVertexArray(g_bird.mesh.vao);
        glDrawElements(GL_TRIANGLES, g_bird.mesh.numTriangles * 3, GL_UNSIGNED_INT, nullptr);
        glBindVertexArray(0);
    }

} // namespace galuszde