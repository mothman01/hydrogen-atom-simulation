#include "volume_renderer.h"
#include "wavefunction.h"
#include "shader.h"

#include <iostream>
#include <cmath>
#include <cassert>

#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <GL/glcorearb.h>
#include <GLFW/glfw3.h>

// ============================================================================
//  Construction / destruction
// ============================================================================
VolumeRenderer::VolumeRenderer()  = default;
VolumeRenderer::~VolumeRenderer()
{
    if (volumeTexture_) glDeleteTextures(1, &volumeTexture_);
    if (shaderProgram_) glDeleteProgram(shaderProgram_);
    if (quadVAO_)       glDeleteVertexArrays(1, &quadVAO_);
    if (quadVBO_)       glDeleteBuffers(1, &quadVBO_);
}

// ============================================================================
//  Initialisation
// ============================================================================
bool VolumeRenderer::init(int volumeResolution)
{
    volumeRes_ = volumeResolution;

    // Load raymarching shaders
    std::string vertPath = std::string(SHADER_DIR) + "/screen_quad.vert";
    std::string fragPath = std::string(SHADER_DIR) + "/raymarch.frag";
    shaderProgram_ = loadShaderProgram(vertPath, fragPath);
    if (shaderProgram_ == 0) {
        std::cerr << "Failed to load volume renderer shaders.\n";
        return false;
    }

    // Cache uniform locations
    uInvVP_        = glGetUniformLocation(shaderProgram_, "uInvVP");
    uCameraPos_    = glGetUniformLocation(shaderProgram_, "uCameraPos");
    uVolumeMin_    = glGetUniformLocation(shaderProgram_, "uVolumeMin");
    uVolumeMax_    = glGetUniformLocation(shaderProgram_, "uVolumeMax");
    uVolumeTex_    = glGetUniformLocation(shaderProgram_, "uVolumeTex");
    uVolumeRes_    = glGetUniformLocation(shaderProgram_, "uVolumeRes");
    uOpacityScale_ = glGetUniformLocation(shaderProgram_, "uOpacityScale");
    uStepScale_    = glGetUniformLocation(shaderProgram_, "uStepScale");
    uExposure_     = glGetUniformLocation(shaderProgram_, "uExposure");

    // Create the 3-D volume texture
    createVolumeTexture();

    // Full-screen quad
    setupFullscreenQuad();

    std::cout << "Volume renderer initialised (resolution = "
              << volumeRes_ << "³)\n";
    return true;
}

// ============================================================================
//  Orbital update
// ============================================================================
void VolumeRenderer::setOrbital(int n, int l, int m)
{
    currentN_ = n;
    currentL_ = l;
    currentM_ = m;

    halfSize_ = suggestedHalfSize(n);

    std::vector<float> data;
    computeVolumeData(n, l, m, volumeRes_, halfSize_, data);

    uploadVolumeData(data);

    std::cout << "Orbital: n=" << n << " l=" << l << " m=" << m
              << "  box=±" << halfSize_ << " a₀\n";
}

// ============================================================================
//  Render one frame
// ============================================================================
void VolumeRenderer::render(const glm::mat4 &view,
                             const glm::mat4 &projection,
                             const glm::vec3  &cameraPos)
{
    glUseProgram(shaderProgram_);

    // Compute inverse view-projection for ray reconstruction
    glm::mat4 vp = projection * view;
    glm::mat4 invVP = glm::inverse(vp);

    glUniformMatrix4fv(uInvVP_, 1, GL_FALSE, glm::value_ptr(invVP));
    glUniform3fv(uCameraPos_, 1, glm::value_ptr(cameraPos));
    glUniform3f(uVolumeMin_,
                static_cast<float>(-halfSize_),
                static_cast<float>(-halfSize_),
                static_cast<float>(-halfSize_));
    glUniform3f(uVolumeMax_,
                static_cast<float>(halfSize_),
                static_cast<float>(halfSize_),
                static_cast<float>(halfSize_));
    glUniform1f(uOpacityScale_, opacityScale_);
    glUniform1f(uStepScale_,    stepScale_);
    glUniform1f(uExposure_,     exposure_);
    glUniform1i(uVolumeRes_,    volumeRes_);

    // Bind 3-D texture to unit 0
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_3D, volumeTexture_);
    glUniform1i(uVolumeTex_, 0);

    // Draw full-screen quad
    glBindVertexArray(quadVAO_);
    glDrawArrays(GL_TRIANGLES, 0, 3);

    glBindVertexArray(0);
    glUseProgram(0);
}

// ============================================================================
//  Internal helpers
// ============================================================================
void VolumeRenderer::createVolumeTexture()
{
    glGenTextures(1, &volumeTexture_);
    glBindTexture(GL_TEXTURE_3D, volumeTexture_);

    // Allocate storage (data will be uploaded later)
    glTexImage3D(GL_TEXTURE_3D, 0, GL_R32F,
                 volumeRes_, volumeRes_, volumeRes_,
                 0, GL_RED, GL_FLOAT, nullptr);

    // Linear interpolation for smooth sampling
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Clamp to edge so we don't sample outside the volume
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    glBindTexture(GL_TEXTURE_3D, 0);
}

void VolumeRenderer::uploadVolumeData(const std::vector<float> &data)
{
    assert(static_cast<int>(data.size()) ==
           volumeRes_ * volumeRes_ * volumeRes_);

    glBindTexture(GL_TEXTURE_3D, volumeTexture_);
    glTexSubImage3D(GL_TEXTURE_3D, 0,
                    0, 0, 0,
                    volumeRes_, volumeRes_, volumeRes_,
                    GL_RED, GL_FLOAT, data.data());
    glBindTexture(GL_TEXTURE_3D, 0);
}

void VolumeRenderer::setupFullscreenQuad()
{
    // A single triangle that covers clip space [-1,1]×[-1,1]
    // Vertex format:  vec2 pos, vec2 uv
    struct Vertex {
        float px, py, u, v;
    };
    const Vertex vertices[] = {
        {-1.0f, -1.0f, 0.0f, 0.0f},
        { 3.0f, -1.0f, 2.0f, 0.0f},
        {-1.0f,  3.0f, 0.0f, 2.0f},
    };

    glGenVertexArrays(1, &quadVAO_);
    glGenBuffers(1, &quadVBO_);

    glBindVertexArray(quadVAO_);
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO_);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // position (location 0)
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE,
                          4 * sizeof(float), (void *)0);
    // texcoord (location 1)
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE,
                          4 * sizeof(float), (void *)(2 * sizeof(float)));

    glBindVertexArray(0);
}
