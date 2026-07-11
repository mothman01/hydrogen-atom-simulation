cmake_minimum_required(VERSION 3.16)

# ── Fetch doctest (single-header, MIT license) ────────────────────────────
include(FetchContent)
FetchContent_Declare(
    doctest
    GIT_REPOSITORY https://github.com/doctest/doctest.git
    GIT_TAG        v2.4.11
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(doctest)

# ── Test executable ───────────────────────────────────────────────────────
# Links only the pure-math translation units — no OpenGL headers required.
add_executable(atom_sim_tests
    main.cpp
    test_wavefunction.cpp
    test_element_data.cpp
    ${CMAKE_SOURCE_DIR}/src/wavefunction.cpp
    ${CMAKE_SOURCE_DIR}/src/element_data.cpp
)

target_include_directories(atom_sim_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(atom_sim_tests PRIVATE
    doctest::doctest
)

target_compile_features(atom_sim_tests PRIVATE cxx_std_17)

# ── CTest registration ────────────────────────────────────────────────────
add_test(NAME unit_tests COMMAND atom_sim_tests)
set_tests_properties(unit_tests PROPERTIES
    LABELS "physics;element_data"
)
