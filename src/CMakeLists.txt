# I create a static library for the core functionality of the latch application. This makes it easier to test and benchmark the core logic headless. It gets linked to the Latch executable target and the tests target. 
add_library(latch_core STATIC
    Circuit.cpp Circuit.h
    Gate.cpp Gate.h
    Wire.cpp Wire.h
    ComponentInfo.h IdManager.h Action.h Pin.h
)

# This is needed to make the header files available to other targets that link against this library
target_include_directories(latch_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# raylib is a dependency of the latch_core library as Circuit, Gate, and Wire classes render themselves for now. 
target_link_libraries(latch_core PUBLIC raylib)


add_executable(Latch main.cpp App.cpp App.h)
target_link_libraries(Latch PRIVATE latch_core imgui rlImGui)
