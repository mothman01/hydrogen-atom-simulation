#include "wavefunction.h"
#include "volume_renderer.h"

#include <GL/glcorearb.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <iostream>
#include <cmath>
#include <cstdlib>
#include <cstdio>

// ============================================================================
//  Orbit camera
// ============================================================================
struct OrbitCamera {
    float azimuth   = 0.0f;     // radians, rotation around Y
    float elevation = 0.4f;     // radians above XZ plane
    float distance  = 18.0f;
    glm::vec3 target{0, 0, 0};

    glm::mat4 viewMatrix() const {
        float cx = std::cos(azimuth);
        float sx = std::sin(azimuth);
        float cy = std::cos(elevation);
        float sy = std::sin(elevation);

        glm::vec3 eye(target.x + distance * cx * cy,
                       target.y + distance * sy,
                       target.z + distance * sx * cy);
        glm::vec3 up(0, 1, 0);
        return glm::lookAt(eye, target, up);
    }

    glm::vec3 eyePos() const {
        float cx = std::cos(azimuth);
        float sx = std::sin(azimuth);
        float cy = std::cos(elevation);
        float sy = std::sin(elevation);
        return glm::vec3(target.x + distance * cx * cy,
                          target.y + distance * sy,
                          target.z + distance * sx * cy);
    }
};

// ============================================================================
//  Global state
// ============================================================================
static GLFWwindow *g_window      = nullptr;
static OrbitCamera g_camera;
static VolumeRenderer *g_renderer = nullptr;
static int g_width  = 1280;
static int g_height = 720;
static int g_orbitalIndex = 0;  // start at 1s

// Mouse state
static bool  g_mousePressed = false;
static float g_lastMouseX   = 0.0f;
static float g_lastMouseY   = 0.0f;
static float g_autoRotate   = 0.3f;  // rad/s

// ============================================================================
//  Callbacks
// ============================================================================
static void framebufferSizeCallback(GLFWwindow * /*win*/, int w, int h) {
    g_width  = w;
    g_height = h;
}

static void keyCallback(GLFWwindow * /*win*/, int key, int /*scancode*/,
                         int action, int /*mods*/) {
    if (action == GLFW_PRESS) {
        switch (key) {
        case GLFW_KEY_ESCAPE:
            glfwSetWindowShouldClose(g_window, GLFW_TRUE);
            break;

        // ---- orbital selection -------------------------------------------
        case GLFW_KEY_RIGHT:
        case GLFW_KEY_D:
            g_orbitalIndex = (g_orbitalIndex + 1) % NUM_ORBITALS;
            break;
        case GLFW_KEY_LEFT:
        case GLFW_KEY_A:
            g_orbitalIndex = (g_orbitalIndex - 1 + NUM_ORBITALS) % NUM_ORBITALS;
            break;
        case GLFW_KEY_UP:
        case GLFW_KEY_W:
            // Jump to next principal quantum number
            {
                int curN = ORBITALS[g_orbitalIndex].n;
                int next = g_orbitalIndex;
                while (next < NUM_ORBITALS - 1) {
                    ++next;
                    if (ORBITALS[next].n > curN) break;
                }
                if (ORBITALS[next].n > curN) g_orbitalIndex = next;
            }
            break;
        case GLFW_KEY_DOWN:
        case GLFW_KEY_S:
            // Jump to previous principal quantum number
            {
                int curN = ORBITALS[g_orbitalIndex].n;
                int prev = g_orbitalIndex;
                while (prev > 0) {
                    --prev;
                    if (ORBITALS[prev].n < curN) break;
                }
                if (ORBITALS[prev].n < curN) g_orbitalIndex = prev;
            }
            break;

        // Direct orbital keys 1–9, 0
        case GLFW_KEY_1:
        case GLFW_KEY_2:
        case GLFW_KEY_3:
        case GLFW_KEY_4:
        case GLFW_KEY_5:
        case GLFW_KEY_6:
        case GLFW_KEY_7:
        case GLFW_KEY_8:
        case GLFW_KEY_9:
            {
                int idx = key - GLFW_KEY_1;
                if (idx < NUM_ORBITALS) g_orbitalIndex = idx;
            }
            break;
        case GLFW_KEY_0:
            if (NUM_ORBITALS > 9) g_orbitalIndex = 9;
            break;

        // ---- render settings ---------------------------------------------
        case GLFW_KEY_KP_ADD:
        case GLFW_KEY_EQUAL:
            if (g_renderer)
                g_renderer->setOpacityScale(g_renderer->opacityScale() * 1.2f);
            break;
        case GLFW_KEY_KP_SUBTRACT:
        case GLFW_KEY_MINUS:
            if (g_renderer)
                g_renderer->setOpacityScale(g_renderer->opacityScale() / 1.2f);
            break;
        case GLFW_KEY_Q:
            if (g_renderer)
                g_renderer->setExposure(g_renderer->exposure() * 1.1f);
            break;
        case GLFW_KEY_E:
            if (g_renderer)
                g_renderer->setExposure(g_renderer->exposure() / 1.1f);
            break;
        case GLFW_KEY_R:
            g_autoRotate = (g_autoRotate > 0.01f) ? 0.0f : 0.3f;
            break;
        case GLFW_KEY_SPACE:
            // Reset camera
            g_camera.azimuth   = 0.0f;
            g_camera.elevation = 0.4f;
            g_camera.distance  = 18.0f;
            g_camera.target    = glm::vec3(0, 0, 0);
            break;

        default:
            break;
        }

        // Trigger orbital update
        const auto &orb = ORBITALS[g_orbitalIndex];
        if (g_renderer) {
            g_renderer->setOrbital(orb.n, orb.l, orb.m);
        }

        // Update window title with orbital info
        char title[256];
        std::snprintf(title, sizeof(title),
                      "Hydrogen Atom — %s (%s)  [n=%d l=%d m=%d]  "
                      "← → / A D : prev-next    W S : n±1    "
                      "1-9 : pick    +/- : opacity    Q/E : exposure    "
                      "R : auto-rotate %s    SPACE : reset view",
                      orb.label, orb.description,
                      orb.n, orb.l, orb.m,
                      (g_autoRotate > 0.0f) ? "ON" : "OFF");
        glfwSetWindowTitle(g_window, title);
    }
}

static void mouseButtonCallback(GLFWwindow * /*win*/, int button, int action,
                                 int /*mods*/) {
    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        g_mousePressed = (action == GLFW_PRESS);
        if (g_mousePressed) {
            double mx, my;
            glfwGetCursorPos(g_window, &mx, &my);
            g_lastMouseX = static_cast<float>(mx);
            g_lastMouseY = static_cast<float>(my);
        }
    }
}

static void cursorPosCallback(GLFWwindow * /*win*/, double xpos, double ypos) {
    float dx = static_cast<float>(xpos) - g_lastMouseX;
    float dy = static_cast<float>(ypos) - g_lastMouseY;
    g_lastMouseX = static_cast<float>(xpos);
    g_lastMouseY = static_cast<float>(ypos);

    if (g_mousePressed) {
        g_camera.azimuth   -= dx * 0.005f;
        g_camera.elevation += dy * 0.005f;
        // Clamp elevation to avoid flipping
        float maxEl = static_cast<float>(M_PI / 2.0 - 0.05);
        if (g_camera.elevation >  maxEl) g_camera.elevation =  maxEl;
        if (g_camera.elevation < -maxEl) g_camera.elevation = -maxEl;
    }
}

static void scrollCallback(GLFWwindow * /*win*/, double /*xoffset*/,
                            double yoffset) {
    g_camera.distance *= std::pow(0.9f, static_cast<float>(yoffset));
    if (g_camera.distance < 2.0f)  g_camera.distance = 2.0f;
    if (g_camera.distance > 80.0f) g_camera.distance = 80.0f;
}

// ============================================================================
//  FPS counter
// ============================================================================
static double g_lastFpsTime = 0.0;
static int    g_frameCount  = 0;

static void updateFPS() {
    g_frameCount++;
    double now = glfwGetTime();
    double elapsed = now - g_lastFpsTime;
    if (elapsed >= 1.0) {
        double fps = g_frameCount / elapsed;
        char title[256];
        const auto &orb = ORBITALS[g_orbitalIndex];
        std::snprintf(title, sizeof(title),
                      "Hydrogen Atom — %s (%s)  [n=%d l=%d m=%d]  "
                      "FPS: %.0f",
                      orb.label, orb.description, orb.n, orb.l, orb.m, fps);
        glfwSetWindowTitle(g_window, title);
        g_frameCount  = 0;
        g_lastFpsTime = now;
    }
}

// ============================================================================
//  Print help
// ============================================================================
static void printHelp() {
    std::cout << "\n"
        "╔══════════════════════════════════════════════════════════╗\n"
        "║      Hydrogen Atom — Schrödinger Wavefunction Sim        ║\n"
        "╠══════════════════════════════════════════════════════════╣\n"
        "║  Controls:                                               ║\n"
        "║    ← → / A D  —  previous / next orbital                 ║\n"
        "║    W S        —  jump to next / previous n               ║\n"
        "║    1 … 9      —  pick orbital directly                   ║\n"
        "║    Mouse drag —  rotate view                              ║\n"
        "║    Scroll     —  zoom in / out                            ║\n"
        "║    +/-        —  opacity scale                            ║\n"
        "║    Q / E      —  exposure (brightness)                    ║\n"
        "║    R          —  toggle auto-rotate                       ║\n"
        "║    SPACE      —  reset camera                             ║\n"
        "║    ESC        —  quit                                     ║\n"
        "╠══════════════════════════════════════════════════════════╣\n"
        "║  Physics:                                                 ║\n"
        "║    ψ_{nlm}(r,θ,φ) = R_{nl}(r) · Y_l^m(θ,φ)              ║\n"
        "║    Probability density = |ψ|²  (shown as electron cloud) ║\n"
        "║    All in atomic units (a₀ = 1, ħ = mₑ = e = 1)         ║\n"
        "║    Energy: Eₙ = −13.6 eV / n²                            ║\n"
        "╚══════════════════════════════════════════════════════════╝\n\n";
}

// ============================================================================
//  main
// ============================================================================
int main()
{
    printHelp();

    // ---- GLFW initialisation -------------------------------------------
    if (!glfwInit()) {
        std::cerr << "Failed to initialise GLFW.\n";
        return 1;
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    g_window = glfwCreateWindow(g_width, g_height,
                                "Hydrogen Atom Simulator",
                                nullptr, nullptr);
    if (!g_window) {
        std::cerr << "Failed to create GLFW window.\n";
        glfwTerminate();
        return 1;
    }
    glfwMakeContextCurrent(g_window);
    glfwSwapInterval(0);  // disable vsync for FPS readout

    std::cout << "OpenGL " << glGetString(GL_VERSION) << "\n";
    std::cout << "GLSL   " << glGetString(GL_SHADING_LANGUAGE_VERSION) << "\n";

    // Callbacks
    glfwSetFramebufferSizeCallback(g_window, framebufferSizeCallback);
    glfwSetKeyCallback(g_window, keyCallback);
    glfwSetMouseButtonCallback(g_window, mouseButtonCallback);
    glfwSetCursorPosCallback(g_window, cursorPosCallback);
    glfwSetScrollCallback(g_window, scrollCallback);

    // ---- volume renderer -----------------------------------------------
    VolumeRenderer renderer;
    if (!renderer.init(128)) {
        std::cerr << "Failed to initialise volume renderer.\n";
        glfwDestroyWindow(g_window);
        glfwTerminate();
        return 1;
    }
    g_renderer = &renderer;

    // Load first orbital
    const auto &orb = ORBITALS[g_orbitalIndex];
    renderer.setOrbital(orb.n, orb.l, orb.m);

    // ---- main loop -----------------------------------------------------
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    g_lastFpsTime = glfwGetTime();

    while (!glfwWindowShouldClose(g_window)) {
        // Auto-rotate
        if (g_autoRotate > 0.0f) {
            // Use a fixed timestep assumption for rotation speed
            g_camera.azimuth += g_autoRotate * 0.016f;
        }

        // Update viewport
        glfwGetFramebufferSize(g_window, &g_width, &g_height);
        glViewport(0, 0, g_width, g_height);

        // Clear
        glClearColor(0.02f, 0.02f, 0.06f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Matrices
        float aspect = static_cast<float>(g_width) /
                       static_cast<float>(std::max(g_height, 1));
        glm::mat4 proj = glm::perspective(glm::radians(45.0f), aspect,
                                           0.1f, 200.0f);
        glm::mat4 view = g_camera.viewMatrix();
        glm::vec3 eye  = g_camera.eyePos();

        // Render orbital
        renderer.render(view, proj, eye);

        // Swap
        glfwSwapBuffers(g_window);
        glfwPollEvents();

        updateFPS();
    }

    // ---- cleanup -------------------------------------------------------
    g_renderer = nullptr;
    glfwDestroyWindow(g_window);
    glfwTerminate();
    return 0;
}
