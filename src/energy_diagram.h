#pragma once

#include <glm/glm.hpp>

/**
 * Energy-level diagram overlay renderer.
 *
 * Displays hydrogenic energy levels  E_n = -13.6 * Z_eff² / n²  for n=1..6,
 * colour-coded by angular momentum (s, p, d, f, g).
 *
 * Usage:
 *   EnergyDiagram::render(screenW, screenH, Z_eff, nMax);
 */
namespace EnergyDiagram {

// Render an energy-level ladder on the right side of the screen.
//   screenW, screenH — current framebuffer size
//   Z_eff           — effective nuclear charge (scales energy axis)
//   nMax            — highest principal quantum number to show (1-7)
void render(int screenW, int screenH, double Z_eff, int nMax);

// Toggle visibility
extern bool visible;
extern float panelX, panelY;  // top-left corner in pixels
extern float panelW, panelH;

// Hit-test: returns orbital index (into ORBITALS[]) or -1 if no hit
int hitTest(float mouseX, float mouseY, int screenW, int screenH,
            double Z_eff, int nMax);

} // namespace EnergyDiagram
