#include "wavefunction.h"
#include "volume_renderer.h"
#include "atom_scene.h"
#include "element_data.h"
#include "text_renderer.h"
#include "energy_diagram.h"
#include "crystal_lattice.h"
#include "time_evolution.h"
#include "molecular.h"
#include "screenshot.h"
#include "async_compute.h"

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <vector>

// ============================================================================
//  Orbit camera
// ============================================================================
struct OrbitCamera {
    float azimuth   = 0.0f;
    float elevation = 0.4f;
    float distance  = 18.0f;
    glm::vec3 target{0, 0, 0};

    glm::mat4 viewMatrix() const {
        float cx = std::cos(azimuth), sx = std::sin(azimuth);
        float cy = std::cos(elevation), sy = std::sin(elevation);
        glm::vec3 eye(target.x + distance * cx * cy,
                       target.y + distance * sy,
                       target.z + distance * sx * cy);
        return glm::lookAt(eye, target, glm::vec3(0,1,0));
    }
    glm::vec3 eyePos() const {
        float cx = std::cos(azimuth), sx = std::sin(azimuth);
        float cy = std::cos(elevation), sy = std::sin(elevation);
        return glm::vec3(target.x + distance * cx * cy,
                          target.y + distance * sy,
                          target.z + distance * sx * cy);
    }
};

// ============================================================================
//  Periodic table layout (standard 18-column format)
// ============================================================================
struct Cell {
    int Z;         // 1–118, 0 = empty
    int col, row;  // grid position
};

// Standard periodic table grid: row, col -> Z (0 = empty)
// Layout follows IUPAC standard with lanthanides/actinides in separate rows
static const int PT_COLS = 18;
static const int PT_ROWS = 10;  // 7 main + 2 lanthanide/actinide + 1 gap

static Cell makeCell(int Z, int col, int row) { return {Z, col, row}; }

static const Cell PT_GRID[] = {
    // Row 1
    makeCell(1,0,0), makeCell(0,1,0),makeCell(0,2,0),makeCell(0,3,0),makeCell(0,4,0),makeCell(0,5,0),makeCell(0,6,0),makeCell(0,7,0),makeCell(0,8,0),makeCell(0,9,0),makeCell(0,10,0),makeCell(0,11,0),makeCell(0,12,0),makeCell(0,13,0),makeCell(0,14,0),makeCell(0,15,0),makeCell(0,16,0),makeCell(2,17,0),
    // Row 2
    makeCell(3,0,1), makeCell(4,1,1), makeCell(0,2,1),makeCell(0,3,1),makeCell(0,4,1),makeCell(0,5,1),makeCell(0,6,1),makeCell(0,7,1),makeCell(0,8,1),makeCell(0,9,1),makeCell(0,10,1),makeCell(0,11,1),makeCell(5,12,1),makeCell(6,13,1),makeCell(7,14,1),makeCell(8,15,1),makeCell(9,16,1),makeCell(10,17,1),
    // Row 3
    makeCell(11,0,2),makeCell(12,1,2),makeCell(0,2,2),makeCell(0,3,2),makeCell(0,4,2),makeCell(0,5,2),makeCell(0,6,2),makeCell(0,7,2),makeCell(0,8,2),makeCell(0,9,2),makeCell(0,10,2),makeCell(0,11,2),makeCell(13,12,2),makeCell(14,13,2),makeCell(15,14,2),makeCell(16,15,2),makeCell(17,16,2),makeCell(18,17,2),
    // Row 4 (full)
    makeCell(19,0,3),makeCell(20,1,3),makeCell(21,2,3),makeCell(22,3,3),makeCell(23,4,3),makeCell(24,5,3),makeCell(25,6,3),makeCell(26,7,3),makeCell(27,8,3),makeCell(28,9,3),makeCell(29,10,3),makeCell(30,11,3),makeCell(31,12,3),makeCell(32,13,3),makeCell(33,14,3),makeCell(34,15,3),makeCell(35,16,3),makeCell(36,17,3),
    // Row 5 (full)
    makeCell(37,0,4),makeCell(38,1,4),makeCell(39,2,4),makeCell(40,3,4),makeCell(41,4,4),makeCell(42,5,4),makeCell(43,6,4),makeCell(44,7,4),makeCell(45,8,4),makeCell(46,9,4),makeCell(47,10,4),makeCell(48,11,4),makeCell(49,12,4),makeCell(50,13,4),makeCell(51,14,4),makeCell(52,15,4),makeCell(53,16,4),makeCell(54,17,4),
    // Row 6
    makeCell(55,0,5),makeCell(56,1,5),makeCell(71,2,5),makeCell(72,3,5),makeCell(73,4,5),makeCell(74,5,5),makeCell(75,6,5),makeCell(76,7,5),makeCell(77,8,5),makeCell(78,9,5),makeCell(79,10,5),makeCell(80,11,5),makeCell(81,12,5),makeCell(82,13,5),makeCell(83,14,5),makeCell(84,15,5),makeCell(85,16,5),makeCell(86,17,5),
    // Row 7
    makeCell(87,0,6),makeCell(88,1,6),makeCell(103,2,6),makeCell(104,3,6),makeCell(105,4,6),makeCell(106,5,6),makeCell(107,6,6),makeCell(108,7,6),makeCell(109,8,6),makeCell(110,9,6),makeCell(111,10,6),makeCell(112,11,6),makeCell(113,12,6),makeCell(114,13,6),makeCell(115,14,6),makeCell(116,15,6),makeCell(117,16,6),makeCell(118,17,6),
    // Lanthanides row
    makeCell(0,0,8),makeCell(0,1,8),makeCell(57,2,8),makeCell(58,3,8),makeCell(59,4,8),makeCell(60,5,8),makeCell(61,6,8),makeCell(62,7,8),makeCell(63,8,8),makeCell(64,9,8),makeCell(65,10,8),makeCell(66,11,8),makeCell(67,12,8),makeCell(68,13,8),makeCell(69,14,8),makeCell(70,15,8),makeCell(0,16,8),makeCell(0,17,8),
    // Actinides row
    makeCell(0,0,9),makeCell(0,1,9),makeCell(89,2,9),makeCell(90,3,9),makeCell(91,4,9),makeCell(92,5,9),makeCell(93,6,9),makeCell(94,7,9),makeCell(95,8,9),makeCell(96,9,9),makeCell(97,10,9),makeCell(98,11,9),makeCell(99,12,9),makeCell(100,13,9),makeCell(101,14,9),makeCell(102,15,9),makeCell(0,16,9),makeCell(0,17,9),
};
static const int PT_CELL_COUNT = sizeof(PT_GRID) / sizeof(PT_GRID[0]);

// Font rendering removed — using terminal for text output


// Category colors
static void getCategoryColor(const char* cat, float& r, float& g, float& b) {
    // Simple color map
    if (strstr(cat, "Alkali"))        { r=1.0f; g=0.3f; b=0.3f; }
    else if (strstr(cat, "Alkaline"))  { r=1.0f; g=0.6f; b=0.2f; }
    else if (strstr(cat, "Transition")){ r=0.8f; g=0.7f; b=0.3f; }
    else if (strstr(cat, "Lanthanide")){ r=0.4f; g=0.8f; b=1.0f; }
    else if (strstr(cat, "Actinide"))  { r=1.0f; g=0.4f; b=0.8f; }
    else if (strstr(cat, "Post-Trans")){ r=0.5f; g=0.8f; b=0.5f; }
    else if (strstr(cat, "Metalloid")) { r=0.3f; g=0.9f; b=0.3f; }
    else if (strstr(cat, "Nonmetal"))  { r=0.3f; g=0.3f; b=0.9f; }
    else if (strstr(cat, "Halogen"))   { r=0.3f; g=0.9f; b=0.9f; }
    else if (strstr(cat, "Noble"))     { r=0.7f; g=0.3f; b=1.0f; }
    else { r=0.5f; g=0.5f; b=0.5f; }
}

// ============================================================================
//  Periodic table 2D overlay renderer
// ============================================================================
static GLuint pt_vao = 0, pt_vbo = 0, pt_shader = 0;
static GLint pt_u_mvp = -1, pt_u_color = -1;

static bool initPTShader() {
    constexpr int SHADER_LOG_SIZE = 512;
    const char* vert = R"(
#version 330 core
layout(location=0) in vec2 aPos;
uniform mat4 uMVP;
void main() { gl_Position = uMVP * vec4(aPos, 0.0, 1.0); }
)";
    const char* frag = R"(
#version 330 core
uniform vec3 uColor;
out vec4 fragColor;
void main() { fragColor = vec4(uColor, 1.0); }
)";
    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs, 1, &vert, nullptr); glCompileShader(vs);
    { GLint ok; glGetShaderiv(vs, GL_COMPILE_STATUS, &ok);
      if (!ok) { char buf[SHADER_LOG_SIZE]; glGetShaderInfoLog(vs, SHADER_LOG_SIZE, nullptr, buf);
                 std::cerr << "PT VS error: " << buf << "\n"; glDeleteShader(vs); return false; } }
    GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs, 1, &frag, nullptr); glCompileShader(fs);
    { GLint ok; glGetShaderiv(fs, GL_COMPILE_STATUS, &ok);
      if (!ok) { char buf[SHADER_LOG_SIZE]; glGetShaderInfoLog(fs, SHADER_LOG_SIZE, nullptr, buf);
                 std::cerr << "PT FS error: " << buf << "\n"; glDeleteShader(vs); glDeleteShader(fs); return false; } }
    pt_shader = glCreateProgram();
    glAttachShader(pt_shader, vs); glAttachShader(pt_shader, fs);
    glLinkProgram(pt_shader);
    glDeleteShader(vs); glDeleteShader(fs);
    { GLint ok; glGetProgramiv(pt_shader, GL_LINK_STATUS, &ok);
      if (!ok) { char buf[SHADER_LOG_SIZE]; glGetProgramInfoLog(pt_shader, SHADER_LOG_SIZE, nullptr, buf);
                 std::cerr << "PT shader link error: " << buf << "\n";
                 glDeleteProgram(pt_shader); pt_shader = 0; return false; } }
    pt_u_mvp = glGetUniformLocation(pt_shader, "uMVP");
    pt_u_color = glGetUniformLocation(pt_shader, "uColor");

    glGenVertexArrays(1, &pt_vao);
    glGenBuffers(1, &pt_vbo);
    return true;
}

static void drawPTQuad(float x, float y, float w, float h, float r, float g, float b,
                        int screenW, int screenH) {
    // Convert pixel coords to NDC (-1 to 1)
    float nx = (x / screenW) * 2.0f - 1.0f;
    float ny = 1.0f - (y / screenH) * 2.0f;  // flip Y
    float nw = (w / screenW) * 2.0f;
    float nh = -(h / screenH) * 2.0f;

    float verts[] = {nx,ny, nx+nw,ny, nx+nw,ny+nh, nx,ny, nx+nw,ny+nh, nx,ny+nh};
    glBindVertexArray(pt_vao);
    glBindBuffer(GL_ARRAY_BUFFER, pt_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
    glUseProgram(pt_shader);
    glm::mat4 mvp(1.0f);
    glUniformMatrix4fv(pt_u_mvp, 1, GL_FALSE, glm::value_ptr(mvp));
    glUniform3f(pt_u_color, r, g, b);
    glDrawArrays(GL_TRIANGLES, 0, 6);
}

// ============================================================================
//  Global state
// ============================================================================
static GLFWwindow *g_window = nullptr;
static OrbitCamera g_camera;
static AtomScene g_scene;
static int g_width = 1280, g_height = 720;
static float g_autoRotate = 0.3f;

static const ElementInfo* g_selectedElement = &ELEMENTS[1]; // hydrogen
static const ElementInfo* g_hoveredElement = nullptr;
static int g_orbitalIndex = 0;

// Mouse
static bool g_mousePressed = false;
static double g_lastMouseX = 0, g_lastMouseY = 0;
static double g_mouseX = 0, g_mouseY = 0;

// Periodic table panel
static bool g_showTable = true;
static bool g_started = false;  // welcome screen
static bool g_showControls = false;   // help/controls overlay
static const float PT_X = 10, PT_Y = 100, PT_CELL = 28, PT_GAP = 2;

// Lattic mode
static bool g_latticeMode = false;
static int g_latticeTypeIdx = 0;
static bool g_molecularMode = false;

// Time evolution
static bool g_timeEvolve = false;
static double g_simTime = 0.0;
static VolumeRenderer* g_timeRenderer = nullptr;  // dedicated renderer for animated orbital

static AsyncVolumeComputer* g_asyncComputer = nullptr;

// Screenshot requested from the key callback; taken before the next swap so
// the back buffer still holds a complete frame.
static bool g_screenshotPending = false;

// ============================================================================
//  Scene helpers
// ============================================================================

// Index into ORBITALS[] of an element's valence (outermost) orbital, m = 0.
static int valenceOrbitalIndex(const ElementInfo* e) {
    for (int i = 0; i < NUM_ORBITALS; ++i)
        if (ORBITALS[i].n == e->outermost_n && ORBITALS[i].l == e->outermost_l &&
            ORBITALS[i].m == 0)
            return i;
    return 0;
}

static void buildLattice() {
    g_scene.clear();
    auto positions = CrystalLattice::generate(
        CrystalLattice::currentType,
        CrystalLattice::defaultA(CrystalLattice::currentType),
        CrystalLattice::defaultN(CrystalLattice::currentType),
        CrystalLattice::defaultN(CrystalLattice::currentType),
        CrystalLattice::defaultN(CrystalLattice::currentType));
    g_scene.setOrbital(ORBITALS[g_orbitalIndex].n, ORBITALS[g_orbitalIndex].l, 0);
    float extent = 0.0f;
    for (auto& pos : positions) {
        g_scene.addAtom(g_selectedElement, pos);
        extent = std::max(extent, glm::length(pos));
    }
    // Pull the camera back so the whole crystal is in view.
    g_camera.distance = std::min(80.0f, std::max(18.0f, extent * 2.2f));
    printf("Lattice: %s (%zu atoms)\n",
           CrystalLattice::name(CrystalLattice::currentType), positions.size());
}

// Display the selected element in its valence orbital (single atom or lattice).
static void showSelectedElement() {
    g_orbitalIndex = valenceOrbitalIndex(g_selectedElement);
    if (g_latticeMode) { buildLattice(); return; }
    g_scene.clear();
    const auto& orb = ORBITALS[g_orbitalIndex];
    g_scene.setOrbital(orb.n, orb.l, orb.m);
    g_scene.addAtom(g_selectedElement, glm::vec3(0, 0, 0));
}

// Add an atom to the molecule, one bond length along +x from the last one,
// then re-centre the molecule on the origin.
static void addMoleculeAtom(const ElementInfo* e) {
    const int n = e->outermost_n;
    // Hydrogenic orbital radius scales as n^2 / Z_eff; H2 bond length is 1.4 a0.
    float bond = std::max(1.4f, 1.4f * n * n / static_cast<float>(e->Z_eff));
    glm::vec3 pos(0.0f);
    if (!MolecularMode::atoms.empty())
        pos = MolecularMode::atoms.back().position + glm::vec3(bond, 0, 0);
    MolecularMode::addAtom(e, pos, n, e->outermost_l, 0);

    glm::vec3 centroid(0.0f);
    for (auto& a : MolecularMode::atoms) centroid += a.position;
    centroid /= static_cast<float>(MolecularMode::atoms.size());
    for (auto& a : MolecularMode::atoms) a.position -= centroid;
}

// ============================================================================
//  Callbacks
// ============================================================================
static void framebufferSizeCallback(GLFWwindow*, int w, int h) { g_width=w; g_height=h; }

static void cursorPosCallback(GLFWwindow* win, double x, double y) {
    // Cursor positions are in screen coordinates, but the UI is laid out in
    // framebuffer pixels; these differ on HiDPI / Retina displays.
    int winW = 1, winH = 1;
    glfwGetWindowSize(win, &winW, &winH);
    if (winW > 0 && winH > 0) {
        x *= static_cast<double>(g_width)  / winW;
        y *= static_cast<double>(g_height) / winH;
    }
    g_mouseX = x; g_mouseY = y;
    float dx = (float)(x - g_lastMouseX), dy = (float)(y - g_lastMouseY);
    g_lastMouseX = x; g_lastMouseY = y;
    if (g_mousePressed) {
        g_camera.azimuth -= dx * 0.005f;
        g_camera.elevation += dy * 0.005f;
        float maxEl = (float)(M_PI/2.0 - 0.05);
        if (g_camera.elevation > maxEl) g_camera.elevation = maxEl;
        if (g_camera.elevation < -maxEl) g_camera.elevation = -maxEl;
    }

    // Hover detection for periodic table
    if (g_showTable) {
        g_hoveredElement = nullptr;
        float mx = (float)x, my = (float)y;
        for (int i = 0; i < PT_CELL_COUNT; ++i) {
            const Cell& c = PT_GRID[i];
            if (c.Z == 0) continue;
            float cx = PT_X + c.col * (PT_CELL + PT_GAP);
            float cy = PT_Y + c.row * (PT_CELL + PT_GAP);
            if (mx >= cx && mx <= cx+PT_CELL && my >= cy && my <= cy+PT_CELL) {
                g_hoveredElement = &ELEMENTS[c.Z];
                break;
            }
        }
    }
}

static void mouseButtonCallback(GLFWwindow*, int button, int action, int) {
    if (!g_started) { if (action == GLFW_PRESS) g_started = true; return; }
    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        g_mousePressed = (action == GLFW_PRESS);
        if (g_mousePressed) {
            g_lastMouseX = g_mouseX; g_lastMouseY = g_mouseY;
            // Check periodic table click — select and display the element
            if (g_hoveredElement) {
                g_mousePressed = false;   // a UI click is not a camera drag
                g_selectedElement = g_hoveredElement;
                std::cout << "Selected: " << g_selectedElement->symbol
                          << " (" << g_selectedElement->name << ") Z="
                          << g_selectedElement->Z << " Z_eff="
                          << g_selectedElement->Z_eff << "\n";
                if (!g_molecularMode && !g_timeEvolve) showSelectedElement();
                return;
            }
            // Check energy diagram click — select orbital
            int orbHit = EnergyDiagram::hitTest((float)g_mouseX, (float)g_mouseY,
                g_width, g_height, g_selectedElement->Z_eff,
                g_selectedElement->outermost_n);
            if (orbHit >= 0) {
                g_mousePressed = false;
                g_orbitalIndex = orbHit;
                g_scene.updateAllOrbitals(ORBITALS[g_orbitalIndex].n,
                                          ORBITALS[g_orbitalIndex].l,
                                          ORBITALS[g_orbitalIndex].m);
                std::cout << "Energy diagram: selected " << ORBITALS[g_orbitalIndex].label << "\n";
            }
        }
    }
}

static void scrollCallback(GLFWwindow*, double, double yoffset) {
    g_camera.distance *= std::pow(0.9f, (float)yoffset);
    if (g_camera.distance < 2.0f) g_camera.distance = 2.0f;
    if (g_camera.distance > 80.0f) g_camera.distance = 80.0f;
}

static void keyCallback(GLFWwindow*, int key, int, int action, int) {
    if (action == GLFW_PRESS) {
        if (!g_started) {
            if (key == GLFW_KEY_SPACE || key == GLFW_KEY_ENTER || key == GLFW_KEY_ESCAPE)
                g_started = true;
            return;
        }
        switch (key) {
        case GLFW_KEY_ESCAPE: glfwSetWindowShouldClose(g_window, GLFW_TRUE); break;
        case GLFW_KEY_TAB: g_showTable = !g_showTable; break;

        // Orbital selection — update all atoms when orbital changes
        case GLFW_KEY_RIGHT: case GLFW_KEY_D:
            g_orbitalIndex = (g_orbitalIndex + 1) % NUM_ORBITALS;
            g_scene.updateAllOrbitals(ORBITALS[g_orbitalIndex].n,
                                      ORBITALS[g_orbitalIndex].l,
                                      ORBITALS[g_orbitalIndex].m);
            break;
        case GLFW_KEY_LEFT: case GLFW_KEY_A:
            g_orbitalIndex = (g_orbitalIndex - 1 + NUM_ORBITALS) % NUM_ORBITALS;
            g_scene.updateAllOrbitals(ORBITALS[g_orbitalIndex].n,
                                      ORBITALS[g_orbitalIndex].l,
                                      ORBITALS[g_orbitalIndex].m);
            break;
        case GLFW_KEY_UP: case GLFW_KEY_W:
            { int curN = ORBITALS[g_orbitalIndex].n; int next=g_orbitalIndex;
              while(next<NUM_ORBITALS-1){++next;if(ORBITALS[next].n>curN)break;}
              if(ORBITALS[next].n>curN) g_orbitalIndex=next;
              g_scene.updateAllOrbitals(ORBITALS[g_orbitalIndex].n,
                                        ORBITALS[g_orbitalIndex].l,
                                        ORBITALS[g_orbitalIndex].m);
            } break;
        case GLFW_KEY_DOWN: case GLFW_KEY_S:
            { int curN=ORBITALS[g_orbitalIndex].n; int prev=g_orbitalIndex;
              while(prev>0){--prev;if(ORBITALS[prev].n<curN)break;}
              if(ORBITALS[prev].n<curN) g_orbitalIndex=prev;
              g_scene.updateAllOrbitals(ORBITALS[g_orbitalIndex].n,
                                        ORBITALS[g_orbitalIndex].l,
                                        ORBITALS[g_orbitalIndex].m);
            } break;
        case GLFW_KEY_1: case GLFW_KEY_2: case GLFW_KEY_3: case GLFW_KEY_4:
        case GLFW_KEY_5: case GLFW_KEY_6: case GLFW_KEY_7: case GLFW_KEY_8:
        case GLFW_KEY_9:
            { int idx=key-GLFW_KEY_1; if(idx<NUM_ORBITALS) {
                g_orbitalIndex=idx;
                g_scene.updateAllOrbitals(ORBITALS[g_orbitalIndex].n,
                                          ORBITALS[g_orbitalIndex].l,
                                          ORBITALS[g_orbitalIndex].m);
            }} break;

        // Place atom — replaces all, uses currently selected orbital
        case GLFW_KEY_SPACE: {
            g_scene.clear();
            int n = ORBITALS[g_orbitalIndex].n;
            int l = ORBITALS[g_orbitalIndex].l;
            int m = ORBITALS[g_orbitalIndex].m;
            g_scene.setOrbital(n, l, m);
            g_scene.addAtom(g_selectedElement, glm::vec3(0, 0, 0));
            break;
        }

        // Place atom at raycast hit point (approximate: place at camera target)
        // Place atom at camera target
        case GLFW_KEY_ENTER: {
            if (g_molecularMode) {
                addMoleculeAtom(g_selectedElement);
                printf("Added %s — %zu atoms in molecule\n",
                       g_selectedElement->symbol, MolecularMode::atoms.size());
            } else {
                int n = ORBITALS[g_orbitalIndex].n;
                int l = ORBITALS[g_orbitalIndex].l;
                int m = ORBITALS[g_orbitalIndex].m;
                g_scene.setOrbital(n, l, m);
                float offset = g_scene.count() * ORBITALS[g_orbitalIndex].n * 6.0f;
                g_scene.addAtom(g_selectedElement, g_camera.target + glm::vec3(offset, 0, 0));
            }
            break;
        }

        // Remove last atom
        case GLFW_KEY_BACKSPACE:
        case GLFW_KEY_DELETE:
            if (g_molecularMode)
                MolecularMode::removeLast();
            else
                g_scene.removeLast();
            break;

        // Clear all atoms
        case GLFW_KEY_C:
            if (g_molecularMode)
                MolecularMode::clear();
            else
                g_scene.clear();
            break;

        // Render settings
        case GLFW_KEY_EQUAL: case GLFW_KEY_KP_ADD:
            for (auto& a : const_cast<std::vector<PlacedAtom>&>(g_scene.atoms()))
                if (a.renderer)
                    a.renderer->setOpacityScale(a.renderer->opacityScale() * 1.2f);
            break;
        case GLFW_KEY_MINUS: case GLFW_KEY_KP_SUBTRACT:
            for (auto& a : const_cast<std::vector<PlacedAtom>&>(g_scene.atoms()))
                if (a.renderer)
                    a.renderer->setOpacityScale(a.renderer->opacityScale() / 1.2f);
            break;
        case GLFW_KEY_R: g_autoRotate = (g_autoRotate>0.01f)?0.0f:0.3f; break;

        // Energy diagram toggle
        case GLFW_KEY_E: EnergyDiagram::visible = !EnergyDiagram::visible; break;
        case GLFW_KEY_H: g_showControls = !g_showControls; break;

        // Lattice mode toggle (excludes molecular & time-evolve)
        case GLFW_KEY_L: {
            g_latticeMode = !g_latticeMode;
            if (g_latticeMode) {
                g_molecularMode = false;
                g_timeEvolve = false;
                MolecularMode::clear();
                buildLattice();
            } else {
                g_camera.distance = 18.0f;   // undo the lattice zoom-out
                showSelectedElement();
            }
            break;
        }

        // Molecular mode toggle
        case GLFW_KEY_M: {
            g_molecularMode = !g_molecularMode;
            if (g_molecularMode) {
                g_latticeMode = false;
                g_timeEvolve = false;
                g_camera.distance = 18.0f;
                g_scene.clear();
                MolecularMode::init();
                // Start with one atom of selected element at origin
                addMoleculeAtom(g_selectedElement);
                printf("Molecular mode ON — ENTER to add atoms, DEL to remove\n");
            } else {
                MolecularMode::clear();
                MolecularMode::shutdown();
                showSelectedElement();
                printf("Molecular mode OFF\n");
            }
            break;
        }

        // Cycle lattice type (when in lattice mode)
        case GLFW_KEY_K: {
            if (!g_latticeMode) break;
            g_latticeTypeIdx = (g_latticeTypeIdx + 1) % CrystalLattice::NUM_TYPES;
            CrystalLattice::currentType = static_cast<CrystalLattice::Type>(g_latticeTypeIdx);
            buildLattice();
            break;
        }

        // Time evolution toggle (excludes molecular & lattice)
        case GLFW_KEY_T: {
            g_timeEvolve = !g_timeEvolve;
            if (g_timeEvolve) {
                if (g_molecularMode) { MolecularMode::clear(); MolecularMode::shutdown(); }
                g_molecularMode = false;
                if (g_latticeMode) {
                    g_latticeMode = false;
                    g_camera.distance = 18.0f;
                    showSelectedElement();
                }
                TimeEvolution::reset();
                g_simTime = 0.0;
                if (!g_timeRenderer) {
                    g_timeRenderer = new VolumeRenderer();
                    g_timeRenderer->init(128);
                }
                printf("Time evolution ON — hydrogen 1s+2p beating  (T to stop)\n");
            } else {
                printf("Time evolution OFF\n");
            }
            break;
        }

        // Screenshot
        case GLFW_KEY_F12:
            g_screenshotPending = true;
            break;
        }
    }
}

// ============================================================================
//  FPS & title
// ============================================================================
static double g_lastFpsTime = 0;
static int g_frameCount = 0;

static void updateTitle() {
    g_frameCount++;
    double now = glfwGetTime();
    double elapsed = now - g_lastFpsTime;
    if (elapsed >= 0.5) {
        double fps = g_frameCount / elapsed;
        char title[512];
        const auto& orb = ORBITALS[g_orbitalIndex];
        std::snprintf(title, sizeof(title),
            "Atom Sim — %s (%s) [n=%d l=%d m=%d]  Atoms:%zu  "
            "Elem:%s  %s%s  FPS:%.0f",
            orb.label, orb.description, orb.n, orb.l, orb.m,
            g_scene.count(),
            g_selectedElement->symbol,
            g_latticeMode ? "[LATTICE] " : (g_molecularMode ? "[MOLECULE] " : ""),
            g_timeEvolve ? "[TIME-EVOLVE] " : "",
            fps);
        glfwSetWindowTitle(g_window, title);
        g_frameCount = 0;
        g_lastFpsTime = now;
    }
}

// ============================================================================
//  Periodic table rendering
// ============================================================================
static void renderPeriodicTable(int screenW, int screenH) {
    if (!g_showTable) return;

    glUseProgram(pt_shader);
    glm::mat4 mvp(1.0f);
    glUniformMatrix4fv(pt_u_mvp, 1, GL_FALSE, glm::value_ptr(mvp));

    for (int i = 0; i < PT_CELL_COUNT; ++i) {
        const Cell& c = PT_GRID[i];
        if (c.Z == 0) continue;
        const ElementInfo& el = ELEMENTS[c.Z];
        float cx = PT_X + c.col * (PT_CELL + PT_GAP);
        float cy = PT_Y + c.row * (PT_CELL + PT_GAP);

        float r, g, b;
        getCategoryColor(el.category, r, g, b);
        if (&el != g_selectedElement && &el != g_hoveredElement) {
            r *= 0.6f; g *= 0.6f; b *= 0.6f;
        }
        if (&el == g_hoveredElement) {
            r = std::min(1.0f, r*1.3f); g = std::min(1.0f, g*1.3f); b = std::min(1.0f, b*1.3f);
        }
        if (&el == g_selectedElement) { r = 1.0f; g = 1.0f; b = 0.3f; }

        drawPTQuad(cx, cy, PT_CELL, PT_CELL, r, g, b, screenW, screenH);

        // Element symbol text
        float tr=1.0f,tg=1.0f,tb=1.0f;
        if (&el==g_selectedElement){tr=0;tg=0;tb=0;}
        float tw=textWidth(el.symbol);
        drawText(el.symbol, cx+(PT_CELL-tw)*0.5f, cy+20, tr, tg, tb, screenW, screenH);
    }

    // Hover tooltip in title
    if (g_hoveredElement) {
        char title[512];
        std::snprintf(title, sizeof(title), "Hover: %s (%s) Z=%d Z_eff=%.2f  %s  [Click to select]",
            g_hoveredElement->symbol, g_hoveredElement->name,
            g_hoveredElement->Z, g_hoveredElement->Z_eff,
            g_hoveredElement->category);
        glfwSetWindowTitle(g_window, title);
    }
}

// ============================================================================
//  Welcome / startup screen
// ============================================================================
static const float WELCOME_FONT_SIZE = 18.0f;

static void renderWelcomeScreen(int screenW, int screenH) {
    glClearColor(0.02f, 0.02f, 0.08f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    float cy = screenH * 0.38f;
    const char* lines[] = {
        "ATOM WAVEFUNCTION SIMULATOR",
        "Explore electron clouds of all 118 elements",
        "using real quantum wavefunctions.",
        "",
        "Click periodic table to select an element",
        "SPACE to display  |  Arrows to change orbital",
        "Mouse drag to rotate  |  Scroll to zoom",
        "",
        "Press SPACE or click to begin",
    };
    int n = sizeof(lines)/sizeof(lines[0]);
    for (int i = 0; i < n; ++i) {
        float tw = textWidth(lines[i]);
        float x = (screenW - tw) * 0.5f;
        float y = cy + i * (WELCOME_FONT_SIZE + 4);
        float r=0.8f,g=0.8f,b=0.85f;
        if (i==0) { r=1.0f;g=0.8f;b=0.2f; }
        else if (i==8) {
            float pulse=(sin(glfwGetTime()*2.5f)+1.0f)*0.5f;
            r=0.2f+pulse*0.8f; g=0.9f; b=0.2f+pulse*0.8f;
        }
        drawText(lines[i], x, y, r, g, b, screenW, screenH);
    }
}

// ============================================================================
//  Controls / help panel overlay
// ============================================================================
static void renderControlsPanel(int screenW, int screenH) {
    // Semi-transparent dark background panel on the right side
    float panelW = 420.0f;
    float panelX = screenW - panelW - 20.0f;
    float panelY = 60.0f;
    float panelH = 520.0f;
    
    // Background rect (using 2D quad or just text)
    const char* title = "CONTROLS (H to hide)";
    float titleW = textWidth(title);
    drawText(title, panelX + (panelW - titleW) * 0.5f, panelY, 1.0f, 0.85f, 0.25f, screenW, screenH);
    
    float y = panelY + 32.0f;
    float lineH = 20.0f;
    float col1 = panelX + 10.0f;
    float col2 = panelX + 210.0f;
    
    struct { const char* key; const char* desc; } binds[] = {
        {"MOUSE DRAG", "Rotate view"},
        {"SCROLL", "Zoom in/out"},
        {"SPACE", "Place selected element"},
        {"ENTER", "Add atom at camera"},
        {"TAB", "Toggle periodic table"},
        {"H", "Toggle this panel"},
        {"E", "Toggle energy diagram"},
        {"L", "Toggle lattice mode"},
        {"K", "Cycle lattice type"},
        {"T", "Toggle time evolution"},
        {"R", "Toggle auto-rotate"},
        {"C", "Clear all atoms"},
        {"DEL / BKSP", "Remove last atom"},
        {"LEFT/RIGHT", "Change orbital"},
        {"UP/DOWN (W/S)", "Jump orbital shell"},
        {"1-9", "Direct orbital select"},
        {"+/-", "Adjust opacity"},
        {"F12", "Screenshot"},
        {"ESC", "Quit"},
    };
    
    for (auto& b : binds) {
        drawText(b.key, col1, y, 0.6f, 1.0f, 0.6f, screenW, screenH);
        drawText(b.desc, col2, y, 0.75f, 0.75f, 0.85f, screenW, screenH);
        y += lineH;
    }
    
    // Current state info at bottom
    y += 10.0f;
    char buf[256];
    snprintf(buf, sizeof(buf), "Element: %s (%s)  Z=%d",
             g_selectedElement->symbol, g_selectedElement->name, g_selectedElement->Z);
    drawText(buf, col1, y, 0.5f, 0.8f, 1.0f, screenW, screenH);
    y += lineH;
    snprintf(buf, sizeof(buf), "Atom count: %zu  Lattice: %s  Time: %s",
             g_scene.count(),
             g_latticeMode ? "ON" : "off",
             g_timeEvolve ? "ON" : "off");
    drawText(buf, col1, y, 0.5f, 0.8f, 1.0f, screenW, screenH);
}

// ============================================================================
//  --selftest: render every element and every mode, check for GL errors, save
//  a few screenshots, then exit (non-zero on failure).
// ============================================================================
static bool g_selfTest = false;
static int  g_selfTestFrame = 0;
static int  g_selfTestGLErrors = 0;
static double g_selfTestT0 = 0.0;

static void pressKey(int key) { keyCallback(g_window, key, 0, GLFW_PRESS, 0); }

// Called once per frame after rendering; returns false when the test is done.
static bool selfTestStep() {
    for (GLenum e = glGetError(); e != GL_NO_ERROR; e = glGetError()) {
        ++g_selfTestGLErrors;
        std::cerr << "selftest: GL error 0x" << std::hex << e << std::dec
                  << " at frame " << g_selfTestFrame << "\n";
    }
    const int f = g_selfTestFrame++;
    const int perElement = 2;
    const int elemFrames = NUM_ELEMENTS * perElement;
    if (f < elemFrames) {
        if (f % perElement == 0) {
            g_selectedElement = &ELEMENTS[f / perElement + 1];
            showSelectedElement();
            if (g_scene.count() != 1)
                { std::cerr << "selftest: " << g_selectedElement->symbol << " not placed\n"; ++g_selfTestGLErrors; }
        }
        const int Z = f / perElement + 1;
        if (f % perElement == 1 && (Z == 1 || Z == 6 || Z == 26 || Z == 92))
            Screenshot::takeScreenshot(g_width, g_height);
        return true;
    }
    // Then exercise each mode for a few frames.
    const int g = f - elemFrames;
    switch (g) {
    case 0:  g_selectedElement = &ELEMENTS[1]; showSelectedElement();
             pressKey(GLFW_KEY_E); pressKey(GLFW_KEY_H);
             g_selfTestT0 = glfwGetTime(); break;
    case 5:  std::cout << "selftest: single atom + overlays "
                       << (glfwGetTime() - g_selfTestT0) * 1000.0 / 5.0 << " ms/frame" << std::endl;
             Screenshot::takeScreenshot(g_width, g_height); pressKey(GLFW_KEY_E); pressKey(GLFW_KEY_H); break;
    case 6:  for (int i = 0; i < NUM_ORBITALS; ++i) pressKey(GLFW_KEY_RIGHT); break;  // all orbitals
    case 7:  pressKey(GLFW_KEY_L); break;                                     // lattice
    case 8:  for (int i = 1; i < CrystalLattice::NUM_TYPES; ++i) pressKey(GLFW_KEY_K); break;
    case 10: g_selfTestT0 = glfwGetTime(); break;             // time 20 lattice frames
    case 30: std::cout << "selftest: lattice (" << g_scene.count() << " atoms) "
                       << (glfwGetTime() - g_selfTestT0) * 1000.0 / 20.0 << " ms/frame" << std::endl;
             Screenshot::takeScreenshot(g_width, g_height); pressKey(GLFW_KEY_L); break;
    case 31: pressKey(GLFW_KEY_M); pressKey(GLFW_KEY_ENTER); break;           // molecule H2
    case 34: Screenshot::takeScreenshot(g_width, g_height); pressKey(GLFW_KEY_M); break;
    case 35: pressKey(GLFW_KEY_T); break;                                     // time evolution
    case 80: Screenshot::takeScreenshot(g_width, g_height); pressKey(GLFW_KEY_T); break;
    case 82: return false;
    default: break;
    }
    return true;
}

int main(int argc, char** argv) {
    for (int i = 1; i < argc; ++i)
        if (std::strcmp(argv[i], "--selftest") == 0) g_selfTest = true;

    std::cout << "\n=== Atom Wavefunction Simulator ===\n"
        "Periodic table: click element to select, SPACE to place atom\n"
        "Orbitals: ← → change, 1-9 direct, W/S jump n\n"
        "TAB: toggle periodic table  |  C: clear all  |  DEL: remove last\n\n";

    // ── Strategy: avoid GLX on Wayland/XWayland ──────────────────────────
    // GLX context creation on XWayland can leave glXGetCurrentDisplay() == NULL
    // causing GLEW to fail. We try in order:
    //   1. Native Wayland platform (EGL-based, no GLX)
    //   2. X11 platform with EGL context creation API
    //   3. Default (GLX fallback)

    bool glfwOk = false;

    // Strategy 1: native Wayland (GLFW 3.4+)
#if defined(GLFW_PLATFORM) && defined(GLFW_PLATFORM_WAYLAND) && defined(GLFW_PLATFORM_X11) && defined(GLFW_ANY_PLATFORM)
    glfwInitHint(GLFW_PLATFORM, GLFW_PLATFORM_WAYLAND);
    if (!glfwInit()) {
        // Strategy 2: X11 with EGL context
        glfwInitHint(GLFW_PLATFORM, GLFW_PLATFORM_X11);
        if (glfwInit()) {
            glfwWindowHint(GLFW_CONTEXT_CREATION_API, GLFW_EGL_CONTEXT_API);
            glfwOk = true;
        }
    } else {
        glfwOk = true;
    }

    if (!glfwOk) {
        // Strategy 3: default / GLX fallback
        glfwInitHint(GLFW_PLATFORM, GLFW_ANY_PLATFORM);
        if (!glfwInit()) { std::cerr << "GLFW init failed.\n"; return 1; }
    }
#else
    if (!glfwInit()) { std::cerr << "GLFW init failed.\n"; return 1; }
#endif

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    // macOS only provides a 3.2+ core context when forward-compatible is set.
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    g_window = glfwCreateWindow(g_width, g_height, "Atom Simulator", nullptr, nullptr);
    if (!g_window) { std::cerr << "Window creation failed.\n"; glfwTerminate(); return 1; }
    glfwMakeContextCurrent(g_window);
    glfwSwapInterval(1);

    glewExperimental = GL_TRUE;

    GLenum glewErr = glewInit();
    if (glewErr != GLEW_OK) {
        // On Wayland/EGL, glXGetCurrentDisplay() returns NULL since there's no
        // X11 display.  GLEW reports GLEW_ERROR_NO_GLX_DISPLAY (4) but all
        // core GL functions are already loaded and work fine.
        if (glewErr == GLEW_ERROR_NO_GLX_DISPLAY) {
            std::cerr << "GLEW: skipping GLX (not available on Wayland/EGL)\n";
        } else {
            std::cerr << "GLEW init failed: " << glewGetErrorString(glewErr) << "\n";
            glfwTerminate();
            return 1;
        }
    }

    glfwSetFramebufferSizeCallback(g_window, framebufferSizeCallback);
    glfwSetKeyCallback(g_window, keyCallback);
    glfwSetMouseButtonCallback(g_window, mouseButtonCallback);
    glfwSetCursorPosCallback(g_window, cursorPosCallback);
    glfwSetScrollCallback(g_window, scrollCallback);

    // Create async volume computer for background computation
    g_asyncComputer = new AsyncVolumeComputer();
    g_asyncComputer->setComputeFunc([](int res, double halfSz, std::vector<float>& data) {
        computeOrbital(ORBITALS[g_orbitalIndex].n, ORBITALS[g_orbitalIndex].l, 
                       ORBITALS[g_orbitalIndex].m, res, halfSz, 
                       g_selectedElement->Z_eff, data);
    });
    g_scene.setAsyncComputer(g_asyncComputer);

    // Init scene
    if (!g_scene.init(128)) { std::cerr << "Scene init failed.\n"; return 1; }

    // Init periodic table shader
    initPTShader();

    // Init font renderer from the first monospace system font found
    if (!initTextRendererFromSystem(18.0f))
        std::cerr << "No usable system font found — on-screen text disabled.\n";

    // Start with hydrogen 1s
    g_scene.setOrbital(ELEMENTS[1].outermost_n, ELEMENTS[1].outermost_l, 0);
    g_scene.addAtom(&ELEMENTS[1], glm::vec3(0,0,0));

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    if (g_selfTest) { g_started = true; glfwSwapInterval(0); }

    g_lastFpsTime = glfwGetTime();
    double lastFrameTime = glfwGetTime();

    while (!glfwWindowShouldClose(g_window)) {
        const double frameNow = glfwGetTime();
        const double frameDt  = std::min(frameNow - lastFrameTime, 0.1);
        lastFrameTime = frameNow;

        glfwGetFramebufferSize(g_window, &g_width, &g_height);
        glViewport(0, 0, g_width, g_height);
        VolumeRenderer::setViewportSize(g_width, g_height);

        // ── Welcome screen (before simulation starts) ──
        if (!g_started) {
            glClearColor(0.02f, 0.02f, 0.08f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);
            renderWelcomeScreen(g_width, g_height);
            glfwSwapBuffers(g_window);
            glfwPollEvents();
            continue;
        }

        if (g_autoRotate > 0.0f)
            g_camera.azimuth += g_autoRotate * static_cast<float>(frameDt);

        glClearColor(0.02f, 0.02f, 0.06f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        float aspect = (float)g_width / std::max(g_height, 1);
        glm::mat4 proj = glm::perspective(glm::radians(45.0f), aspect, 0.1f, 200.0f);
        glm::mat4 view = g_camera.viewMatrix();
        glm::vec3 eye = g_camera.eyePos();

        // ── Time evolution update ──
        if (g_timeEvolve && g_timeRenderer) {
            g_simTime += frameDt;
            int volRes = 128;
            double halfSz = suggestedHalfSize(
                std::max(TimeEvolution::currentState.n1, TimeEvolution::currentState.n2));
            std::vector<float> data;
            TimeEvolution::computeFrame(TimeEvolution::currentState, 1,
                                        g_simTime, volRes, halfSz, data);
            g_timeRenderer->uploadRawData(data, halfSz);
            g_timeRenderer->render(view, proj, eye);
        }

        // Render 3D atoms or molecular mode (time evolution replaces the scene)
        if (g_timeEvolve) {
            // already drawn above
        } else if (g_molecularMode) {
            double halfSz = 12.0;
            // Adapt half-size based on atom positions: cover all atoms plus orbital extent
            double maxR = 0.0;
            for (auto& a : MolecularMode::atoms) {
                double r = glm::length(a.position);
                if (r > maxR) maxR = r;
            }
            halfSz = maxR + suggestedHalfSize(g_selectedElement->outermost_n);
            if (halfSz < 8.0) halfSz = 8.0;
            MolecularMode::updateAndRender(view, proj, eye, 128, halfSz);
        } else {
            g_scene.render(view, proj, eye);
        }

        // Render periodic table overlay
        glDisable(GL_DEPTH_TEST);
        renderPeriodicTable(g_width, g_height);
        // Keep the energy diagram clear of the controls panel when both are open
        EnergyDiagram::rightMargin = g_showControls ? 460.0f : 10.0f;
        EnergyDiagram::render(g_width, g_height, g_selectedElement->Z_eff,
                              g_selectedElement->outermost_n);
        if (g_showControls) {
            renderControlsPanel(g_width, g_height);
        }
        glEnable(GL_DEPTH_TEST);

        if (g_selfTest && !selfTestStep())
            glfwSetWindowShouldClose(g_window, GLFW_TRUE);

        if (g_screenshotPending) {
            g_screenshotPending = false;
            Screenshot::takeScreenshot(g_width, g_height);
        }

        glfwSwapBuffers(g_window);
        glfwPollEvents();
        updateTitle();
    }

    if (g_selfTest)
        std::cout << "selftest: " << (g_selfTestGLErrors ? "FAILED" : "passed")
                  << " (" << g_selfTestGLErrors << " errors, "
                  << g_selfTestFrame << " frames)\n";

    glfwDestroyWindow(g_window);
    delete g_asyncComputer;
    glfwTerminate();
    return (g_selfTest && g_selfTestGLErrors) ? 2 : 0;
}
