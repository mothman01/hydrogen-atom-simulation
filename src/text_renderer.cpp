#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"
#include "text_renderer.h"

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

// ── Glyph atlas ──────────────────────────────────────────────────────────────

struct Glyph {
    float ax, ay;     // advance
    float tx0, ty0;   // texcoords
    float tx1, ty1;
    float w, h;       // size in pixels
    float ox, oy;     // offset from baseline
};

static Glyph g_glyphs[128];
static GLuint g_tex = 0;
static GLuint g_vao = 0, g_vbo = 0, g_shader = 0;
static GLint g_u_mvp = -1, g_u_tex = -1, g_u_color = -1;
static float g_fontSize = 24.0f;
static float g_scale = 1.0f;

// ── Shader ───────────────────────────────────────────────────────────────────

static bool buildShader() {
    const char* vs = R"(#version 330 core
layout(location=0) in vec2 aPos;
layout(location=1) in vec2 aTex;
uniform mat4 uMVP;
out vec2 vTex;
void main() { gl_Position = uMVP * vec4(aPos, 0.0, 1.0); vTex = aTex; }
)";
    const char* fs = R"(#version 330 core
in vec2 vTex;
uniform sampler2D uTex;
uniform vec3 uColor;
out vec4 fragColor;
void main() {
    float a = texture(uTex, vTex).r;
    fragColor = vec4(uColor, a);
}
)";
    GLuint vs_id = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs_id, 1, &vs, nullptr); glCompileShader(vs_id);
    GLuint fs_id = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs_id, 1, &fs, nullptr); glCompileShader(fs_id);

    // Check compile errors
    GLint ok;
    glGetShaderiv(vs_id, GL_COMPILE_STATUS, &ok);
    if (!ok) { char buf[512]; glGetShaderInfoLog(vs_id, 512, nullptr, buf); fprintf(stderr, "Text VS error: %s\n", buf); }
    glGetShaderiv(fs_id, GL_COMPILE_STATUS, &ok);
    if (!ok) { char buf[512]; glGetShaderInfoLog(fs_id, 512, nullptr, buf); fprintf(stderr, "Text FS error: %s\n", buf); }

    g_shader = glCreateProgram();
    glAttachShader(g_shader, vs_id); glAttachShader(g_shader, fs_id);
    glLinkProgram(g_shader);
    glDeleteShader(vs_id); glDeleteShader(fs_id);

    g_u_mvp   = glGetUniformLocation(g_shader, "uMVP");
    g_u_tex   = glGetUniformLocation(g_shader, "uTex");
    g_u_color = glGetUniformLocation(g_shader, "uColor");

    glGenVertexArrays(1, &g_vao);
    glGenBuffers(1, &g_vbo);
    return true;
}

// ── Init ─────────────────────────────────────────────────────────────────────

bool initTextRenderer(const char* fontPath, float fontSize) {
    g_fontSize = fontSize;

    // Read font file
    FILE* fp = fopen(fontPath, "rb");
    if (!fp) {
        fprintf(stderr, "Text renderer: cannot open font %s\n", fontPath);
        return false;
    }
    fseek(fp, 0, SEEK_END);
    long sz = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    unsigned char* ttf = (unsigned char*)malloc(sz);
    fread(ttf, 1, sz, fp);
    fclose(fp);

    // Init stb_truetype
    stbtt_fontinfo font;
    if (!stbtt_InitFont(&font, ttf, 0)) {
        fprintf(stderr, "Text renderer: stbtt_InitFont failed\n");
        free(ttf); return false;
    }

    g_scale = stbtt_ScaleForPixelHeight(&font, fontSize);

    // Build glyph atlas texture
    const int ATLAS_W = 512, ATLAS_H = 512;
    unsigned char* atlas = (unsigned char*)calloc(ATLAS_W * ATLAS_H, 1);
    int px = 1, py = 1, rowH = 0;

    int ascent, descent, lineGap;
    stbtt_GetFontVMetrics(&font, &ascent, &descent, &lineGap);

    for (int c = 32; c < 127; ++c) {
        int w, h, xoff, yoff;
        unsigned char* bmp = stbtt_GetCodepointBitmap(&font, 0, g_scale, c, &w, &h, &xoff, &yoff);

        if (px + w + 1 >= ATLAS_W) { px = 1; py += rowH + 1; rowH = 0; }

        // Skip glyphs that would overflow the atlas vertically
        if (py + h >= ATLAS_H) { stbtt_FreeBitmap(bmp, nullptr); continue; }

        for (int y = 0; y < h; ++y)
            for (int x = 0; x < w; ++x)
                atlas[(py + y) * ATLAS_W + (px + x)] = bmp[y * w + x];

        Glyph& g = g_glyphs[c];
        g.tx0 = (float)px / ATLAS_W;
        g.ty0 = (float)py / ATLAS_H;
        g.tx1 = (float)(px + w) / ATLAS_W;
        g.ty1 = (float)(py + h) / ATLAS_H;
        g.w = (float)w; g.h = (float)h;
        g.ox = (float)xoff; g.oy = (float)yoff;

        int adv, lsb;
        stbtt_GetCodepointHMetrics(&font, c, &adv, &lsb);
        g.ax = adv * g_scale;
        g.ay = 0;

        px += w + 1;
        if (h > rowH) rowH = h;
        stbtt_FreeBitmap(bmp, nullptr);
    }
    free(ttf);

    // Upload atlas texture
    glGenTextures(1, &g_tex);
    glBindTexture(GL_TEXTURE_2D, g_tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, ATLAS_W, ATLAS_H, 0, GL_RED, GL_UNSIGNED_BYTE, atlas);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    free(atlas);

    buildShader();
    fprintf(stderr, "Text renderer: loaded %s at %.0fpx\n", fontPath, fontSize);
    return true;
}

// ── Draw ─────────────────────────────────────────────────────────────────────

void drawText(const char* str, float x, float y,
              float r, float g, float b,
              int screenW, int screenH) {
    if (!g_shader || !g_tex) return;

    // Build vertex data for the string
    struct Vert { float px, py, u, v; };
    std::vector<Vert> verts;
    float cx = x, cy = y;

    for (const char* p = str; *p; ++p) {
        int c = (unsigned char)*p;
        if (c < 32 || c > 126) c = '?';
        const Glyph& gl = g_glyphs[c];

        float x0 = cx + gl.ox;
        float y0 = cy + gl.oy;
        float x1 = x0 + gl.w;
        float y1 = y0 + gl.h;

        verts.push_back({x0, y0, gl.tx0, gl.ty0});
        verts.push_back({x1, y0, gl.tx1, gl.ty0});
        verts.push_back({x1, y1, gl.tx1, gl.ty1});
        verts.push_back({x0, y0, gl.tx0, gl.ty0});
        verts.push_back({x1, y1, gl.tx1, gl.ty1});
        verts.push_back({x0, y1, gl.tx0, gl.ty1});

        cx += gl.ax;
    }

    if (verts.empty()) return;

    // Ortho projection: pixel coords -> NDC
    glm::mat4 mvp = glm::ortho(0.0f, (float)screenW, (float)screenH, 0.0f);

    glUseProgram(g_shader);
    glUniformMatrix4fv(g_u_mvp, 1, GL_FALSE, glm::value_ptr(mvp));
    glUniform3f(g_u_color, r, g, b);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, g_tex);
    glUniform1i(g_u_tex, 0);

    glBindVertexArray(g_vao);
    glBindBuffer(GL_ARRAY_BUFFER, g_vbo);
    glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(Vert), verts.data(), GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vert), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vert), (void*)(2*sizeof(float)));
    glDrawArrays(GL_TRIANGLES, 0, (GLsizei)verts.size());

    glBindVertexArray(0);
    glUseProgram(0);
}

float textWidth(const char* str) {
    float w = 0;
    for (const char* p = str; *p; ++p) {
        int c = (unsigned char)*p;
        if (c >= 32 && c <= 126)
            w += g_glyphs[c].ax;
    }
    return w;
}

void shutdownTextRenderer() {
    if (g_tex) glDeleteTextures(1, &g_tex);
    if (g_shader) glDeleteProgram(g_shader);
    if (g_vao) glDeleteVertexArrays(1, &g_vao);
    if (g_vbo) glDeleteBuffers(1, &g_vbo);
}
