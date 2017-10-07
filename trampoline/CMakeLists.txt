cmake_minimum_required(VERSION 3.5)

project(trampoline)

set(CMAKE_CXX_STANDART 17)

set(CMAKE_CXX_FLAGS "-std=c++1z")

#set(CMAKE_CXX_FLAGS_DEBUG "-Wall -Wextra -W -Wconversion \
#        -Wunreachable-code -Winline -pedantic-errors \
#        -fsanitize=address")

set(CMAKE_CXX_FLAGS_RELEASE "-O2")

add_executable(trampoline
    test_correctness_main.cpp
    allocator.cpp allocator.h
    trampoline.h
    arguments.h)
