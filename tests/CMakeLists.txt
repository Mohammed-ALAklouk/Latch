include(FetchContent)
FetchContent_Declare(Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG        v3.7.1
)
FetchContent_MakeAvailable(Catch2)

add_executable(latch_tests
    CircuitTests.cpp
    GateTests.cpp
    WireTests.cpp
    IdManagerTests.cpp
)
# latch_core = your code under test; Catch2WithMain provides the test main().
target_link_libraries(latch_tests PRIVATE latch_core Catch2::Catch2WithMain)

# Auto-discover TEST_CASEs and register each with CTest.
include(Catch)
catch_discover_tests(latch_tests)
