#include "shader.h"

#include <fstream>
#include <sstream>
#include <iostream>
#include <stdexcept>

#include <GL/glcorearb.h>
#include <GLFW/glfw3.h>

// We link against system GL; the GL headers provide function declarations.
// glad/glew is not strictly necessary when linking directly, but we keep
// the pattern in case extensions are needed later.

static void checkGLError(const char *context)
{
    GLenum err;
    while ((err = glGetError()) != GL_NO_ERROR) {
        std::cerr << "OpenGL error (" << context << "): 0x"
                  << std::hex << err << std::dec << "\n";
    }
}

// ---------------------------------------------------------------------------
std::string readShaderFile(const std::string &path)
{
    std::ifstream file(path);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open shader file: " + path);
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

// ---------------------------------------------------------------------------
uint32_t compileShader(uint32_t type, const std::string &source)
{
    uint32_t shader = glCreateShader(type);
    const char *src = source.c_str();
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);

    GLint success = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[1024];
        glGetShaderInfoLog(shader, sizeof(infoLog), nullptr, infoLog);
        std::cerr << "Shader compilation failed:\n" << infoLog << "\n";
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

// ---------------------------------------------------------------------------
uint32_t createShaderProgram(const std::string &vertSrc,
                             const std::string &fragSrc)
{
    uint32_t vert = compileShader(GL_VERTEX_SHADER, vertSrc);
    uint32_t frag = compileShader(GL_FRAGMENT_SHADER, fragSrc);
    if (vert == 0 || frag == 0) {
        if (vert) glDeleteShader(vert);
        if (frag) glDeleteShader(frag);
        return 0;
    }

    uint32_t prog = glCreateProgram();
    glAttachShader(prog, vert);
    glAttachShader(prog, frag);
    glLinkProgram(prog);

    GLint success = 0;
    glGetProgramiv(prog, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[1024];
        glGetProgramInfoLog(prog, sizeof(infoLog), nullptr, infoLog);
        std::cerr << "Shader linking failed:\n" << infoLog << "\n";
        glDeleteProgram(prog);
        prog = 0;
    }

    // Shaders are no longer needed after linking
    glDetachShader(prog, vert);
    glDetachShader(prog, frag);
    glDeleteShader(vert);
    glDeleteShader(frag);

    checkGLError("createShaderProgram");
    return prog;
}

// ---------------------------------------------------------------------------
uint32_t loadShaderProgram(const std::string &vertPath,
                           const std::string &fragPath)
{
    return createShaderProgram(readShaderFile(vertPath),
                               readShaderFile(fragPath));
}
