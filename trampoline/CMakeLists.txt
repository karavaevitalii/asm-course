cmake_minimum_required(VERSION 2.8)

project(trampoline)

set(CMAKE_CXX_STANDART 11)

set(CMAKE_CXX_FLAGS_DEBUG "${CMKAKE_CXX_FLAGS_DEBUG} -Wall -Wextra -W -Wfloat-equal -Wconversion \
        -Wunreachable-code -Winline -pedantic-errors -fsanitize=address -g -time")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2")

add_executable(trampoline main.cpp trampoline.h)
