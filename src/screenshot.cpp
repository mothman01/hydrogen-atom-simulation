#include "screenshot.h"

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <string>
#include <vector>

#include <GL/glew.h>

// ─────────────────────────────────────────────────────────────────────────────
//  Minimal dependency-free PNG writer.
//
//  Uses zlib "stored" (uncompressed) deflate blocks, which every PNG decoder
//  accepts.  Files are larger than compressed PNGs but need no extra library
//  and no external tools.
// ─────────────────────────────────────────────────────────────────────────────
namespace {

uint32_t crcTable[256];
bool     crcReady = false;

uint32_t crc32(const unsigned char* data, std::size_t len, uint32_t crc = 0)
{
    if (!crcReady) {
        for (uint32_t n = 0; n < 256; ++n) {
            uint32_t c = n;
            for (int k = 0; k < 8; ++k)
                c = (c & 1) ? 0xEDB88320u ^ (c >> 1) : c >> 1;
            crcTable[n] = c;
        }
        crcReady = true;
    }
    crc = ~crc;
    for (std::size_t i = 0; i < len; ++i)
        crc = crcTable[(crc ^ data[i]) & 0xFF] ^ (crc >> 8);
    return ~crc;
}

void putU32(std::vector<unsigned char>& out, uint32_t v)
{
    out.push_back(static_cast<unsigned char>(v >> 24));
    out.push_back(static_cast<unsigned char>(v >> 16));
    out.push_back(static_cast<unsigned char>(v >> 8));
    out.push_back(static_cast<unsigned char>(v));
}

void writeChunk(FILE* fp, const char type[4], const std::vector<unsigned char>& body)
{
    std::vector<unsigned char> buf;
    putU32(buf, static_cast<uint32_t>(body.size()));
    buf.insert(buf.end(), type, type + 4);
    buf.insert(buf.end(), body.begin(), body.end());
    uint32_t crc = crc32(buf.data() + 4, buf.size() - 4);
    putU32(buf, crc);
    fwrite(buf.data(), 1, buf.size(), fp);
}

// rgb: tightly packed top-to-bottom RGB rows.
bool writePNG(const char* filename, int w, int h, const unsigned char* rgb)
{
    FILE* fp = std::fopen(filename, "wb");
    if (!fp) return false;

    static const unsigned char sig[8] = {137, 80, 78, 71, 13, 10, 26, 10};
    fwrite(sig, 1, 8, fp);

    std::vector<unsigned char> ihdr;
    putU32(ihdr, static_cast<uint32_t>(w));
    putU32(ihdr, static_cast<uint32_t>(h));
    ihdr.insert(ihdr.end(), {8, 2, 0, 0, 0});   // 8-bit RGB, no interlace
    writeChunk(fp, "IHDR", ihdr);

    // Raw scanlines, each prefixed with filter type 0.
    const std::size_t rowBytes = static_cast<std::size_t>(w) * 3;
    std::vector<unsigned char> raw;
    raw.reserve((rowBytes + 1) * static_cast<std::size_t>(h));
    for (int y = 0; y < h; ++y) {
        raw.push_back(0);
        raw.insert(raw.end(), rgb + y * rowBytes, rgb + (y + 1) * rowBytes);
    }

    // zlib stream of stored blocks (max 65535 bytes each) + Adler-32.
    std::vector<unsigned char> z = {0x78, 0x01};
    uint32_t a = 1, b = 0;
    for (unsigned char c : raw) { a = (a + c) % 65521; b = (b + a) % 65521; }
    std::size_t pos = 0;
    do {
        const std::size_t len = std::min<std::size_t>(65535, raw.size() - pos);
        const bool last = pos + len == raw.size();
        z.push_back(last ? 1 : 0);
        z.push_back(static_cast<unsigned char>(len));
        z.push_back(static_cast<unsigned char>(len >> 8));
        z.push_back(static_cast<unsigned char>(~len));
        z.push_back(static_cast<unsigned char>(~len >> 8));
        z.insert(z.end(), raw.begin() + pos, raw.begin() + pos + len);
        pos += len;
    } while (pos < raw.size());
    putU32(z, (b << 16) | a);
    writeChunk(fp, "IDAT", z);

    writeChunk(fp, "IEND", {});
    const bool ok = std::ferror(fp) == 0;
    std::fclose(fp);
    return ok;
}

// ~/Pictures (or %USERPROFILE%\Pictures) if it exists, else the current dir.
std::string pictureDir()
{
#ifdef _WIN32
    const char* home = std::getenv("USERPROFILE");
#else
    const char* home = std::getenv("HOME");
#endif
    if (home && *home) {
        std::string dir = std::string(home) + "/Pictures";
        std::string probe = dir + "/.atom-sim-write-test";
        if (FILE* t = std::fopen(probe.c_str(), "wb")) {
            std::fclose(t);
            std::remove(probe.c_str());
            return dir;
        }
    }
    return ".";
}

} // namespace

namespace Screenshot {

bool savePNG(const char* filename, int w, int h) {
    if (w <= 0 || h <= 0) return false;
    const std::size_t W = static_cast<std::size_t>(w), H = static_cast<std::size_t>(h);
    std::vector<unsigned char> pixels(W * H * 3);

    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadBuffer(GL_BACK);
    glReadPixels(0, 0, w, h, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());

    // OpenGL gives bottom-to-top; flip vertically
    std::vector<unsigned char> flipped(W * H * 3);
    for (std::size_t y = 0; y < H; ++y)
        std::memcpy(&flipped[y * W * 3], &pixels[(H - 1 - y) * W * 3], W * 3);

    return writePNG(filename, w, h, flipped.data());
}

void takeScreenshot(int w, int h) {
    time_t now = time(nullptr);
    struct tm* t = localtime(&now);

    char filename[1024];
    const std::string dir = pictureDir();
    // Add a -N suffix so several shots within one second don't overwrite.
    for (int n = 0; n < 1000; ++n) {
        std::snprintf(filename, sizeof(filename),
            n == 0 ? "%s/atom-sim-%04d%02d%02d-%02d%02d%02d.png"
                   : "%s/atom-sim-%04d%02d%02d-%02d%02d%02d-%d.png",
            dir.c_str(),
            t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,
            t->tm_hour, t->tm_min, t->tm_sec, n);
        FILE* existing = std::fopen(filename, "rb");
        if (!existing) break;
        std::fclose(existing);
    }

    if (savePNG(filename, w, h))
        printf("Screenshot saved: %s\n", filename);
    else
        printf("Screenshot failed.\n");
}

} // namespace Screenshot
