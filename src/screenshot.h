#pragma once

/**
 * Framebuffer screenshot / export.
 *
 * Saves the current OpenGL back buffer to a PNG file (built-in encoder,
 * no external libraries or tools required).
 */
namespace Screenshot {

// Save current default framebuffer to a PNG file.
//   filename — output path (e.g. "orbital_3d.png")
//   w, h     — framebuffer size in pixels
// Returns true on success.
bool savePNG(const char* filename, int w, int h);

// Take a screenshot with timestamped filename.
// Saves to ~/Pictures/atom-sim-YYYYMMDD-HHMMSS.png (or the current directory)
void takeScreenshot(int w, int h);

} // namespace Screenshot
