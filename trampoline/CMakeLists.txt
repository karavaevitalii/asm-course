cmake_minimum_required(VERSION 3.5)

project(trampoline)

set(CMAKE_CXX_STANDART 11)

set(CMAKE_CXX_FLAGS_DEBUG "-Wall -Wextra -W -Wfloat-equal -Wconversion \
        -Wunreachable-code -Winline -pedantic-errors -fsanitize=address -g -time")

set(CMAKE_CXX_FLAGS_RELEASE "-O2")

add_executable(trampoline
    test_correctness_main.cpp
    trampoline.h
    arguments.h)
