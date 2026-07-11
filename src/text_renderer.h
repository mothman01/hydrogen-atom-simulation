#pragma once
#include <string>

// Initialize the text renderer. Call once after OpenGL context is created.
// fontPath: path to a .ttf file (e.g. /usr/share/fonts/.../SomeFont.ttf)
// fontSize: pixel height of rendered glyphs
bool initTextRenderer(const char* fontPath, float fontSize);

// Draw a string at pixel coordinates (x, y) with given color.
// (0,0) = top-left of the window.
// screenW/screenH = current framebuffer size.
void drawText(const char* str, float x, float y,
              float r, float g, float b,
              int screenW, int screenH);

// Get the pixel width of a string (for centering calculations).
float textWidth(const char* str);

// Clean up GL resources.
void shutdownTextRenderer();
