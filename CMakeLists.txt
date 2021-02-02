cmake_minimum_required(VERSION 3.8)

project(uwlkv)
enable_testing()
link_libraries(gcov)
include_directories(src src/include)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED on)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Werror -pedantic -fstrict-aliasing")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wdouble-promotion -Wswitch-enum -Wfloat-equal -Wundef")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wconversion -Wsign-promo")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wsign-conversion -Wcast-align")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wtype-limits -Wzero-as-null-pointer-constant -Wnon-virtual-dtor")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Woverloaded-virtual -Wsign-promo")
set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS}   -Wall -Wextra -Werror -pedantic")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fprofile-arcs -ftest-coverage -g -O0")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fprofile-arcs -ftest-coverage -g -O0")
set(LCOV_REMOVE_EXTRA "'test/*'")

cmake_policy(SET CMP0076 NEW)
add_executable(uwlkv tests/main.cpp)
target_sources(uwlkv PUBLIC tests/nvram_mock.cpp)
target_sources(uwlkv PUBLIC tests/tests.cpp src/uwlkv.c src/map.c src/entry.c src/storage.c) 

add_custom_target("coverage")
add_custom_command(TARGET "coverage" COMMAND gcov ${CMAKE_BINARY_DIR}/CMakeFiles/uwlkv.dir/src/*.c.o)

set_property(TARGET uwlkv PROPERTY C_STANDARD 99)
