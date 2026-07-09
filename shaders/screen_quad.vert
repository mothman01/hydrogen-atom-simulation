#version 330 core

// Full-screen quad vertex shader.
// A single giant triangle covers the entire viewport.

layout(location = 0) in vec2 aPos;
layout(location = 1) in vec2 aTexCoord;

out vec2 vTexCoord;

void main() {
    gl_Position = vec4(aPos, 0.0, 1.0);
    vTexCoord   = aTexCoord;
}
