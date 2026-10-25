#include "volume_renderer.h"
#include "wavefunction.h"
#include "shader.h"
#include "embedded_shaders.h"

#include <iostream>
#include <algorithm>
#include <cmath>
#include <cassert>
#include <map>
#include <memory>
#include <tuple>
#include <utility>

#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <GL/glew.h>
#include <GLFW/glfw3.h>

// ============================================================================
//  Shared GL resources
//
//  Every VolumeRenderer uses the same shader program and full-screen
//  triangle, so they are created once and reference-counted.  Volume
//  textures for analytic orbitals are cached by their parameters: placing
//  N identical atoms (e.g. a crystal lattice) computes and uploads the
//  orbital once instead of N times, and uses one texture instead of N.
// ============================================================================
struct VolumeRenderer::VolumeTexture {
    GLuint id = 0;
    int    res = 0;
    explicit VolumeTexture(int resolution) : res(resolution) {
        glGenTextures(1, &id);
        glBindTexture(GL_TEXTURE_3D, id);
        glTexImage3D(GL_TEXTURE_3D, 0, GL_R32F, res, res, res,
                     0, GL_RED, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
        glBindTexture(GL_TEXTURE_3D, 0);
    }
    ~VolumeTexture() { if (id) glDeleteTextures(1, &id); }
    VolumeTexture(const VolumeTexture&) = delete;
    VolumeTexture& operator=(const VolumeTexture&) = delete;
};

namespace {

struct SharedGL {
    int    refs    = 0;
    GLuint program = 0;
    GLuint vao     = 0;
    GLuint vbo     = 0;
    GLint  uInvVP = -1, uCameraPos = -1, uVolumeMin = -1, uVolumeMax = -1,
           uVolumeTex = -1, uVolumeRes = -1, uOpacityScale = -1,
           uStepScale = -1, uExposure = -1;
};
SharedGL g_gl;
int g_viewportW = 0, g_viewportH = 0;

bool acquireSharedGL()
{
    if (g_gl.refs > 0) { ++g_gl.refs; return true; }

    g_gl.program = createShaderProgram(EMBEDDED_SCREEN_QUAD_VERT,
                                       EMBEDDED_RAYMARCH_FRAG);
    if (g_gl.program == 0) {
        std::cerr << "Failed to load volume renderer shaders.\n";
        return false;
    }

    g_gl.uInvVP        = glGetUniformLocation(g_gl.program, "uInvVP");
    g_gl.uCameraPos    = glGetUniformLocation(g_gl.program, "uCameraPos");
    g_gl.uVolumeMin    = glGetUniformLocation(g_gl.program, "uVolumeMin");
    g_gl.uVolumeMax    = glGetUniformLocation(g_gl.program, "uVolumeMax");
    g_gl.uVolumeTex    = glGetUniformLocation(g_gl.program, "uVolumeTex");
    g_gl.uVolumeRes    = glGetUniformLocation(g_gl.program, "uVolumeRes");
    g_gl.uOpacityScale = glGetUniformLocation(g_gl.program, "uOpacityScale");
    g_gl.uStepScale    = glGetUniformLocation(g_gl.program, "uStepScale");
    g_gl.uExposure     = glGetUniformLocation(g_gl.program, "uExposure");

    const float vertices[] = {
        -1.0f, -1.0f, 0.0f, 0.0f,
         3.0f, -1.0f, 2.0f, 0.0f,
        -1.0f,  3.0f, 0.0f, 2.0f,
    };
    glGenVertexArrays(1, &g_gl.vao);
    glGenBuffers(1, &g_gl.vbo);
    glBindVertexArray(g_gl.vao);
    glBindBuffer(GL_ARRAY_BUFFER, g_gl.vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float),
                          (void *)(2 * sizeof(float)));
    glBindVertexArray(0);

    g_gl.refs = 1;
    return true;
}

void releaseSharedGL()
{
    if (g_gl.refs <= 0 || --g_gl.refs > 0) return;
    if (g_gl.program) glDeleteProgram(g_gl.program);
    if (g_gl.vao)     glDeleteVertexArrays(1, &g_gl.vao);
    if (g_gl.vbo)     glDeleteBuffers(1, &g_gl.vbo);
    g_gl = SharedGL{};
}

struct OrbitalKey {
    double Z;
    int n, l, m, res;
    double halfSize;
    bool operator<(const OrbitalKey& o) const {
        return std::tie(Z, n, l, m, res, halfSize) <
               std::tie(o.Z, o.n, o.l, o.m, o.res, o.halfSize);
    }
};
std::map<OrbitalKey, std::weak_ptr<VolumeRenderer::VolumeTexture>> g_orbitalCache;

} // namespace

void VolumeRenderer::setViewportSize(int width, int height)
{
    g_viewportW = width;
    g_viewportH = height;
}

// ============================================================================
//  Construction / destruction
// ============================================================================
VolumeRenderer::VolumeRenderer()  = default;
VolumeRenderer::~VolumeRenderer()
{
    texture_.reset();
    if (initialised_) releaseSharedGL();
}

// ============================================================================
//  Initialisation
// ============================================================================
bool VolumeRenderer::init(int volumeResolution)
{
    volumeRes_ = volumeResolution;
    if (initialised_) return true;
    if (!acquireSharedGL()) return false;
    initialised_ = true;
    return true;
}

// ============================================================================
//  Orbital update
// ============================================================================
void VolumeRenderer::setOrbital(int n, int l, int m)
{
    setOrbital(1.0, n, l, m);
}

void VolumeRenderer::setOrbital(double Z_eff, int n, int l, int m)
{
    currentZ_ = Z_eff;
    currentN_ = n;
    currentL_ = l;
    currentM_ = m;

    halfSize_ = suggestedHalfSize(n);
    halfSize_ /= Z_eff;
    if (halfSize_ < 1.0) halfSize_ = 1.0;

    const OrbitalKey key{Z_eff, n, l, m, volumeRes_, halfSize_};
    auto it = g_orbitalCache.find(key);
    if (it != g_orbitalCache.end()) {
        if (auto shared = it->second.lock()) {
            texture_ = std::move(shared);
            return;
        }
        g_orbitalCache.erase(it);
    }

    std::vector<float> data;
    computeOrbital(n, l, m, volumeRes_, halfSize_, Z_eff, data);

    texture_ = std::make_shared<VolumeTexture>(volumeRes_);
    uploadVolumeData(data);
    g_orbitalCache[key] = texture_;

    std::cout << "Orbital: Z_eff=" << Z_eff << " n=" << n << " l=" << l << " m=" << m
              << "  box=" << halfSize_ << " a0\n";
}

// ============================================================================
//  Raw data upload
// ============================================================================
void VolumeRenderer::uploadRawData(const std::vector<float>& data, double halfSize) {
    halfSize_ = halfSize;
    if (!texture_ || texture_.use_count() > 1 || texture_->res != volumeRes_) {
        texture_ = std::make_shared<VolumeTexture>(volumeRes_);
    } else {
        for (auto& entry : g_orbitalCache) {
            if (entry.second.lock() == texture_) {
                texture_ = std::make_shared<VolumeTexture>(volumeRes_);
                break;
            }
        }
    }
    uploadVolumeData(data);
}

// ============================================================================
//  Render one frame
// ============================================================================
void VolumeRenderer::render(const glm::mat4 &view,
                             const glm::mat4 &projection,
                             const glm::vec3  &cameraPos)
{
    if (!initialised_ || !texture_) return;

    glm::vec3 localCam = cameraPos - position_;
    glm::mat4 model = glm::translate(glm::mat4(1.0f), position_);
    glm::mat4 vp = projection * view * model;
    glm::mat4 invVP = glm::inverse(vp);

    // Restrict the full-screen raymarch to the volume's on-screen bounding
    // rectangle.  With many atoms (crystal lattices) this avoids running the
    // fragment shader over the whole screen once per atom, and atoms that are
    // entirely off-screen are skipped.
    bool useScissor = g_viewportW > 0 && g_viewportH > 0;
    float minX = 1.0f, minY = 1.0f, maxX = -1.0f, maxY = -1.0f;
    const float hs = static_cast<float>(halfSize_);
    for (int c = 0; c < 8 && useScissor; ++c) {
        glm::vec4 p = vp * glm::vec4((c & 1) ? hs : -hs, (c & 2) ? hs : -hs,
                                     (c & 4) ? hs : -hs, 1.0f);
        if (p.w <= 1e-4f) { useScissor = false; break; }   // corner behind camera
        float x = p.x / p.w, y = p.y / p.w;
        minX = std::min(minX, x); maxX = std::max(maxX, x);
        minY = std::min(minY, y); maxY = std::max(maxY, y);
    }
    if (useScissor) {
        if (maxX < -1.0f || minX > 1.0f || maxY < -1.0f || minY > 1.0f) return;
        const GLint vpRect[4] = {0, 0, g_viewportW, g_viewportH};
        auto toPx = [](float ndc, GLint origin, GLint size) {
            return origin + (std::max(-1.0f, std::min(1.0f, ndc)) * 0.5f + 0.5f) * size;
        };
        GLint x0 = static_cast<GLint>(std::floor(toPx(minX, vpRect[0], vpRect[2])));
        GLint y0 = static_cast<GLint>(std::floor(toPx(minY, vpRect[1], vpRect[3])));
        GLint x1 = static_cast<GLint>(std::ceil (toPx(maxX, vpRect[0], vpRect[2])));
        GLint y1 = static_cast<GLint>(std::ceil (toPx(maxY, vpRect[1], vpRect[3])));
        glEnable(GL_SCISSOR_TEST);
        glScissor(x0, y0, std::max(0, x1 - x0), std::max(0, y1 - y0));
    }

    // Every volume is a full-screen triangle at the same depth; with depth
    // testing on, the first atom drawn over a pixel would block all others.
    GLboolean depthWasOn = glIsEnabled(GL_DEPTH_TEST);
    if (depthWasOn) glDisable(GL_DEPTH_TEST);

    glUseProgram(g_gl.program);

    glUniformMatrix4fv(g_gl.uInvVP, 1, GL_FALSE, glm::value_ptr(invVP));
    glUniform3fv(g_gl.uCameraPos, 1, glm::value_ptr(localCam));
    glUniform3f(g_gl.uVolumeMin,
                static_cast<float>(-halfSize_),
                static_cast<float>(-halfSize_),
                static_cast<float>(-halfSize_));
    glUniform3f(g_gl.uVolumeMax,
                static_cast<float>(halfSize_),
                static_cast<float>(halfSize_),
                static_cast<float>(halfSize_));
    glUniform1f(g_gl.uOpacityScale, opacityScale_);
    glUniform1f(g_gl.uStepScale,    stepScale_);
    glUniform1f(g_gl.uExposure,     exposure_);
    glUniform1i(g_gl.uVolumeRes,    texture_->res);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_3D, texture_->id);
    glUniform1i(g_gl.uVolumeTex, 0);

    glBindVertexArray(g_gl.vao);
    glDrawArrays(GL_TRIANGLES, 0, 3);

    glBindVertexArray(0);
    glUseProgram(0);
    if (useScissor) glDisable(GL_SCISSOR_TEST);
    if (depthWasOn) glEnable(GL_DEPTH_TEST);
}

// ============================================================================
//  Internal helpers
// ============================================================================
void VolumeRenderer::uploadVolumeData(const std::vector<float> &data)
{
    assert(texture_);
    assert(data.size() == static_cast<std::size_t>(texture_->res) *
                          static_cast<std::size_t>(texture_->res) *
                          static_cast<std::size_t>(texture_->res));

    glBindTexture(GL_TEXTURE_3D, texture_->id);
    glTexSubImage3D(GL_TEXTURE_3D, 0,
                    0, 0, 0,
                    texture_->res, texture_->res, texture_->res,
                    GL_RED, GL_FLOAT, data.data());
    glBindTexture(GL_TEXTURE_3D, 0);
}
