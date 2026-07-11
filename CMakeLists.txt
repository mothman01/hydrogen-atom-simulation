cmake_minimum_required(VERSION 3.16)
project(atom_sim VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ── Options ──────────────────────────────────────────────────────────────
option(BUILD_INSTALLER "Create install target" ON)
option(BUILD_TESTS "Build the unit-test suite" ON)

# ── Dependencies ─────────────────────────────────────────────────────────
find_package(OpenGL REQUIRED)
find_package(glfw3 REQUIRED)
find_package(glm REQUIRED)
find_package(GLEW REQUIRED)

# ── Source files ─────────────────────────────────────────────────────────
add_executable(atom_sim
    src/main.cpp
    src/wavefunction.cpp
    src/shader.cpp
    src/volume_renderer.cpp
    src/element_data.cpp
    src/atom_scene.cpp
    src/text_renderer.cpp
)

target_include_directories(atom_sim PRIVATE src)

target_link_libraries(atom_sim PRIVATE
    OpenGL::GL
    GLEW::GLEW
    glfw
    glm::glm
)

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
        SHADER_DIR="${CMAKE_INSTALL_PREFIX}/share/atom-sim/shaders"
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

# ── Tests ─────────────────────────────────────────────────────────────────
if(BUILD_TESTS)
    include(CTest)
    add_subdirectory(tests)
endif()
