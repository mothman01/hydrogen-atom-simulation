cmake_minimum_required(VERSION 3.16)
project(atom_sim VERSION 2.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Default to an optimised build; without this a plain `cmake -B build`
# compiles with no optimisation and the orbital math runs many times slower.
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# ── Options ──────────────────────────────────────────────────────────────
option(BUILD_INSTALLER "Create install target" ON)
option(BUILD_TESTING   "Build the headless physics tests" ON)
# Portable mode is what the release downloads use: GLFW/GLEW/GLM are built
# from source and linked statically (plus the C/C++ runtime where possible),
# so the result runs on a machine with nothing extra installed.
option(ATOM_SIM_PORTABLE "Self-contained build for distribution" OFF)

if(ATOM_SIM_PORTABLE AND MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

# ── Dependencies ─────────────────────────────────────────────────────────
# Use system packages when available (and not in portable mode); otherwise
# fetch and build them, so `cmake -B build` works with only a compiler.
include(FetchContent)
find_package(OpenGL REQUIRED)
find_package(Threads REQUIRED)

if(NOT ATOM_SIM_PORTABLE)
    find_package(glfw3 3.3 QUIET)
    find_package(glm QUIET)
    find_package(GLEW QUIET)
endif()

if(NOT TARGET glfw)
    message(STATUS "Building GLFW from source")
    set(GLFW_BUILD_DOCS     OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_TESTS    OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(GLFW_INSTALL        OFF CACHE BOOL "" FORCE)
    set(BUILD_SHARED_LIBS   OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(glfw
        GIT_REPOSITORY https://github.com/glfw/glfw.git
        GIT_TAG        3.4
        GIT_SHALLOW    TRUE)
    FetchContent_MakeAvailable(glfw)
endif()

if(NOT TARGET glm::glm)
    message(STATUS "Fetching GLM")
    FetchContent_Declare(glm
        GIT_REPOSITORY https://github.com/g-truc/glm.git
        GIT_TAG        1.0.1
        GIT_SHALLOW    TRUE)
    FetchContent_MakeAvailable(glm)
endif()

if(TARGET GLEW::GLEW)
    set(ATOM_SIM_GLEW GLEW::GLEW)
else()
    message(STATUS "Building GLEW from source")
    set(glew-cmake_BUILD_SHARED OFF CACHE BOOL "" FORCE)
    set(glew-cmake_BUILD_STATIC ON  CACHE BOOL "" FORCE)
    set(ONLY_LIBS               ON  CACHE BOOL "" FORCE)
    # glew-cmake declares cmake_minimum_required(2.8); CMake 4 needs a floor.
    set(CMAKE_POLICY_VERSION_MINIMUM 3.5)
    if(APPLE)
        # glew-cmake links the AGL framework, which current macOS SDKs no
        # longer ship; point its lookup at the OpenGL framework instead.
        set(AGL_LIBRARY "${OPENGL_gl_LIBRARY}" CACHE FILEPATH "" FORCE)
    endif()
    FetchContent_Declare(glew
        GIT_REPOSITORY https://github.com/Perlmint/glew-cmake.git
        GIT_TAG        glew-cmake-2.2.0
        GIT_SHALLOW    TRUE)
    FetchContent_MakeAvailable(glew)
    set(ATOM_SIM_GLEW libglew_static)
endif()

# ── Embedded shaders ─────────────────────────────────────────────────────
# Shaders are compiled into the binary so the app never depends on the
# working directory it was launched from.
set(SHADER_FILES
    ${CMAKE_SOURCE_DIR}/shaders/screen_quad.vert
    ${CMAKE_SOURCE_DIR}/shaders/raymarch.frag)
file(READ ${CMAKE_SOURCE_DIR}/shaders/screen_quad.vert SCREEN_QUAD_VERT_SRC)
file(READ ${CMAKE_SOURCE_DIR}/shaders/raymarch.frag    RAYMARCH_FRAG_SRC)
configure_file(src/embedded_shaders.h.in
    ${CMAKE_BINARY_DIR}/generated/embedded_shaders.h @ONLY)
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${SHADER_FILES})

# ── Physics library (no OpenGL) ──────────────────────────────────────────
add_library(atom_physics STATIC
    src/wavefunction.cpp
    src/element_data.cpp
    src/time_evolution.cpp
)
target_include_directories(atom_physics PUBLIC src)
target_link_libraries(atom_physics PUBLIC glm::glm Threads::Threads)
if(WIN32)
    target_compile_definitions(atom_physics PUBLIC _USE_MATH_DEFINES NOMINMAX)
endif()

# ── Application ──────────────────────────────────────────────────────────
add_executable(atom_sim
    src/main.cpp
    src/shader.cpp
    src/volume_renderer.cpp
    src/atom_scene.cpp
    src/text_renderer.cpp
    src/energy_diagram.cpp
    src/crystal_lattice.cpp
    src/molecular.cpp
    src/async_compute.cpp
    src/screenshot.cpp
)

target_include_directories(atom_sim PRIVATE src ${CMAKE_BINARY_DIR}/generated)

target_link_libraries(atom_sim PRIVATE
    atom_physics
    ${ATOM_SIM_GLEW}
    glfw
    OpenGL::GL
)

# ── Platform-specific ────────────────────────────────────────────────────
if(WIN32)
    if(ATOM_SIM_PORTABLE)
        # No console window when double-clicked; keep the standard main().
        set_target_properties(atom_sim PROPERTIES WIN32_EXECUTABLE TRUE)
        if(MSVC)
            target_link_options(atom_sim PRIVATE /ENTRY:mainCRTStartup)
        else()
            target_link_options(atom_sim PRIVATE -static)
        endif()
    endif()
elseif(APPLE)
    target_compile_definitions(atom_sim PRIVATE GL_SILENCE_DEPRECATION)
    set_target_properties(atom_sim PROPERTIES
        OUTPUT_NAME "AtomSim"
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_GUI_IDENTIFIER "com.atomsim.app"
        MACOSX_BUNDLE_BUNDLE_NAME "Atom Wavefunction Simulator"
        MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
        MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION}
    )
else()
    # GLEW_NO_GLX: the context may come from EGL (Wayland), see main.cpp.
    target_compile_definitions(atom_sim PRIVATE GLEW_NO_GLX)
    if(ATOM_SIM_PORTABLE)
        target_link_options(atom_sim PRIVATE -static-libstdc++ -static-libgcc)
    endif()
endif()

# ── Tests ────────────────────────────────────────────────────────────────
if(BUILD_TESTING)
    enable_testing()
    add_executable(atom_sim_tests tests/test_physics.cpp)
    target_link_libraries(atom_sim_tests PRIVATE atom_physics)
    add_test(NAME physics COMMAND atom_sim_tests)
endif()

# ── Install ──────────────────────────────────────────────────────────────
if(BUILD_INSTALLER)
    include(GNUInstallDirs)

    install(TARGETS atom_sim
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        BUNDLE DESTINATION .
    )

    # Linux desktop entry
    if(UNIX AND NOT APPLE)
        install(FILES assets/atom-sim.desktop
            DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/applications
        )
        install(FILES assets/atom-sim.svg
            DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/scalable/apps
            OPTIONAL
        )
    endif()
endif()
