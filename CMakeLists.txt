cmake_minimum_required(VERSION 3.16)
project(hydrogen_atom_sim VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Find dependencies
find_package(OpenGL REQUIRED)
find_package(glfw3 REQUIRED)
find_package(glm REQUIRED)

# Shader directory - copy shaders to build dir
file(COPY ${CMAKE_SOURCE_DIR}/shaders DESTINATION ${CMAKE_BINARY_DIR})

add_executable(hydrogen_sim
    src/main.cpp
    src/wavefunction.cpp
    src/shader.cpp
    src/volume_renderer.cpp
)

target_include_directories(hydrogen_sim PRIVATE
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(hydrogen_sim PRIVATE
    OpenGL::GL
    glfw
    glm::glm
)

# Set shader path as a compile definition for runtime loading
target_compile_definitions(hydrogen_sim PRIVATE
    SHADER_DIR="${CMAKE_BINARY_DIR}/shaders"
    GLFW_INCLUDE_GLCOREARB
    GL_GLEXT_PROTOTYPES
)

# Copy shaders on every build (in case they were edited)
add_custom_command(TARGET hydrogen_sim POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/shaders
    ${CMAKE_BINARY_DIR}/shaders
)
