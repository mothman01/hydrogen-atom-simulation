#!/usr/bin/env python3
"""Generate a clean main.cpp with NO custom font rendering — all text goes to terminal."""
import os

path = "/home/mothman01/general_programs/quantum_particle_sim/src/main.cpp"

# Read the current file
with open(path) as f:
    content = f.read()

# Replace the welcome screen function
old_welcome = '''static void renderWelcomeScreen(int screenW, int screenH) {
    // Clean welcome — instructions printed to terminal, window shows visual prompt
    float cx = screenW * 0.5f, cy = screenH * 0.5f;

    // Large pulsing prompt rectangle
    float pulse = (sin(glfwGetTime() * 2.5f) + 1.0f) * 0.5f;
    float r = 0.15f + pulse * 0.3f;
    float g = 0.5f + pulse * 0.5f;
    float b = 0.15f + pulse * 0.3f;

    float bw = screenW * 0.55f, bh = screenH * 0.08f;
    drawPTQuad(cx - bw*0.5f, cy - bh*0.5f, bw, bh, r, g, b, screenW, screenH);

    // Top decorative line
    drawPTQuad(cx - bw*0.6f, cy - bh*0.5f - bh*3.5f, bw*1.2f, bh*0.15f,
               0.9f, 0.7f, 0.2f, screenW, screenH);
    // Bottom decorative line
    drawPTQuad(cx - bw*0.6f, cy - bh*0.5f + bh*2.0f, bw*1.2f, bh*0.15f,
               0.9f, 0.7f, 0.2f, screenW, screenH);
}'''

new_welcome = '''static void renderWelcomeScreen(int screenW, int screenH) {
    // Clean animated welcome — no custom font, just colored bars
    float cx = screenW * 0.5f, cy = screenH * 0.5f;
    float pulse = (sin(glfwGetTime() * 2.5f) + 1.0f) * 0.5f;
    float r = 0.15f + pulse * 0.35f;
    float g = 0.5f + pulse * 0.5f;
    float b = 0.15f + pulse * 0.35f;
    float bw = screenW * 0.55f, bh = screenH * 0.08f;

    // Large pulsing "press to continue" bar
    drawPTQuad(cx - bw*0.5f, cy - bh*0.5f, bw, bh, r, g, b, screenW, screenH);

    // Decorative gold bars (title area)
    drawPTQuad(cx - bw*0.6f, cy - bh*0.5f - bh*3.5f, bw*1.2f, bh*0.15f,
               0.95f, 0.7f, 0.15f, screenW, screenH);
    drawPTQuad(cx - bw*0.6f, cy - bh*0.5f + bh*2.2f, bw*1.2f, bh*0.15f,
               0.95f, 0.7f, 0.15f, screenW, screenH);

    // Subtitle bar
    drawPTQuad(cx - bw*0.4f, cy - bh*0.5f - bh*1.8f, bw*0.8f, bh*0.12f,
               0.4f, 0.65f, 0.9f, screenW, screenH);
}'''

content = content.replace(old_welcome, new_welcome)

# Update the printHelp or main function to print welcome instructions
old_main_start = '''    std::cout << "\\n=== Atom Wavefunction Simulator ===\\n"
        "Periodic table: click element to select, SPACE to place atom\\n"
        "Orbitals: \\xe2\\x86\\x90 \\xe2\\x86\\x92 change, 1-9 direct, W/S jump n\\n"
        "TAB: toggle periodic table  |  C: clear all  |  DEL: remove last\\n\\n";'''

new_main_start = '''    std::cout << "\\n"
        "╔══════════════════════════════════════════════════════════╗\\n"
        "║     ATOM WAVEFUNCTION SIMULATOR                         ║\\n"
        "╠══════════════════════════════════════════════════════════╣\\n"
        "║  Explore electron clouds of all 118 elements.           ║\\n"
        "║  Hydrogenic wavefunctions with effective nuclear charge. ║\\n"
        "╠══════════════════════════════════════════════════════════╣\\n"
        "║  CONTROLS:                                               ║\\n"
        "║    Click cell in periodic table = Select element         ║\\n"
        "║    SPACE                       = Display element          ║\\n"
        "║    LEFT/RIGHT arrows           = Change orbital           ║\\n"
        "║    1-9 keys                    = Direct orbital select    ║\\n"
        "║    Mouse drag                  = Rotate view              ║\\n"
        "║    Scroll                      = Zoom                     ║\\n"
        "║    TAB                         = Toggle periodic table    ║\\n"
        "║    R                           = Auto-rotate              ║\\n"
        "║    ESC                         = Quit                     ║\\n"
        "╚══════════════════════════════════════════════════════════╝\\n\\n";'''

content = content.replace(old_main_start, new_main_start)

with open(path, 'w') as f:
    f.write(content)

print("Patched successfully.")
