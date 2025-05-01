cmake_minimum_required(VERSION 3.11)
project(tests LANGUAGES C CXX)

# General settings
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

cmake_policy(SET CMP0076 NEW)

# Enable testing and code coverage
enable_testing()

include_directories(src src/include)

# Fetch Catch2 for testing
Include(FetchContent)
FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG v3.4.0
)
FetchContent_MakeAvailable(Catch2)

# Core library
add_library(uwlkv STATIC
    src/uwlkv.c
    src/map.c
    src/entry.c
    src/storage.c
)

# Test executable
add_executable(tests
    tests/tests.cpp
    tests/nvram_mock.cpp
)
target_link_libraries(tests PRIVATE Catch2::Catch2WithMain uwlkv)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(uwlkv PRIVATE
        -Wall -Wextra -Werror -pedantic
        --coverage
    )
    target_compile_options(tests PRIVATE
        -Wall -Wextra -Werror -pedantic
        -fstrict-aliasing
        -Wdouble-promotion -Wswitch-enum -Wfloat-equal -Wundef
        -Wconversion -Wsign-promo -Wsign-conversion -Wcast-align
        -Wtype-limits -Wzero-as-null-pointer-constant -Wnon-virtual-dtor
        -Woverloaded-virtual
        --coverage
        -g -O0
    )
    target_link_options(tests PRIVATE --coverage)

    set(LCOV_REMOVE_EXTRA "'test/*'")
    add_custom_target(coverage COMMAND gcov ${CMAKE_BINARY_DIR}/CMakeFiles/uwlkv.dir/src/*.c.o)
elseif(MSVC)
    # MSVC-specific warning levels
    target_compile_options(uwlkv PRIVATE /W4 /WX)
    target_compile_options(tests PRIVATE /W4 /WX)
endif()
