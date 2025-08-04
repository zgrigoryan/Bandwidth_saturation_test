cmake_minimum_required(VERSION 3.10)
project(Bandwidth_saturation_test LANGUAGES CXX)

# C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Put executables in the build/ dir so you can run them easily
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

# Threads (portable: pthreads on UNIX, MSVC runtime on Windows)
find_package(Threads REQUIRED)

# Program
add_executable(my_program main.cpp)
target_link_libraries(my_program PRIVATE Threads::Threads)

# Warnings
if(MSVC)
  target_compile_options(my_program PRIVATE /W4 /permissive-)
else()
  target_compile_options(my_program PRIVATE -Wall -Wextra -Wpedantic)
endif()
