cmake_minimum_required(VERSION 3.16)
project(atom_sim VERSION 2.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ── Options ──────────────────────────────────────────────────────────────
option(BUILD_INSTALLER "Create install target" ON)
option(USE_OPENMP "Enable OpenMP parallelism for volume computation" ON)

# ── Dependencies ─────────────────────────────────────────────────────────
find_package(OpenGL REQUIRED)
find_package(glfw3 REQUIRED)
find_package(glm REQUIRED)
find_package(GLEW REQUIRED)

if(USE_OPENMP)
    find_package(OpenMP)
    if(OpenMP_FOUND)
        message(STATUS "OpenMP found — parallel volume computation enabled")
    else()
        message(STATUS "OpenMP not found — single-threaded fallback")
    endif()
endif()

# ── Source files ─────────────────────────────────────────────────────────
add_executable(atom_sim
    src/main.cpp
    src/wavefunction.cpp
    src/shader.cpp
    src/volume_renderer.cpp
    src/element_data.cpp
    src/atom_scene.cpp
    src/text_renderer.cpp
    src/energy_diagram.cpp
    src/crystal_lattice.cpp
    src/time_evolution.cpp
    src/molecular.cpp
    src/async_compute.cpp
    src/screenshot.cpp
)

target_include_directories(atom_sim PRIVATE src)

target_link_libraries(atom_sim PRIVATE
    OpenGL::GL
    GLEW::GLEW
    glfw
    glm::glm
)

if(OpenMP_FOUND AND USE_OPENMP)
    target_link_libraries(atom_sim PRIVATE OpenMP::OpenMP_CXX)
    target_compile_definitions(atom_sim PRIVATE USE_OPENMP)
endif()

# ── Platform-specific ────────────────────────────────────────────────────
if(WIN32)
    # Windows: shaders go next to the executable
    set(SHADER_INSTALL_DIR .)
    target_compile_definitions(atom_sim PRIVATE
        SHADER_DIR="./shaders"
        _USE_MATH_DEFINES
    )
elseif(APPLE)
    set(SHADER_INSTALL_DIR ../Resources/shaders)
    target_compile_definitions(atom_sim PRIVATE SHADER_DIR="../Resources/shaders")
    set_target_properties(atom_sim PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_GUI_IDENTIFIER "com.atomsim.app"
        MACOSX_BUNDLE_BUNDLE_NAME "Atom Wavefunction Simulator"
        MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    )
else()
    # Linux: install to share directory
    set(SHADER_INSTALL_DIR share/atom-sim/shaders)
    target_compile_definitions(atom_sim PRIVATE
            SHADER_DIR="./shaders"
            GLEW_NO_GLX
        )
endif()

# ── Copy shaders to build dir ────────────────────────────────────────────
file(COPY ${CMAKE_SOURCE_DIR}/shaders DESTINATION ${CMAKE_BINARY_DIR})
add_custom_command(TARGET atom_sim POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/shaders
    $<TARGET_FILE_DIR:atom_sim>/shaders
)

# ── Install ──────────────────────────────────────────────────────────────
include(GNUInstallDirs)

install(TARGETS atom_sim
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    BUNDLE DESTINATION .
)

install(DIRECTORY shaders/
    DESTINATION ${SHADER_INSTALL_DIR}
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
