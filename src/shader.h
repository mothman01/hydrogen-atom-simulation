#pragma once

#include <string>
#include <cstdint>

/**
 * Minimal OpenGL shader helper.
 *
 * Loads GLSL source from file, compiles, links, and reports errors.
 * All OpenGL functions are loaded via glad / the system GL loader.
 */

// Read entire file into a string.  Exits on failure.
std::string readShaderFile(const std::string &path);

// Compile a single shader stage.  Returns 0 on failure.
uint32_t compileShader(uint32_t type, const std::string &source);

// Link vertex + fragment shader into a program.  Returns 0 on failure.
uint32_t createShaderProgram(const std::string &vertSrc,
                             const std::string &fragSrc);

// Convenience: load + compile + link from two file paths.
uint32_t loadShaderProgram(const std::string &vertPath,
                           const std::string &fragPath);
