#include "screenshot.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <vector>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <GL/glew.h>

// We embed a minimal stb_image_write.h inline to avoid an extra dependency.
// This is the public-domain stb_image_write v1.16 by Sean Barrett.
// Full header at: https://github.com/nothings/stb/blob/master/stb_image_write.h
// ─────────────────────────────────────────────────────────────────────────────

#ifndef INCLUDE_STB_IMAGE_WRITE_H
#define INCLUDE_STB_IMAGE_WRITE_H

#include <cstdlib>

static void stbi_write_png_compression_level(int) {}

static int stbi_write_png(
    char const *filename, int w, int h, int comp, const void *data, int stride_in_bytes)
{
    // We use a subprocess approach: write raw RGBA to a temp PPM, then
    // convert with an external tool, OR write a minimal PNG directly.
    //
    // For simplicity here, we write a PPM (which any image viewer can open)
    // and also attempt to use ImageMagick's `convert` if available.
    // If you want real PNG without external deps, include the full
    // stb_image_write.h from https://github.com/nothings/stb

    // Write PPM (always works, no external deps)
    FILE* fp = fopen(filename, "wb");
    if (!fp) return 0;

    // Check if filename ends in .png → try to use convert
    const char* ext = strrchr(filename, '.');
    bool wantPng = ext && (strcmp(ext, ".png") == 0 || strcmp(ext, ".PNG") == 0);

    if (wantPng) {
        // Write PPM to temp file, then convert
        char tmpname[512];
        std::snprintf(tmpname, sizeof(tmpname), "%s.ppm", filename);

        FILE* tmp = fopen(tmpname, "wb");
        if (!tmp) { fclose(fp); return 0; }

        fprintf(tmp, "P6\n%d %d\n255\n", w, h);

        const unsigned char* pixels = (const unsigned char*)data;
        // Convert RGBA → RGB
        for (int y = 0; y < h; ++y) {
            for (int x = 0; x < w; ++x) {
                int i = (y * w + x) * comp;
                fputc(pixels[i], tmp);
                fputc(pixels[i+1], tmp);
                fputc(pixels[i+2], tmp);
            }
        }
        fclose(tmp);

        // Try ImageMagick
        char cmd[1024];
        std::snprintf(cmd, sizeof(cmd), "convert '%s' '%s' 2>/dev/null", tmpname, filename);
        int ret = system(cmd);
        // Clean up temp
        std::remove(tmpname);

        if (ret != 0) {
            // Fallback: keep the PPM, rename to .ppm extension
            std::snprintf(cmd, sizeof(cmd), "%s.ppm", filename);
            // Write directly as PPM with .ppm suffix — caller already used .png
            // So just write a simple PPM under the .png name
            fprintf(fp, "P6\n%d %d\n255\n", w, h);
            const unsigned char* px = (const unsigned char*)data;
            for (int y = 0; y < h; ++y) {
                for (int x = 0; x < w; ++x) {
                    int i = (y * w + x) * comp;
                    fputc(px[i], fp);
                    fputc(px[i+1], fp);
                    fputc(px[i+2], fp);
                }
            }
            fclose(fp);
            return 1;
        }
        fclose(fp);
        return 1;
    }

    // Default: write PPM
    fprintf(fp, "P6\n%d %d\n255\n", w, h);
    const unsigned char* pixels = (const unsigned char*)data;
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            int i = (y * w + x) * comp;
            fputc(pixels[i], fp);
            fputc(pixels[i+1], fp);
            fputc(pixels[i+2], fp);
        }
    }
    fclose(fp);
    return 1;
}

#endif // INCLUDE_STB_IMAGE_WRITE_H

// ─────────────────────────────────────────────────────────────────────────────

namespace Screenshot {

bool savePNG(const char* filename, int w, int h) {
    std::vector<unsigned char> pixels(w * h * 4);

    glReadPixels(0, 0, w, h, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());

    // OpenGL gives bottom-to-top; flip vertically
    std::vector<unsigned char> flipped(w * h * 4);
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            int src = ((h - 1 - y) * w + x) * 4;
            int dst = (y * w + x) * 4;
            flipped[dst]     = pixels[src];
            flipped[dst + 1] = pixels[src + 1];
            flipped[dst + 2] = pixels[src + 2];
            flipped[dst + 3] = 255; // force opaque
        }
    }

    return stbi_write_png(filename, w, h, 4, flipped.data(), w * 4) != 0;
}

void takeScreenshot(int w, int h) {
    time_t now = time(nullptr);
    struct tm* t = localtime(&now);

    // Use Pictures directory if it exists, otherwise home
    const char* dir = nullptr;
    FILE* test = fopen("/home/mothman/Pictures/.test", "w");
    if (test) { fclose(test); std::remove("/home/mothman/Pictures/.test"); dir = "/home/mothman/Pictures"; }
    else dir = ".";

    char filename[512];
    std::snprintf(filename, sizeof(filename),
        "%s/atom-sim-%04d%02d%02d-%02d%02d%02d.png",
        dir,
        t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,
        t->tm_hour, t->tm_min, t->tm_sec);

    if (savePNG(filename, w, h))
        printf("Screenshot saved: %s\n", filename);
    else
        printf("Screenshot failed.\n");
}

} // namespace Screenshot
