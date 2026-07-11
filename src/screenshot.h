#pragma once

/**
 * Framebuffer screenshot / export.
 *
 * Saves the current OpenGL framebuffer to a PNG file using stb_image_write.
 * Also supports setting up an off-screen FBO for higher-resolution exports.
 */
namespace Screenshot {

// Save current default framebuffer to a PNG file.
//   filename — output path (e.g. "orbital_3d.png")
//   w, h     — framebuffer size in pixels
// Returns true on success.
bool savePNG(const char* filename, int w, int h);

// Take a screenshot with timestamped filename.
// Saves to ~/Pictures/atom-sim-YYYYMMDD-HHMMSS.png
void takeScreenshot(int w, int h);

} // namespace Screenshot
