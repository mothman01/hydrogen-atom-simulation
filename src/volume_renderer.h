#pragma once

#include <cstdint>
#include <vector>
#include <string>

#include <glm/glm.hpp>

/**
 * GPU volume renderer for hydrogen orbital probability densities.
 *
 * Pre-computes a 3-D floating-point texture and ray-marches through it
 * on the GPU using a full-screen fragment shader.
 */
class VolumeRenderer {
public:
    VolumeRenderer();
    ~VolumeRenderer();

    // ---- setup --------------------------------------------------------
    bool init(int volumeResolution);

    // ---- orbital data -------------------------------------------------
    // Compute volume data for the given quantum state and upload to GPU.
    void setOrbital(int n, int l, int m);

    // ---- rendering ----------------------------------------------------
    void render(const glm::mat4 &view,
                const glm::mat4 &projection,
                const glm::vec3  &cameraPos);

    // ---- settings -----------------------------------------------------
    void setOpacityScale(float s)  { opacityScale_ = s; }
    void setStepScale(float s)     { stepScale_ = s; }
    void setExposure(float e)      { exposure_ = e; }

    float opacityScale() const { return opacityScale_; }
    float stepScale() const    { return stepScale_; }
    float exposure() const     { return exposure_; }

    // ---- info ---------------------------------------------------------
    int  currentN() const { return currentN_; }
    int  currentL() const { return currentL_; }
    int  currentM() const { return currentM_; }

private:
    void createVolumeTexture();
    void uploadVolumeData(const std::vector<float> &data);
    void setupFullscreenQuad();

    // GL objects
    uint32_t shaderProgram_  = 0;
    uint32_t volumeTexture_  = 0;
    uint32_t quadVAO_        = 0;
    uint32_t quadVBO_        = 0;

    // uniforms
    int32_t  uInvVP_         = -1;
    int32_t  uCameraPos_     = -1;
    int32_t  uVolumeMin_     = -1;
    int32_t  uVolumeMax_     = -1;
    int32_t  uVolumeTex_     = -1;
    int32_t  uVolumeRes_     = -1;
    int32_t  uOpacityScale_  = -1;
    int32_t  uStepScale_     = -1;
    int32_t  uExposure_      = -1;

    // settings
    int    volumeRes_        = 128;
    float  opacityScale_     = 8.0f;
    float  stepScale_        = 1.0f;
    float  exposure_         = 1.5f;
    double halfSize_         = 10.0;

    // current orbital
    int    currentN_         = 1;
    int    currentL_         = 0;
    int    currentM_         = 0;
};
