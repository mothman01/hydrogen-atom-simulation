#pragma once

#include <cstdint>
#include <memory>
#include <vector>
#include <string>

#include <glm/glm.hpp>
#include "async_compute.h"

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
    // Hydrogenic version with (effective, possibly fractional) nuclear charge.
    void setOrbital(double Z_eff, int n, int l, int m);

    // Upload pre-computed raw volume data (for time evolution etc.)
    void uploadRawData(const std::vector<float>& data, double halfSize);

    // ---- async computation ----------------------------------------------
    // Set an async computer for background computation (optional)
    void setAsyncComputer(AsyncVolumeComputer* async) { async_ = async; }

    // Framebuffer size used for per-atom scissor rectangles; call once per
    // frame (querying GL_VIEWPORT per draw stalls some drivers).
    static void setViewportSize(int width, int height);

    // ---- position offset -----------------------------------------------
    void setPosition(const glm::vec3& pos) { position_ = pos; }
    const glm::vec3& position() const { return position_; }
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

    // Owns one GL 3-D texture. Shared (via shared_ptr) between renderers
    // that display the same orbital, e.g. every atom in a crystal lattice.
    struct VolumeTexture;

    VolumeRenderer(const VolumeRenderer&) = delete;
    VolumeRenderer& operator=(const VolumeRenderer&) = delete;

private:
    void uploadVolumeData(const std::vector<float> &data);

    bool initialised_ = false;   // holds a reference on the shared GL program
    std::shared_ptr<VolumeTexture> texture_;

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
    double currentZ_         = 1.0;
    glm::vec3 position_{0, 0, 0};
    AsyncVolumeComputer* async_ = nullptr;
};
