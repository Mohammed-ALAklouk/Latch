cmake_minimum_required(VERSION 3.21)
project(Latch LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

# raylib ships its own CMake build, so MakeAvailable gives us the target.
FetchContent_Declare(
    raylib
    GIT_REPOSITORY https://github.com/raysan5/raylib.git
    GIT_TAG 6.0
)

# ImGui and rlImGui do NOT ship CMakeLists.txt, so we only populate their
# sources and build library targets for them ourselves below.
FetchContent_Declare(imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG        v1.92.1
    GIT_SHALLOW    TRUE
)
FetchContent_Declare(rlimgui
    GIT_REPOSITORY https://github.com/raylib-extras/rlImGui.git
    GIT_TAG        Raylib_6_0
    GIT_SHALLOW    TRUE
)

FetchContent_MakeAvailable(raylib)
FetchContent_MakeAvailable(imgui)
FetchContent_MakeAvailable(rlimgui)

# --- ImGui core library ---
add_library(imgui STATIC
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/imgui_demo.cpp
)
target_include_directories(imgui PUBLIC ${imgui_SOURCE_DIR})

# --- rlImGui (raylib backend for ImGui) ---
add_library(rlImGui STATIC
    ${rlimgui_SOURCE_DIR}/rlImGui.cpp
)
target_include_directories(rlImGui PUBLIC ${rlimgui_SOURCE_DIR})
target_link_libraries(rlImGui PUBLIC imgui raylib)

add_subdirectory(src)

# Make Latch (not the ALL_BUILD utility project) the default VS startup project.
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT Latch)

option(LATCH_BUILD_TESTS "Build the Latch test suite" ${PROJECT_IS_TOP_LEVEL})
if(LATCH_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

